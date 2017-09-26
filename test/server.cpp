#include "../include/rdma_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MSG_LEN 32
#define MSG "A AMessage from server!@#$%^&*()"
#define MSG_COUNT 10

#define DEFAULT_PORT 40000

int main(int argc, char** argv) {
    AMessage *msg;
    Receiver *re = receiver_build();
    receiver_bind(re, DEFAULT_PORT);
    for(int i=0; true; i++) {
        msg = receiver_recv(re);
        printf("Get a AMessage : %s, from %d\n", msg->buffer, msg->node_id);
        AMessage_destroy(msg);
        msg = NULL;
        printf("%d\n", i);
    }
    receiver_close(re);
}