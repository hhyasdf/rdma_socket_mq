#include "../include/rdma_socket.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MSG_LEN 32
#define MSG "A message from client!@#$%^&*()"
#define MSG_COUNT 1000
#define THREAD_NUM 10

void *send_process(void *socket){
    Message *msg;

    for(int i = 0; i < MSG_COUNT; i ++) {
        msg = Message_create((void *)MSG, sizeof(MSG), SND_MORE_FLAG);
        printf("Send a message: %s!\n", MSG);

        send_((Socket *)socket, msg);
        Message_destroy(msg);
    }
    close_((Socket *)socket);
}



int main(int argc, char **argv) {

    Socket *socket;
    pthread_t p_id[THREAD_NUM];

    for(int i = 0; i < THREAD_NUM; i ++){
        socket = socket_(RDMA_PS_TCP);
        connect_(&socket, argv[1], argv[2]);
        printf("connect : %p\n", connect);
        pthread_create(p_id + i, NULL, send_process, socket);
    }
    
    for(int i = 0; i < THREAD_NUM; i ++){
        pthread_join(p_id[i], NULL);
    }
}