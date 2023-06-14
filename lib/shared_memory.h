#ifndef _SHARED_MEMORY_HH
#define _SHARED_MEMORY_HH

#include <stdlib.h>


struct dati{
    int nColonne;
    int nRighe;
    int timer;
    char pedina[2];
    int indirizzamento[2];
    int turno[2];
    int pidClient[2];
    int fineGioco;
    int giocoAutomatico;
    int mossaBot;
};

void freeShm(void *ptr_sh);

void removeShm(int shmid);

#endif