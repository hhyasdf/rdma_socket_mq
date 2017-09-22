#include "message.h"
#include "rdma_socket.h"
#include <stdlib.h>


Message *Message_create(void *buffer, int length, int flag) {          // buffer要自己分配
    Message *msg = (Message *)malloc(sizeof(Message));
    msg->buffer = buffer;
    msg->flag = flag;
    msg->length = length;

    return msg;
}

bool Message_check_sndmore(Message *msg) {
    return (msg->flag == SNDMORE_FLAG);
}

void Message_destroy(Message *msg) {
    free(msg->buffer);
    free(msg);
}
