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

//ESEGUE UN'OPERAZIONE BLOCCANTE SU UN SET DI SEMAFORI
void semOp (int semid, unsigned short sem_num, short sem_op) {
	struct sembuf sop = {
		sop.sem_num = sem_num,
		sop.sem_op = sem_op,
		sop.sem_flg = 0};
	int ret;
	do{
		ret = semop(semid, &sop, 1);
	}while(ret == -1 && errno == EINTR);
	/*if ((semop(semid, &sop, 1)) == -1){
		errExit("<Semaphore.c>: (SEMOP) Errore durante l'operazione al set di semafori.\n");
	}*/
}

//RIMUOVE UN SET DI SEMAFORI
void semRemove(int semid){
	if((semctl(semid, 0, IPC_RMID, NULL)) == -1)
		errExit("<Semaphore.c>: (SEMCTL) Errore durante la rimozione di un set di semafori.\n");
}
