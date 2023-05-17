#include <stdio.h>
#include <stdlib.h>

//controllo la casella, se libera scrivo altrimenti scrivo nella casella sopra
void occupato(int pos, int colonne, char *arr, char param){
    if(arr[pos] == ' ')
        arr[pos] = param;
    else if(pos <= 0){
        printf("ERRORE: colonna occupata\n");
        return;
    }
    else return occupato(pos-colonne, colonne, arr, param);
};

//calcolo la posizione nell'array in base alla colonna scelta
int posizione(int colonna_scelta, int nRighe, int nColonne){
    return ((nRighe-1) * nColonne + colonna_scelta) - 1;
};

//stampo la griglia
void stampa(int nRighe, int nColonne, char *arr){
    int indice = 0;

    for(int i = 0; i < nRighe; i++){
        for(int j = 0; j < nColonne; j++){
            printf("|%c", arr[indice]);
            indice++;
        }
        printf("|\n");
    }

    printf("-");

    while(nColonne >= 0){
        printf("--");
        nColonne--;
    }

    printf("\n");
};