
#include "../lib/errExit.h"
#include "../lib/semaphore.h"

#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

void semOp (int semid, unsigned short sem_num, short sem_op) {
	struct sembuf sop = {
		sop.sem_num = sem_num,
		sop.sem_op = sem_op,
		sop.sem_flg = 0};
	int ret;
	do{
		ret = semop(semid, &sop, 1);
	}while(ret == -1 && errno == EINTR);
}

void semRemove(int semid){
	if((semctl(semid, 0, IPC_RMID, NULL)) == -1)
		errExit("<Semaphore.c>: (SEMCTL) Errore durante la rimozione di un set di semafori.\n");
}
/************************************
*VR474005
*Marica Bottega
*16 giugno 2023
*************************************/

