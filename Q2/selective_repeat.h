#ifndef __SELECTIVE_REPEAT
#define __SELECTIVE_REPEAT
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <stdbool.h>
#include "packet.h"

typedef struct WINDOW
{
    PACKET* p;
    bool is_acked;
}WINDOW;

#endif