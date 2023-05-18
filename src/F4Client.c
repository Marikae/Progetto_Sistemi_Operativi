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

void gioca();

void giocatore1(int semIdS);

void giocatore2(int semIdS);

void stampaMatrice(int nRighe, int nColonne, char * griglia){
    int indice = 0;
    for(int i = 0; i < nColonne; i++){
        for(int j = 0; j < nRighe; j++){
            printf("|%c", griglia[indice]);
            indice++;
        }
        printf("|\n");
    }
}

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char * griglia;
    int gestione[2];

    //---------------------MEMORIA CONDIVISA DATI----------------------------------
    ssize_t sizeMemD = sizeof(struct dati);
    key_t chiaveD = ftok("./keys/chiaveDati.txt", 'a');
    if(chiaveD == -1){
        printf("Client: creazione chiave dati fallita\n");
        exit(1);
    }

    int shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdD == -1){
        printf("Client: creazione shm dati fallita\n");
        exit(1);
    }
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    nColonne = dati->nColonne;
    nRighe = dati->nRighe;
    gestione[0] = dati->gestione[0];
    gestione[1] = dati->gestione[1];
    
    //-------------------------------MEMORIA CONDIVISA GRIGLIA-------------------------------
    key_t chiaveG = ftok("./keys/chiaveGriglia.txt", 'b');
    if(chiaveG == -1){
        printf("Client: creazione chiave griglia fallita\n");
        exit(1);
    }
    ssize_t sizeMemG = (nRighe * nColonne) * sizeof(char);
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1){
        printf("Client: creazione shm griglia fallita\n");
        exit(1);
    }
    griglia = (char *)shmat(shmIdG, NULL, 0);
    //-------------------------------------------------------------------------------------------

    stampaMatrice(nRighe, nColonne, griglia); //stampa vuoto
    
    
    //-------------------------SEMAFORI INIT-------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');
    int semIdS = semget(chiaveSem, 8, IPC_CREAT | S_IRUSR | S_IWUSR);
    
    
    //ciclo finchè non termina il gioco (arresa/vittoria/stallo)
    //giocatore1 e giocatore2
    
    //sem: s, c1, c2, b, mutex, s1, s2
    if(gestione[0] == 0){
        gestione[0] = 1;
        giocatore1(semIdS);
    }else if(gestione[0] == 1 && gestione[1] == 0){
        gestione[1] = 1;
        giocatore2(semIdS);
    }
}

void giocatore1(int semIdS){
    //sem: s, c1, c2, b, mutex, s1, s2
    //V(s1) -> avvisa il server che è arrivato (sblocca)
    semOp(semIdS, 5, 1); 
    printf("v(s1)\n");

    while(1){
        //P(c1) //all'inzio aspetta il giocatore 2
        printf("Attesa giocatore 2...\n");
        semOp(semIdS, 1, -1); 
        //P(b) -> aspetto server
        printf("Attesa server...\n");
        semOp(semIdS, 3, -1); 
        //P(mutex)
        semOp(semIdS, 4, -1);
        printf("Scegli una mossa (pmutex)\n"); 
        gioca();
        //V(mutex)
        semOp(semIdS, 4, 1);
        printf("Invio al server...\n");
        //V(s) -> invio al server (mss queue)
        semOp(semIdS, 0, 1); 
        printf("Attesa giocatore 2...\n");
        //V(c2)
        semOp(semIdS, 2, 1); 
    }
}

void giocatore2(int semIdS){
    //sem: s, c1, c2, b, mutex, s1, s2
    //V(s2) -> avvisa il server che è arrivato (sblocca)
    semOp(semIdS, 6, 1);
    //semOp(semIdS, 7, 1); 

    printf("giocatore 2 arrivato\n");
    //V(c1) //all'inzio sblocca giocatore 2
    semOp(semIdS, 1, 1);
    printf("Turno del giocatore 1...");
    while(1){
        //P(c2) -> blocco giocatore 2, sbloccato da giocatore 1
        printf("Attesa giocatore 1...");
        semOp(semIdS, 2, -1);
        //P(b) -> aspetto server
        printf("Attesa server...");
        semOp(semIdS, 3, -1); 
        //P(mutex)
        semOp(semIdS, 4, -1);
        printf("Tocca a te, scegli una colonna..."); 
        gioca();
        //V(mutex)
        semOp(semIdS, 4, 1);
        //V(s) -> invio al server (mss queue)
        printf("Invio al server..."); 
        semOp(semIdS, 0, 1); 
        //V(c1)
        printf("Attesa giocatore 1...");
        semOp(semIdS, 1, 1); 
    }
}

void gioca(){
    int colonna = 0;
    printf("scegli mossa:\n");
    scanf("%i", &colonna);
    //invia al server la scelta tramite queue
    printf("hai scelto la colonna: %i \n", colonna);
}