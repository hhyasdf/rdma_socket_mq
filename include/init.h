#ifndef INIT
#define INIT


#include "rdma_socket.h"

void build_params(struct rdma_conn_param *params);
Socket *buildConnection(struct rdma_cm_id *id, Receiver *receiver);



#endif