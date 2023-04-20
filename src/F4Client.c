#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>
//#include "shared_memory.h"

struct tavolo_da_gioco{
    int nRighe;
    int nColonne;
    char * param1;
    char * param2;
    //char tab[][];
};

void stampaMatrice(int nRighe, int nColonne){ 
//stampa la matrice ad ogni turno di gioco (aggiornata)
// la deve costruire secondo la mem condivisa

    char matrice[nRighe][nColonne];
    //riempimento matrice;
    for(int r = 0; r < nRighe; r++){
        for(int c = 0; c < nColonne; c++){
            matrice[r][c] = 'O';
        }
    }
    //stampa matrice
    int righine = 0;
    for(int r = 0; r < nRighe; r++){
        for(int c = 0; c < nColonne; c++){
            printf("| %c |", matrice[r][c]);
        }
        printf("\n");
        while(righine != nColonne){
            printf("-----");
            righine = righine + 1;
        }
        printf("\n");
        righine = 0;
    }
}

int main(int argc, char * argv[]){
    /*int nRighe;
    int nColonne;
    char *param1, *param2;
    */
    ssize_t sizeMem = sizeof(struct tavolo_da_gioco);
    key_t chiave = 3945;//atoi(argv[0]); //chiave generata a mano
    int shmTavId = shmget(chiave, sizeMem, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmTavId == -1)
        printf("Errore shmget\n");

    //allaccio memoria
    struct tavolo_da_gioco *allaccio = (struct tavolo_da_gioco *)shmat(shmTavId, NULL, 0);
    
    printf("%d\n", allaccio->nColonne);
    printf("%d\n", allaccio->nRighe);
}