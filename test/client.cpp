#include "../include/rdma_socket.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MSG_LEN 32
#define MSG "A AMessage from client!@#$%^&*()"
#define MSG_COUNT 10
#define THREAD_NUM 10

void *send_process(void *socket){
    AMessage *msg;

    for(int i = 0; i < MSG_COUNT; i ++) {
        msg = AMessage_create((void *)MSG, sizeof(MSG), 0);
        printf("Send a AMessage: %s!\n", MSG);

        send_((Socket *)socket, msg);
        AMessage_destroy(msg);
    }
    close_((Socket *)socket);
}



int main(int argc, char **argv) {

    Socket *socket;
    pthread_t p_id[THREAD_NUM];

    for(int i = 0; i < THREAD_NUM; i ++){
        socket = socket_(RDMA_PS_TCP);
        connect_(&socket, argv[1], argv[2], i);
        printf("connect : %p\n", connect);
        pthread_create(p_id + i, NULL, send_process, socket);
    }
    
    for(int i = 0; i < THREAD_NUM; i ++){
        pthread_join(p_id[i], NULL);
    }
}