#include "../include/rdma_socket.h"
#include <stdio.h>

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
        if(send_(connect,msg,MSG_LEN))break;
        printf("%s\n", msg);
    }
    
    close_(connect);
}