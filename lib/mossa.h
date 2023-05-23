#ifndef _MOSSA_HH
#define _MOSSA_HH

#include <stdlib.h>

// the structure defines a order sent by a
// client to a supplier.
struct mossa {
    long mtype;
    unsigned int colonnaScelta;
};

#endif