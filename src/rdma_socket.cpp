#include "../include/rdma_socket.h"
#include "../include/post_wr.h"
#include "../include/poll_wc.h"
#include "../include/init.h"
#include "../include/amessage.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <rdma/rdma_verbs.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>


Socket* socket_(enum rdma_port_space type) {

    struct rdma_cm_id *id = NULL;
    struct rdma_event_channel *ec = NULL;

    TEST_Z(ec = rdma_create_event_channel());
    TEST_NZ(rdma_create_id(ec, &id, NULL, type));    // type can be RDMA_PS_TCP or RDMA_PS_UDP

    Socket *socket_ = (Socket *)malloc(sizeof(Socket));
    // memset(socket_, 0, sizeof(Socket));
    
    socket_->id = id;
    socket_->ec = ec;

    socket_->pd = NULL;

    return socket_;
}


int bind_(Socket *socket_, void *addr, int protocol) {     // protocol can be AF_INET6 or AF_INET
                                                            // addr指定本地服务端的地址为 sockaddr_in6 或 sockaddr_in6 结构
    return rdma_bind_addr(socket_->id, (struct sockaddr *)addr);    
}


void listen_(Socket *socket_, int backlog) {

    TEST_NZ(rdma_listen(socket_->id, backlog));
}


static int send_close_md(Socket *socket_) {

    if(pthread_mutex_trylock(&socket_->close_lock)) {
        return 0;
    }
    pthread_mutex_unlock(&socket_->close_lock);

    struct ibv_wc wc;
    struct ibv_mr *send_mr;
    MetaData metadata;

    while(socket_->ack_counter < socket_->metadata_counter) {
        if(poll_wc(socket_, &wc) < 0) {
            return 0;
        }
        resolve_wr_queue(socket_);
        // resolve_wr_queue_flag(socket_);
    }

    memset(&metadata, 0, sizeof(metadata));
    metadata.type = METADATA_CLOSE;

    send_mr = post_send_wr(socket_, &metadata);

    while(poll_wc(socket_, &wc) > 0);

    ibv_dereg_mr(send_mr);

    if(wc.status != IBV_WC_SUCCESS) {

        // printf("send close error !\n");

        return 0;        // 连接已断开
    }
    return 1;
}


// static void pause_thread(Socket *socket_) {
//     pthread_mutex_lock(&socket_->close_lock);  
//     pthread_cond_wait(&socket_->close_cond, &socket_->close_lock);  
//     printf("pthread wait!\n");  
//     pthread_mutex_unlock(&socket_->close_lock);
// }


// static void run_thread(Socket *socket_) {
//     pthread_mutex_lock(&socket_->close_lock);  
//     pthread_cond_signal(&socket_->close_cond);  
//     printf("run pthread!\n");  
//     pthread_mutex_unlock(&socket_->close_lock);
// }


static void *wait_for_close(void *socket_) {

    struct rdma_cm_event *event = NULL;
    Socket *sock = (Socket *)socket_;

    while (rdma_get_cm_event(sock->ec, &event) == 0) {

        struct rdma_cm_event event_copy;
        memcpy(&event_copy, event, sizeof(*event));
        rdma_ack_cm_event(event);

        if (event_copy.event == RDMA_CM_EVENT_DISCONNECTED) {

            pthread_mutex_lock(&sock->close_lock);

            rdma_disconnect(sock->id);
            
            struct ibv_wc wc, *wc_save;
            struct ibv_cq *cq;
            Rinfo *rinfo;
            MetaData *recv_buffer;

            while(ibv_poll_cq(sock->cq, 1, &wc) == 1) {
                wc_save = (struct ibv_wc *)malloc(sizeof(struct ibv_wc));
                memcpy(wc_save, &wc, sizeof(wc));
                queue_push(sock->wr_queue, wc_save);
            }
            while((wc_save = (struct ibv_wc *)queue_pop(sock->wr_queue)) != NULL) {
                close_handle(sock, wc_save);
            }

            rdma_destroy_qp(sock->id);
            ibv_destroy_cq(sock->cq);
            ibv_destroy_comp_channel(sock->cc);

            // free(sock->metaData_buffer);

            // rdma_destroy_id(sock->id);
            // rdma_destroy_event_channel(sock->ec);
        
            // ibv_dealloc_pd(sock->pd);

            // resolve_wr_queue(sock);
            // // resolve_wr_queue_flag(sock);
            
        
            // queue_destroy(sock->recv_queue);
            // queue_destroy(sock->wr_queue);
        
            // pthread_mutex_destroy(&sock->close_lock);
            // pthread_mutex_destroy(&sock->peer_buff_count_lock);
            // pthread_mutex_destroy(&sock->metadata_counter_lock);
            // pthread_mutex_destroy(&sock->ack_counter_lock);
        
            // free(sock);
        
            return 0;
        }
    }
}


Socket *accept_(Socket *socket_, struct Receiver_ *receiver) {

    struct rdma_cm_event *event = NULL;
    struct rdma_event_channel *ec = socket_->ec;
    Socket *new_socket_ = NULL;
    struct rdma_cm_event event_copy;
    struct rdma_conn_param cm_params;
    
    while (rdma_get_cm_event(ec, &event) == 0) {
        
        memcpy(&event_copy, event, sizeof(*event));

        rdma_ack_cm_event(event);

        if(event_copy.event == RDMA_CM_EVENT_CONNECT_REQUEST) {
            
            new_socket_ = buildConnection(event_copy.id, 0);
            new_socket_->receiver = receiver;
            ec = new_socket_->ec;

            build_params(&cm_params);
            rdma_accept(new_socket_->id, &cm_params);
        } else if (event_copy.event == RDMA_CM_EVENT_ESTABLISHED) {

            AMessage *id_msg = recv_(new_socket_);
            new_socket_->node_id = *((int *)(id_msg->buffer));

            pthread_create(&new_socket_->close_pthread, NULL, wait_for_close, new_socket_);
            return new_socket_;
        } else if (event_copy.event == RDMA_CM_EVENT_DISCONNECTED) {
            return NULL;
        }
    }
}


int connect_(Socket **socket_, const char *address, char *port, int node_id) {         
    
    struct addrinfo *addr;
    TEST_NZ(getaddrinfo(address, port, NULL, &addr));
    rdma_resolve_addr((*socket_)->id, NULL, addr->ai_addr, TIMEOUT_IN_MS);

    struct rdma_cm_event *g_event = NULL;
    struct rdma_conn_param con_params;
    struct rdma_event_channel *ec = (*socket_)->ec;
    Socket *new_socket_;
    struct rdma_cm_event event;

    while (rdma_get_cm_event(ec, &g_event) == 0) {
        memcpy(&event, g_event, sizeof(event));
        rdma_ack_cm_event(g_event);

        if (event.event == RDMA_CM_EVENT_ADDR_ERROR) {
            printf("RDMA_CM_EVENT_ADDR_ERROR");
            exit(0);
        } else if (event.event == RDMA_CM_EVENT_ADDR_RESOLVED) {
            new_socket_ = buildConnection(event.id, node_id);
            ec = new_socket_->ec;
            free(*socket_);
            *socket_ = new_socket_;            
            rdma_resolve_route(event.id, TIMEOUT_IN_MS);
        } else if (event.event == RDMA_CM_EVENT_ROUTE_RESOLVED) {
            build_params(&con_params);
            rdma_connect(event.id, &con_params);
        } else if (event.event == RDMA_CM_EVENT_ESTABLISHED) {
            
            AMessage *id_msg = AMessage_create(&node_id, sizeof(int), 0);
            send_(*socket_, id_msg);

            pthread_create(&((*socket_)->close_pthread), NULL, wait_for_close, *socket_);

            return 1;
        } else if (event.event == RDMA_CM_EVENT_DISCONNECTED) {
            return 0;
        } else {
            printf("event: %d\n", event.event);
            return 0;
        }
    }
}


void close_(Socket *socket_) {                   // 释放socket结构体和其中的两个动态分配的队列
    
    if(socket_->pd == NULL) {
        // rdma_disconnect(socket_->id);
        return;
    }

    rdma_disconnect(socket_->id);
    if(socket_->close_pthread == 0) {
        free(socket_);
        return;
    }
    pthread_join(socket_->close_pthread, NULL);

    rdma_destroy_id(socket_->id);
    rdma_destroy_event_channel(socket_->ec);

    ibv_dealloc_pd(socket_->pd);

    resolve_wr_queue(socket_);
    // resolve_wr_queue_flag(socket_);
    

    queue_destroy(socket_->recv_queue);
    queue_destroy(socket_->wr_queue);

    pthread_mutex_destroy(&socket_->close_lock);
    pthread_mutex_destroy(&socket_->peer_buff_count_lock);
    pthread_mutex_destroy(&socket_->metadata_counter_lock);
    pthread_mutex_destroy(&socket_->ack_counter_lock);

    free(socket_->metaData_buffer);
    free(socket_);
}


int send_(Socket *socket_, AMessage *msg) {      // 当一次性send操作数超过初始的缓冲区大小会被阻塞

    struct ibv_send_wr *bad_wr = NULL;
    struct ibv_mr *send_mr, *msg_mr;
    struct ibv_cq *cq;
    struct ibv_wc wc;
    int length = msg->length;
    void *recv_buffer;

    void *buffer_copy = malloc(length);

    memcpy(buffer_copy, msg->buffer, length);

    MetaData metadata;

    if(pthread_mutex_trylock(&socket_->close_lock)) {
        return -1;
    }
    pthread_mutex_unlock(&socket_->close_lock);
    
    TEST_Z(msg_mr = ibv_reg_mr(
        socket_->pd,
        buffer_copy,
        length,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ));
    
    metadata.length = length;
    metadata.msg_addr = (uint64_t)msg_mr->addr;//(uint64_t)buffer_copy;
    metadata.rkey = msg_mr->rkey;
    metadata.mr_addr = (uint64_t)msg_mr;
    metadata.type = METADATA_NORMAL;
    metadata.flag = msg->flag;

    pthread_mutex_lock(&socket_->peer_buff_count_lock);
    if(socket_->peer_buff_count <= 0) {
        poll_wc(socket_, NULL);
    }
    pthread_mutex_unlock(&socket_->peer_buff_count_lock);

    send_mr = post_send_wr(socket_, &metadata);

    pthread_mutex_lock(&socket_->peer_buff_count_lock);
    socket_->peer_buff_count --;
    pthread_mutex_unlock(&socket_->peer_buff_count_lock);

    while(poll_wc(socket_, &wc));
    if(wc.status != IBV_WC_SUCCESS) {
        printf("send error: %d!\n", wc.status);
        return -1;        // 连接已断开
    }
    ibv_dereg_mr(send_mr);

    pthread_mutex_lock(&socket_->metadata_counter_lock);
    socket_->metadata_counter ++;
    pthread_mutex_unlock(&socket_->metadata_counter_lock);
    
    if(resolve_wr_queue(socket_) == -1) {
        socket_->close_flag = 1;
    }

    // if(resolve_wr_queue_flag(socket_) == -1) {
    //     socket_->close_flag = 1;
    // }

    return 0;
}


AMessage *recv_(Socket *socket_) {            // 用户提供指针地址，函数来填充 *recv_buffer,用户需要自己free 
    int flag = 1;
    struct ibv_wc wc;
    void *wc_save;
    struct ibv_cq *cq;
    AMessage *recv_msg;

    if(pthread_mutex_trylock(&socket_->close_lock)) {
        return NULL;
    }
    pthread_mutex_unlock(&socket_->close_lock);

    while(ibv_poll_cq(socket_->cq, 1, &wc) == 1){
        if(wc.opcode == IBV_WC_SEND || wc.opcode == IBV_WC_RDMA_READ){
            continue;
        } else {
            wc_save = malloc(sizeof(struct ibv_wc));
            memcpy(wc_save, &wc, sizeof(wc));
            queue_push(socket_->wr_queue, wc_save);
        }
    }
    flag = resolve_wr_queue(socket_);
    if (flag == -1) {
        socket_->close_flag = 1;
    }

    if((recv_msg = (AMessage *)queue_pop(socket_->recv_queue)) != NULL) {      
        return recv_msg;
    } else if(pthread_mutex_trylock(&socket_->close_lock)) {    // 往下 *recv_buffer 都为 NULL
        return NULL;
    } else if (socket_->close_flag == 1) {
        return NULL;                                   
    } else {
        pthread_mutex_unlock(&socket_->close_lock);                          
        // if(socket_->close_flag == 1){
        //     return ;
        // }
        while(flag == 1){
            if(poll_wc(socket_, NULL) == -1) {
                return NULL;
            }
            flag = resolve_wr_queue(socket_);
        }
    }

    if(flag == -1) { 
        socket_->close_flag = 1;                  // 断开连接会将 close_flag 设成 1
    }
    return (AMessage *)queue_pop(socket_->recv_queue);
}

// 想改成一次性将socket的recv_queue 都放进去的操作

// Socket *recv_(Socket *socket_, Queue *de_queue) {
//     int flag = 1;
//     struct ibv_wc wc;
//     void *wc_save;
//     struct ibv_cq *cq;
//     AMessage *recv_msg;

//     if(pthread_mutex_trylock(&socket_->close_lock)) {
//         return NULL;
//     }
//     pthread_mutex_unlock(&socket_->close_lock);

//     while(ibv_poll_cq(socket_->cq, 1, &wc) == 1){
//         if(wc.opcode == IBV_WC_SEND || wc.opcode == IBV_WC_RDMA_READ){
//             continue;
//         } else {
//             wc_save = malloc(sizeof(struct ibv_wc));
//             memcpy(wc_save, &wc, sizeof(wc));
//             queue_push(socket_->wr_queue, wc_save);
//         }
//     }
//     flag = resolve_wr_queue_flag(socket_);
//     if (flag == -1) {
//         socket_->close_flag = 1;
//     }

//     if(!queue_if_empty(socket_->recv_queue)) {
//         queue_push_q(de_queue, socket_->recv_queue);
//         queue_reset(socket_->recv_queue);
//         printf("num of de_queue %p: %d\n", de_queue ,de_queue->node_num);     
//         return socket_;
//     } else if(pthread_mutex_trylock(&socket_->close_lock)) {    // 往下 *recv_buffer 都为 NULL
//         return NULL;
//     } else if (socket_->close_flag == 1) {
//         return NULL;                                   
//     } else {
//         pthread_mutex_unlock(&socket_->close_lock);                          
//         // if(socket_->close_flag == 1){
//         //     return ;
//         // }
//         while(flag == 1){
//             if(poll_wc(socket_, NULL) == -1) {
//                 return NULL;
//             }
//             flag = resolve_wr_queue_flag(socket_);
//         }
//     }

//     queue_push_q(de_queue, socket_->recv_queue);
//     queue_reset(socket_->recv_queue);

//     if(flag == -1) { 
//         socket_->close_flag = 1;                  // 断开连接会将 close_flag 设成 1
//         return NULL;
//     }

//     return socket_;
// }


void die(char *msg) {
    fprintf(stdout, "%s\n%s\n", msg, strerror(errno));
    exit(1);
}
