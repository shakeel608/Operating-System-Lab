#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <semaphore.h>

#include "babble_types.h"


void produce(sem_t* empty,sem_t* full,sem_t* mutexlock,void** currentBuff,void* obj,int* input, int opt){
    
    sem_wait(empty);
    sem_wait(mutexlock);

    printf("input: %d\n",*input);

    //printf("currentBuff : %d\n", cmd->cid);
    if(opt == 1){

        currentBuff[*input] = (command_t *)obj;
        printf("producing cmd\n");
        *input = (*input + 1) % BABBLE_PRODCONS_SIZE;
       
    }
    else if(opt == 2){
        currentBuff[*input] = (answer_t *)obj;
        printf("producing ans\n");
        *input = (*input + 1) % ANS_BUFFER_SIZE;
    }

    sem_post(mutexlock);
    sem_post(full);
    
}

void* consume(sem_t* empty,sem_t* full,sem_t* mutexlock,void** currentBuff,int* output, int opt){
    void* obj;
    sem_wait(full);
    sem_wait(mutexlock);

    printf("output: %d\n",*output);

    if(opt == 1){
        obj = (command_t *)currentBuff[*output];
        printf("consuming cmd\n");

        *(output) = (*output + 1) % BABBLE_PRODCONS_SIZE;

    }
    else if(opt == 2){
        obj = (answer_t *)currentBuff[*output];
        printf("consuming ans\n");

        *(output) = (*output + 1) % ANS_BUFFER_SIZE;
    }

    sem_post(mutexlock);
    sem_post(empty);
    
    
    return obj;

}
