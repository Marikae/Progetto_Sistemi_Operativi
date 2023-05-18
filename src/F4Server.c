#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>

//nostre librerie
#include "../lib/shared_memory.h"
#include "../lib/errExit.h"
#include "../lib/matrixLib.h"
#include "../lib/semaphore.h"

struct dati;
union semun;

void guida(){
    printf("Inserimento non valido!\nInput atteso: ./F4Server righe colonne param1 param2\nN.B. il numero delle righe e delle colonne deve essere maggiore o uguale a 5\n");
}

void controlloInput(int argc, char * argv[]){ 
    if(argc != 5 || atoi(argv[1]) < 5 || atoi(argv[2]) < 5){
        guida();
        exit(1);
    }
}


int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char *param1, *param2; //non serve condividerli per ora
    char *griglia;
    
    controlloInput(argc, argv); //funzione CONTROLLO INPUT 

    //salvataggio degli input
    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    param1 = argv[3];
    param2 = argv[4];
    
    //--------------------MEMORIA CONDIVISA DATI------------------------------------
    key_t chiaveD = ftok("./keys/chiaveDati.txt", 'a');
    if(chiaveD == -1){
        printf("Server: errore nella creazione della chiave dei Dati\n");
        exit(1);
    }
    size_t sizeMemD = sizeof(struct dati);
    
    int shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdD == -1){
        printf("Server: errore nella creazione della shm dei dati (shmget)\n");
        //exit(1);
    }
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    dati->nColonne = nColonne;
    dati->nRighe = nRighe;
    dati->gestione[0] = 0;
    dati->gestione[1] = 0;

    //--------------------MEMORIA CONDIVISA DELLA GRIGLIA DI GIOCO-----------------------
    key_t chiaveG = ftok("./keys/chiaveGriglia.txt", 'b');
    if (chiaveG == -1){
        printf("Server: errore nella creazione della chiave della griglia di gioco\n");
        exit(1);
    }
    
    size_t sizeMemG = nRighe * nColonne * sizeof(char);
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1){
        printf("Server: Errore creazione shm griglia (shmget)\n");
        exit(1);
    }
    //allaccio memoria
    griglia = (char *)shmat(shmIdG, NULL, 0);
    //-----------------------------------------------------------------------------------------
    //pulizia iniziale della griglia
    for(int i = 0; i < nRighe*nColonne; i++){
        griglia[i] = ' ';
    }
    

    //-------------------------------------SEMAFORI---------------------------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');

    int semIdS = semget(chiaveSem, 8, IPC_CREAT | S_IRUSR | S_IWUSR);
    //sem: s, c1, c2, b, mutex, s1, s2, sp
    int valori[] = {0, 0, 0, 0, 0, 0, 0, 0};
    union semun arg;
    arg.array = valori;

    //inizializza i semafori
    if (semctl(semIdS, 0, SETALL, arg) == -1){
        printf("semctl SETALL\n");
    }

    //sem: s, c1, c2, b, mutex, s1, s2
    //P(s1) -> attesa client 1
    printf("Attesa giocatori...\n");
    semOp(semIdS, 5, -1);
    printf("giocatore 1 arrivato\n");

    //P(s2) -> attesa client 2
    semOp(semIdS, 6, -1);
    //semOp(semIdS, 7, -1);
    printf("giocatore 2 arrivato\n");

    while(1){
        //P(s) -> attesa mossa giocatore, sbloccato da client
        semOp(semIdS, 0, -1);
        printf("mossa scelta\n");
        
        //P(mutex)
        semOp(semIdS, 4, -1);
        printf("pedina inserita correttamente\n"); //inserimento nella tabella

        //V(mutex)
        semOp(semIdS, 4, 1);
        
        //V(B) -> sblocca client
        semOp(semIdS, 3, 1);
        printf("attesa mossa...\n");
    }

    //-------------------CHIUSURA MEM------------------

    //shm dati chiusa alla fine del collegamento (per ora)
        if(shmctl(shmIdD, IPC_RMID, 0) == -1)
            printf("Server: rimozione della shm dati fallita\n");
    //chiusura shm griglia a conclusione
        if(shmctl(shmIdG, IPC_RMID, 0) == -1)
            printf("Server: rimozione della shm griglia fallita\n");
    
    
}
