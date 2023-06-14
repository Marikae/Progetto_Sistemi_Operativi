/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.
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