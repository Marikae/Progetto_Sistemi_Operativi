#include <sys/sem.h>

#include "../lib/semaphore.h"
#include "../lib/errExit.h"


/**
 * The function performs a semaphore operation on a specified semaphore.
 * 
 * @param semid semid is an integer representing the ID of the semaphore set that the operation will be
 * performed on.
 * @param sem_num The index of the semaphore within the semaphore set. It is used to identify which
 * semaphore in the set to perform the operation on.
 * @param sem_op sem_op is a short integer that specifies the operation to be performed on the
 * semaphore. It can be a positive or negative integer value. If sem_op is positive, it will increment
 * the value of the semaphore by that amount. If sem_op is negative, it will decrement the value of the
 * semaphore by
 */

void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        errExit("semop failed");
}
