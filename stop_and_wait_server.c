#include "stop_and_wait.h"
#include "packet.h"
#include <time.h>
#include <string.h>
#define PDR 0.1

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
    

    
}