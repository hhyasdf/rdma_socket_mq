#ifndef MSG
#define MSG

typedef struct Message_{
    void *buffer;
    int flag;
}Message;

void Message_init(Message *msg, void *buffer, int flag);
bool Message_check_sndmore(Message *msg);


#endif