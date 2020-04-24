#include "stop_and_wait.h"
#include "packet.h"
#include <time.h>
#include <string.h>
#define PDR 0.1
#define PORT 1234
#define FILE_NAME "output.txt"

// builds a buffer to store 10 packets
char* get_buffer()
{
    char* buf = (char*) malloc(sizeof(char) * PACKET_SIZE * 10);
    memset(buf, '\0', sizeof(buf));
    return buf;

}

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

int main(void)
{
    srand(time(NULL));
    struct sockaddr_in si_me, si_other1, si_other2;
    PACKET *packet1, *packet2;
    int master_socket;
    int yes = 1;

    // opening the file, to transfer
    FILE* fp = fopen(FILE_NAME, "w");

    // creating the sockets
    if((master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
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
    si_me.sin_addr.s_addr = inet_addr(INADDR_ANY);

    //bind socket to port
    if(bind(master_socket, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
    {
        perror("binf failed");
        exit(2);
    }

    




}