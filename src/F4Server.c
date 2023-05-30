#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

//librerie sys call
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>

//nostre librerie
#include "../lib/shared_memory.h"
#include "../lib/errExit.h"
#include "../lib/matrixLib.h"
#include "../lib/semaphore.h"
#include "../lib/mossa.h"

#define CLIENT1 0
#define CLIENT2 1
#define SERVER 2
#define B 3
#define MUTEX 4
#define SINC 5
#define INS 6
#define TERM 7

struct mossa mossa;

void gioco(char * griglia, int msqid, struct dati * dati);

void rimozioneIpc(struct dati * dati, char * griglia, int shmIdD, int shmIdG, int semid, int msqid);

int abbandonoClient(struct dati * dati);

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char param1;
    char param2;
    char * griglia;

    controlloInput(argc, argv); //funzione CONTROLLO INPUT 

    //salvataggio degli input - (messo sotto perchè prima controlla e poi salva)
    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    param1 = argv[3][0];
    param2 = argv[4][0];
    
    //--------------------MEMORIA CONDIVISA DATI------------------------------------
    key_t chiaveD = ftok("./keys/chiaveDati.txt", 'a');
    if(chiaveD == -1){
        errExit("Server: errore nella creazione della chiave dei Dati\n");
    }
    size_t sizeMemD = sizeof(struct dati);
    int shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdD == -1){
        errExit("Server: errore nella creazione shm dati\n");
    }
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    
    //riempimento memoria condivisa
    dati->nColonne = nColonne;
    dati->nRighe = nRighe;
    dati->param1 = param1;
    dati->param2 = param2;
    dati->indirizzamento[CLIENT1] = 0;
    dati->indirizzamento[CLIENT2] = 0;
    dati->turno[CLIENT1] = 1;
    dati->turno[CLIENT2] = 0;
    dati->fineGioco = 0;

    //--------------------MEMORIA CONDIVISA DELLA GRIGLIA DI GIOCO-----------------------
    key_t chiaveG = ftok("./keys/chiaveGriglia.txt", 'b');
    if (chiaveG == -1){
        errExit("Server: errore nella creazione della chiave della griglia di gioco\n");
    }
    size_t sizeMemG = nRighe * nColonne * sizeof(char);
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1){
        errExit("Server: Errore creazione shm griglia\n");
    }
    griglia = (char *)shmat(shmIdG, NULL, 0);
    //pulizia iniziale della griglia da caratteri inderiderati
    for(int i = 0; i < nRighe*nColonne; i++){
        griglia[i] = ' ';
    }
    
    //------------------------MSG QUEUE---------------------------
    key_t chiaveMsq = ftok("./keys/chiaveMessaggi.txt", 'a');
    if(chiaveMsq == -1){
        errExit("Server: errore crezione chiave della msg queue\n");
    }
    int msqId = msgget(chiaveMsq, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(msqId == -1 ){
        errExit("Server: errore crezione msg queue\n");
    }

    //-------------------------------------SEMAFORI---------------------------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');
    if(chiaveSem == -1){
        errExit("Server: errore crezione chiave semafori\n");
    }
    int semIdS = semget(chiaveSem, 8, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(semIdS == -1){
        errExit("Server: errore crezione semafori\n");
    }

    //sem: CLIENT1, CLIENT2, SERVER, B, MUTEX, SINC, INS, TERM
    unsigned short valori[] = {0, 0, 0, 1, 1, 0, 0, 0};
    union semun arg;
    arg.array = valori;
    if (semctl(semIdS, 0, SETALL, arg) == -1){
        errExit("Server: errore inizializzazione semafori\n");
    }

    //----------------------------------AVVIO GIOCO------------------------------------
    //SINCRONIZZAZIONE SERVER/CLIENTS
    printf("Attesa giocatori...\n");
    //P(sinc) -> attesa client 1
    semOp(semIdS, SINC, -1);
    printf("-> Giocatore 1 arrivato\n");
    //P(sinc) -> attesa client 2
    semOp(semIdS, SINC, -1);
    printf("-> Giocatore 2 arrivato\n");
    printf("----------------Sincronizzazione avvenuta------------\n");

    while(dati->fineGioco == 0){     
        fflush(stdout);
        //P(s) -> attesa mossa giocatore, sbloccato da client
        semOp(semIdS, SERVER, -1);
        fflush(stdout);
        printf("server sbloccato \n");
        //P(mutex)
        if(abbandonoClient(dati) == 1){
            rimozioneIpc(dati, griglia, shmIdD, shmIdG, semIdS, msqId);
            exit(0);
        }
        semOp(semIdS, MUTEX, -1);

        gioco(griglia, msqId, dati);
        //V(mutex)
        fflush(stdout);
        semOp(semIdS, MUTEX, 1);
        //V(INS)
        semOp(semIdS, INS, 1);
        fflush(stdout);
        //V(B) -> sblocca client
        semOp(semIdS, B, 1);
        printf("Attesa mossa...\n");        
    }

    //------------------------------FINE GIOCO-----------------------
    
    printf("gioco finito... attesa disconnessione clients\n");
    
    //-------------------RIMOZIONE IPC-----------------------
    //P(TERM)
    semOp(semIdS, TERM, -1);
    //P(TERM)
    semOp(semIdS, TERM, -1);

    rimozioneIpc(dati, griglia, shmIdD, shmIdG, semIdS, msqId);
    printf("IPC rimosse\n");
}

void gioco(char * griglia, int msqid, struct dati *dati){
    int colonnaScelta = 0;
    int nColonne = dati->nColonne;
    int nRighe = dati->nRighe;
    size_t mSize = sizeof(struct mossa)-sizeof(long);
    
    //ricevuta del messaggio
    if (msgrcv(msqid, &mossa, mSize, 3, IPC_NOWAIT) == -1)
        printf("%s\n", strerror(errno));
    colonnaScelta = mossa.colonnaScelta;
    int pos = posizione(colonnaScelta, nRighe, nColonne, griglia);

    //Inserimento della pedina nella griglia (if per mettere pedina giusta)
    if(dati->turno[CLIENT1] == 1 && dati->turno[CLIENT2] == 0){  
        inserisci(pos, colonnaScelta, griglia, dati->param1);
    }else if(dati->turno[CLIENT2] == 1 && dati->turno[CLIENT1] == 0){
        inserisci(pos, colonnaScelta, griglia, dati->param2);
    }
    
    //controllo se è la pedina che riempie la matrice
    if(tabella_piena(dati->nRighe, dati->nColonne, griglia) == true){
        printf("tabella piena\n");
        dati->fineGioco = 2;
    }
    
    printf("VITTORIA V: %i\n", vittoria_verticale(pos, nRighe, nColonne, griglia));
    printf("VITTORIA O: %i\n", vittoria_orizzontale(pos, colonnaScelta, nRighe, nColonne, griglia));
    printf("VITTORIA D: %i\n", vittoria_diagonale(pos, colonnaScelta, nRighe, nColonne, griglia));
    printf("PARITA: %i\n", tabella_piena(nRighe, nColonne, griglia));

    dati->fineGioco = fine_gioco(pos, colonnaScelta, nRighe, nColonne, griglia);
}   

void rimozioneIpc(struct dati * dati, char * griglia, int shmIdD, int shmIdG, int semid, int msqid){
    semRemove(semid);
    freeShm(dati);
    freeShm(griglia);
    removeShm(shmIdG);
    removeShm(shmIdD);
    if (msgctl(msqid, IPC_RMID, NULL) == -1)
        errExit("msgctl failed");
}

int abbandonoClient(struct dati * dati){
    if(dati->pidClient[CLIENT1] == 0){
        kill(dati->pidClient[CLIENT2], SIGKILL);
        printf("partita finita per abbandono del giocatore 1\n");
        return 1;
    }else if(dati->pidClient[CLIENT2] == 0){
        kill(dati->pidClient[CLIENT1], SIGKILL);
        printf("partita finita per abbandono del giocatore 2\n");
        return 1;
    }
    return 0;
}