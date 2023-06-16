
#ifndef _SEMAPHORE_HH
#define _SEMAPHORE_HH

#include <sys/sem.h>
#include <sys/types.h>
#pragma once

union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};

//ESEGUE UN'OPERAZIONE BLOCCANTE SU UN SET DI SEMAFORI
void semOp (int semid, unsigned short sem_num, short sem_op);

//RIMUOVE UN SET DI SEMAFORI
void semRemove(int semid);

#endif
/************************************
*VR474005
*Marica Bottega
*16 giugno 2023
*************************************/
