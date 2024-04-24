#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread_unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <ctype.h>
#include <sys/mman.h>

FILE *file;

int arg_check(int arg, int i){
        switch (i)
        {
                case 1:
                        if (arg > 0 && arg < 20000){ return arg; }//---L pocet lyzaru
                        else{  fprintf(stderr, "Invalid number of arg: %d", i); return 1;  }
                case 2:
                        if (arg > 0 && arg <= 10){ return arg; }//---Z pocet zastavek
                        else{  fprintf(stderr, "Invalid number of arg: %d", i); return 1;  }
                case 3:
                        if (10 <= arg && arg <= 100){ return arg; }//---K kapacita skibusu
                        else{  fprintf(stderr, "Invalid number of arg: %d", i); return 1;  }
                case 4:
                        if (0 <= arg && arg <= 10000){ return arg; }//---TL time to leave
                        else{  fprintf(stderr, "Invalid number of arg: %d", i); return 1;  }
                case 5:
                        if (0 <= arg && arg <= 1000){ return arg; }//---TB time between stops
                        else{  fprintf(stderr, "Invalid number of arg: %d", i); return 1;  }
                default:
                        return 1;
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

bool validateArguments(int argc, char *argv[], int expected_args) {
    if (argc != expected_args) {
        fprintf(stderr, "Invalid number of arguments\n");
        return false;
    }

    for (int i = 1; i < expected_args; i++) {
        if (!isNumber(argv[i])) {
            fprintf(stderr, "Argument number %d is not a number\n", i);
            return false;
        }
    }

    return true;
}

void parseArguments(int arg_res[], char *argv[], int expected_args) {
    for (int i = 1; i < expected_args; i++) {
        arg_res[i] = strtol(argv[i], NULL, 10);
        arg_res[i] = arg_check(arg_res[i], i);
    }
}

void Fnc_sem_init(){
    //MAPS

    //SEMS
;
   
}
void Fnc_sem_destroy(){
    //MAPS

    //SEMS
}

int main (int argc, char *argv[])
{
    file = fopen("proj2.out", "w");
    // checking arguments are valid START
    const int expected_args = 6;
    long int arg_res[expected_args];

    if (!validateArguments(argc, argv, expected_args)) {
        exit(1);
    }

    parseArguments(arg_res, argv, expected_args);
    // checking arguments are
    int NumOf_L = arg_res[1];
    int L_ID[NumOf_L+1];
    for (int i = 1; i<= NumOf_L; i++) { //lyzari
        L_ID[i]=i;
    }
    int NumOf_Z = arg_res[2];
    int Z_ID[NumOf_Z];
    for (int i = 1; i<= NumOf_Z; i++) { //zastavky
        Z_ID[i]=i;
    }
    int K = arg_res[3];          //kapacita
    int TL = arg_res[4];      //time to leave
    int TB = arg_res[5];       //time between

    setbuf(file, NULL); //turn off buffer
    Fnc_sem_init();
    
  
    


    Fnc_sem_destroy();
    fclose(file);
    return 0;
}