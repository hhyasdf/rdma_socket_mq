#include "../include/rdma_socket.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define MSG_LEN 32
#define MSG "A message from client!@#$%^&*()"
#define MSG_COUNT 20
#define THREAD_NUM 2

void *send_process(void *socket){
    Message *msg;

    for(int i = 0; i < MSG_COUNT; i ++) {
        msg = Message_create((void *)MSG, sizeof(MSG), 0);
        printf("Send a message: %s!\n", MSG);
        sleep(5);
        send_((Socket *)socket, msg);
        free(msg);
    }
    close_((Socket *)socket);
}



int main(int argc, char **argv) {
    //int msg_len, 
    Socket *socket, *connect;
    pthread_t p_id[THREAD_NUM];
    
    socket = socket_(RDMA_PS_TCP);

    for(int i = 0; i < THREAD_NUM; i ++){
        connect = connect_(socket, argv[1], argv[2]);
        printf("connect : %p\n", connect);
        pthread_create(p_id + i, NULL, send_process, connect);
    }
    
    // for(int i = 0; i < THREAD_NUM; i ++){
    //     pthread_join(p_id[i], NULL);
    // }
    close_(socket);
}