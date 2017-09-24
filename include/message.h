#ifndef MSG
#define MSG


#define SND_MORE_FLAG 128

typedef struct Message_{
    void *buffer;
    int flag;
    int length;
    bool if_free_buffer;
    int node_id;
}Message;


Message *Message_create(void *buffer, int length, int flag, bool if_free_buffer = false);
bool Message_check_sndmore(Message *msg);
void Message_destroy(Message *msg);


#endif