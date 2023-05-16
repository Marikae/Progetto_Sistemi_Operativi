#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../inc/shared_memory.h"
//struct tavolo_da_gioco tavolo;
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
    
    //salvataggio dati dalla mem condivisa
    //SHM DATI
    ssize_t sizeMemD = sizeof(struct dati);
    key_t chiaveD = 6263;
    int shmIdD;
    shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    struct dati * dat = (struct dati *)shmat(shmIdD, NULL, 0);
    nColonne = dat->nColonne;
    nRighe = dat->nRighe;
    //rimozione memoria per i dati
    /*
    if (shmdt(dat) == -1 || shmctl(shmIdD, IPC_RMID, NULL) == -1)
        printf("Client: shmdt failed\n");
    
    */
    //SHM GRIGLIA
    key_t chiaveG = 4000;
    ssize_t sizeMemG = (nRighe * nColonne) * sizeof(char);
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1)
        printf("Client: Errore shmget\n");
    
    //allaccio memoria
    griglia = (char *)shmat(shmIdG, NULL, 0);
    
    
    
    stampaMatrice(nRighe, nColonne, griglia);

    if(shmctl(shmIdD, IPC_RMID, 0) == -1)
        printf("Server: rimozione della memoria fallita\n");
}