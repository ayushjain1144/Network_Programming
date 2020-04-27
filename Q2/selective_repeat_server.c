#include "selective_repeat.h"
#include "packet.h"
#include <time.h>
#include <string.h>
#define PDR 0.1
#define PORT 8787
#define FILE_NAME "output.txt"
#define BUFFER_SIZE 10
bool is_last_packet = false;

char* buffer[BUFFER_SIZE];

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

PACKET* make_ack_packet(int seqNo, int channelID, bool isLastPacket)
{
    PACKET* p = (PACKET* ) malloc(sizeof(PACKET));
    strcpy(p->payload, "ACK");
    p->channelID = channelID;
    p->isDataNotACK = false;
    p->isLastPacket = isLastPacket;
    p->seqNo = seqNo;
    p->size = sizeof(p->payload);
    is_last_packet = isLastPacket;
    return p;
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

// NULLify the buffer
void initialise_buffer(void)
{
    for(int i = 0; i < BUFFER_SIZE; i++)
        buffer[i] = NULL;
    return;
}

void write_buffer(FILE* fp)
{
    int i = 0;
    while(buffer[i] != NULL)
    {
        fwrite(buffer[i], 1, strlen(buffer[i]), fp);
        free(buffer[i]);
        buffer[i] = NULL;
        i++;
    }
}

// returns -1 if buffer is already full at that place, otherwise 0
int buffer_manager(FILE* fp, PACKET* p)
{
    int eff_seq_no = p->seqNo % BUFFER_SIZE;
    
    // buffer is not empty at that place
    if(buffer[eff_seq_no] != NULL)
    {
        return -1;
    }

    // buffer at that position is empty, so put your packet
    buffer[eff_seq_no] = (char*) malloc(sizeof(p->payload));
    strcpy(buffer[eff_seq_no], p->payload);

    // check if buffer is full
    for(int i = 0; i < BUFFER_SIZE; i++)
    {
        // buffer is not full, our job is done
        if(buffer[i] == NULL)
            return 0;
    }

    //buffer is full, so write to file
    printf("Buffer became full\n");
    write_buffer(fp);
    return 0;
}

int main(void)
{
    srand(time(NULL));
    initialise_buffer();
    struct sockaddr_in si_me, si_other;
    int si_other_len = sizeof(si_other);
    int master_socket;
    int yes = 1;
    
    // opening the file, to transfer
    FILE* fp = fopen(FILE_NAME, "w");

    // creating the sockets
    if((master_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
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

     // setting up server address
    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    //bind socket to port
    if(bind(master_socket, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
    {
        perror("bind failed");
        exit(2);
    }


    while(true)
    {
                
        PACKET p;
        int temp;
        if((temp = recvfrom(master_socket, &p, sizeof(p), 0, (struct sockaddr*) &si_other, (socklen_t*) &si_other_len)) == -1)
        {
            printf("Error in recv: %d\n", master_socket);
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
            
            int out = buffer_manager(fp, &p);
            if(out == -1)
            {
                printf("Duplicate Packet recvd at Server  Server  R  %s  DATA  %d  RELAY%d  SERVER. Ignored\n", get_sys_time(), p.seqNo, p.channelID);
                continue;
            }

            printf("Packet recvd at Server  Server  R  %s  DATA  %d  RELAY%d  SERVER\n", get_sys_time(), p.seqNo, p.channelID);
            PACKET* packet = make_ack_packet(p.seqNo, p.channelID, p.isLastPacket);
            if(sendto(master_socket, packet, sizeof(*packet),
                                0, (struct sockaddr*) &si_other, si_other_len) == -1)
            {
                perror("Send failed");
                exit(2);
            }
            else
                printf("ACK sent at Server  Server  S  %s  ACK  %d  SERVER  RELAY%d\n", get_sys_time(), p.seqNo, p.channelID);
        }        
              
    }
    write_buffer(fp);
    fclose(fp);
    return 0;
}


//TO DO
// Modify MAKE_ACK_PACKET
// Moidfy MANAGE_BUFFER: Implement a sliding window buffer here.