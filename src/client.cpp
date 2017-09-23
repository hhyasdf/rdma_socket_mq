#include "../include/rdma_socket.h"
#include <stdio.h>
#include <unistd.h>

#define MSG_LEN 32
#define MSG "A message from client!@#$%^&*()"
#define MSG_COUNT 3
#define THREAD_NUM 3

void *send_process(void *socket){
    Message *msg;

    for(int i = 0; i < MSG_COUNT; i ++) {
        msg = Message_create((void *)MSG, sizeof(MSG), 0);
        printf("Send a message: %s!\n", MSG);
        send_((Socket *)socket, msg);
        Message_destroy(msg);
    }
    close_((Socket *)socket);
}



int main(int argc, char **argv) {
    //int msg_len, 
    Socket *socket, *connect;
    char ch;
    pthread_t p_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    socket = socket_(RDMA_PS_TCP);

    connect = connect_(socket, argv[1], argv[2]);
    for(int i = 0; i < THREAD_NUM; i ++){
        pthread_create(&p_id, &attr, send_process, connect);
    }
    
    close_(connect);
}