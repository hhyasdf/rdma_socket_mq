#include "rdma_socket.h"

#ifndef POLLWC
#define POLLWC

int poll_wc(Socket *socket_, struct ibv_wc *send_wc);
void close_handle(Socket *socket_, struct ibv_wc *wc);
int recv_wc_handle(Socket *socket_, struct ibv_wc *wc, Message *recv_msg);
int resolve_wr_queue(Socket *socket_);


#endif

