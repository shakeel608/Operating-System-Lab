#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>

#include "babble_server.h"
#include "babble_types.h"
#include "babble_utils.h"
#include "babble_communication.h"
#include "babble_server_answer.h"
#include "babble_producer_consumer.c"

command_t* command_buffer[BABBLE_PRODCONS_SIZE];


answer_t* answer_buffer[ANS_BUFFER_SIZE];

answer_t* publish_buffer[BABBLE_PRODCONS_SIZE];

sem_t empty,full,mutexlock;

sem_t empty_ans,full_ans,mutexlock_ans;

sem_t empty_pub,full_pub,mutexlock_pub;


int input=0; int output=0;
int input_ans=0; int output_ans=0;
int input_pub=0; int output_pub=0;
int publish_count=0;

void executor_thread_func(void *arg );
void communicator_thread_func(void* fdesc);
void answer_thread_func(void * arg);


static void display_help(char *exec)
{
    printf("Usage: %s -p port_number\n", exec);
}


static int parse_command(char* str, command_t *cmd)
{
    char *name = NULL;

    /* start by cleaning the input */
    str_clean(str);

    /* get command id */
    cmd->cid=str_to_command(str, &cmd->answer_expected);

    switch(cmd->cid){
    case LOGIN:
        if(str_to_payload(str, cmd->msg, BABBLE_ID_SIZE)){
            name = get_name_from_key(cmd->key);
            fprintf(stderr,"Error from [%s]-- invalid LOGIN -> %s\n", name, str);
            free(name);
            return -1;
        }
        break;
    case PUBLISH:
        if(str_to_payload(str, cmd->msg, BABBLE_SIZE)){
            name = get_name_from_key(cmd->key);
            fprintf(stderr,"Warning from [%s]-- invalid PUBLISH -> %s\n", name, str);
            free(name);
            return -1;
        }
        break;
    case FOLLOW:
        if(str_to_payload(str, cmd->msg, BABBLE_ID_SIZE)){
            name = get_name_from_key(cmd->key);
            fprintf(stderr,"Warning from [%s]-- invalid FOLLOW -> %s\n", name, str);
            free(name);
            return -1;
        }
        break;
    case TIMELINE:
        cmd->msg[0]='\0';
        break;
    case FOLLOW_COUNT:
        cmd->msg[0]='\0';
        break;
    case RDV:
        cmd->msg[0]='\0';
        break;
    default:
        name = get_name_from_key(cmd->key);
        fprintf(stderr,"Error from [%s]-- invalid client command -> %s\n", name, str);
        free(name);
        return -1;
    }

    return 0;
}


/* processes the command and eventually generates an answer */
static int process_command(command_t *cmd, answer_t **answer)
{
    int res=0;
    printf("cmd cid %d\n", cmd->cid);
    switch(cmd->cid){
    case LOGIN:
        res = run_login_command(cmd, answer);
        break;
    case PUBLISH:
        res = run_publish_command(cmd, answer);
        break;
    case FOLLOW:
        res = run_follow_command(cmd, answer);
        break;
    case TIMELINE:
        res = run_timeline_command(cmd, answer);
        break;
    case FOLLOW_COUNT:
        res = run_fcount_command(cmd, answer);
        break;
    case RDV:
        res = run_rdv_command(cmd, answer);
        break;
    default:
        fprintf(stderr,"Error -- Unknown command id\n");
        return -1;
    }

    if(res){
        fprintf(stderr,"Error -- Failed to run command ");
        display_command(cmd, stderr);
    }

    return res;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    int portno=BABBLE_PORT;

    int opt;
    int nb_args=1;

    char* recv_buff=NULL;
    int recv_size=0;

    unsigned long client_key=0;
    char client_name[BABBLE_ID_SIZE+1];

    command_t *cmd;
    answer_t *answer=NULL;

    //command semaphores
    sem_init(&empty,0,BABBLE_PRODCONS_SIZE);
    sem_init(&full,0,0);
    sem_init(&mutexlock,0,1);

    //answer semaphores
    sem_init(&empty_ans,0,ANS_BUFFER_SIZE);
    sem_init(&full_ans,0,0);
    sem_init(&mutexlock_ans,0,1);

    //publish semaphores
    sem_init(&empty_pub,0,BABBLE_PRODCONS_SIZE);
    sem_init(&full_pub,0,0);
    sem_init(&mutexlock_pub,0,1);

    while ((opt = getopt (argc, argv, "+p:")) != -1){
        switch (opt){
        case 'p':
            portno = atoi(optarg);
            nb_args+=2;
            break;
        case 'h':
        case '?':
        default:
            display_help(argv[0]);
            return -1;
        }
    }

    if(nb_args != argc){
        display_help(argv[0]);
        return -1;
    }

    server_data_init();

    if((sockfd = server_connection_init(portno)) == -1){
        return -1;
    }

    printf("Babble server bound to port %d\n", portno);

    //Define thread variables
    pthread_t communicator_thread;
    pthread_t executor_thread[BABBLE_EXECUTOR_THREADS];
    pthread_t answer_thread[BABBLE_ANSWER_THREADS];

    // Create Executor Thread
    for(int i = 0; i<BABBLE_EXECUTOR_THREADS; i++) {

        pthread_create(&executor_thread[i], NULL, executor_thread_func, NULL);
    }

    for(int i = 0; i<BABBLE_ANSWER_THREADS; i++) {

      pthread_create(&answer_thread[i], NULL, answer_thread_func, NULL);

    }
    int count =0;
    /* main server loop */
    while(1){

        if((newsockfd= server_connection_accept(sockfd))==-1){
            return -1;
        }


        pthread_create(&communicator_thread, NULL, communicator_thread_func, (void *)newsockfd);

        printf("count: %d\n",count++);
    }
    close(sockfd);
    return 0;
}



void communicator_thread_func(void* arg) {
    // 1. Login client
    // 2. place cmds in buffer
    // 3. unregister client
    command_t *cmd;
    answer_t *answer=NULL;
    char* recv_buff=NULL;
    int recv_size=0;
    char client_name[BABBLE_ID_SIZE+1];
    int newsockfd = (int) arg;//casting thread srg
    unsigned long client_key=0;

    printf("newsockfd :%d\n",newsockfd);

    memset(client_name, 0, BABBLE_ID_SIZE+1);

        if((recv_size = network_recv(newsockfd, (void**)&recv_buff)) < 0){   //reads data from client
            fprintf(stderr, "Error -- recv from client\n");
            close(newsockfd);
            //continue;
        }


        cmd = new_command(0);

        if(parse_command(recv_buff, cmd) == -1 || cmd->cid != LOGIN){       //checks cmd msg if correct
            fprintf(stderr, "Error -- in LOGIN message\n");
            close(newsockfd);
            free(cmd);
            //continue;
        }

        /* before processing the command, we should register the
         * socket associated  with the new client; this is to be done only
         * for the LOGIN command */
        cmd->sock = newsockfd;

        if(process_command(cmd, &answer) == -1){ //process login command
            fprintf(stderr, "Error -- in LOGIN\n");
            close(newsockfd);
            free(cmd);
            //continue;
        }


        /* notify client of registration */
        if(send_answer_to_client(answer) == -1){
            fprintf(stderr, "Error -- in LOGIN ack\n");
            close(newsockfd);
            free(cmd);
            free_answer(answer);
            //continue;
        }
        else{
            free_answer(answer);
        }

        /* let's store the key locally */
        client_key = cmd->key;
        printf("client_key after login%d\n", client_key);

        strncpy(client_name, cmd->msg, BABBLE_ID_SIZE);
        free(recv_buff);
        free(cmd);//vsu


    /* looping on client commands */
        while((recv_size=network_recv(newsockfd, (void**) &recv_buff)) > 0){

            cmd = new_command(client_key);

            printf("client_key in comm thread:%d\n", client_key);

            if(parse_command(recv_buff, cmd) == -1){//process further cmds from registered client
                fprintf(stderr, "Warning: unable to parse message from client %s\n", client_name);
                notify_parse_error(cmd, recv_buff, &answer);
                send_answer_to_client(answer);
                free_answer(answer);
                free(cmd);
            }
            else{
              //Put publish commands in the publish buffer
                if (cmd->cid ==PUBLISH) {

                    produce(&empty_pub,&full_pub,&mutexlock_pub,&publish_buffer,cmd,&input_pub,1);
                }
                //put other commands in command buffer
                else {
                    produce(&empty,&full,&mutexlock,&command_buffer,cmd,&input,1);
              }
                printf("input server %d\n",input);

            }
            free(recv_buff);
        }


        if(client_name[0] != 0){
            cmd = new_command(client_key);
            cmd->cid= UNREGISTER;

            if(unregisted_client(cmd)){
                fprintf(stderr,"Warning -- failed to unregister client %s\n",client_name);
            }
            free(cmd);
        }
}

void executor_thread_func(void *arg){
    //1. retrieves cmds and processes them
    //2. places answer in answer buffer
    printf("exec thread%s\n");
    command_t* cmd;
    long unsigned client_key;
    answer_t *answer=NULL;
    char client_name[BABBLE_ID_SIZE+1];

    while(1) {

      if (publish_count!=0) {

          cmd = consume(&empty_pub,&full_pub,&mutexlock_pub,&publish_buffer,&output_pub,1);

      }

        cmd = consume(&empty,&full,&mutexlock,&command_buffer,&output,1);
        
        printf("output server%d\n",output);
        //printf("cmd cid in exec:%d\n",cmd->cid);

        if(process_command(cmd, &answer) == -1){//run command
            fprintf(stderr, "Warning: unable to process command from client %lu\n", cmd->key);
        }
        free(cmd);

        //put answer in answer buffer

        produce(&empty_ans,&full_ans,&mutexlock_ans,&answer_buffer,answer,&input_ans,2);

        //vsu

    }

}

void answer_thread_func(void *temp) {
    answer_t *ans = NULL;

    while(1) {

          ans = consume(&empty_ans,&full_ans,&mutexlock_ans,&answer_buffer,&output_ans,2);
          //vsu

          if(ans != NULL){//vsu
                if(send_answer_to_client(ans) == -1){
                    fprintf(stderr, "Warning: unable to answer command from client %lu\n", ans->key);
                    free_answer(ans);
                }
          }
    }
}