#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../inc/shared_memory.h"
struct tavolo_da_gioco tavolo;

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
void vittoria(){
    //controlla la vincita cioè entra nella memoria condivisa, 
    //fa i suoi calcoli e comunica ai processi se c'è una vincita.
}



int main(int argc, char * argv[]){
    //VARIABILI
    int nRighe;
    int nColonne;
    /*
    char *param1, *param2;
    char ** griglia;
    //char tab[nRighe][nColonne];
    */
    //parametri per memoria condivisa
    key_t chiave; //chiave shared mem.
    int shmTavId; //id shared mem.
    size_t sizeMem; //dimensione da allocare

    controlloInput(argc, argv); //funzione CONTROLLO INPUT 

    //salvataggio degli input
    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    /*
    param1 = argv[3];
    param2 = argv[4];
    */

    //printf("righe: %d\ncolonne: %d\nparam1: %s\nparam2: %s\n", nRighe, nColonne, param1, param2);
    sizeMem = sizeof(int *) + sizeof(int *); //dimensione della memoria da allocare
    //+ sizeof(char *) + sizeof(char *) + sizeof(nColonne * sizeof(char)) + sizeof(nRighe * sizeof(char))

    //alloco memoria condivisa
    chiave = 3945;//atoi(argv[0]); //chiave generata a mano
    shmTavId = shmget(chiave, sizeMem, IPC_CREAT | S_IRUSR | S_IWUSR); //creazione shm
    if(shmTavId == -1)
        printf("Errore shmget\n");

    //allaccio memoria
    struct tavolo_da_gioco *allaccio = (struct tavolo_da_gioco *)shmat(shmTavId, NULL, 0); //scrittura e lettura
    
    //salvataggio dei dati in memoria
    allaccio->nColonne = nColonne;
    allaccio->nRighe = nRighe;
    /*
    allaccio->param1 = param1;
    allaccio->param2 = param2;
    */
    //allaccio->tab[][] = tab[][];
}