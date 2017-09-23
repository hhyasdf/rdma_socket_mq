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
    return (msg->flag == SND_MORE_FLAG);
}

void Message_destroy(Message *msg) {
    if(msg->buffer != NULL) {
        free(msg->buffer);
    }
    msg->buffer = NULL;
    free(msg);
}
