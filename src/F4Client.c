#include <stdlib.h>
#include <stdio.h>


void stampaMatrice(int nRighe, int nColonne){ //QUESTO DEVE FARLO IL CLIENT
    int matrice[nRighe][nColonne];
    //riempimento matrice;
    for(int r = 0; r < nRighe; r++){
        for(int c = 0; c < nColonne; c++){
            matrice[r][c] = 0;
        }
    }
    //stampa matrice
    int righine = 0;
    for(int r = 0; r < nRighe; r++){
        for(int c = 0; c < nColonne; c++){
            printf("| %i |", matrice[r][c]);
        }
        printf("\n");
        while(righine != nColonne){
            printf("-----");
            righine = righine + 1;
        }
        printf("\n");
        righine = 0;
    }
}

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char *param1, *param2;
    
}