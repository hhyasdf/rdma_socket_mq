#include "../include/rdma_socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define MSG_LEN 32
#define MSG "A message from server!@#$%^&*()"
#define MSG_COUNT 10

#define DEFAULT_PORT 40000

int main(int argc, char** argv) {
    int port;
    
// IPv4
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(DEFAULT_PORT);
    addr.sin_family = AF_INET;

// // IPv6
//     struct sockaddr_in6 addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin6_family = AF_INET6;

    Socket *socket, *listen;

    socket = socket_(RDMA_PS_TCP);
    bind_(socket, &addr, AF_INET);

    port = ntohs(rdma_get_src_port(socket->id));
    printf("listen to port %d\n", port);

    listen_(socket, 10);

    while(1) {
        listen = accept_(socket);
        void *buffer = (void *)1;
        
        // sleep(5);          // 测试缓冲计数 达到缓冲最大值时send会阻塞 默认是10
        // printf("ready\n");
        while(buffer != NULL){
            recv_(listen, &buffer);

            if(buffer == NULL){

                printf("stop recv !\n");

                break;
            }
            printf("%s\n", buffer);
            free(buffer);
        }

        printf("\n");

        close_(listen);
    }
}