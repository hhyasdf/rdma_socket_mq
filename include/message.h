#ifndef MSG
#define MSG

typedef struct Message_{
    void *buffer;
    int flag;
    int length;
}Message;

Message *Message_create(void *buffer, int length, int flag);
bool Message_check_sndmore(Message *msg);
void Message_destroy(Message *msg);


#endif