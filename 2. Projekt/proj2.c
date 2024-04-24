#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <ctype.h>
#include <sys/mman.h>

#define WPAUSE 10
#define NUMOFSERVICES 3
sem_t *tobeserved;
sem_t *post_open;
sem_t *worker_rdy;
sem_t *queue[4];
int *cnt;
int *selected_service;
int *queues[4];
int *tmp;
int *NS;
FILE *file;
//bool overtimes = true;

long int arg_check(int arg, int i){
        switch (i)
        {
                case 1:
                        if (arg > 0){ return arg; }//---NumOfCustomers
                        else{  return -1;  }
                case 2:
                        if (arg > 0){ return arg; }//---NumOfWorkers
                        else{  return -1;  }
                case 3:
                        if (0 <= arg && arg <= 10000){ return arg; }//---TtobeServed
                        else{  return -1;  }
                case 4:
                        if (0 <= arg && arg <= 1000){ return arg; }//---TWorkerpause
                        else{  return -1;  }
                case 5:
                        if (0 <= arg && arg <= 10000){ return arg; }//---TPostclosed
                        else{  return -1;  }
                default:
                        return -1;
        }
}

int isNumber(char number[]){
    if (number[0] == '-'){
        fprintf(file, "\nNegative numbers are not allowed");
        return 0;
    }
    for (int i = 0; number[i]!= '\0'; i++)
    {
        if (!isdigit(number[i])) return 0;
    }
    return 1;
}
int selection(int upper, int lower){
    return rand() %(upper - lower + 1) + lower;
}

void Fnc_sem_init(){
    //MAPS
    tobeserved = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    post_open = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    worker_rdy = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    queue[1] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    queue[2] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    queue[3] = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    cnt = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    selected_service = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    queues[1] = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    queues[2] = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    queues[3] = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    *queues[1] = 0;
    *queues[2] = 0;
    *queues[3] = 0;
    tmp = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
    NS = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);


    //SEMS
    sem_init(tobeserved, 1, 1);
    sem_init(post_open, 1, 1);
    sem_init(worker_rdy, 1, 1);
    sem_init(queue[1], 1, 0);
    sem_init(queue[2], 1,0);
    sem_init(queue[3], 1, 0);
   
}
void Fnc_sem_destroy(){
    //MAPS
    munmap(tobeserved, sizeof(sem_t));
    munmap(post_open, sizeof(sem_t));  
    munmap(worker_rdy, sizeof(sem_t)); 
    munmap(queue[1], sizeof(sem_t));
    munmap(queue[2], sizeof(sem_t));  
    munmap(queue[3], sizeof(sem_t));
    munmap(cnt, sizeof(int));   
    munmap(selected_service, sizeof(int));
    munmap(queues[1], sizeof(int));
    munmap(queues[2], sizeof(int));  
    munmap(queues[3], sizeof(int));   
    munmap(tmp, sizeof(int));  
    munmap(NS, sizeof(int));  


    //SEMS
    sem_destroy(queue[1]);
    sem_destroy(queue[2]);
    sem_destroy(queue[3]);
}

int main (int argc, char *argv[])
{
    file = fopen("proj2.out", "w");
    // checking arguments are valid START
    const int expected_args = 6;
    long int arg_res [expected_args];
    if (argc == expected_args )
    {
        for (int i = 1; i < expected_args; i++) {
                if ((isNumber(argv[i])) == 0){
                    printf("\nArgument number: %d is not a number", i);
                    exit(1);
                }

                arg_res[i] = strtol(argv[i], NULL, 10);
                arg_res[i] = arg_check(arg_res[i], i);

                if (arg_res[i] == -1){
                    fprintf(stderr, "\nFailed to load argument number: %d", i);
                    exit(1);
                }
        }
    }else{
        fprintf(stderr, "Invalid number of arguments");
        exit(1);
    }
    int NumOfCustomers = arg_res[1];
    int customersID[NumOfCustomers+1];
    for (int i = 1; i<= NumOfCustomers; i++) {
        customersID[i]=i;
    }
    int NumOfWorkers = arg_res[2];
    int workersID[NumOfWorkers];
    for (int i = 1; i<= NumOfWorkers; i++) {
        workersID[i]=i;
    }
    int TtobeServed = arg_res[3];          //milisec
    int TWorkerpause = arg_res[4];      //milisec
    int TPostclosed = arg_res[5];       //milisec
    //Assinging values DONE
    setbuf(file, NULL); //turn off buffer
    Fnc_sem_init();
    
    int NothingtobeServed = 1;
    int opened_post = 0;
    *NS = 1;
    

    pid_t idZ;
    for (int i = 1; i <= NumOfCustomers; i++) {
        idZ = fork();
        if(idZ == -1) 
            exit(1);
        else if (idZ == 0)
        {
        fprintf(file, "\n%d: Z %d: started", ++(*cnt), customersID[i]);//1: Z x: started
        usleep(selection(TtobeServed, TtobeServed/2));// waits random invetarval
        if(opened_post == 1){

                fprintf(file, "\n%d: Z %d: going home", ++(*cnt), customersID[i]); // going home
                if (i == NumOfCustomers-1) {NothingtobeServed = 0;
                    *NS = 0;
                     exit(0); }
                exit(0);
        }
        //sem_wait(worker_rdy);

            if(opened_post == 1){

                fprintf(file, "\n%d: Z %d: going home", ++(*cnt), customersID[i]); // going home
                if (i == NumOfCustomers) {NothingtobeServed = 0; exit(0); }
                exit(0);
            }else {

                //sem_wait(worker_rdy);
                *selected_service = selection(NUMOFSERVICES, 1);
                fprintf(file, "\n%d: Z %d: entering office for a service %d", ++(*cnt), customersID[i], *selected_service); //entering service
                (*queues[*selected_service]) = *queues[*selected_service] + 1;
                sem_post(tobeserved);
                sem_wait(queue[*selected_service]);
                fprintf(file, "\n%d: Z %d: called by office worker", ++(*cnt), customersID[i]); // called by worker
                usleep(selection(WPAUSE, 0));
                fprintf(file, "\n%d: Z %d: going home", ++(*cnt), customersID[i]);// going home
                if (i == NumOfCustomers) {NothingtobeServed = 0; *NS=0; exit(0); }
                exit(0);
           }           
        }
        continue;
    }
    pid_t idU;
    for (int i = 1; i<=NumOfWorkers; i++) {
        idU = fork();
        if(idU == -1) 
            exit(1);
        else if (idU == 0){
        while (NothingtobeServed == 1 && *NS == 1) 
        {        fprintf(file, "\n%d: U %d: started", ++(*cnt), workersID[i]);//2: U x: started
        sem_post(worker_rdy);
        usleep(selection(WPAUSE, 0));
        
        if ((NothingtobeServed == 0) && (opened_post == 1)){
            i=NumOfWorkers;
            fprintf(file, "\n%d: U %d: going home", ++(*cnt), workersID[i]);
            exit(0);
        }
        if (NothingtobeServed == 0) {
            fprintf(file, "\n%d: U %d: taking a break", ++(*cnt), workersID[i]);//taking break
            usleep(selection(TWorkerpause, 0));//taking break
            fprintf(file, "\n%d: U %d: break finished", ++(*cnt), workersID[i]);//break finished
            i--;
            continue;
        }
        
        sem_wait(tobeserved);
        *selected_service = selection(NUMOFSERVICES, 1); 
       while ((*queues[*selected_service]) <= 0) {
            *selected_service = selection(NUMOFSERVICES, 1); // chossing from queue where are not 0 people
            if(*queues[*selected_service] > 0){
                break;
            }
        }
        (*queues[*selected_service]) = *queues[*selected_service]-1;
        sem_post(queue[*selected_service]);
        fprintf(file, "\n%d: U %d: serving a service of type %d ", ++(*cnt), workersID[i], *selected_service);//2: U x: started
        usleep(selection(WPAUSE, 0));
        fprintf(file, "\n%d: U %d: service finished", ++(*cnt), workersID[i]);//2: U x: started

        continue;


        }
        exit(0);
        }else{
            continue;
        }
        if (*NS == 0) {
            break;
        }
        exit(0);

    }
    
    usleep(selection(TPostclosed, TPostclosed/2)*10);
    sem_post(post_open);
    opened_post = 1;
    sem_wait(worker_rdy);
    sem_wait(tobeserved);
   
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    fprintf(file, "\n%d: closing", ++(*cnt));//.........closing



        
    Fnc_sem_destroy();
    fclose(file);
        return 0;
}