#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

//nostre librerie
#include "../lib/shared_memory.h"
#include "../lib/errExit.h"
#include "../lib/matrixlib.h"

struct dati;

void guida(){
    printf("Inserimento non valido!\nInput atteso: ./F4Server righe colonne param1 param2\nN.B. il numero delle righe e delle colonne deve essere maggiore o uguale a 5\n");
}

void controlloInput(int argc, char * argv[]){ //CONTROLLI INSERIMENTO DA BASH
    //controllo degli argomenti inseriti:
    //    - se argomenti diversi da 5 -> ERRORE
    //    - se n. righe o n. colonne minore di 5 -> ERRORE
    if(argc != 5 || atoi(argv[1]) < 5 || atoi(argv[2]) < 5){
        guida(); //->funzione che stampa la guida
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
    key_t chiaveD = ftok("./src/chiaveDati.txt", 'a');
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
    dati->collegamento[0] = 1;
    dati->collegamento[1] = 0;
    dati->collegamento[2] = 0;
    

    //--------------------MEMORIA CONDIVISA DELLA GRIGLIA DI GIOCO-----------------------
    key_t chiaveG = ftok("./src/chiaveGriglia.txt", 'b');
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

    //pulizia iniziale della griglia
    for(int i = 0; i < nRighe*nColonne; i++){
        griglia[i] = ' ';
    }
    
    //chiusura shm dei dati
    if(dati->collegamento[1] == 1 ){
        if(shmctl(shmIdD, IPC_RMID, 0) == -1)
            printf("Server: rimozione della shm dati fallita\n");
    }
    if(dati->collegamento[2] == 1){
        if(shmctl(shmIdG, IPC_RMID, 0) == -1)
            printf("Server: rimozione della shm griglia fallita\n");
    }
    
}
