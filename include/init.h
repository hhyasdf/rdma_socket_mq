#include "rdma_socket.h"

#ifndef INIT
#define INIT



void build_params(struct rdma_conn_param *params);
Socket *buildConnection(struct rdma_cm_id *id, int node_id);



#endif