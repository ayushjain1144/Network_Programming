#include "selective_repeat.h"
#include "packet.h"
#include <time.h>
#include <string.h>
#define PDR 0.1
#define PORT_RELAY1 1234
#define PORT_SERVER 8787

// returns 1 if packet should be dropped
int toss()
{
    if(PDR > 1)
    {
        printf("PDR value must be between 0 and 1, got %f\n", PDR);
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


int main(void)
{
    srand(time(NULL));
    struct sockaddr_in si_me, si_server;
    int master_socket;
    int yes = 1;
    fd_set read_fds, master_fds;
    int max_fd;
    int socket1, socket2;
    socket1 = 0;
    socket2 = 0;

    // creating the sockets
    if((master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_UDP)) == -1)
    {
        perror("master socket failed\n");
        exit(1);
    }
    else
        printf("Master Socket created successfully\n");

    //for reusing the same sockets :)
    if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0 )   
    {   
        perror("setsockopt1");   
        exit(EXIT_FAILURE);   
    }

     // setting up my(relay) address
    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT_RELAY1);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if(bind(master_socket, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
    {
        perror("bind failed");
        exit(2);
    }

    int addrlen_me = sizeof(si_me);

    // setting up server address
    memset(&si_server, 0, sizeof(si_me));
    si_server.sin_family = AF_INET;
    si_server.sin_port = htons(PORT_SERVER);
    si_server.sin_addr.s_addr = inet_addr("127.0.0.1");

    //accept connections and data
    while(true)
    {
        FD_ZERO(&master_fds);
        FD_ZERO(&read_fds);
        max_fd = master_socket;
        // add the sockets to both the sets
        FD_SET(master_socket, &master_fds);
        FD_SET(socket1, &master_fds);
        FD_SET(socket2, &master_fds);
        read_fds = master_fds;
        
        if(socket1 > max_fd)
            max_fd = socket1;
        if(socket2 > max_fd)
            max_fd = socket2;


        if(select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select\n");
            exit(2);
        }

        
        // obviously a request for connection
        if(FD_ISSET(master_socket, &read_fds))
        {
            int new_socket;
            if((new_socket = accept(master_socket, (struct sockaddr*) &si_me, (socklen_t*)&addrlen)) == -1)
            {
                perror("accept failed");
                exit(2);
            }

            printf("Accepted new connection from socket: %d, IP: %s, Port: %d\n", new_socket, inet_ntoa(si_me.sin_addr), ntohs(si_me.sin_port));

            if (socket1 == 0)
                socket1 = new_socket;
            else
                socket2 = new_socket;    
        }

        // must be data on some port
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
                if((temp = recv(i, &p, sizeof(p), 0)) == -1)
                {
                    printf("Error in recv: %d\n", i);
                    exit(2);
                }
                // closed connection
                else if(temp == 0)
                {
                    printf("Closed connection\n");
                    write_buffer(fp);
                    fclose(fp);
                    return 0;
                }
                else
                {
                    int toss_result = toss();
                    if(toss_result)
                    {
                        printf("OOPS, packet dropped at server\n");
                        continue;
                    }
                    printf("RECV PKT: Seq. No %d of size %d Bytes from channel %d\n", p.seqNo, p.size, p.channelID);
                    //print_packet(&p);
                    int out = buffer_manager(fp, &p);
                    if(out == -1)
                    {
                        printf("Buffer full. Packet with seq %d from channel %d is out of order and cannot fit buffer. Dropped\n", p.seqNo, p.channelID);
                        continue;
                    }
                    PACKET* packet = make_ack_packet(p.seqNo, p.channelID, p.isLastPacket);
                    if(send(i, packet, sizeof(*packet), 0) == -1)
                    {
                        perror("send failed\n");
                        exit(3);
                    }
                    else
                        printf("Sent ACK: Seq. No %d of size %d Bytes from channel %d\n", packet->seqNo, packet->size, packet->channelID);

                }

            }
        }
            
            

           
    }
    return 0;
}