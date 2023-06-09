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

void gioco(char * griglia, int I, struct dati * dati);

void rimozioneIpc(struct dati * dati, char * griglia, int shmIdD, int shmIdG, int semId, int msqId);

int abbandonoClient(struct dati * dati);

void abbadonoServer();

void sigHandler(int sig);

void sigHandler2(int sig);

int abbandono = 0;


struct dati * dati;
char * griglia;
int shmIdD;
int shmIdG; 
int semId;
int msqId;

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char param1;
    char param2;


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
    shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdD == -1){
        errExit("Server: errore nella creazione shm dati\n");
    }
    dati = (struct dati *)shmat(shmIdD, NULL, 0);
    
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
    shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
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
    msqId = msgget(chiaveMsq, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(msqId == -1 ){
        errExit("Server: errore crezione msg queue\n");
    }

    //-------------------------------------SEMAFORI---------------------------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');
    if(chiaveSem == -1){
        errExit("Server: errore crezione chiave semafori\n");
    }
    semId = semget(chiaveSem, 8, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(semId == -1){
        errExit("Server: errore crezione semafori\n");
    }

    //sem: CLIENT1, CLIENT2, SERVER, B, MUTEX, SINC, INS, TERM
    unsigned short valori[] = {0, 0, 0, 1, 1, 0, 0, 0};
    union semun arg;
    arg.array = valori;
    if (semctl(semId, 0, SETALL, arg) == -1){
        errExit("Server: errore inizializzazione semafori\n");
    }

    //----------------------------------AVVIO GIOCO------------------------------------
    //SINCRONIZZAZIONE SERVER/CLIENTS
    printf("Attesa giocatori...\n");
    //P(sinc) -> attesa client 1
    semOp(semId, SINC, -1);
    
    printf("-> Giocatore 1 arrivato\n");
    //P(sinc) -> attesa client 2
    semOp(semId, SINC, -1);
    printf("-> Giocatore 2 arrivato\n");
    printf("----------------Sincronizzazione avvenuta------------\n");

    while(dati->fineGioco == 0){   
        abbadonoServer();
        fflush(stdout);
        //P(s) -> attesa mossa giocatore, sbloccato da client
        semOp(semId, SERVER, -1);

        fflush(stdout);
        printf("server sbloccato \n");
        //P(mutex)
        if(abbandonoClient(dati) == 1){
            rimozioneIpc(dati, griglia, shmIdD, shmIdG, semId, msqId);
            exit(0);
        }
        semOp(semId, MUTEX, -1);
        gioco(griglia, msqId, dati);
        //V(mutex)
        fflush(stdout);
        semOp(semId, MUTEX, 1);
        //V(INS)
        semOp(semId, INS, 1);
        fflush(stdout);
        //V(B) -> sblocca client
        semOp(semId, B, 1);
        printf("Attesa mossa...\n"); 
    }

    //------------------------------FINE GIOCO-----------------------
    
    printf("gioco finito... attesa disconnessione clients\n");
    
    //-------------------RIMOZIONE IPC-----------------------
    //P(TERM)
    semOp(semId, TERM, -1);
    //P(TERM)
    semOp(semId, TERM, -1);

    rimozioneIpc(dati, griglia, shmIdD, shmIdG, semId, msqId);
    printf("IPC rimosse\n");
}

void gioco(char * griglia, int I, struct dati *dati){
    int colonnaScelta = 0;
    int nColonne = dati->nColonne;
    int nRighe = dati->nRighe;
    size_t mSize = sizeof(struct mossa)-sizeof(long);
    //ricevuta del messaggio
    if (msgrcv(I, &mossa, mSize, 3, IPC_NOWAIT) == -1)
        printf("%s\n", strerror(errno));
    if(mossa.colonnaScelta != -1){
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
    }else{
        printf("turno saltato\n");
    }
}   

void rimozioneIpc(struct dati * dati, char * griglia, int shmIdD, int shmIdG, int semId, int msqId){
    semRemove(semId);
    freeShm(dati);
    freeShm(griglia);
    removeShm(shmIdG);
    removeShm(shmIdD);
    if (msgctl(msqId, IPC_RMID, NULL) == -1)
        errExit("Server: rimozione msg queue fallita");
}

int abbandonoClient(struct dati * dati){
    if(dati->pidClient[CLIENT1] == 0){
        kill(dati->pidClient[CLIENT2], SIGUSR1);
        printf("partita finita per abbandono del giocatore 1\n");
        return 1;
    }else if(dati->pidClient[CLIENT2] == 0){
        kill(dati->pidClient[CLIENT1], SIGUSR1);
        printf("partita finita per abbandono del giocatore 2\n");
        return 1;
    }
    return 0;
}

void abbadonoServer(){
    if (signal(SIGINT, sigHandler) == SIG_ERR){
        printf("errore signal\n");
        errExit("change signal handler failed");
    }
}

void sigHandler(int sig){
    if(abbandono == 0){
        sigset_t mySet;
        sigfillset(&mySet);
        sigdelset(&mySet, SIGINT);
        abbandono = 1;
        printf("\nal prossimo ctrl c il gioco terminerà\n");
        sigsuspend(&mySet);
        printf("\n---gioco terminato---\n");
        

        kill(dati->pidClient[CLIENT1], SIGUSR2);
        kill(dati->pidClient[CLIENT2], SIGUSR2);
        semOp(semId, TERM, -1);
        semOp(semId, TERM, -1);
        rimozioneIpc(dati, griglia, shmIdD, shmIdG, semId, msqId);
        exit(0);
    }
    
}