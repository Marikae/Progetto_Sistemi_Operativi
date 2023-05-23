#include <sys/shm.h>
#include <sys/stat.h>

#include <sys/shm.h>
#include <sys/stat.h>

#include "../lib/errExit.h"
#include "../lib/shared_memory.h"

int allocShm(key_t shmKey, size_t size) {
    // get, or create, a shared memory segment
    int shmid = shmget(shmKey, size, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid == -1)
        errExit("shmget failed");

    return shmid;
}

void *getShm(int shmid, int shmflg) {
    // attach the shared memory
    void *atShm = shmat(shmid, NULL, shmflg);
    if (atShm == (void *)-1)
        errExit("shmat failed");

    return atShm;
}

void freeShm(void *atShm) {
    // detach the shared memory segments
    if (shmdt(atShm) == -1)
        errExit("shmdt failed");
}

void removeShm(int shmid) {
    // delete the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        errExit("shmctl failed");
}
