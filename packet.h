#ifndef __PACKET
#define __PACKET
#include <stdbool.h>

typedef struct PACKET
{
    int size;
    int seqNo;
    bool isLastPacket;
    bool isDataNotACK;
    int channelID;
    char* payload;
}PACKET;

#endif