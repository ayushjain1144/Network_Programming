#include "selective_repeat.h"
#include "packet.h"
#include <time.h>
#include<sys/time.h>
#define PORT_RELAY1 1234
#define PORT_RELAY2 1235
#define FILE_NAME "input.txt"
#define WINDOW_SIZE 10

int seq_num = 0;
bool is_file_end = false;
bool is_last_packet;
int last_ack_received = -1;
int last_segment_sent = -1;
int window_border = 0;
int socket1, socket2;
struct sockaddr_in relayAddr1, relayAddr2;
int relayAddr1_len, relayAddr2_len;

struct timeval* getTimevalStruct ()
{
    struct timeval* t = (struct timeval*) malloc(sizeof(struct timeval));
    t->tv_sec = timeout;
    t->tv_usec = 0;
    return t;
}

// makes packet by reading from file
PACKET* make_packet(FILE* fp, int seqNo, int channelID, int last_acked_data_seq)
{
    PACKET* p = (PACKET*) malloc(sizeof(PACKET));
    char* payload = (char*) malloc(sizeof(char) * (PACKET_SIZE));
    memset(payload, '\0', PACKET_SIZE);
    int num_read = fread(payload, 1, PACKET_SIZE - 1, fp);

    // less data read than packet size
    if(num_read < PACKET_SIZE - 1)
    {
        p->isLastPacket = true;
        is_file_end = true;
    }
    else
        p->isLastPacket = false;
    
    p->isDataNotACK = true;
    p->size = (num_read + 1) * sizeof(char);
    p->seqNo = seqNo;
    p->channelID = channelID;
    p->last_acked_data_seq = last_acked_data_seq;
    p->window_size = WINDOW_SIZE / 2;
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

// takes seq no as input and sends packet to appropriate relay
void send_packet(PACKET* w, int seq_no)
{
    int socket_send;
    struct sockaddr_in relayAddr;
    int relayAddr_len;
    char dest[7];
    if(seq_no % 2 == 0)
    {
        socket_send = socket1;
        relayAddr = relayAddr1;
        relayAddr_len = relayAddr1_len;
        strcpy(dest, "relay1");
    }
    else
    {
        socket_send = socket2;
        relayAddr = relayAddr2;
        relayAddr_len = relayAddr2_len;
        strcpy(dest, "relay2");
    }

    if(sendto(socket_send, w, sizeof(*w),
            0, (struct sockaddr*) &relayAddr, relayAddr_len) == -1)
    {
        perror("Send failed");
        exit(2);
    }
    else
        printf("Packet sent at Client  Client  S  %s  DATA  %d  CLIENT  %s\n", get_sys_time(), w->seqNo, dest);
    
}

void slide_window(FILE* fp, WINDOW* window[])
{
    int temp = window_border;
    
    //replace the current border packet with a new packet and send it
    do
    {
        int channelID;
        PACKET* p;
        if(window_border % 2 == 0)
            channelID = 1;
        else
            channelID = 2;
        
        if(!is_file_end)
            p = make_packet(fp, seq_num++, channelID, last_ack_received);
        else
            break;
        window[window_border]->p = p;
        window[window_border]->is_acked = false;

        send_packet(window[window_border]->p, window[window_border]->p->seqNo);

        window_border = (window_border + 1) % WINDOW_SIZE;

        //if next packet is not acked yet, dont slide more
        if(window[window_border]->is_acked == false)
            break;
    } while (window_border != temp);
    
}


int main(void)
{
    struct timeval* t = getTimevalStruct();
    int yes = 1;
    fd_set read_fds, master;
    int fdmax;
    // int state = 0;
    // int last_ack = -1;

    // opening the file, to transfer
    FILE* fp = fopen(FILE_NAME, "r");

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

    // setting up relay addresses
    memset(&relayAddr1, 0, sizeof(relayAddr1));
    relayAddr1.sin_family = AF_INET;
    relayAddr1.sin_port = htons(PORT_RELAY1);
    relayAddr1.sin_addr.s_addr = inet_addr("127.0.0.1");
    relayAddr1_len = sizeof(relayAddr1);

    memset(&relayAddr2, 0, sizeof(relayAddr2));
    relayAddr2.sin_family = AF_INET;
    relayAddr2.sin_port = htons(PORT_RELAY2);
    relayAddr2.sin_addr.s_addr = inet_addr("127.0.0.1");
    relayAddr2_len = sizeof(relayAddr2);


    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    // add the sockets to both the sets
    FD_SET(socket1, &master);
    FD_SET(socket2, &master);

    fdmax = socket2;
    if(socket1 > socket2)
        fdmax = socket1;
    
    bool is_exit = false;
    bool send1_over = false;
    bool send2_over = false;

    // buffer for packet window
    WINDOW* window[WINDOW_SIZE];
    for(int i = 0; i < WINDOW_SIZE; i++)
    {
        window[i]->p = NULL;
        window[i]->is_acked = false;
    }

    // fill the window with packets
    for(int i = 0; i < WINDOW_SIZE; i++)
    {
        int channelID;
        PACKET* p;
        if(i % 2 == 0)
            channelID = 1;
        else
            channelID = 2;
        
        if(!is_file_end)
            p = make_packet(fp, seq_num++, channelID, last_ack_received);
        else
            break;
        window[i]->p = p;
    }

    //send all packets
    for(int i = 0; i < WINDOW_SIZE; i++)
    {
        if(window[i]->p != NULL)
            send_packet(window[i]->p, window[i]->p->seqNo);
    }

    // managing the different read fds
    while(!is_exit && !(send1_over && send2_over))
    {
        read_fds = master;

        // wait for acks on one of socket. 
        // since ack can come from any channel, important thing is to see its ack seq number

        int out;
        t = getTimevalStruct();
        if((out = select(fdmax + 1, &read_fds, NULL, NULL, t)) == -1)
        {
            perror("select\n");
            exit(2);
        }

        //if timeout on whole window, send all the unacked packets.
        if(out == 0)
        {
            //searching for unacked packets
            for(int i = 0; i < WINDOW_SIZE; i++)
            {
                // found unacked packet
                if(window[i]->p != NULL && window[i]->is_acked == false)
                {
                    send_packet(window[i]->p, i);
                }
            }
        }

        //Not a timeout, we received an ACK for some packet
        //iterating over all the connections
        for(int i = 0; i <= fdmax; i++)
        {
            
            if(FD_ISSET(i, &read_fds))
            {
                // socket1 has received ack
                PACKET p;
                if(i == socket1)
                {
                    if(recvfrom(socket1, &p, sizeof(p), 0, (struct sockaddr*) &relayAddr1, (socklen_t*)&relayAddr1_len) == -1)
                    {
                        perror("Receive from socket1 failed");
                        exit(4);
                    }
                    else
                        printf("Ack from relay1  Client  R  %s ACK  %d  RELAY1  CLIENT\n", get_sys_time(), p.seqNo);
                    last_ack_received  = p.seqNo;

                }
                else if(i == socket2)
                {
                    if(recvfrom(socket2, &p, sizeof(p), 0, (struct sockaddr*) &relayAddr2, (socklen_t*)&relayAddr2_len) == -1)
                    {
                        perror("Receive from socket2 failed");
                        exit(4);
                    }
                    else
                        printf("Ack from relay2  Client  R  %s ACK  %d  RELAY2  CLIENT\n", get_sys_time(), p.seqNo);
                    last_ack_received = p.seqNo;
                }

                //update the ack in window
                int window_index = last_ack_received % WINDOW_SIZE;
                window[window_index]->is_acked = true;
                
                //slide the window and all, if needed.
                if(window[window_border]->is_acked)
                    slide_window(fp, window);
            }
        }
    }
}