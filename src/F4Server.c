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




int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char *param1, *param2; //non serve condividerli per ora
    char *griglia;

    //parametri shm per Griglia
    key_t chiaveG; 
    int shmIdG;
    size_t sizeMemG;

    controlloInput(argc, argv); //funzione CONTROLLO INPUT 

    //salvataggio degli input
    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    param1 = argv[3];
    param2 = argv[4];
    

    sizeMemG = sizeof(nColonne * sizeof(char)) + sizeof(nRighe * sizeof(char)); //dimensione colonne x righe in char
    

    //alloco memoria condivisa
    //TODO
    //generare chiave con ftok tramite nomefile e stesso char per ogni chiamata alla mem
    chiaveG = 3945;
    shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR); //creazione shm

    if(shmIdG == -1){
        //TODO
        //gestire meglio gli errori
        printf("Errore shmget\n"); //ERRORE
        exit(1);
    }

    //allaccio memoria
    griglia = (char *)shmat(shmIdG, NULL, 0);
    //TODO
    //gestire errore shmat
    
    //TEST
    griglia[0] = 'a';
    griglia[1] = 'b';

}