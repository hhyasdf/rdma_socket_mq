#include "../include/receiver.h"
#include "../include/rdma_socket.h"
#include "../include/queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


Receiver *receiver_build() {
    Receiver *n = (Receiver *)malloc(sizeof(Receiver));

    n->listener = NULL;
    n->recv_queue = queue_init();
    n->socket_queue = queue_init();

    n->cond = PTHREAD_COND_INITIALIZER;
    return n;
}

static void *recv_process(void *listen) {
    Socket *l = (Socket *)listen;
    Queue *more_queue = l->more_queue;
    AMessage *msg; 
    while(1) {
        msg = recv_(l);
        if(msg == NULL){
            // printf("more queue length: %d\n", more_queue->node_num);
            queue_push_q(l->receiver->recv_queue, more_queue);
            queue_push_q(l->receiver->recv_queue, l->recv_queue);
            queue_reset(l->recv_queue);
            queue_reset(more_queue);
        
            pthread_cond_signal(&l->receiver->cond);  
            break;
        }

        if (msg->flag == SND_MORE_FLAG) {
            queue_push(l->more_queue, (void *)msg);
        } else {
            if(!queue_if_empty(more_queue)) {
                queue_push(l->more_queue, (void *)msg);
                queue_push_q(l->receiver->recv_queue, more_queue);
                queue_reset(more_queue);
            } else {
                queue_push(l->receiver->recv_queue, (void *)msg);
            }

            pthread_cond_signal(&l->receiver->cond);
        }

        // 想改成一次性将socket的recv_queue 都放进去的操作

        // listen = recv_(l, l->receiver->recv_queue);
        // printf("Success recv!\n");
        // printf("num of recv_queue %p: %d\n", l->receiver->recv_queue, l->receiver->recv_queue->node_num);
        // printf("listen: %p\n", listen);

        // if(listen == NULL) {
        //     break;
        // }
        // pthread_cond_signal(&l->receiver->cond);
    }
}

static void *listen_process(void *re) {
    Socket *listen = NULL;
    pthread_t p_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    while(1) {
        // printf("%s :%d\n", __FILE__, __LINE__);
        listen = accept_(((Receiver *)re)->listener, (Receiver *)re);
        if(listen == NULL) {
            break;
        }
        // printf("%s :%d\n", __FILE__, __LINE__);
        listen->receiver = (Receiver *)re;
        
        queue_push(((Receiver *)re)->socket_queue, static_cast<void *>(listen));
        pthread_create(&p_id, &attr, recv_process, (void *)listen);
    }
}


int receiver_bind(Receiver* re, int port) {
    struct sockaddr_in addr;
    pthread_t p_id;
    int flag = 0;

    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;

    Socket *socket = NULL;

    socket = socket_(RDMA_PS_TCP);
    flag = bind_(socket, &addr, AF_INET);

    listen_(socket, 100);
    re->listener = socket;

    pthread_create(&static_cast<Receiver *>(re)->p_id, NULL, listen_process, (void *)re);
    return flag;
}

AMessage *receiver_recv(Receiver* re) {
    while(1){
        AMessage *msg;
        pthread_mutex_lock(&re->recv_queue->queue_lock);
        if(queue_if_empty(re->recv_queue)) {
            pthread_cond_wait(&re->cond, &re->recv_queue->queue_lock);
        }
        pthread_mutex_unlock(&re->recv_queue->queue_lock);

        msg = static_cast<AMessage *>(queue_pop(re->recv_queue));
        if(msg != NULL)return msg;
    }
}


void receiver_close(Receiver *re) {
    Socket *socket = NULL;
    close_(re->listener);
    while((socket = static_cast<Socket *>(queue_pop(re->socket_queue))) != NULL) {
        close_(socket);
    }
    queue_destroy(re->socket_queue);
    queue_destroy(re->recv_queue);

    pthread_cancel(re->p_id);
}

