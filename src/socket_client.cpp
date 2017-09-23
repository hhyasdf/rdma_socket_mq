#include "../include/rdma_socket.h"
#include <stdio.h>
#include <stdlib.h>

#define MSG_LEN 32
#define MSG "A message from client!@#$%^&*()"
#define MSG_COUNT 20

int main(int argc, char **argv) {
    //int msg_len, 
    Socket *socket, *connect;
    char ch;
    
    socket = socket_(RDMA_PS_TCP);

    connect = connect_(socket, argv[1], argv[2]);
    int i;
    char msg[31];
    memcpy(msg,MSG,MSG_LEN);
    for(; 1;){
        printf("Waiting for send command\n");
        scanf("%c",&ch);
        while(ch=='\n') scanf("%c",&ch);
        // send msg
        Message *msg;
        msg = Message_create((void *)MSG, sizeof(MSG), 0);
        if(send_(connect, msg))break;
        printf("%s\n", msg->buffer);
        free(msg);
    }
    
    close_(connect);
}