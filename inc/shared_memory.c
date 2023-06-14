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
    // delete the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        errExit("shmctl failed");
}
