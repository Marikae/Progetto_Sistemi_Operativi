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


//CREA/CONNETTE AD UN SET DI N SEMAFORI
int semGet(key_t key, int n);

//MODIFICA I VALORI DI UN SET DI SEMAFORI
void semSet(int semid, union semun arg);

//ESEGUE UN'OPERAZIONE BLOCCANTE SU UN SET DI SEMAFORI
void semOp (int semid, unsigned short sem_num, short sem_op);

//ESEGUE UN'OPERAZIONE NON BLOCCANTE SU UN SET DI SEMAFORI
int semOpNoBlock (int semid, unsigned short sem_num, short sem_op);

//RIMUOVE UN SET DI SEMAFORI
void semRemove(int semid);

//lEGGE IL VALORE DI UN SEMAFORO
int getValueSem(int semid, int num);

//MODIFICA IL VALORE DI UN SEMAFORO
int setValueSem(int semid, int num, union semun arg);

#endif