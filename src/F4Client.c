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

    //---------------------MEMORIA CONDIVISA DATI----------------------------------
    ssize_t sizeMemD = sizeof(struct dati);
    key_t chiaveD = ftok("./src/chiaveDati.txt", 'a');
    if(chiaveD == -1){
        printf("Client: creazione chiave dati fallita\n");
        exit(1);
    }

    int shmIdD = shmget(chiaveD, sizeMemD, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdD == -1){
        printf("Client: creazione shm dati fallita\n");
        exit(1);
    }
    struct dati * dati = (struct dati *)shmat(shmIdD, NULL, 0);
    nColonne = dati->nColonne;
    nRighe = dati->nRighe;

    //collegamento server - clients
    if(dati->collegamento[1] == 0){
        dati->collegamento[1] = 1;
        printf("Client 1: giocatore %s\n", argv[1]);
    }else if(dati->collegamento[1] == 1 && dati->collegamento[2] == 0){
        dati->collegamento[2] = 1;
        printf("Client 2: giocatore %s\n", argv[1]);
    }else {
        return 0;
    }
    
    //-------------------------------MEMORIA CONDIVISA GRIGLIA-------------------------------
    key_t chiaveG = ftok("./src/chiaveGriglia.txt", 'b');
    if(chiaveG == -1){
        printf("Client: creazione chiave griglia fallita\n");
        exit(1);
    }
    ssize_t sizeMemG = (nRighe * nColonne) * sizeof(char);
    int shmIdG = shmget(chiaveG, sizeMemG, IPC_CREAT | S_IRUSR | S_IWUSR);
    if(shmIdG == -1){
        printf("Client: creazione shm griglia fallita\n");
        exit(1);
    }
    griglia = (char *)shmat(shmIdG, NULL, 0);
    
    stampaMatrice(nRighe, nColonne, griglia);

    if(dati->collegamento[2] == 1){
        printf("ottimo è arrivato anche il secondo client\n");
        if(shmctl(shmIdD, IPC_RMID, 0) == -1)
            printf("CLient: rimozione della sham dati fallita\n");
        if(shmctl(shmIdG, IPC_RMID, 0) == -1)
            printf("Client: rimozione della shm griglia fallita\n");
    }
}