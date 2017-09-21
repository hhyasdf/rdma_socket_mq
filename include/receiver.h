#ifndef RECIEVER
#define RECIEVER

#include "rdma_socket.h"
#include <pthread.h>


typedef struct Receiver_{
    Socket *listener;
    Queue *recv_queue;                // 用来缓存这个reciver所有接收到的消息
    Queue *socket_queue;

    pthread_cond_t cond;
    pthread_t p_id;
}Receiver;

Receiver *receiver_build();
void reciever_bind(Receiver* re, int port);
Message *receiver_recv(Receiver* re);
void receiver_close(Receiver *re);

#endif