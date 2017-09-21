#include "rdma_socket.h"

#ifndef POSTWR
#define POSTWR

void post_recv_wr(Socket *socket_, void *recv_buffer);
struct ibv_mr *post_send_wr(Socket *socket_, void *send_buffer);

#endif