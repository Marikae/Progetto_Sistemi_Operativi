#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

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
#define WAIT 1
//variabili globali
struct mossa mossa;
struct dati * dati;
char * griglia;
int shmIdD;
int shmIdG; 
int semId;
int msqId;
int abbandono = 0;
int fileDes[2];
int n;
struct mossa mossa;

//funzioni del programma
void gioco();
void giocoAutomatico();
void rimozioneIpc();
int abbandonoClient();
void generaMossa();
void abbadonoServer();
void sigHandlerServer(int sig);
void sigHandler2(int sig);

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char pedina1;
    char pedina2;
    controlloInput(argc, argv); //funzione CONTROLLO INPUT 
    //salvataggio degli input - (messo sotto perchè prima controlla e poi salva)
    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    pedina1 = argv[3][0];
    pedina2 = argv[4][0];
    //-----------------------------------MEMORIA CONDIVISA DATI------------------------------------------
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
    dati->pedina[CLIENT1] = pedina1;
    dati->pedina[CLIENT2] = pedina2;
    dati->indirizzamento[CLIENT1] = 0;
    dati->indirizzamento[CLIENT2] = 0;
    dati->turno[CLIENT1] = 1;
    dati->turno[CLIENT2] = 0;
    dati->fineGioco = 0;
    dati->giocoAutomatico = 0;
    //---------------------------MEMORIA CONDIVISA DELLA GRIGLIA DI GIOCO--------------------------------
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
    //-------------------------------------------PIPE---------------------------------------------------
    n = (nColonne * nRighe)/2;

    if (pipe(fileDes) == -1)
        printf("errore PIPE\n");
    //------------------------------------------MSG QUEUE-----------------------------------------------
    key_t chiaveMsq = ftok("./keys/chiaveMessaggi.txt", 'a');
    if(chiaveMsq == -1){
        errExit("Server: errore crezione chiave della msg queue\n");
    }
    msqId = msgget(chiaveMsq, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(msqId == -1 ){
        errExit("Server: errore crezione msg queue\n");
    }
    
    //------------------------------------------SEMAFORI---------------------------------------------------
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

    //-----------------------------------------AVVIO GIOCO----------------------------------------------
    //SINCRONIZZAZIONE SERVER/CLIENTS
    printf("Attesa giocatori...\n");
    //P(sinc) -> attesa client 1
    semOp(semId, SINC, -1);
    printf("-> Giocatore 1 arrivato\n");
    if(dati->giocoAutomatico == 0){
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
                rimozioneIpc();
                exit(0);
            }
            semOp(semId, MUTEX, -1);
            gioco();
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
        //--------------------------------------FINE GIOCO-------------------------------------------------
        printf("gioco finito... attesa disconnessione clients\n");
        //--------------------------------------RIMOZIONE IPC----------------------------------------------
        //P(TERM)
        semOp(semId, TERM, -1);
        //P(TERM)
        semOp(semId, TERM, -1);
    }else{
        printf("gioco automatico!\n");
        semOp(semId, SERVER, -1);
        giocoAutomatico();
    }
    printf("Gioco terminato\n");
    rimozioneIpc();
    printf("IPC rimosse\n");
}

void gioco(){
    int colonnaScelta = 0;
    int nColonne = dati->nColonne;
    int nRighe = dati->nRighe;
    size_t mSize = sizeof(struct mossa) - sizeof(long);
    //ricevuta del messaggio
    if (msgrcv(msqId, &mossa, mSize, 3, IPC_NOWAIT) == -1)
        printf("%s\n", strerror(errno));

    if(mossa.colonnaScelta != -1){
        colonnaScelta = mossa.colonnaScelta;
        int pos = posizione(colonnaScelta, nRighe, nColonne, griglia);
        //Inserimento della pedina nella griglia (if per mettere pedina giusta)
        if(dati->turno[CLIENT1] == 1 && dati->turno[CLIENT2] == 0){  
            inserisci(pos, colonnaScelta, griglia, dati->pedina[CLIENT1]);
        }else if(dati->turno[CLIENT2] == 1 && dati->turno[CLIENT1] == 0){
            inserisci(pos, colonnaScelta, griglia, dati->pedina[CLIENT2]);
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

void rimozioneIpc(){
    semRemove(semId);
    freeShm(dati);
    freeShm(griglia);
    removeShm(shmIdG);
    removeShm(shmIdD);
    if (msgctl(msqId, IPC_RMID, NULL) == -1)
        errExit("Server: rimozione msg queue fallita");
}

int abbandonoClient(){
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
    if (signal(SIGINT, sigHandlerServer) == SIG_ERR){
        errExit("Errore ricevimento segnale CTRL C");
    }
}

void sigHandlerServer(int sig){
    if(abbandono == 0){
        sigset_t set;
        sigfillset(&set);
        sigdelset(&set, SIGINT);
        abbandono = 1;
        printf("\nal prossimo ctrl c il gioco terminerà\n");
        sigsuspend(&set);
        printf("\n---gioco terminato---\n");
        kill(dati->pidClient[CLIENT1], SIGUSR2);
        kill(dati->pidClient[CLIENT2], SIGUSR2);
        semOp(semId, TERM, -1);
        semOp(semId, TERM, -1);
        rimozioneIpc();
        exit(0);
    }
}

void giocoAutomatico(){
    printf("sincronizzazione avvenuta\n");
    //V(CLIENT)
    semOp(semId, CLIENT1, 1); //turno del giocatore - sblocca giocatore
    printf("sincronizzazione avvenuta\n");
    while(dati->fineGioco == 0){
        //P(SERVER)
        semOp(semId, SERVER, -1);
        //if(dati->turno[CLIENT1] == 0){ //se è il turno del giocatore 1 prende mossa dalla msgq
            size_t mSize = sizeof(struct mossa) - sizeof(long);
            //ricevuta del messaggio
            if (msgrcv(msqId, &mossa, mSize, 3, IPC_NOWAIT) == -1){
                errExit("Server: errore nella ricezione del messaggio\n");
            }
            int colonnaScelta = mossa.colonnaScelta;
            int pos = posizione(colonnaScelta, dati->nRighe, dati->nColonne, griglia);
            //Inserimento della pedina nella griglia (if per mettere pedina giusta)
            semOp(semId, MUTEX, 1);
            inserisci(pos, colonnaScelta, griglia, dati->pedina[CLIENT1]);
            semOp(semId, MUTEX, -1);
            //controllo tabella piena
            if(tabella_piena(dati->nRighe, dati->nColonne, griglia) == true){
                printf("tabella piena\n");
                dati->fineGioco = 2;
            }
            dati->fineGioco = fine_gioco(pos, colonnaScelta, dati->nRighe, dati->nColonne, griglia);
        //} 
        //V(iNS)
        semOp(semId, INS, 1);
        //P(SERVER) 
        semOp(semId, SERVER, -1);
        //----------------MOSSA CASUALE-----------------
        //if(dati->turno[CLIENT2] == 1){ //se è il turno del bot allora prende mossa da PIPE
            generaMossa();
            //P(SERVER)
            semOp(semId, SERVER, -1);
            /*
            int ran[0];
            ssize_t nBys;
            // close unused write-end
            if (close(fileDes[1]) == -1)
                printf("chiusura write fallita\n");
            // reading from the PIPE
            nBys = read(fileDes[0], ran, sizeof(int));
            if (nBys != sizeof(ran)) {
                printf("write fallita\n");
            }
            // close read-end of PIPE
            if (close(fileDes[0]) == -1)
                printf("chiusura read fallita\n");
            */
            
            int colonnaSce = dati->giocoAutomatico;
            int pox = posizione(colonnaSce, dati->nRighe, dati->nColonne, griglia);
            //Inserimento della pedina nella griglia (if per mettere pedina giusta)
            semOp(semId, MUTEX, 1);
            inserisci(pox, colonnaSce, griglia, dati->pedina[CLIENT2]);
            semOp(semId, MUTEX, -1);
            //controllo tabella piena
            if(tabella_piena(dati->nRighe, dati->nColonne, griglia) == true){
                printf("tabella piena\n");
                dati->fineGioco = 2;
            }
            dati->fineGioco = fine_gioco(pox, colonnaSce, dati->nRighe, dati->nColonne, griglia);
       // }
        //V(INS)
        semOp(semId, CLIENT1, 1);
        fflush(stdout);
        printf("Attesa mossa...\n");
        
    }
}

void generaMossa(){
    // generate a subprocess
    pid_t pid = fork();
    if(pid == -1){
        printf("figlio non creato\n");
    }else if (pid == 0) {
        int ran;
        srand(time(NULL));
        do{
            ran = rand() % (dati->nColonne + 1);
        }while(!controllo_colonna(ran, dati->nColonne) || colonna_piena(ran, dati->nRighe, dati->nColonne, griglia));
        
        //mossa generata, metterla in pipe e cambiare turno
        printf("mossa %i\n", ran);

        /*
        //SCRITTURA mossa nella pipe
        ssize_t nBys;
        // close unused read-end
        if (close(fileDes[0]) == -1)
            printf("chiusura read fallita\n");
        // write to the PIPE
        nBys = write(fileDes[1], &ran, sizeof(int));
        // checkig if write successed
        if (nBys != sizeof(ran)) {
            printf("write fallita\n");
        }
        // close write-end of PIPE
        if (close(fileDes[1]) == -1)
            printf("chiusura write fallita\n");
        */
        dati->giocoAutomatico = ran;
        exit(0);
        wait(NULL);
    }
    semOp(semId, SERVER, 1);
}
