#include "../include/rdma_socket.h"
#include "../include/post_wr.h"

#include <stdlib.h>
#include <rdma/rdma_verbs.h>
#include <stdio.h>



void post_recv_wr(Socket *socket_, void *recv_buffer) {
    Rinfo *recv_Rinfo = (Rinfo *)malloc(sizeof(Rinfo));

    TEST_Z(recv_Rinfo->mr = ibv_reg_mr(
        socket_->pd,
        recv_buffer,
        sizeof(MetaData),
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ));

    // printf("reg: %d\n", recv_Rinfo->mr);

    recv_Rinfo->buffer = recv_buffer;

    queue_push(socket_->rinfo_queue, recv_Rinfo);

    TEST_NZ(rdma_post_recv(socket_->id, recv_Rinfo, recv_buffer, sizeof(MetaData), recv_Rinfo->mr));
}


struct ibv_mr *post_send_wr(Socket *socket_, void *send_buffer) {
    struct ibv_mr *send_mr;

    TEST_Z(send_mr = ibv_reg_mr(
        socket_->pd,
        send_buffer,
        sizeof(MetaData),
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ));

    rdma_post_send(socket_->id, NULL, send_buffer, sizeof(MetaData), send_mr, IBV_SEND_SIGNALED);         // !!
    return send_mr;
}