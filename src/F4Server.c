#include <stdlib.h>
#include <stdio.h>

void guida(){
    printf("Inserimento non valido!\nInput atteso: ./F4Server righe colonne param1 param2\nN.B. il numero delle righe e delle colonne deve essere maggiore o uguale a 5\n");
}
int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char *param1, *param2;

    //CONTROLLI INSERIMENTO DA BASH
    //controllo degli argomenti inseriti, se diversi da 5 allora stampa aiuto
    if(argc != 5){
        guida();
        exit(1);
    }//controllo che sia una matrice di almeno 5 righe e 5 colonne
    if(atoi(argv[1]) < 5 || atoi(argv[2]) < 5){
        guida();
        exit(1);
    }

    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    param1 = argv[3];
    param2 = argv[4];

    printf("righe: %d \n colonne: %d \n param1: %s \n param2: %s\n", nRighe, nColonne, param1, param2);
}