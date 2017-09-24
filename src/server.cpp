#include "../include/rdma_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MSG_LEN 32
#define MSG "A message from server!@#$%^&*()"
#define MSG_COUNT 10

#define DEFAULT_PORT 40000

int main(int argc, char** argv) {
    Message *msg;
    Receiver *re = receiver_build();
    receiver_bind(re, DEFAULT_PORT);
    while(1) {
        msg = receiver_recv(re);
        // printf("Get a message : %s\n", msg->buffer);
        free(msg->buffer);
        free(msg);
        msg = NULL;
    }
}