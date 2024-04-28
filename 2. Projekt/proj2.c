#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>


FILE *file;
sem_t *main_process;
sem_t *access_A;
sem_t *boarding_sem;
#define EXPECTED_ARGS 6

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

int selection(int upper){
    return (rand() % upper) + 1;
}

void Fnc_sem_init(int L, int Z, sem_t *skibus[], sem_t *lyzar[], int **A, int **on_bus){
    //mainPROCCES
    main_process =  mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, 0, 0); //MAP_ANONYMOUS for some reason my c debugger kept calling error on identifeir map anonymous so i wen to definition and it was this offset
    sem_init(main_process, 1, 0);
    //skibuses
    for(int i = 1; i < Z; i++){
        skibus[i] =  mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, 0, 0); //MAP_ANONYMOUS for some reason my c debugger kept calling error on identifeir map anonymous so i wen to definition and it was this offset
        sem_init(skibus[i], 1, 0);
    }
    //skiers
    for(int i = 1; i < L; i++){
        lyzar[i] =  mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, 0, 0); //MAP_ANONYMOUS for some reason my c debugger kept calling error on identifeir map anonymous so i wen to definition and it was this offset
        sem_init(lyzar[i], 1, 0);
    }       
    // A a on_bus
    access_A = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, -1, 0);
    sem_init(access_A, 1, 1); 
    *A =  mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, 0, 0); //MAP_ANONYMOUS for some reason my c debugger kept calling error on identifeir map anonymous so i wen to definition and it was this offset
    **A = 1;
    *on_bus =  mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, 0, 0); //MAP_ANONYMOUS for some reason my c debugger kept calling error on identifeir map anonymous so i wen to definition and it was this offset
    **on_bus = 0;
    //boarding sem
    boarding_sem =  mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | 0x20, 0, 0); //MAP_ANONYMOUS for some reason my c debugger kept calling error on identifeir map anonymous so i wen to definition and it was this offset
    sem_init(boarding_sem, 1, 0);
}

void Fnc_sem_destroy(int L, int Z, sem_t *skibus[], sem_t *lyzar[], int **A, int **on_bus){
    //mainproc
    munmap(main_process, sizeof(sem_t));
    sem_destroy(main_process);
    //skibuses
    for(int i = 1; i < Z; i++){
        munmap(skibus[i], sizeof(sem_t));
        sem_destroy(skibus[i]);
    }
    //skiers
    for(int i = 1; i < L; i++){
        munmap(lyzar[i], sizeof(sem_t));
        sem_destroy(lyzar[i]);
    }
    // A a on_bus
    munmap(access_A, sizeof(sem_t));
    sem_destroy(access_A);
    munmap(*A, sizeof(int)); 
    munmap(*on_bus, sizeof(int)); 
    //boarding sem
    munmap(boarding_sem, sizeof(sem_t));
    sem_destroy(boarding_sem);
}

void lyzar_process(int *A, int idL, int idZ, int TL, int Z, sem_t *skibus[], int K, int *on_bus, sem_t *lyzar[]) {
    /*• Každý lyžař je jednoznačně identifikován číslem idL, 0<idL<=L
    • Po spuštění vypíše: A: L idL: started
    • Dojí snídani---čeká v intervalu <0,TL> mikrosekund.
    • Jde na přidělenou zastávku idZ.
    • Vypíše: A: L idL: arrived to idZ
    • Čeká na příjezd skibusu
    • Po příjezdu skibusu nastoupí (pokud je volná kapacita)
    • Vypíše: A: L idL: boarding
    • Vyčká na příjezd skibusu k lanovce.
    • Vypíše: A: L idL: going to ski
    • Proces končí*/
    //start
    sem_wait(access_A);
    printf("%d: L %d: started\n", (*A)++, idL);
    sem_post(access_A);
    usleep(selection(TL));    //breakfast
    int Zastavka_lyzare = selection(Z);     //prideleni zastavky
    sem_wait(access_A);
    printf("%d: L %d: arrived to %d\n", (*A)++, idL, Zastavka_lyzare); //dosel na zastavku
    sem_post(lyzar[Zastavka_lyzare]);
    sem_post(access_A);


        sem_wait(skibus[Zastavka_lyzare]);



            sem_wait(access_A);
            printf("%d: L %d: boarding\n", (*A)++, idL);
            sem_post(access_A);
            (*on_bus)++;
            sem_wait(lyzar[Zastavka_lyzare]);//dekrementace lyzaru na zastavce



            //cekani na posledniho
            int posledni = 0;
            sem_getvalue(lyzar[Zastavka_lyzare], &posledni);
            if (posledni == 0){
                sem_post(boarding_sem);
            }


    sem_wait(main_process);
    sem_wait(access_A);
    printf("%d: L %d: going to ski\n", (*A)++, idL);
    sem_post(access_A);
    (*on_bus)--;


}

void skibus_process(int *A, int idZ, int Z, int TB, sem_t *skibus[], int K, int *on_bus, int L, int idL[], sem_t *lyzar[]) {
    /*• Po spuštění vypíše: A: BUS: started
    • (#) idZ=1 (identifikace zastávky)
    • (*) Jede na zastávku idZ---čeká pomocí volání usleep náhodný čas v intervalu <0,TB>.
    • Vypíše: A: BUS: arrived to idZ
    • Nechá nastoupit všechny čekající lyžaře do kapacity autobusu
    • Vypíše: A: BUS: leaving idZ
    • Pokud idZ<Z, tak idZ=idZ+1 a pokračuje bodem (*)
    • Jinak jede na výstupní zastávku---čeká pomocí volání usleep náhodný čas v intervalu <0,TB>.
    • Vypíše: A: BUS: arrived to final
    • Nechá vystoupit všechny lyžaře
    • Vypíše: A: BUS: leaving final
    • Pokud ještě nějací lyžaři čekají na některé ze zastávek/mohou přijít na zastávku, tak pokračuje bodem (#)
    • Jinak vypíše: A: BUS: finish
    • Proces končí*/
    //start
    sem_wait(access_A);
    printf("%d: BUS: started\n", (*A)++);
    sem_post(access_A);
    int pom;
    int zbyvajici = 1;//abych vesel do whilu
    //sem_getvalue(main_process, &main_process_value);
    while (zbyvajici > 0)
    {
        zbyvajici = 0;
        idZ=1;
        while (idZ <= Z) {
            //jede na zastavku
            usleep(selection(TB));
            sem_wait(access_A);
            printf("%d: BUS: arrived to %d\n", (*A)++, idZ);
            sem_post(access_A);
            


            int on_bus_now = (*on_bus);
            //Nechá nastoupit všechny čekající lyžaře do kapacity autobusu
            for (int i = 1; i < Z-on_bus_now; i++)
            {
                sem_post(skibus[idZ]);
            }
            


            //todo wait aby vsichni nastoupili-------------------------------
            int prazdnazastavka;
            sem_getvalue(lyzar[idZ], &prazdnazastavka);
            if(prazdnazastavka != 0){
                sem_wait(boarding_sem);
            }

            //resetnu posty na zastavku
            int reset;
            sem_getvalue(skibus[idZ], &reset);
            for (int i = 1; i < reset; i++)
            {
                sem_wait(skibus[idZ]);
            }


            sem_wait(access_A);
            printf("%d: BUS: leaving %d\n", (*A)++, idZ);
            sem_post(access_A);

            idZ++;
        }
        sem_wait(access_A);
        printf("%d: BUS: arrived to final\n", (*A)++);
        sem_post(access_A);

        //necham vystoupit--------------------------------
        int clearbus = (*on_bus);
        while(clearbus != 0){
            sem_post(main_process);
            clearbus--;
        }


        //vystoupili vsichni?

        sem_wait(access_A);
        printf("%d: BUS: leaving final\n", (*A)++);
        sem_post(access_A);

        for (int j = 1; j <= Z; j++)//mam jet znovu ? 
        {
            sem_getvalue(lyzar[j], &pom);
            printf("%d\n", pom);
            zbyvajici+=pom;
            printf("%d\n", zbyvajici);
        }

    }


    // All stops have been visited, so the process finishes
    sem_wait(access_A);
    printf("%d: BUS: finish\n", (*A)++);
    sem_post(access_A);
}

int main (int argc, char *argv[])
{
    file = fopen("proj2.out", "w");
    // checking arguments are valid START
    int arg_res[EXPECTED_ARGS];
    if (!validateArguments(argc, argv, EXPECTED_ARGS)) {
        exit(1);
    }

    parseArguments(arg_res, argv, EXPECTED_ARGS);
    // checking arguments are
    int L = arg_res[1];
    int idL[L+1];
    for (int i = 1; i<= L; i++) { //lyzari
        idL[i]=i;
    }
    int Z = arg_res[2];
    int idZ[Z+1];
    for (int i = 1; i<= Z; i++) { //zastavky
        idZ[i]=i;
    }
    int K = arg_res[3];          //kapacita
    int TL = arg_res[4];      //time to leave
    int TB = arg_res[5];       //time between
    int *A = NULL;
    int *on_bus = NULL;
    setbuf(file, NULL); //turn off buffer
    sem_t *skibus[Z];
    sem_t *lyzar[L];
    Fnc_sem_init(L, Z, skibus, lyzar, &A, &on_bus);
    srand(time(NULL)); //random numbers
    srand(time(NULL) ^ (getpid()<<16));
    //MAIN PROCESS START
    /*• Proces vytváří ihned po spuštění proces skibus.
    • Dále vytvoří L procesů lyžařů.
    • Každému lyžaři při jeho spuštění náhodně přidělí nástupní zastávku.
    • Poté čeká na ukončení všech procesů, které aplikace vytváří.
    • Jakmile jsou tyto procesy ukončeny, ukončí se i hlavní proces s kódem (exit code) 0.*/
    //skibus
    pid_t skibus_pid = fork();

    if (skibus_pid == 0) {
        skibus_process(A, 1, Z, TB, skibus, K, on_bus, L, idL, lyzar); 
        exit(0);
    } else if (skibus_pid < 0) {
        fprintf(stderr, "fork error"); 
        exit(1);
    }
    //lyzari
        for (int i = 0; i < L; i++) {
        pid_t pid = fork();
        srand(time(NULL) ^ (getpid()<<16));
        if (pid == 0) {
            //srand(getpid()); // Seed random number generator
            lyzar_process(A, i + 1, rand() % Z, TL, Z, skibus, K, on_bus, lyzar); 
            exit(0);
        } else if (pid < 0) {
            fprintf(stderr, "fork error");
            exit(1);
        }
    }


    wait(NULL);
    Fnc_sem_destroy(L, Z, skibus, lyzar, &A, &on_bus);
    fclose(file);
    return 0;
}