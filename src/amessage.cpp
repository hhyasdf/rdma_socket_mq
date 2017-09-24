#include "../include/amessage.h"
#include "../include/rdma_socket.h"
#include <stdlib.h>



AMessage *AMessage_create(void *buffer, int length, int flag, bool if_free_buffer) {          // buffer要自己分配
    AMessage *msg = (AMessage *)malloc(sizeof(AMessage));
    msg->buffer = buffer;
    msg->flag = flag;
    msg->length = length;
    msg->if_free_buffer = if_free_buffer;
    msg->node_id = 0;

    return msg;
}

bool AMessage_check_sndmore(AMessage *msg) {
    return (msg->flag == SND_MORE_FLAG);
}

void AMessage_destroy(AMessage *msg) {
    if(msg->if_free_buffer) {
        free(msg->buffer);
    }
    free(msg);
}
