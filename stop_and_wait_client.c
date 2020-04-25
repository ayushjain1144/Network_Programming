#include "stop_and_wait.h"
#include "packet.h"
#include <time.h>
#define PORT 1234
#define FILE_NAME "input.txt"

int seq = 0;
bool is_file_end = false;
bool send1_over = false;
bool send2_over = false;

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
    {
        p->isLastPacket = true;
        is_file_end = true;
    }
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
    fd_set read_fds, master;
    int fdmax;
    int state = 0;

    // opening the file, to transfer
    FILE* fp = fopen(FILE_NAME, "r");

    // creating the sockets
    if((socket1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("socket1 failed\n");
        exit(1);
    }
    else
        printf("Socket1 created successfully\n");

    if((socket2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
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
    printf("%d %d\n", socket1, socket2);

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
    FD_ZERO(&read_fds);

    // add the sockets to both the sets
    FD_SET(socket1, &master);
    FD_SET(socket2, &master);

    fdmax = socket2;
    if(socket1 > socket2)
        fdmax = socket1;
    

    // managing the different read fds
    while(!is_file_end && !send1_over && !send2_over)
    {
        read_fds = master;
        switch(state)
        {
            //when both channels are available 
            case 0:
            {
                // make packets and send
                PACKET* packet1, *packet2;
                if(!is_file_end)
                    packet1 = make_packet(fp, seq++, 0);
                else
                {
                    send1_over = true;
                    send2_over = true;
                    break;
                }
                
                if(send(socket1, &packet1, sizeof(&packet1), 0) == -1)
                {
                    perror("send1 failed\n");
                    exit(3);
                }

                if(!is_file_end)
                    packet2 = make_packet(fp, seq++, 1);
                else
                {
                    send2_over = true;
                    state = 3;
                    break;
                }

                if(send(socket2, &packet2, sizeof(&packet2), 0) == -1)
                {
                    perror("send2 failed\n");
                    exit(3);
                }

                // transfer to state 3
                state = 3;
                break;
            }

            case(1):
            {
                // 1 can send, 2 is still waiting
                PACKET* packet1;
                if(!is_file_end)
                    packet1 = make_packet(fp, seq++, 0);
                else
                {
                    send1_over = true;
                    state = 3;
                    break;
                }
                
                if(send(socket1, &packet1, sizeof(&packet1), 0) == -1)
                {
                    perror("send1 failed\n");
                    exit(3);
                }

                state = 3;
                break;

            }

            case(2):
            {
                PACKET* packet2;
                if(!is_file_end)
                    packet2 = make_packet(fp, seq++, 0);
                else
                {
                    send2_over = true;
                    state = 3;
                    break;
                }
                
                if(send(socket2, &packet2, sizeof(&packet2), 0) == -1)
                {
                    perror("send2 failed\n");
                    exit(3);
                }

                state = 3;
                break;
            }

            // both waiting for Ack
            case(3):
            {
                if(select(fdmax + 1, &read_fds, NULL, NULL, t) == -1)
                {
                    perror("select\n");
                    exit(2);
                }

                //iterating over all the connections
                for(int i = 0; i < fdmax; i++)
                {
                    if(FD_ISSET(i, &read_fds))
                    {
                        // socket1 has received ack or timeout
                        if(i == socket1)
                        {
                            // we assume ack is valid. Else modify late
                            char buf[PACKET_SIZE];
                            if(recv(socket1, buf, sizeof(buf), 0) == -1)
                            {
                                perror("Receive from socket1 failed");
                                exit(4);
                            }
                            state = 1;

                        }
                        else if(i == socket2)
                        {
                            char buf[PACKET_SIZE];
                            if(recv(socket2, buf, sizeof(buf), 0) == -1)
                            {
                                perror("Receive from socket2 failed");
                                exit(4);
                            }
                            if(state == 1)
                                state = 0;
                            else
                                state = 2;
                        }

                    }
                }

                break;
            }

            default:
            {
                printf("This should not have happenned: %d\n", state);
                exit(3);
            }
        }
        

    }
}