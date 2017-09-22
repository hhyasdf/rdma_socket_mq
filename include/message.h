#ifndef MSG
#define MSG


#define SND_MORE_FLAG 128

typedef struct Message_{
    void *buffer;
    int flag;
    int length;
}Message;

Message *Message_create(void *buffer, int length, int flag);       // buffer要自己动态分配
bool Message_check_sndmore(Message *msg);
void Message_destroy(Message *msg);


#endif