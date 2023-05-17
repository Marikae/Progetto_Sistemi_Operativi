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

bool vittoria(int pos, int nRighe, int nColonne, char *arr);

#endif