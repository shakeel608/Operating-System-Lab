#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>

#include "babble_types.h"


void produce(sem_t* empty,sem_t* full,sem_t* mutexlock,command_t** currentBuff,void* cmd,int* input){
    
    sem_wait(empty);
    sem_wait(mutexlock);

    printf("input: %d\n",*input);

    //printf("currentBuff : %d\n", cmd->cid);
    
    currentBuff[*input] = cmd;

    printf("producing cmd\n");
    
    *input = (*input + 1) % BABBLE_PRODCONS_SIZE;

    sem_post(mutexlock);
    sem_post(full);
    
}

command_t* consume(sem_t* empty,sem_t* full,sem_t* mutexlock,command_t** currentBuff,int* output){
    void* cmd;
    sem_wait(full);
    sem_wait(mutexlock);

    printf("output: %d\n",*output);

    cmd = currentBuff[*output];
    printf("consuming cmd\n");

    *(output) = (*output + 1) % BABBLE_PRODCONS_SIZE;

    sem_post(mutexlock);
    sem_post(empty);  
    
    return cmd;

}
