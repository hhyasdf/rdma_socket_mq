#ifndef MSG
#define MSG


#define SND_MORE_FLAG 128

typedef struct AMessage_{
    void *buffer;
    int flag;
    int length;
    bool if_free_buffer;
    int node_id;
}AMessage;


AMessage *AMessage_create(void *buffer, int length, int flag, bool if_free_buffer = false);
bool AMessage_check_sndmore(AMessage *msg);
void AMessage_destroy(AMessage *msg);


#endif