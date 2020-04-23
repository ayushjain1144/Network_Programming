#include "stop_and_wait.h"
#include "packet.h"
#include <time.h>
#define PORT 1234
#define FILE_NAME "input.txt"

struct timeval* getTimevalStruct ()
{
    struct timeval* t = (struct timeval*) malloc(sizeof(struct timeval));
    t->tv_usec = timeout;
    t->tv_usec = 0;
}

// makes packet by reading from file
PACKET* make_packet(FILE* fp, int seqNo, int channelID)
{
    PACKET* p = (PACKET*) malloc(sizeof(PACKET));
    char* payload = (char*) malloc(sizeof(char) * (PACKET_SIZE + 1));
    memset(payload, '\0', PACKET_SIZE + 1);
    int num_read = fread(payload, PACKET_SIZE, 1, fp);

    // less data read than packet size
    if(num_read < PACKET_SIZE)
        p->isLastPacket = true;
    else
        p->isLastPacket = false;
    
    p->isDataNotACK = true;
    p->size = num_read * sizeof(char);
    p->seqNo = seqNo;
    p->channelID = channelID;
    p->payload = payload;

    return p;
}

void print_packet(PACKET* p)
{
    printf("data: %s\n", p->payload);
    printf("size: %d\n", p->size);
    printf("seqNo: %d\n", p->seqNo);
    printf("isLastPacket: %d\n", p->isLastPacket);
    printf("isDataNotAck: %d\n", p->isDataNotACK);
    printf("channelID: %d\n\n\n", p->payload);
    return;
}

int main(void)
{
    struct timeval* t = getTimevalStruct();
    struct sockaddr_in si_other;
    int socket1, socket2, i;
    int slen = sizeof(si_other);
    PACKET *send_packet1, *send_packet2;

    // opening the file, to transfer
    FILE* fp = fopen(FILE_NAME, 'r');
    if((socket1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP)) == -1)
    {
        perror("socket1 failed");
        exit(1);
    }
    



}