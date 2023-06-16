#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../lib/errExit.h"
#include "../lib/shared_memory.h"

void freeShm(void *atShm) {
    if (shmdt(atShm) == -1)
        errExit("shmdt failed");
}

void removeShm(int shmid) {
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        errExit("shmctl failed");
}
/************************************
*VR474005
*Marica Bottega
*16 giugno 2023
*************************************/

