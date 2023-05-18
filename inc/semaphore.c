/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.
#include "../lib/errExit.h"
#include "../lib/semaphore.h"

#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>


//CREA/CONNETTE AD UN SET DI N SEMAFORI
int semGet(key_t key, int n){

	errno = 0;
	int semid = semget(key, n, IPC_CREAT | S_IRUSR | S_IWUSR );
	if(semid == -1){
		perror("errno");
		errExit("<Semaphore.c>: (SEMGET) Errore durante la connessione al set di semafori.\n");
	}

	return semid;
}


//MODIFICA I VALORI DI UN SET DI SEMAFORI
void semSet(int semid, union semun arg){
	if((semctl(semid, 0, SETALL, arg)) == -1){
		errExit("<Semaphore.c>: (SEMCTL) Errore durante l'inizializzazione del set di semafori.\n");
	}
}


//ESEGUE UN'OPERAZIONE BLOCCANTE SU UN SET DI SEMAFORI
void semOp (int semid, unsigned short sem_num, short sem_op) {

	struct sembuf sop = {
		sop.sem_num = sem_num,
		sop.sem_op = sem_op,
		sop.sem_flg = 0};

	if ((semop(semid, &sop, 1)) == -1){
		errExit("<Semaphore.c>: (SEMOP) Errore durante l'operazione al set di semafori.\n");
	}
}


//ESEGUE UN'OPERAZIONE NON BLOCCANTE SU UN SET DI SEMAFORI
int semOpNoBlock (int semid, unsigned short sem_num, short sem_op) {
	struct sembuf sop = {
		sop.sem_num = sem_num,
		sop.sem_op = sem_op,
		sop.sem_flg = IPC_NOWAIT};

	errno = 0;

	if ((semop(semid, &sop, 1)) == -1){
		if(errno == EAGAIN){
			return -1;
		}else{
			errExit("<Semaphore.c>: (SEMOP) Errore durante l'operazione al set di semafori.\n");
		}
	}

	return 0;
}


//RIMUOVE UN SET DI SEMAFORI
void semRemove(int semid){

	if((semctl(semid, 0, IPC_RMID, NULL)) == -1)
		errExit("<Semaphore.c>: (SEMCTL) Errore durante la rimozione di un set di semafori.\n");
	}


//lEGGE IL VALORE DI UN SEMAFORO
int getValueSem(int semid, int num){

	int value = semctl(semid, num, GETVAL, NULL);
	if( value == -1){
		errExit("<Semaphore.c>: (SEMCTL) Errore durante la letura di un semaforo.\n");
	}
	return value;
}


//MODIFICA IL VALORE DI UN SEMAFORO
int setValueSem(int semid, int num, union semun arg){

	int value = semctl(semid, num, SETVAL, arg);
	if(value == -1){
		errExit("<Semaphore.c>: (SEMCTL) Errore durante la scrittura di un semaforo.\n");
	}
	return value;
}
