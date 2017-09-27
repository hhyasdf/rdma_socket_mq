#include "../include/rdma_socket.h"
#include "../include/queue.h"
#include "../include/poll_wc.h"

#include <stdlib.h>
#include <stdio.h>
#include <rdma/rdma_cma.h>
#include <rdma/rdma_verbs.h>


#define RDMAREADSOLVED 8
#define ACKSOLVED 9
#define ERRORWC 10
#define CLOSERESOLVED 11


void close_handle(Socket *socket_, struct ibv_wc *wc) {                          // 关闭时处理wc的函数
    Rinfo *rinfo  = (Rinfo *)wc->wr_id;
    if(rinfo == NULL || wc->opcode != IBV_WC_RECV) {
        return ;
    }
    MetaData *recv_buffer = (MetaData *)rinfo->buffer;

    if(recv_buffer->type == METADATA_ACK) {
        if(recv_buffer->mr_addr != NULL) {            
            ibv_dereg_mr((struct ibv_mr *)recv_buffer->mr_addr);
            recv_buffer->mr_addr = NULL;
        }
        free((void *)recv_buffer->msg_addr);
    }

    ibv_dereg_mr(rinfo->mr);
    free(rinfo);
}



int poll_wc(Socket *socket_, struct ibv_wc *send_wc) {       // 获取一个提醒，处理cq中的所有cqe， 每一次调用都清空cq
    struct ibv_cq *cq;
    struct ibv_wc wc, s_wc;
    void *wc_save, *tmp_context;    // !!
    int flag = 1;

    if(pthread_mutex_trylock(&socket_->close_lock)) {
        return -1;
    }
    pthread_mutex_unlock(&socket_->close_lock);

    if(ibv_get_cq_event(socket_->cc, &cq, &tmp_context)) {
        return -1;
    }

    ibv_ack_cq_events(cq, 1);

    while(ibv_poll_cq(cq, 1, &wc) == 1) {
        if(wc.opcode == IBV_WC_SEND || wc.opcode == IBV_WC_RDMA_READ){
            if(send_wc != NULL) {
                memcpy(send_wc, &wc, sizeof(wc));
            }
            flag = 0;
        } else if(wc.opcode == IBV_WC_RECV){
            wc_save = malloc(sizeof(struct ibv_wc));
            memcpy(wc_save, &wc, sizeof(wc));
            queue_push(socket_->wr_queue, wc_save);
        }
    }

    TEST_NZ(ibv_req_notify_cq(cq, 0));
    return flag;
}



int resolve_wr_queue(Socket *socket_) {               // 处理 wr_queue 中的 wc
    struct ibv_wc *wc;
    int flag = 1, stat;
    AMessage *recv_msg = NULL;

    while((wc = (struct ibv_wc *)queue_pop(socket_->wr_queue)) != NULL) {
        if((stat = recv_wc_handle(socket_, wc, &recv_msg)) == RDMAREADSOLVED) {
            queue_push(socket_->recv_queue, recv_msg);
            flag = 0;
        } else if (stat == ERRORWC) {
            return -1;
        }
        free(wc);
    }
    return flag;
}

// 想改成一次性将socket的recv_queue 都放进去的操作

// int resolve_wr_queue_flag(Socket *socket_) {               // 处理 wr_queue 中的 wc
//     struct ibv_wc *wc;
//     int flag = 1, stat;
//     AMessage *recv_msg = NULL;

//     while((wc = (struct ibv_wc *)queue_pop(socket_->wr_queue)) != NULL) {
//         if((stat = recv_wc_handle(socket_, wc, &recv_msg)) == RDMAREADSOLVED) {
//             if(recv_msg->flag == SND_MORE_FLAG){
//                 queue_push(socket_->more_queue, recv_msg);
//             } else {
//                 if(!queue_if_empty(socket_->more_queue)) {
//                     queue_push_q(socket_->recv_queue, socket_->more_queue);
//                     queue_reset(socket_->more_queue);
//                 }
//                 queue_push(socket_->recv_queue, recv_msg);
//                 printf("add a node: %s\n", recv_msg->buffer);
//                 printf("num of socket->recv_queue: %d\n", socket_->recv_queue->node_num);
//             }
//             flag = 0;
//         } else if (stat == CLOSERESOLVED){
//             queue_push_q(socket_->recv_queue, socket_->more_queue);
//             queue_reset(socket_->more_queue);
//             return -1;
//         } else if (stat == ERRORWC) {
//             return -1;
//         }
//         free(wc);
//     }
//     return flag;
// }




int recv_wc_handle(Socket *socket_, struct ibv_wc *wc, AMessage **recv_msg) {         // 处理每一个recv的wc
    if(wc->status != IBV_WC_SUCCESS) {
        // printf("err recv code: %d\n", wc->status);
        // die("quit");
        return ERRORWC;
    }

    Rinfo *rinfo;
    MetaData *md_buffer;

    rinfo = (Rinfo *)wc->wr_id;
    md_buffer = (MetaData *)rinfo->buffer;

    if(md_buffer->type == METADATA_NORMAL) {
        struct ibv_mr *read_mr;
        struct ibv_wc wc;

        if(md_buffer->length != 0) {

            *recv_msg = AMessage_create(malloc(md_buffer->length), md_buffer->length, md_buffer->flag, true);
            
            TEST_Z(read_mr = ibv_reg_mr(
                socket_->pd,
                (*recv_msg)->buffer,
                md_buffer->length,
                IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ));

            TEST_NZ(rdma_post_read(
                socket_->id, 
                NULL, 
                (*recv_msg)->buffer, 
                md_buffer->length, 
                read_mr, 
                IBV_SEND_SIGNALED,
                md_buffer->msg_addr,
                md_buffer->rkey));
        
            while(poll_wc(socket_, &wc));
            if(wc.status != IBV_WC_SUCCESS){
                printf("remote read error: %d!\n", wc.status);
                return ERRORWC;
            }

            ibv_dereg_mr(read_mr);

        } else {
            *recv_msg = AMessage_create((void *)0x1, md_buffer->length, md_buffer->flag);
        }

        (*recv_msg)->node_id = socket_->node_id;

        while(1) {
            md_buffer->type = METADATA_ACK;
            TEST_NZ(rdma_post_send(socket_->id,
            rinfo, 
            md_buffer,
            sizeof(MetaData), 
            rinfo->mr, 
            IBV_SEND_SIGNALED));

            while(poll_wc(socket_, &wc));
            if(wc.status == IBV_WC_SUCCESS) {
                // printf("send ack error: %d!\n", wc.status);
                break;
            }
        }

        md_buffer->type = 0;

        TEST_NZ(rdma_post_recv(socket_->id, 
        rinfo, 
        md_buffer, 
        sizeof(MetaData), 
        (struct ibv_mr *)rinfo->mr));

        return RDMAREADSOLVED;

    } else if (md_buffer->type == METADATA_ACK) {

        pthread_mutex_lock(&socket_->peer_buff_count_lock);
        socket_->peer_buff_count ++;
        pthread_mutex_unlock(&socket_->peer_buff_count_lock);
        
        if(md_buffer->mr_addr != NULL) {
            ibv_dereg_mr((struct ibv_mr *)md_buffer->mr_addr);
            free((void *)md_buffer->msg_addr);
        }

        pthread_mutex_lock(&socket_->ack_counter_lock);
        socket_->ack_counter ++;
        pthread_mutex_unlock(&socket_->ack_counter_lock);

        md_buffer->type = 0;

        TEST_NZ(rdma_post_recv(socket_->id, 
        rinfo, 
        md_buffer, 
        sizeof(MetaData), 
        (struct ibv_mr *)rinfo->mr));

        return ACKSOLVED;
    } else if (md_buffer->type == METADATA_CLOSE) {

        TEST_NZ(rdma_post_recv(socket_->id, 
        rinfo, 
        md_buffer, 
        sizeof(MetaData), 
        (struct ibv_mr *)rinfo->mr));

        return ERRORWC;
    }
}


