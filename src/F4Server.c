#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../inc/shared_memory.h"
//struct tavolo_da_gioco tavolo;
struct dati;

void guida(){
    printf("Inserimento non valido!\nInput atteso: ./F4Server righe colonne param1 param2\nN.B. il numero delle righe e delle colonne deve essere maggiore o uguale a 5\n");
}

void controlloInput(int argc, char * argv[]){//CONTROLLI INSERIMENTO DA BASH
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

    //parametri shm per Griglia
    key_t chiaveG; 
    int shmIdG;
    size_t sizeMemG;

    //parametri shm per Dati
    key_t chiaveD; 
    int shmIdD;
    size_t sizeMemD;

    controlloInput(argc, argv); //funzione CONTROLLO INPUT 

    //salvataggio degli input
    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    param1 = argv[3];
    param2 = argv[4];
    
    sizeMemD = sizeof(struct dati);
    //alloco memoria condivisa per Dati
    chiaveD = ftok("./src/chiaveDati.txt", 'a');
    shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    dati->nColonne = nColonne;
    dati->nRighe = nRighe;
    dati->collegamento[0] = 1;
    dati->collegamento[1] = 0;
    dati->collegamento[2] = 0;
    //distaccamento memoria dati
    /*
    if (shmdt(dati) == -1)
        printf("Server: shmdt failed\n");
    shmctl(dati, IPC_RMID, 0);
    */

    //alloco memoria condivisa per Griglia
    //TODO
    //generare chiave con ftok tramite nomefile e stesso char per ogni chiamata alla mem
    chiaveG = ftok("./src/chiaveGriglia.txt", 'a');
    if (chiaveG == -1)
        printf("creazione chiave fallita\n");
    sizeMemG = nRighe * nColonne * sizeof(char); //dimensione colonne x righe in char
    shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR); //creazione shm
    if(shmIdG == -1){
        //TODO
        //gestire meglio gli errori
        printf("Server: Errore shmget\n"); //ERRORE
        exit(1);
    }

    //allaccio memoria
    griglia = (char *)shmat(shmIdG, NULL, 0);
    //TODO
    //gestire errore shmat
    
    //TEST
    
    for(int i = 0; i < nRighe*nColonne; i++){
        griglia[i] = ' ';
    }
    
    //execl("./client", NULL);
    if(dati->collegamento[1] == 1 ){
        if(shmctl(shmIdD, IPC_RMID, 0) == -1)
        printf("Server: rimozione della memoria fallita\n");
    }
    
}
