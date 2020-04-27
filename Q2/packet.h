#ifndef __PACKET
#define __PACKET
#include <stdbool.h>
#include<sys/time.h>
#define PACKET_SIZE 100
#define timeout 5

typedef struct PACKET
{
    int size;
    int seqNo;
    int window_size;
    int last_acked_data_seq;
    bool isLastPacket;
    bool isDataNotACK;
    int channelID;
    char payload[PACKET_SIZE];
}PACKET;

void print_packet(PACKET* p);
#endif