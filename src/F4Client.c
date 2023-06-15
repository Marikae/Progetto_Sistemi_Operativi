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
#define BOT 1
#define SERVER 2
#define B 3 //btree
#define MUTEX 4 //mutua esclusione
#define SINC 5 //sincronizzazione
#define INS 6 //inserimento
#define TERM 7 //terminazione

//Variabili globali
struct mossa mossa;
struct dati * dati;
char * griglia;
int shmIdG; //SHM griglia
int shmIdD; //SHM dati
int semId;
int msqId;

//Funzioni utilizzate
void gioca();
void giocatore1(char * nomeG1);
void giocatore2(char * nomeG2);
void giocoAutomatico(char * nomeG1);
void pulisciInput(FILE *const in);
void rimozioneIpc();
void fineGioco();
void abbandonoClient();
void abbandonoServer();
void sigHandlerAbbandono(int sig);
void sigHandlerTavolino(int sig);
void sigHandlerServer(int sig);
void sigHandlerTempo(int sig);


int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char * bot = "bot";
    controlloInputClient(argc, argv);
    //------------------------------MEMORIA CONDIVISA DATI----------------------------------
    ssize_t sizeMemD = sizeof(dati);
    key_t chiaveD = ftok("./keys/chiaveDati.txt", 'a');
    if(chiaveD == -1){
        errExit("Client: creazione chiave dati fallita\n");
    }
    shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdD == -1){
        errExit("Client: creazione shm dati fallita\n");
    }
    dati = (struct dati *)shmat(shmIdD, NULL, 0);
    nColonne = dati->nColonne;
    nRighe = dati->nRighe;
    //-------------------------------MEMORIA CONDIVISA GRIGLIA-------------------------------
    key_t chiaveG = ftok("./keys/chiaveGriglia.txt", 'a');
    if(chiaveG == -1){
        errExit("Client: creazione chiave griglia fallita\n");
    }
    ssize_t sizeMemG = (nRighe * nColonne) * sizeof(char);
    shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1){
        errExit("Client: creazione shm griglia fallita\n");
    }
    griglia = (char *)shmat(shmIdG, NULL, 0);
    //------------------------------------MSG QUEUE-------------------------------------------
    key_t chiaveMsq = ftok("./keys/chiaveMessaggi.txt", 'a');
    if(chiaveMsq == -1){
        errExit("Client: creazione chiave msg queue fallita\n");
    }
    msqId = msgget(chiaveMsq,  S_IRUSR | S_IWUSR);
    if  (msqId == -1){
        errExit("Client: creazione msg queue fallita\n");
    }
    //-----------------------------------SEMAFORI INIT---------------------------------------
    key_t chiaveSem = ftok("./keys/chiaveSem.txt", 'a');
    if(chiaveSem == -1){
        errExit("Client: creazione chiave semafori fallita\n");
    }
    semId = semget(chiaveSem, 8, IPC_CREAT | S_IRUSR | S_IWUSR);
    if  (semId == -1){
        errExit("Client: creazione semafori fallita\n");
    }
    //indirizzamento dei processi nelle rispettive funzioni
    if(dati->indirizzamento[CLIENT1] == 0){//arriva client 1
        if(argc < 3){ //gioco tra due client
            dati->indirizzamento[CLIENT1] = 1;
            giocatore1(argv[1]);
        }else if(strcmp(argv[2], bot) == 0){ //gioco automatico -> se viene scritto "bot" come terzo elemento
            dati->indirizzamento[CLIENT1] = 1;
            dati->indirizzamento[CLIENT2] = 1;
            giocoAutomatico(argv[1]);
        }
    }else if(dati->indirizzamento[CLIENT1] == 1 && dati->indirizzamento[CLIENT2] == 0){ //arriva client 2
        dati->indirizzamento[CLIENT1] = 1;
        giocatore2(argv[1]);
    }else if(dati->indirizzamento[CLIENT1] == 1 && dati->indirizzamento[CLIENT2] == 1){ //arriva client indesiderato
        printf("Non puoi giocare adesso\n");
        exit(0);
    }
    
    fineGioco();
    //-------------------RIMOZIONE IPC-----------------------
    //le IPC vengono rimosse dal server
    //V(DISC)
    semOp(semId, TERM, 1);
}

void giocatore1(char * nomeG1){
    dati->pidClient[CLIENT1] = getpid(); //pid processo client 1
    //V(sinc) -> avvisa il server che è arrivato (sblocca)
    printf("Giocatore %s: la tua pedina è %c\n", nomeG1, dati->pedina[CLIENT1]);
    printf("Giocatore %s: Attesa giocatore 2...\n", nomeG1);
    semOp(semId, SINC, 1); 
    int c2arrivato = 1;
    while(dati->fineGioco == 0){
        //P(client1) //all'inzio aspetta il giocatore 2
        abbandonoClient(); //se si abbandona mentre è il turno dell'altro giocatore
        abbandonoServer(); 
        semOp(semId, CLIENT1, -1);
        if(c2arrivato == 1){
            printf("Giocatore 2 arrivato\n");
            c2arrivato = 0;
        }
        if(dati->fineGioco != 0 ){
            break;
        }
        //P(b) -> aspetto server
        semOp(semId, B, -1); 
        fflush(stdout);
        //P(mutex)
        semOp(semId, MUTEX, -1);
        gioca(); //MUTUA
        //V(mutex)
        semOp(semId, MUTEX, 1);
        fflush(stdout);
        //V(s) -> invio al server (mss queue)
        semOp(semId, SERVER, 1); 
        //P(INS)
        semOp(semId, INS, -1);
        stampa(dati->nRighe, dati->nColonne, griglia);
        fflush(stdout);
        //V(c2)
        semOp(semId, CLIENT2, 1); 
        fflush(stdout);
        if(dati->fineGioco == 0){
            printf("Giocatore %s: Turno del giocatore 2...\n", nomeG1);    
        }
        
    };
    printf("---fine gioco---\n");
    stampa(dati->nRighe, dati->nColonne, griglia);
}

void giocatore2(char * nomeG2){
    dati->pidClient[CLIENT2] = getpid();
    printf("Giocatore %s: la tua pedina è questa: %c\n", nomeG2, dati->pedina[CLIENT2]);
    //V(s2) -> avvisa il server che è arrivato (sblocca)
    semOp(semId, SINC, 1);
    //V(c1) //all'inzio sblocca giocatore 1
    semOp(semId, CLIENT1, 1); 

    while(dati->fineGioco == 0){
        //P(c2) -> blocco giocatore 2, sbloccato da giocatore 1
        if(dati->fineGioco == 0)
            printf("Giocatore %s: Turno del giocatore 1...\n", nomeG2);
        fflush(stdout);
        abbandonoClient();
        abbandonoServer();
        semOp(semId, CLIENT2, -1);        
        if(dati->fineGioco != 0){
            break;
        }
        //P(B) -> aspetto server
        semOp(semId, B, -1);
        //P(mutex)
        fflush(stdout);
        semOp(semId, MUTEX, -1);
        fflush(stdin);
        gioca();
        //V(mutex)
        semOp(semId, MUTEX, 1);
        //V(s) -> invio al server (mss queue)
        semOp(semId, SERVER, 1); 
        //P(INS)
        semOp(semId, INS, -1);
        stampa(dati->nRighe, dati->nColonne, griglia);
        //V(c1)
        semOp(semId, CLIENT1, 1); 
    }
    printf("---fine gioco---\n");
    stampa(dati->nRighe, dati->nColonne, griglia);
}

void gioca(){
    int colonna = -1;
    int tempo;
    stampa(dati->nRighe, dati->nColonne, griglia); //stampa mossa del giocatore precedente

    fflush(stdout);
    abbandonoServer();
    abbandonoClient();
    if (signal(SIGALRM, sigHandlerTempo) == SIG_ERR)
        errExit("change signal handler failed");

    if(dati->timer != -1){
        tempo = dati->timer;
        alarm(tempo); // setting a tempo
        printf("Hai %i secondi per fare la tua mossa \n", tempo);
    }
    
    do{
        pulisciInput(stdin);
        printf("scegli mossa:\n");
        abbandonoClient();
        scanf("%i", &colonna);
        abbandonoClient(); //abbandono dell'altro giocatore
    }while(!controllo_colonna(colonna, dati->nColonne) || colonna_piena(colonna, dati->nRighe, dati->nColonne, griglia));
    
    tempo = alarm(0); //tempo fermato

    //invia al server la scelta tramite queue
    printf("hai scelto la colonna: %i \n", colonna);
    //--------------Turno--------------
    if(dati->turno[CLIENT1] == 1){
        dati->turno[CLIENT1] = 0;
        dati->turno[CLIENT2] = 1;
    }else if(dati->turno[CLIENT2] == 1){
        dati->turno[CLIENT2] = 0;
        dati->turno[CLIENT1] = 1;
    }
    mossa.mtype = 3;
    mossa.colonnaScelta = colonna;
    size_t mSize = sizeof(mossa) - sizeof(long);
    //-------------invio messaggio-------------
    if (msgsnd (msqId, &mossa, mSize, 0) == -1){
        errExit("Errore nell'invio della mossa\n");
    }
}

void giocoAutomatico(char * nomeG1){
    printf("Ciao %s, hai scelto l'opzione gioco automatico!\nGiocherai contro il nostro SuperComputer42\n", nomeG1);
    dati->giocoAutomatico = 1;
    dati->pidClient[CLIENT1] = getpid();
    //V(SINC)
    semOp(semId, SINC, 1); //sincro con server
    //V(SERVER)
    semOp(semId, SERVER, 1);
    //printf("sincronizzazione avvenuta\n");
    while(dati->fineGioco == 0){
        //abbandonoServer(); 
        //P(CLIENT1)
        semOp(semId, CLIENT1, -1);
        if(dati->fineGioco != 0 ){
            break;
        }
        fflush(stdout);
        //P(mutex)
        semOp(semId, MUTEX, -1);
        abbandonoClient();
        gioca(); //MUTUA
        //V(mutex)
        semOp(semId, MUTEX, 1);
        fflush(stdout);
        //V(SERVER) -> invio al server (mss queue)
        semOp(semId, SERVER, 1); 
        //P(INS)
        semOp(semId, INS, -1);
        stampa(dati->nRighe, dati->nColonne, griglia);
        fflush(stdout);
        //V(SERVER)
        semOp(semId, SERVER, 1);  
        fflush(stdout);
        if(dati->fineGioco == 0){
            printf("Giocatore %s: Turno del Server...\n", nomeG1);    
        }
    };
    printf("---fine gioco---\n");
    stampa(dati->nRighe, dati->nColonne, griglia);
    
}

void fineGioco(){
    if(dati->giocoAutomatico == 0){
        if(dati->fineGioco == 2){
            printf("Partita finita in parità!\n");
        }else if(dati->fineGioco == 1){
            if(dati->turno[CLIENT1] == 0){
                printf("ha vinto il giocatore 1\n");
            }else{
                printf("ha vinto il giocatore 2\n");
            }
        }
    }else{
        if(dati->fineGioco == 2){
            printf("Partita finita in parità!\n");
        }else if(dati->fineGioco == 1){
            if(dati->turno[CLIENT1] == 0){
                printf("HAI VINTO!\n");
            }else{
                printf("HA VINTO IL SERVER\n");
            }
        }
    }
    
}

void abbandonoClient(){
    if (signal(SIGINT, sigHandlerAbbandono) == SIG_ERR || signal(SIGHUP, sigHandlerAbbandono) == SIG_ERR)
        errExit("change signal handler failed");
    if (signal(SIGUSR1, sigHandlerTavolino) == SIG_ERR)
        errExit("change signal handler failed");
}

void abbandonoServer(){
    if (signal(SIGUSR2, sigHandlerServer) == SIG_ERR)
        errExit("change signal handler failed");
}


void sigHandlerServer(int sig){
    printf("il server ha terminato il gioco\n");
    semOp(semId, TERM, 1);
    exit(0);
}

void sigHandlerAbbandono(int sig) {
    printf("\nHai abbandonato la partita\n");
    if(dati->giocoAutomatico == 0){
        if(getpid() == dati->pidClient[CLIENT1]){
            dati->pidClient[CLIENT1] = 0;
            fflush(stdout);
        }else if(getpid() == dati->pidClient[CLIENT2]){
            dati->pidClient[CLIENT2] = 0;
            fflush(stdout);
        }
    }else{
        dati->pidClient[BOT] = -3;
    }
    //V(SERVER)
    semOp(semId, SERVER, 1); 
    exit(0);
}

void sigHandlerTavolino(int sig) {
    printf("Hai vinto a tavolino! l'altro giocatore ha abbandonato la partita\n");
    exit(0);
}

void sigHandlerTempo(int sig){
    printf("Timer scaduto! hai abbandonato la partita\n");
    //per avvisare chi ha vinto
    if(dati->turno[CLIENT1] == 1){ //turno giocatore 1
        dati->pidClient[CLIENT1] = 0;
    }else if(dati->turno[CLIENT2] == 1){ //turno giocatore 1
        dati->pidClient[CLIENT2] = 0;
    }
    if(dati->giocoAutomatico == 1){
        dati->fineGioco = 5;
    }
    semOp(semId, MUTEX, 1);
    semOp(semId, SERVER, 1);
    exit(0);
}

void pulisciInput(FILE * const input){ //da cambiare nome variabili
    if(input){
        long const descrittore = fileno(input);
        int dummy;
        int flags;
        flags = fcntl(descrittore, F_GETFL);
        fcntl(descrittore, F_SETFL, flags | O_NONBLOCK);
        do{
            dummy = getc(input);
        }while(dummy != EOF); //fine del file
        fcntl(descrittore, F_SETFL, flags);
    }
}

void rimozioneIpc(){
    semRemove(semId);
    freeShm(dati);
    freeShm(griglia);
    removeShm(shmIdG);
    removeShm(shmIdD);
    if (msgctl (msqId, IPC_RMID, NULL) == -1)
        errExit("msgctl failed");
}
