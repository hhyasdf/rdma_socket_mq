#include "../include/message.h"
#include "../include/rdma_socket.h"
#include <stdlib.h>



Message *Message_create(void *buffer, int length, int flag, bool if_free_buffer) {          // buffer要自己分配
    Message *msg = (Message *)malloc(sizeof(Message));
    msg->buffer = buffer;
    msg->flag = flag;
    msg->length = length;
    msg->if_free_buffer = if_free_buffer;
    msg->node_id = 0;

    return msg;
}

bool Message_check_sndmore(Message *msg) {
    return (msg->flag == SND_MORE_FLAG);
}

void Message_destroy(Message *msg) {
    if(msg->if_free_buffer) {
        free(msg->buffer);
    }
    free(msg);
}
