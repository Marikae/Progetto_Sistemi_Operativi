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
//nostre librerie
#include "../lib/shared_memory.h"
#include "../lib/errExit.h"
#include "../lib/matrixLib.h"
#include "../lib/semaphore.h"
#include "../lib/mossa.h"

#define SERVER 0
#define CLIENT1 1
#define CLIENT2 2
#define B 3
#define MUTEX 4
#define SINC 5
#define INS 6 
#define TERM 7


struct mossa mossa;

void gioco(char * griglia, int msqid, struct dati * dati);

void rimozioneIpc(struct dati * dati, char * griglia, int shmIdD, int shmIdG, int semid, int msqid);

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char * param1;
    char * param2; //non serve condividerli per ora
    char * griglia;

    controlloInput(argc, argv); //funzione CONTROLLO INPUT 

    //salvataggio degli input - (messo sotto perchè prima controlla e poi salva)
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
        exit(1);
    }
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    
    dati->nColonne = nColonne;
    dati->nRighe = nRighe;
    dati->param1 = param1;
    dati->param2 = param2;
    dati->gestione[0] = 0;
    dati->gestione[1] = 0;
    dati->fineGioco = 0;
    dati->g1 = 0;
    dati->g2 = 1;

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
    griglia = (char *)shmat(shmIdG, NULL, 0);
    //pulizia iniziale della griglia
    for(int i = 0; i < nRighe*nColonne; i++){
        griglia[i] = ' ';
    }
    
    //------------------------MSG QUEUE---------------------------
    key_t chiaveMsq = ftok("./keys/chiaveMessaggi.txt", 'a');
    int msqId = msgget(chiaveMsq, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(msqId == -1 )
        printf("errore creazione msq\n");

    //-------------------------------------SEMAFORI---------------------------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');
    int semIdS = semget(chiaveSem, 8, IPC_CREAT | S_IRUSR | S_IWUSR);
    //sem: SERVER, CLIENT1, CLIENT2, B, MUTEX, SINC, INS, TERM
    unsigned short valori[] = {0, 0, 0, 1, 1, 0, 0, 0};
    union semun arg;
    arg.array = valori;
    if (semctl(semIdS, 0, SETALL, arg) == -1){
        printf("semctl SETALL\n");
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
        //P(mutex)
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
    if(dati->g1 == 1 && dati->g2 == 0){  
        inserisci(pos, colonnaScelta, griglia, dati->param1);
    }else if(dati->g2 == 1 && dati->g1 == 0){
        inserisci(pos, colonnaScelta, griglia, dati->param2);
    }
    
    
    printf("VITTORIA V: %i\n", vittoria_verticale(pos, nRighe, nColonne, griglia));
    printf("VITTORIA O: %i\n", vittoria_orizzontale(pos, colonnaScelta, nRighe, nColonne, griglia));
    printf("VITTORIA D: %i\n", vittoria_diagonale(pos, colonnaScelta, nRighe, nColonne, griglia));
    printf("PARITA: %i\n", parita(nRighe, nColonne, griglia));

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