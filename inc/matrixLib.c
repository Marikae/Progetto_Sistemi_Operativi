#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 

#include "../lib/matrixLib.h"

//controllo se la casella è libera
bool casella_libera(int pos, char *arr){
    if(arr[pos] == ' ')
        return true;
    else
        return false;
};

//RITORNA LA RIGA IN CUI SEI
int riga(int pos, int nColonne){
    return pos/nColonne;
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

//controllo se la colonna è piena
bool colonna_piena(int colonna_scelta, int nRighe, int nColonne, char *arr){
    int colonna = colonna_scelta-1;
    for(int r = nRighe; r >= 0; r--)
        if(arr[coordinate(r, colonna, nColonne)] == ' ')
            return false;
    
    return true;
}

//controllo che la colonna scelta sia valida
bool controllo_colonna(int colonna_scelta, int colonne){
    if(colonna_scelta <= 0 || colonna_scelta > colonne)
        return false;
    else
        return true;
};

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
    int colonne = nColonne;

    for(int i = 0; i < nRighe; i++){
        for(int j = 0; j < nColonne; j++){
            printf("| %c ", arr[indice]);
            indice++;
        }
        printf("|\n");
    }

    while(colonne > 0){
        printf("----");
        colonne--;
    }

    printf("-");
    printf("\n");

    for(int i = 1; i <= nColonne; i++)
        printf("| %d ", i);

    printf("|\n");
};

//controllo vittoria verticale
bool vittoria_verticale(int pos, int nRighe, int nColonne, char *arr){
    int uguali = 0;

    //CONTROLLO LE CASELLE SOTTO, SE NE TROVO 4 UGUALI DI FILA VINCO
    while(pos + nColonne < nRighe * nColonne && uguali < 3){
        if(arr[pos] == arr[pos + nColonne])
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

//VITTORIA ORIZZONTALE
bool vittoria_orizzontale(int pos, int colonna_scelta, int nRighe, int nColonne, char *arr){
    char param = arr[pos];
    int colonna = colonna_scelta - 1;
    int count = 0;
    int r = riga(pos, nColonne);

    for(int c = colonna + 1; c < nColonne && param == arr[coordinate(r, c,nColonne)] && count < 3; c++)
        count++;
    
    for(int c = colonna - 1; c >= 0 && param == arr[coordinate(r, c,nColonne)] && count < 3; c--)
        count++;
    
    
    if(count == 3)
        return true;
    return false;
};

int coordinate(int riga, int colonna_scelta, int nColonne){
    return riga * nColonne + colonna_scelta ;
};

//PARITA'
bool parita(int nRighe, int nColonne, char *arr){
    for(int i = 0; i < nRighe*nColonne; i++)
        if(arr[i] == ' ')
            return false;
    
    return true;
};

bool vittoria_diagonale(int pos, int colonna_scelta, int nRighe, int nColonne, char *arr){
    char param = arr[pos];
    int r, c;
    int count = 0;

    int row = riga(pos, nColonne);
    int column = colonna_scelta - 1;
    
    //CONTROLLO RIGA SOTTO COLONNA DESTRA
    for(r = row + 1, c = column + 1; r < nRighe && c < nColonne && param == arr[coordinate(r, c, nColonne)] && count < 3; r++, c++){
        count++;
    }
    //CONTROLLO RIGA SOPRA COLONNA SINISTRA
    for(r = row - 1, c = column - 1; r >= 0 && c >= 0 && param == arr[coordinate(r, c, nColonne)] && count < 3; r--, c--){
        count++;
    }
    
    //CONTROLLO RIGA SOTTO COLONNA SINISTRA
    for(r = row + 1, c = column - 1; r < nRighe && c >= 0 && param == arr[coordinate(r, c, nColonne)] && count < 3; r++, c-- )
        count++;

    //CONTROLLO RIGA SOPRA COLONNA DESTRA
    for(r = row - 1, c = column + 1; r >= 0 && c < nColonne && param == arr[coordinate(r, c, nColonne)] && count < 3; r--, c++)
        count++;
    
    if(count == 3)
        return true;

    return false;

}

bool fine_gioco(int pos, int colonna_scelta, int nRighe, int nColonne, char *arr){
    if(vittoria_verticale(pos, nRighe, nColonne, arr))
        return true;
    if(vittoria_orizzontale(pos, colonna_scelta, nRighe, nColonne, arr))
        return true;
    if(vittoria_diagonale(pos, colonna_scelta, nRighe, nColonne, arr))
        return true;
    if(parita(nRighe, nColonne, arr))
        return true;
    
    return false;
};