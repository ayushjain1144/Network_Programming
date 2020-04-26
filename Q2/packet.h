#ifndef __PACKET
#define __PACKET
#include <stdbool.h>
#define PACKET_SIZE 100
#define timeout 2

typedef struct PACKET
{
    int size;
    int seqNo;
    bool isLastPacket;
    bool isDataNotACK;
    int channelID;
    char payload[PACKET_SIZE];
}PACKET;

PACKET* make_packet(FILE* fp, int seqNo, int channelID);
void print_packet(PACKET* p);
#endif