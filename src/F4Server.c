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
#define BOT 1
#define SERVER 2
#define B 3
#define MUTEX 4
#define SINC 5
#define INS 6
#define TERM 7

#define CLOCK 10

//variabili globali
struct mossa mossa;
struct dati * dati;
char * griglia;
int shmIdD;
int shmIdG; 
int semId;
int msqId;
int abbandono = 0;

//funzioni del programma
void gioco();
void giocoAutomatico();
void generaMossa();
int abbandonoClient();
void abbadonoServer();
void sigHandlerServer(int sig);
void rimozioneIpc();

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    int timer = -1;
    char pedina1;
    char pedina2;
    controlloInputServer(argc, argv); //funzione CONTROLLO INPUT 
    //salvataggio degli input - (messo sotto perchè prima controlla e poi salva)
    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    pedina1 = argv[3][0];
    pedina2 = argv[4][0];
    
    if(argc > 5){ //se ci sono più di 5 argomenti allora c'è un timer
        timer = atoi(argv[5]);
    }
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
    dati->timer = timer;
    dati->pedina[CLIENT1] = pedina1;
    dati->pedina[CLIENT2] = pedina2;
    dati->indirizzamento[CLIENT1] = 0;
    dati->indirizzamento[CLIENT2] = 0;
    dati->turno[CLIENT1] = 1;
    dati->turno[CLIENT2] = 0;
    dati->fineGioco = 0;
    dati->giocoAutomatico = 0;
    dati->mossaBot = 0;
    //---------------------------MEMORIA CONDIVISA DELLA GRIGLIA DI GIOCO--------------------------------
    key_t chiaveG = ftok("./keys/chiaveGriglia.txt", 'a');
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
    //P(SINC)
    semOp(semId, SINC, -1); //attesa client1
    printf("-> Giocatore 1 arrivato\n");
    if(dati->giocoAutomatico == 0){ //se il gioco è tra due utenti:
        //P(sinc)
        semOp(semId, SINC, -1); //attesa client2
        printf("-> Giocatore 2 arrivato\n");
        printf("----------------Sincronizzazione avvenuta------------\n");
        //inizio del ciclo che termina solo quando il gioco termina (vittoria - abbandono - parità)
        while(dati->fineGioco == 0){   
            abbadonoServer(); //il server è sempre qua quando vuole abbandonare perchè troppo veloce
            //P(s)
            semOp(semId, SERVER, -1); //attesa della mossa del giocatore
            fflush(stdout);
            if(abbandonoClient() == 1){
                rimozioneIpc();
                exit(0);
            }
            //P(mutex)
            semOp(semId, MUTEX, -1);
            gioco();
            //V(mutex)
            fflush(stdout);
            semOp(semId, MUTEX, 1);
            //V(INS)
            semOp(semId, INS, 1); //semaforo per la stampa della matrice aggiornata della mossa
            fflush(stdout);
            //V(B)
            semOp(semId, B, 1); //sblocca client
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
        //P(SERVER) 
        semOp(semId, SERVER, -1);
        giocoAutomatico();
        //P(TERM)
        semOp(semId, TERM, -1);
    }
    printf("Gioco terminato\n");
    rimozioneIpc(); //rimozione delle IPC allocate
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
            inserisci(pos, colonnaScelta, griglia, dati->pedina[CLIENT2]);
        }else if(dati->turno[CLIENT2] == 1 && dati->turno[CLIENT1] == 0){
            inserisci(pos, colonnaScelta, griglia, dati->pedina[CLIENT1]);
        }
        //controllo se è la pedina che riempie la matrice
        if(tabella_piena(dati->nRighe, dati->nColonne, griglia) == true){
            printf("tabella piena\n");
            dati->fineGioco = 2;
        }
        dati->fineGioco = fine_gioco(pos, colonnaScelta, nRighe, nColonne, griglia);
    }else{
        printf("turno saltato\n");
    }
}   


void giocoAutomatico(){
    //V(CLIENT)
    semOp(semId, CLIENT1, 1); //turno del giocatore - sblocca giocatore
    printf("sincronizzazione avvenuta\n");
    while(dati->fineGioco == 0){
        abbadonoServer();
        //P(SERVER)
        semOp(semId, SERVER, -1);  //appena il client sceglie la mossa e la invia sblocca il server
        if(abbandonoClient() == 1){
            printf("IPC rimosse \n");
            rimozioneIpc();
            exit(0);
        }
        size_t mSize = sizeof(struct mossa) - sizeof(long);
        //ricevuta della mossa del client
        if (msgrcv(msqId, &mossa, mSize, 3, IPC_NOWAIT) == -1){
            errExit("Server: errore nella ricezione del messaggio\n");
        }
        int colonnaScelta = mossa.colonnaScelta;
        int pos = posizione(colonnaScelta, dati->nRighe, dati->nColonne, griglia);
        //Inserimento della pedina nella griglia
        semOp(semId, MUTEX, 1); //modifica della mem condivisa - sezione critica
        inserisci(pos, colonnaScelta, griglia, dati->pedina[CLIENT1]);
        semOp(semId, MUTEX, -1);
        //controllo tabella piena
        if(tabella_piena(dati->nRighe, dati->nColonne, griglia) == true){
            printf("tabella piena\n");
            dati->fineGioco = 2;
        }   
        //verifica della vittoria     
        dati->fineGioco = fine_gioco(pos, colonnaScelta, dati->nRighe, dati->nColonne, griglia);
        //V(INS) --------------------
        semOp(semId, INS, 1); //semaforo per far stampare al client la griglia aggiornata con la mossa del clien
        //P(SERVER)------------------
        semOp(semId, SERVER, -1); //appena il client stampa la griglia aggiornata, tocca al bot
        if(abbandonoClient() == 1){
            rimozioneIpc();
            exit(0);
        }
        //----------------MOSSA CASUALE-----------------
        if(dati->fineGioco == 0 || dati->turno[BOT] == 0){
            generaMossa(); //generata mossa casuale e inserita in memoria condivisa
            //P(SERVER)
            semOp(semId, SERVER, -1); //si sblocca quando il processo figlio finisce di fare le sue cose
            
            int mossaBot = dati->mossaBot;
            printf("mossa bot %i\n", mossaBot);
            int pox = posizione(mossaBot, dati->nRighe, dati->nColonne, griglia);
            //Inserimento della pedina nella griglia
            semOp(semId, MUTEX, 1); //modifica della sezione critica
            inserisci(pox, mossaBot, griglia, dati->pedina[BOT]);
            semOp(semId, MUTEX, -1);
            //controllo tabella piena
            if(tabella_piena(dati->nRighe, dati->nColonne, griglia) == true){
                printf("tabella piena\n");
                dati->fineGioco = 2;
            }
            if(dati->turno[CLIENT1] == 1){ //turno giocatore 1
                dati->turno[CLIENT1] = 0;
                dati->turno[BOT] = 1;
            }else if(dati->turno[BOT] == 1){ //turno giocatore due
                dati->turno[CLIENT1] = 1;
                dati->turno[BOT] = 0;
            }
            //verifica vittoria
            dati->fineGioco = fine_gioco(pox, mossaBot, dati->nRighe, dati->nColonne, griglia);
            abbadonoServer();
            //V(CLIENT)
            semOp(semId, CLIENT1, 1); //sblocco turno del client
        }
        fflush(stdout);
        printf("Attesa mossa...\n");
    }
}

void generaMossa(){
    int kazoo = 0; //wait a minute... who are you? 
    int mossaBot;
    srand(time(NULL));

    pid_t pid = fork();
    if(pid == -1){
        printf("figlio non creato\n");
    }else if (pid == 0) {
        do{
            mossaBot = rand() % (dati->nColonne) + 1;
            printf("mossa generata %i\n", mossaBot); //controlo
        }while(!controllo_colonna(mossaBot, dati->nColonne) || colonna_piena(mossaBot, dati->nRighe, dati->nColonne, griglia));
        
        printf("mossa valida %i\n", mossaBot); //controllo
        sleep(kazoo); //il processo si addormenta per un secondo prima di mettere la mossa nella shm
        dati->mossaBot = mossaBot;
        exit(0);
    }
    wait(NULL);
    semOp(semId, SERVER, 1);
}

int abbandonoClient(){
    if(dati->giocoAutomatico == 0){ //gioco in coppia
        if(dati->pidClient[CLIENT1] == 0){
            kill(dati->pidClient[CLIENT2], SIGUSR1);
            printf("partita finita per abbandono del giocatore 1\n");
            return 1;
        }else if(dati->pidClient[CLIENT2] == 0){
            kill(dati->pidClient[CLIENT1], SIGUSR1);
            printf("partita finita per abbandono del giocatore 2\n");
            return 1;
        }
    }else{ //gioco automatico
        if(dati->pidClient[BOT] == -3){
            printf("Il giocatore ha abbandonato\n");
            return 1;
        }
        if(dati->fineGioco == 5){ //fine gioco = 5 timer scaduto
            printf("Il Timer è scaduto\n");
            return 1;
        }
    }
    return 0;
}

void abbadonoServer(){
    if (signal(SIGINT, sigHandlerServer) == SIG_ERR){
        errExit("Errore ricevimento segnale CTRL C");
    }
}

void sigHandlerServer(int sig){
    abbandono++;
    signal(SIGALRM, sigHandlerServer);
    signal(SIGINT, sigHandlerServer);
    
    if(sig == SIGALRM){
        printf("\nCTRL-C resettato\n");
        abbandono = 0;
        return;
    }
    if(abbandono == 1){
        alarm(CLOCK);
        printf("\npremi un'altra volta CTRL-C entro %i secondi per terminare il gioco\n", CLOCK);
    }else if(abbandono == 2){
        printf("\n---gioco terminato---\n");
            if(dati->giocoAutomatico == 0){
                kill(dati->pidClient[CLIENT1], SIGUSR2);
                kill(dati->pidClient[CLIENT2], SIGUSR2);
                semOp(semId, TERM, -1);
                semOp(semId, TERM, -1);
            }else{
                kill(dati->pidClient[CLIENT1], SIGUSR2);
                semOp(semId, TERM, -1);
            }
            rimozioneIpc();
            exit(0);
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