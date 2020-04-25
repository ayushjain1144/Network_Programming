#include "stop_and_wait.h"
#include "packet.h"
#include <time.h>
#define PORT 1234
#define FILE_NAME "input1.txt"

int seq = 0;
bool is_file_end = false;
bool is_last_packet;

struct timeval* getTimevalStruct ()
{
    struct timeval* t = (struct timeval*) malloc(sizeof(struct timeval));
    t->tv_usec = timeout;
    t->tv_usec = 0;
    return t;
}

// makes packet by reading from file
PACKET* make_packet(FILE* fp, int seqNo, int channelID)
{
    PACKET* p = (PACKET*) malloc(sizeof(PACKET));
    char* payload = (char*) malloc(sizeof(char) * (PACKET_SIZE + 1));
    memset(payload, '\0', PACKET_SIZE + 1);
    int num_read = fread(payload, 1, PACKET_SIZE, fp);

    // less data read than packet size
    if(num_read < PACKET_SIZE)
    {
        printf("inside:  %d\n", num_read);
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
    int socket1, socket2;
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
    while(!is_last_packet)
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
                    printf("packet1 exhausted\n");
                    break;
                }
                if(send(socket1, packet1, sizeof(*packet1), 0) == -1)
                {
                    perror("send1 failed\n");
                    exit(3);
                }
                else
                    printf("Sent PKT: Seq. No %d of size %d Bytes from channel %d\n", packet1->seqNo, packet1->size, packet1->channelID);
                
                if(!is_file_end)
                    packet2 = make_packet(fp, seq++, 1);
                else
                {
                    printf("packet2 exhausted\n");
                    break;
                }

                if(send(socket2, packet2, sizeof(*packet2), 0) == -1)
                {
                    perror("send2 failed\n");
                    exit(3);
                }
                else
                    printf("Sent PKT: Seq. No %d of size %d Bytes from channel %d\n", packet2->seqNo, packet2->size, packet2->channelID);

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
                    state = 3;
                    break;
                }
                
                if(send(socket1, packet1, sizeof(*packet1), 0) == -1)
                {
                    perror("send1 failed\n");
                    exit(3);
                }
                else
                    printf("Sent PKT: Seq. No %d of size %d Bytes from channel %d\n", packet1->seqNo, packet1->size, packet1->channelID);
                
                state = 3;
                break;

            }

            case(2):
            {
                PACKET* packet2;
                if(!is_file_end)
                    packet2 = make_packet(fp, seq++, 1);
                else
                {
                    state = 3;
                    break;
                }
                
                if(send(socket2, packet2, sizeof(*packet2), 0) == -1)
                {
                    perror("send2 failed\n");
                    exit(3);
                }
                else
                    printf("Sent PKT: Seq. No %d of size %d Bytes from channel %d\n", packet2->seqNo, packet2->size, packet2->channelID);

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

                int count= 0;
                //iterating over all the connections
                for(int i = 0; i <= fdmax; i++)
                {
                    
                    if(FD_ISSET(i, &read_fds))
                    {
                        // socket1 has received ack or timeout
                        PACKET p;
                        if(i == socket1)
                        {
                            
                            if(recv(socket1, &p, sizeof(p), 0) == -1)
                            {
                                perror("Receive from socket1 failed");
                                exit(4);
                            }
                            else
                                printf("RECV ACK: Seq. No %d of size %d Bytes from channel %d\n", p.seqNo, p.size, p.channelID);
                            state = 1;
                            count++;

                        }
                        else if(i == socket2)
                        {
                            if(recv(socket2, &p, sizeof(p), 0) == -1)
                            {
                                perror("Receive from socket2 failed");
                                exit(4);
                            }
                            else
                                printf("RECV ACK: Seq. No %d of size %d Bytes from channel %d\n", p.seqNo, p.size, p.channelID);

                            state = 2;
                            count++;
                        }

                        is_last_packet = p.isLastPacket;
                    }
                }
                // both ack's received
                if(count == 2)
                    state = 0;
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