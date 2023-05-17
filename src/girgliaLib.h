#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

//controllo se la casella è libera
bool casella_libera(int pos, char *arr){
    if(arr[pos] == ' ')
        return true;
    else
        return false;
};

//se libera inserisco il segno, altrimenti lo inserisco nella riga sopra
//se la posizione è uguale o minore di 0 la colonna è piena
void inserisci(int pos, int colonne, char *arr, char param){
    if(pos < 0){   
            printf("ERRORE: colonna occupata\n");   
            return;
    }
    if(casella_libera(pos, arr) == true){
            arr[pos] = param;
    }
    else return inserisci(pos-colonne, colonne, arr, param);

};

//controllo che la colonna scelta sia valida
bool controllo_colonna(int colonna_scelta, int colonne){
    if(colonna_scelta <= 0 || colonna_scelta > colonne)
        return false;
    else
        return true;
}

//calcolo la posizione nell'array in base alla colonna scelta
int posizione(int colonna_scelta, int nRighe, int nColonne, char *arr){
    int pos = ((nRighe-1) * nColonne + colonna_scelta) - 1;
    if(casella_libera(pos, arr) == true)
        return pos;
    else{
        while(casella_libera(pos, arr) == false){
            pos = pos - nColonne;
        }
        return pos;
    }
};

//stampo la griglia
void stampa(int nRighe, int nColonne, char *arr){
    int indice = 0;

    for(int i = 0; i < nRighe; i++){
        for(int j = 0; j < nColonne; j++){
            printf("| %c ", arr[indice]);
            indice++;
        }
        printf("|\n");
    }

    printf("--");

    while(nColonne >= 0){
        printf("---");
        nColonne--;
    }

    printf("\n");
};

bool vittoria_verticale(int pos, int nRighe, int nColonne, char *arr){
    int uguali = 0;
    char param = arr[pos];

    //CONTROLLO LE CASELLE SOTTO, SE NE TROVO 4 UGUALI DI FILA VINCO
    while(pos + nColonne < nRighe * nColonne && uguali<3){
        if(param == arr[pos + nColonne])
            uguali++;    
        else 
            uguali = 0;
        pos = pos + nColonne;
    }

    if(uguali == 3)
        return true;
    else
        return false;
};

bool vittoria(int pos, int nRighe, int nColonne, char *arr){
    if(vittoria_verticale(pos, nRighe, nColonne, arr))
        return true;
}