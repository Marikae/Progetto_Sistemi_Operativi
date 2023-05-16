#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../inc/shared_memory.h"
struct tavolo_da_gioco tavolo;

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
    int nRighe;
    int nColonne;
    char *griglia;
    
    ssize_t sizeMemG = sizeof(struct tavolo_da_gioco);
    key_t chiaveG = 3945;
    
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1)
        printf("Errore shmget\n");
    
    //allaccio memoria
    griglia = (char *)shmat(shmIdG, NULL, 0);
    
    printf("%c\n", griglia[0]);
    printf("%c\n", griglia[1]);
    //stampaMatrice(nRighe, nColonne);
    
}