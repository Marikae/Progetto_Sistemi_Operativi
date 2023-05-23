#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>

//nostre librerie
#include "../lib/shared_memory.h"
#include "../lib/errExit.h"
#include "../lib/matrixLib.h"
#include "../lib/semaphore.h"
#include "../lib/mossa.h"

//posizione semafori
#define SERVER 0
#define CLIENT1 1
#define CLIENT2 2
#define B 3
#define MUTEX 4
#define SINC 5
#define INS 6

struct mossa mossa;

void gioca(int nRighe, int nColonne, char * griglia, int msqid);

void giocatore1(int semIdS, int nRighe, int nColonne, char * griglia, int msqid);

void giocatore2(int semIdS, int nRighe, int nColonne, char * griglia, int msqid);

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char * griglia;

    //------------------------------MEMORIA CONDIVISA DATI----------------------------------
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
    //--------------------------------------------------------------

    //------------------------MSG QUEUE---------------------------
    key_t chiaveMsq = ftok("./keys/chiaveMessaggi.txt", 'a');
    int msqid = msgget(chiaveMsq,  S_IRUSR | S_IWUSR);
    if (msqid == -1)
        errExit("msgget failed\n");
    
    //-------------------------SEMAFORI INIT-------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');
    int semIdS = semget(chiaveSem, 7, IPC_CREAT | S_IRUSR | S_IWUSR);
    
    
    //ciclo finchè non termina il gioco (arresa/vittoria/stallo)
    
    //sem: s, c1, c2, b, mutex, s1, s2
    if(dati->gestione[0] == 0){
        dati->gestione[0] = 1;
        giocatore1(semIdS, nRighe, nColonne, griglia, msqid);
    }else if(dati->gestione[0] == 1 && dati->gestione[1] == 0){
        dati->gestione[1] = 1;
        giocatore2(semIdS, nRighe, nColonne, griglia, msqid);
    }else if(dati->gestione[0] == 1 && dati->gestione[1] == 1){
        printf("ci sono già due giocatori!\n");
        exit(0);
    }
}

void giocatore1(int semIdS, int nRighe, int nColonne, char * griglia, int msqid){
    //sem: s, c1, c2, b, mutex, s1, s2
    //V(sinc) -> avvisa il server che è arrivato (sblocca)
    printf("Attesa giocatore 2...\n");
    semOp(semIdS, SINC, 1); 
    
    while(1){
        //P(client1) //all'inzio aspetta il giocatore 2
        printf("Turno del giocatore 2...\n");
        semOp(semIdS, CLIENT1, -1);

        //P(b) -> aspetto server
        semOp(semIdS, B, -1); 
        fflush(stdout);
        //P(mutex)
        semOp(semIdS, MUTEX, -1);

        gioca(nRighe, nColonne, griglia, msqid); //MUTUA

        //V(mutex)
        semOp(semIdS, MUTEX, 1);

        fflush(stdout);
        //printf("Invio al server...\n");
        //V(s) -> invio al server (mss queue)
        semOp(semIdS, SERVER, 1); 
        //P(INS)
        semOp(semIdS, INS, -1);
        stampa(nRighe, nColonne, griglia);
        fflush(stdout);
        //V(c2)
        semOp(semIdS, CLIENT2, 1); 
        fflush(stdout);
    };
}

void giocatore2(int semIdS, int nRighe, int nColonne, char * griglia, int msqid){
    //sem: s, c1, c2, b, mutex, s1, s2
    //V(s2) -> avvisa il server che è arrivato (sblocca)
    semOp(semIdS, SINC, 1);
    printf("giocatore 2 arrivato\n");
    //V(c1) //all'inzio sblocca giocatore 1
    semOp(semIdS, CLIENT1, 1); 
    while(1){
        //sem: s, c1, c2, b, mutex, s1, s2
        //P(c2) -> blocco giocatore 2, sbloccato da giocatore 1
        printf("Turno del giocatore 1...\n");
        fflush(stdout);
        semOp(semIdS, CLIENT2, -1);
        //P(B) -> aspetto server
        semOp(semIdS, B, -1); 
        //P(mutex)
        fflush(stdout);
        semOp(semIdS, MUTEX, -1);
        
        gioca(nRighe, nColonne, griglia, msqid);

        //V(mutex)
        semOp(semIdS, MUTEX, 1);
        //V(s) -> invio al server (mss queue)
        //printf("Invio al server..."); 
        semOp(semIdS, SERVER, 1); 
        //P(INS)
        semOp(semIdS, INS, -1);
        stampa(nRighe, nColonne, griglia);
        //V(c1)
        semOp(semIdS, CLIENT1, 1); 
    }
}

void gioca(int nRighe, int nColonne, char * griglia, int msqid){
    unsigned int colonna = 0;
    stampa(nRighe, nColonne, griglia); //stampa mossa del giocatore precedente

    fflush(stdout);
    do{
        printf("scegli mossa:\n");
        scanf("%i", &colonna);
    }while(!controllo_colonna(colonna, nColonne));

    //invia al server la scelta tramite queue
    printf("hai scelto la colonna: %i \n", colonna);
    
    mossa.mtype = 1;
    // message contains the following numbers
    mossa.colonnaScelta = colonna;
    // size of m is only the size of its mtext attribute!
    size_t mSize = sizeof(mossa) - sizeof(long);
    // sending the message in the queue
    if (msgsnd(msqid, &mossa, mSize, 0) == -1)
        printf("%s", strerror(errno));
    
}