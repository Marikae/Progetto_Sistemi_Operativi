#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

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

//posizione semafori
#define CLIENT1 0
#define CLIENT2 1
#define SERVER 2
#define B 3
#define MUTEX 4
#define SINC 5
#define INS 6
#define TERM 7

struct mossa mossa;

struct dati * dati;

int semIdS;

void gioca(char * griglia, int msqId, struct dati *dati);

void giocatore1(char * nomeG1, int semIdS, char * griglia, int msqId, struct dati *dati);

void giocatore2(char * nomeG2, int semIdS, char * griglia, int msqId, struct dati *dati);

void rimozioneIpc(struct dati * dati, char * griglia, int shmIdD, int shmIdG, int semid, int msqId);

void sigHandler(int sig);

void sigHandler2(int sig);

void abbandonoClient(struct dati * dati);

void pulisciInput(FILE *const in);

void fineGioco();

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char * griglia;

    //------------------------------MEMORIA CONDIVISA DATI----------------------------------
    ssize_t sizeMemD = sizeof(dati);
    key_t chiaveD = ftok("./keys/chiaveDati.txt", 'a');
    if(chiaveD == -1){
        errExit("Client: creazione chiave dati fallita\n");
    }
    int shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdD == -1){
        errExit("Client: creazione shm dati fallita\n");
    }
    dati = (struct dati *)shmat(shmIdD, NULL, 0);
    nColonne = dati->nColonne;
    nRighe = dati->nRighe;

    //-------------------------------MEMORIA CONDIVISA GRIGLIA-------------------------------
    key_t chiaveG = ftok("./keys/chiaveGriglia.txt", 'b');
    if(chiaveG == -1){
        errExit("Client: creazione chiave griglia fallita\n");
    }
    ssize_t sizeMemG = (nRighe * nColonne) * sizeof(char);
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1){
        errExit("Client: creazione shm griglia fallita\n");
    }
    griglia = (char *)shmat(shmIdG, NULL, 0);

    //------------------------------------MSG QUEUE-------------------------------------------
    key_t chiaveMsq = ftok("./keys/chiaveMessaggi.txt", 'a');
    if(chiaveMsq == -1){
        errExit("Client: creazione chiave msg queue fallita\n");
    }
    int msqId = msgget(chiaveMsq,  S_IRUSR | S_IWUSR);
    if  (msqId == -1){
        errExit("Client: creazione msg queue fallita\n");
    }

    //-----------------------------------SEMAFORI INIT---------------------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');
    if(chiaveSem == -1){
        errExit("Client: creazione chiave semafori fallita\n");
    }
    semIdS = semget(chiaveSem, 8, IPC_CREAT | S_IRUSR | S_IWUSR);
    if  (semIdS == -1){
        errExit("Client: creazione semafori fallita\n");
    }
    
    //indirizzamento dei processi nelle rispettive funzioni
    
    if(dati->indirizzamento[CLIENT1] == 0){//arriva client 1
        dati->indirizzamento[CLIENT1] = 1;
        giocatore1(argv[1], semIdS, griglia, msqId, dati);
    }else if(dati->indirizzamento[CLIENT1] == 1 && dati->indirizzamento[CLIENT2] == 0){ //arriva client 2
        dati->indirizzamento[CLIENT1] = 1;
        giocatore2(argv[1], semIdS, griglia, msqId, dati);
    }else if(dati->indirizzamento[CLIENT1] == 1 && dati->indirizzamento[CLIENT2] == 1){ //arriva client indesiderato
        printf("Ci sono già due giocatori!\n");
        exit(0);
    }
    
    fineGioco();
    //-------------------RIMOZIONE IPC-----------------------
    //V(DISC)
    semOp(semIdS, TERM, 1);
    
}

void giocatore1(char * nomeG1, int semIdS, char * griglia, int msqId, struct dati * dati){
    dati->pidClient[CLIENT1] = getpid(); //pid processo client 1
    //V(sinc) -> avvisa il server che è arrivato (sblocca)
    printf("Giocatore %s: la tua pedina è %c\n", nomeG1, dati->param1);
    printf("Giocatore %s: Attesa giocatore 2...\n", nomeG1);
    semOp(semIdS, SINC, 1); 
    int c2arrivato = 1;
    
    while(dati->fineGioco == 0){
        //P(client1) //all'inzio aspetta il giocatore 2
        abbandonoClient(dati); //se si abbandona mentre è il turno dell'altro giocatore
        semOp(semIdS, CLIENT1, -1);
        if(c2arrivato == 1){
            printf("Giocatore 2 arrivato\n");
            c2arrivato = 0;
        }
        if(dati->fineGioco != 0 ){
            break;
        }

        //P(b) -> aspetto server
        semOp(semIdS, B, -1); 
        fflush(stdout);
        //P(mutex)
        semOp(semIdS, MUTEX, -1);
        
        gioca(griglia, msqId, dati); //MUTUA
        //V(mutex)
        semOp(semIdS, MUTEX, 1);
        fflush(stdout);
        //V(s) -> invio al server (mss queue)
        semOp(semIdS, SERVER, 1); 
        //P(INS)
        semOp(semIdS, INS, -1);
        stampa(dati->nRighe, dati->nColonne, griglia);
        fflush(stdout);
        //V(c2)
        semOp(semIdS, CLIENT2, 1); 
        fflush(stdout);
        if(dati->fineGioco == 0){
            printf("Giocatore %s: Turno del giocatore 2...\n", nomeG1);    
        }
        
    };
    printf("---fine gioco---\n");
    stampa(dati->nRighe, dati->nColonne, griglia);
}

void giocatore2(char * nomeG2, int semIdS, char * griglia, int msqId, struct dati * dati){
    dati->pidClient[CLIENT2] = getpid();
    printf("Giocatore %s: la tua pedina è questa: %c\n", nomeG2, dati->param2);
    //V(s2) -> avvisa il server che è arrivato (sblocca)
    semOp(semIdS, SINC, 1);
    //V(c1) //all'inzio sblocca giocatore 1
    semOp(semIdS, CLIENT1, 1); 

    while(dati->fineGioco == 0){
        //P(c2) -> blocco giocatore 2, sbloccato da giocatore 1
        if(dati->fineGioco == 0)
            printf("Giocatore %s: Turno del giocatore 1...\n", nomeG2);
        fflush(stdout);
        abbandonoClient(dati);
        semOp(semIdS, CLIENT2, -1);        
        if(dati->fineGioco != 0){
            break;
        }
        //P(B) -> aspetto server
        semOp(semIdS, B, -1);
        //P(mutex)
        fflush(stdout);
        semOp(semIdS, MUTEX, -1);
        fflush(stdin);
        gioca(griglia, msqId, dati);
        //V(mutex)
        semOp(semIdS, MUTEX, 1);
        //V(s) -> invio al server (mss queue)
        semOp(semIdS, SERVER, 1); 
        //P(INS)
        semOp(semIdS, INS, -1);
        stampa(dati->nRighe, dati->nColonne, griglia);
        //V(c1)
        semOp(semIdS, CLIENT1, 1); 
    }
    printf("---fine gioco---\n");
    stampa(dati->nRighe, dati->nColonne, griglia);
}

void gioca(char * griglia, int msqId, struct dati * dati){
    int colonna = 0;
    stampa(dati->nRighe, dati->nColonne, griglia); //stampa mossa del giocatore precedente

    fflush(stdout);
    do{
        pulisciInput(stdin);
        printf("scegli mossa:\n");
        scanf("%i", &colonna);
    }while(!controllo_colonna(colonna, dati->nColonne) || colonna_piena(colonna, dati->nRighe, dati->nColonne, griglia));

    //invia al server la scelta tramite queue
    printf("hai scelto la colonna: %i \n", colonna);
    
    mossa.mtype = 3;
    mossa.colonnaScelta = colonna;
    size_t mSize = sizeof(mossa) - sizeof(long);

    //--------------Turno--------------
    if(dati->turno[CLIENT1] == 1){
        dati->turno[CLIENT1] = 0;
        dati->turno[CLIENT2] = 1;
    }else if(dati->turno[CLIENT2] == 1){
        dati->turno[CLIENT2] = 0;
        dati->turno[CLIENT1] = 1;
    }
    //-------------invio messaggio-------------
    if (msgsnd (msqId, &mossa, mSize, 0) == -1)
        printf("%s", strerror(errno));
    
}

void rimozioneIpc(struct dati * dati, char * griglia, int shmIdD, int shmIdG, int semid, int msqId){
    semRemove(semid);
    freeShm(dati);
    freeShm(griglia);
    removeShm(shmIdG);
    removeShm(shmIdD);
    if (msgctl (msqId, IPC_RMID, NULL) == -1)
        errExit("msgctl failed");
}

void abbandonoClient(struct dati * dati){
    if (signal(SIGINT, sigHandler) == SIG_ERR)
        errExit("change signal handler failed");
    if (signal(SIGUSR1, sigHandler2) == SIG_ERR)
        errExit("change signal handler failed");
}

void sigHandler(int sig) {
    printf("\nHai abbandonato la partita\n");
    if(getpid() == dati->pidClient[CLIENT1]){
        dati->pidClient[CLIENT1] = 0;
        fflush(stdout);
    }else if(getpid() == dati->pidClient[CLIENT2]){
        dati->pidClient[CLIENT2] = 0;
        fflush(stdout);
    }
    //V(SERVER)
    semOp(semIdS, SERVER, 1); 
    exit(0);
}

void sigHandler2(int sig) {
    printf("L'altro giocatore ha abbandonato la partita\n");
    exit(0);
}

void pulisciInput(FILE * const in){
    if(in){
        long const descriptor = fileno(in);
        int dummy;
        int flags;
        flags = fcntl(descriptor, F_GETFL);
        fcntl(descriptor, F_SETFL, flags | O_NONBLOCK);
        do{
            dummy = getc(in);
        }while(dummy != EOF);
        fcntl(descriptor, F_SETFL, flags);
    }
}

void fineGioco(){
    if(dati->fineGioco == 2){
        printf("Partita finita in parità!\n");
    }else{
        if(dati->turno[CLIENT1] == 0){
            printf("ha vinto il giocatore 1\n");
        }else{
            printf("ha vinto il giocatore 2\n");
        }
    }
}