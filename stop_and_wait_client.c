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
    strcpy(p->payload, payload);
    free(payload);
    return p;
}

void print_packet(PACKET* p)
{
    printf("data: %s\n", p->payload);
    printf("size: %d\n", p->size);
    printf("seqNo: %d\n", p->seqNo);
    printf("isLastPacket: %d\n", p->isLastPacket);
    printf("isDataNotAck: %d\n", p->isDataNotACK);
    printf("channelID: %d\n\n\n", p->channelID);
    return;
}

int main(void)
{
    struct timeval* t = getTimevalStruct();
    struct sockaddr_in serverAddr;
    int socket1, socket2, i;
    PACKET *send_packet1, *send_packet2;
    int yes = 1;
    fd_set write_fds, master;
    int fdmax;

    // opening the file, to transfer
    FILE* fp = fopen(FILE_NAME, "r");

    // creating the sockets
    if((socket1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP)) == -1)
    {
        perror("socket1 failed\n");
        exit(1);
    }
    else
        printf("Socket1 created successfully\n");

    if((socket1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_TCP)) == -1)
    {
        perror("Socket2 failed\n");
        exit(1);
    }
    else
        printf("Socket2 created successfully\n");

    //for reusing the same sockets :)
    if(setsockopt(socket1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0 )   
    {   
        perror("setsockopt1");   
        exit(EXIT_FAILURE);   
    }
    if(setsockopt(socket2, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0 )   
    {   
        perror("setsockopt2");   
        exit(EXIT_FAILURE);   
    }

    // setting up server address
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    //establishing connection

    if(connect(socket1, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Connection 1 failed\n");
        exit(1);
    }
    else
        printf("Connection1 established\n");

    if(connect(socket2, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Connection 2 failed\n");
        exit(1);
    }
    else
        printf("Connection 2 established\n");

    FD_ZERO(&master);
    FD_ZERO(&write_fds);

    // add the sockets to both the sets
    FD_SET(socket1, &master);
    FD_SET(socket2, &master);
    FD_SET(socket1, &write_fds);
    FD_SET(socket2, &write_fds);

    fdmax = socket2;
    if(socket1 > socket2)
        fdmax = socket1;

    // managing the different read fds
    while(true)
    {
        write_fds = master;


        if(select(fdmax + 1, NULL, &write_fds, NULL, &t) == -1)
        {
            perror("select\n");
            exit(2);
        }
    }
}