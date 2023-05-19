#ifndef _MATRIXLIB_HH
#define _MATRIXLIB_HH

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 


bool casella_libera(int pos, char *arr);

void inserisci(int pos, int colonne, char *arr, char param);

bool controllo_colonna(int colonna_scelta, int colonne);

int posizione(int colonna_scelta, int nRighe, int nColonne, char *arr);

void stampa(int nRighe, int nColonne, char *arr);

bool vittoria_verticale(int pos, int nRighe, int nColonne, char *arr);

bool vittoria_orrizontale(int pos, int nColonne, char *arr);

bool vittoria_diagonale(int pos, int colonna_scelta, int nRighe, int nColonne, char *arr);

bool parita(int nRighe, int nColonne, char *arr);

int coordinate(int riga, int colonna_scelta, int nColonne);

int riga(int pos, int nColonne);

bool fine_gioco(int pos, int nRighe, int nColonne, char *arr);


#endif