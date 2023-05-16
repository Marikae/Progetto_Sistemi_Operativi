#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../inc/shared_memory.h"
struct tavolo_da_gioco tavolo;
struct dati;

void stampaMatrice(int nRighe, int nColonne, char * griglia){ 
    //int numTot = nRighe * nColonne;
    int indice = 0;

    for(int i = 0; i < nColonne; i++){
        for(int j = 0; j < nRighe; j++){
            printf("|%c", griglia[indice]);
            indice++;
        }

        printf("|\n");
    }
}

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char * griglia;
    
    //dati mem condivise
    ssize_t sizeMemG = sizeof(struct tavolo_da_gioco);
    key_t chiaveG = 3945;
    ssize_t sizeMemD = sizeof(struct dati);
    key_t chiaveD = 6263;
    int shmIdD;

    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1)
        printf("Errore shmget\n");
    
    //allaccio memoria
    griglia = (char *)shmat(shmIdG, NULL, 0);
    
    /*
    printf("%c\n", griglia[0]);
    printf("%c\n", griglia[1]);*/
    
    
    //salvataggio dati dalla mem condivisa
    shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    nColonne = dati->nColonne;
    nRighe = dati->nRighe;

    stampaMatrice(nRighe, nColonne, griglia);
    
}