#include "../include/rdma_socket.h"
#include "../include/post_wr.h"
#include "../include/init.h"

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>  
#include <sys/types.h>   
#include <unistd.h> 


void build_params(struct rdma_conn_param *params)                // remote read 必须要设置连接参数
{
  memset(params, 0, sizeof(*params));
 
  params->initiator_depth = params->responder_resources = 9;
  params->rnr_retry_count = 7; /* infinite retry */
}


Socket *buildConnection(struct rdma_cm_id *id, int node_id) {  // 用收到的id中的verbs作为

    Socket *new_socket_ = (Socket *)malloc(sizeof(Socket));
    memset(new_socket_, 0, sizeof(Socket));

    //new_socket_->id = NULL;
    new_socket_->id = id;
    new_socket_->node_id = node_id;
    new_socket_->ec = rdma_create_event_channel();
    TEST_NZ(rdma_migrate_id(new_socket_->id, new_socket_->ec));     // 将一个已存在的id迁移到新的ec上，并将原来ec上的所有于id相关的事件迁移到新的ec上

	TEST_Z(new_socket_->pd = ibv_alloc_pd(id->verbs));

	TEST_Z(new_socket_->cc = ibv_create_comp_channel(id->verbs));
    TEST_Z(new_socket_->cq = ibv_create_cq(id->verbs, MDBUFFERSIZE * 2, NULL, new_socket_->cc, 0));
    TEST_NZ(ibv_req_notify_cq(new_socket_->cq, 0));

	struct ibv_qp_init_attr qp_attr;
    memset(&qp_attr, 0, sizeof(qp_attr));     // !!

	qp_attr.send_cq = new_socket_->cq;
	qp_attr.recv_cq = new_socket_->cq;

	qp_attr.qp_type = IBV_QPT_RC;

	qp_attr.cap.max_send_wr = MDBUFFERSIZE;
	qp_attr.cap.max_recv_wr = MDBUFFERSIZE;
	qp_attr.cap.max_send_sge = 1;
	qp_attr.cap.max_recv_sge = 1;

	if (rdma_create_qp(id, new_socket_->pd, &qp_attr) == -1)
        printf("%s\n", strerror(errno));

    new_socket_->qp = id->qp;

    new_socket_->rinfo_queue = queue_init();

    MetaData *recv_buffer = (MetaData *)malloc(MDBUFFERSIZE * sizeof(MetaData));
    memset(recv_buffer, 0, MDBUFFERSIZE * sizeof(MetaData));
    new_socket_->metaData_buffer = recv_buffer;
    for(int i=0; i < MDBUFFERSIZE; i++) {                                         // 初始化放一堆 recv 到 qp
        post_recv_wr(new_socket_, recv_buffer ++);
    }

    new_socket_->recv_queue = queue_init();
    new_socket_->wr_queue = queue_init();
    new_socket_->more_queue = queue_init();

    pthread_mutex_init(&new_socket_->peer_buff_count_lock, NULL);
    pthread_mutex_init(&new_socket_->ack_counter_lock, NULL);
    pthread_mutex_init(&new_socket_->metadata_counter_lock, NULL);
    pthread_mutex_init(&new_socket_->close_lock, NULL);
    
    new_socket_->peer_buff_count = MDBUFFERSIZE;
    new_socket_->ack_counter = 0;
    new_socket_->metadata_counter = 0;
    new_socket_->close_flag = 0;
    new_socket_->receiver = NULL;

    // pthread_mutex_init(&new_socket_->close_lock, NULL);
    // pthread_cond_init(&new_socket_->close_cond, NULL);
    // pthread_barrier_init(&new_socket_->close_barrier, NULL, 2);

    return new_socket_;
}