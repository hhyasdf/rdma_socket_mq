#include <rdma/rdma_cma.h>
#include <stdint.h>
#include "queue.h"
#include <netdb.h>
#include <pthread.h>
#include "amessage.h"
#include "receiver.h"


#ifndef RDMA_SOCKET
#define RDMA_SOCKET

#define METADATA_ACK 14
#define METADATA_NORMAL 15
#define METADATA_CLOSE 16

#define TIMEOUT_IN_MS 500
#define BLOCKRECV 0
#define NOTBLOCKRECV 1


#define MDBUFFERSIZE 10                // 缓冲区大小


#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

typedef struct Msg_ {
        uint64_t size;
        void *data;
        struct ibv_mr *mr;
        int flag;
}Msg;

typedef struct MetaData_ {
    int type;                              //  METADATA_ACK 和 #define METADATA_NORMAL 和 #define METADATA_CLOSE
    int flag;
    size_t length;                         
    uint64_t msg_addr;
    uint64_t mr_addr;
    uint32_t rkey;
}MetaData;

typedef struct Rinfo_ {
    void *buffer;
    struct ibv_mr *mr;
}Rinfo;

typedef struct Socket_ {
    struct rdma_event_channel *ec;              
    struct rdma_cm_id *id;                       // rdma_create_qp() 需要调用
    struct ibv_pd *pd;                           // ibv_reg_mr() 需要调用
    struct ibv_qp *qp;                           // ibv_post_recv() 需要调用
    
    struct ibv_cq *cq;
    struct ibv_comp_channel *cc;

    Queue *recv_queue;
    Queue *wr_queue;
    Queue *more_queue;

    MetaData *metaData_buffer;

    pthread_t close_pthread;
    pthread_mutex_t close_lock;

    pthread_mutex_t peer_buff_count_lock;
    int peer_buff_count;

    // pthread_mutex_t close_lock;
    // pthread_cond_t close_cond;
    // pthread_barrier_t close_barrier;

    pthread_mutex_t ack_counter_lock;
    pthread_mutex_t metadata_counter_lock;
    unsigned long ack_counter;                   // 收到的ack
    unsigned long metadata_counter;              // 发送的metadata

    int close_flag;

    struct Receiver_ *receiver;

    int node_id;

}Socket;

void die(char *msg);

Socket* socket_(enum rdma_port_space type);
void listen_(Socket *socket_, int backlog);
int connect_(Socket **socket_, char *address, char *port, int node_id);
Socket *accept_(Socket *socket_, struct Receiver_ *receiver);
void bind_(Socket *socket_, void *addr, int protocol);
void close_(Socket *socket_);
AMessage *recv_(Socket *socket_);
Socket *recv_(Socket *socket_, Queue *de_queue);
int send_(Socket *socket_, AMessage *msg);



#endif


