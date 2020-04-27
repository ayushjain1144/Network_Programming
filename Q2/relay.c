#include "selective_repeat.h"
#include "packet.h"
#include <time.h>
#include <string.h>
#define PDR 0
#define PORT_RELAY1 1234
#define PORT_RELAY2 1235
#define PORT_SERVER 8787
#define MAX_ACK_DELAY 0

// returns 1 if packet should be dropped
int toss()
{
    if(PDR > 1)
    {
        printf("PDR value must be between 0 and 1, got %d\n", PDR);
        exit(1);
    }
    float num = rand() / (float)RAND_MAX;
    if(num < PDR)
        return 1;
    else
        return 0;
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

char* get_sys_time(void)
{
    char* time_str = (char*) malloc(sizeof(char) * 20);
    time_t current_time = time(NULL);
    struct tm* tp = localtime(&current_time);
    struct timeval t;
    gettimeofday(&t, NULL);
    strftime(time_str, 20, "%H:%M:%S", tp);
    char milliseconds[8];
    sprintf(milliseconds, ".%06ld", t.tv_usec);
    strcat(time_str, milliseconds);
    return time_str;
}

int main(void)
{
    srand(time(NULL));
    struct sockaddr_in si_relay1, si_relay2, si_server, si_client, si_other;
    int si_other_len = sizeof(si_other);
    int yes = 1;
    fd_set read_fds, master;
    int fdmax;
    int socket1, socket2, socket_server;
    socket1 = 0;
    socket2 = 0;
    socket_server = 0;

    // creating the sockets
    if((socket1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socket1 failed\n");
        exit(1);
    }
    else
        printf("Socket1 created successfully\n");

    if((socket2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socket2 failed\n");
        exit(1);
    }
    else
        printf("Socket2 created successfully\n");

    // creating the sockets
    if((socket_server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("socket_server failed\n");
        exit(1);
    }
    else
        printf("Socket Server created successfully\n");

    //for reusing the same sockets :)
    if(setsockopt(socket1, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0 )   
    {   
        perror("setsockopt1");   
        exit(EXIT_FAILURE);   
    }

    //for reusing the same sockets :)
    if(setsockopt(socket2, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0 )   
    {   
        perror("setsockopt2");   
        exit(EXIT_FAILURE);   
    }

    if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0 )   
    {   
        perror("setsockopt_server");   
        exit(EXIT_FAILURE);   
    }

     // setting up relay1 address
    memset(&si_relay1, 0, sizeof(si_relay1));
    si_relay1.sin_family = AF_INET;
    si_relay1.sin_port = htons(PORT_RELAY1);
    si_relay1.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if(bind(socket1, (struct sockaddr*)&si_relay1, sizeof(si_relay1)) == -1)
    {
        perror("bind failed");
        exit(2);
    }

    int addrlen_relay1 = sizeof(si_relay1);

    // setting up relay2 address
    memset(&si_relay2, 0, sizeof(si_relay2));
    si_relay2.sin_family = AF_INET;
    si_relay2.sin_port = htons(PORT_RELAY2);
    si_relay2.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if(bind(socket2, (struct sockaddr*)&si_relay2, sizeof(si_relay2)) == -1)
    {
        perror("bind failed");
        exit(2);
    }

    int addrlen_relay2 = sizeof(si_relay2);

    // setting up server address
    memset(&si_server, 0, sizeof(si_server));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORT_SERVER);
    si_server.sin_addr.s_addr = inet_addr("127.0.0.1");

    int addrlen_server = sizeof(addrlen_server);

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // add the sockets to both the sets
    FD_SET(socket1, &master);
    FD_SET(socket2, &master);
    FD_SET(socket_server, &master);

    fdmax = socket2;
    if(socket1 > socket2)
        fdmax = socket1;
    if(socket_server > fdmax)
        fdmax = socket_server;

    //accept and send data
    while(true)
    {
        
        read_fds = master;

        // look for data
        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select\n");
            exit(2);
        }

        // if data for relay from client
        // decide if you want to accept, if yes then send ack (add delay)
        for(int j = 0; j < 2; j++)
        {   
            int i;

            if(j == 0)
                i = socket1;
            else
                i = socket2;

            if(FD_ISSET(i, &read_fds))
            {   
                PACKET p;
                int temp;
                memset(&si_other, 0, si_other_len);
                if((temp = recvfrom(i, &p, sizeof(p), 0, (struct sockaddr*) &si_other,(socklen_t*) &si_other_len)) == -1)
                {
                    printf("Error in recv: %d\n", i);
                    exit(2);
                }
                // closed connection
                else if(temp == 0)
                {
                    printf("Closed connection\n");
                    return 0;
                }
                else
                {

                    //packet can be from server or form client
                    //if data it is from client, just send it to server
                    //else ack, either drop or send it with delay

                    // We have got data, send it to server
                    if(p.isDataNotACK)
                    {
                        // store client info for future use.
                        si_client = si_other;
                        int toss_result = toss();
                        if(toss_result)
                        {
                            printf("OOPS, packet dropped at relay%d\n", j + 1);
                            continue;
                        }

                        printf("Packet recvd at Relay%d  Relay%d  R  %s  DATA  %d  SERVER  RELAY%d\n", j + 1, j + 1, get_sys_time(), p.seqNo, j+1);

                        if(sendto(socket_server, &p, sizeof(p),
                                0, (struct sockaddr*) &si_server, addrlen_server) == -1)
                        {
                            perror("Send failed");
                            exit(2);
                        }
                        else
                            printf("Packet sent at Relay%d  Relay%d  S  %s  DATA  %d  RELAY%d  SERVER\n", j + 1, j + 1, get_sys_time(), p.seqNo, j+1);
                    }
                    
                    // It's ACK!!
                    
                    else
                    {
                        printf("Packet recvd at Relay%d  Relay%d  R  %s  ACK  %d  SERVER  RELAY%d\n", j + 1, j + 1, get_sys_time(), p.seqNo, j+1);
                        
                        usleep((rand() % MAX_ACK_DELAY) * 1000);
                        int addrlen_client = sizeof(si_client);
                        if(sendto(i, &p, sizeof(p),
                                0, (struct sockaddr*) &si_client, addrlen_client) == -1)
                        {
                            perror("Send failed");
                            exit(2);
                        }
                        else
                            printf("Packet sent at Relay%d  Relay%d  S  %s  DATA  %d  RELAY%d  SERVER\n", j + 1, j + 1, get_sys_time(), p.seqNo, j+1);
                    }
                }

            }
        }
        


            

           
    }
    return 0;
}