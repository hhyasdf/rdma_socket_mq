#include "message.h"
#include "rdma_socket.h"


void Message_init(Message* msg, void *buffer, int flag) {
    msg->buffer = buffer;
    msg->flag = flag;
}

bool Message_check_sndmore(Message *msg) {
    return (msg->flag == SNDMORE_FLAG);
}
