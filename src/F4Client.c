#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../inc/shared_memory.h"
struct dati;

void stampaMatrice(int nRighe, int nColonne, char * griglia){ 
    
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
    key_t chiaveD = ftok("./src/chiaveDati.txt", 'a');
    int shmIdD;
    shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    nColonne = dati->nColonne;
    nRighe = dati->nRighe;

    //collegamento server - clients
    if(dati->collegamento[1] == 0){
        dati->collegamento[1] = 1;
    }else if(dati->collegamento[2] == 0){
        dati->collegamento[2] = 1;
    }else{
        return 0;
    }
    

    
    //rimozione memoria per i dati
    
    //SHM GRIGLIA
    key_t chiaveG = ftok("./src/chiaveGriglia.txt", 'a');
    ssize_t sizeMemG = (nRighe * nColonne) * sizeof(char);
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1)
        printf("Client: Errore shmget\n");
    
    //allaccio memoria
    griglia = (char *)shmat(shmIdG, NULL, 0);
    
    
    
    stampaMatrice(nRighe, nColonne, griglia);

    if(dati->collegamento[2] == 1){
        printf("ottimo è arrivato anche il secondo client\n");
        if(shmctl(shmIdD, IPC_RMID, 0) == -1)
            printf("Server: rimozione della memoria fallita\n");
    }
    
}