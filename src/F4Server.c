#include <stdlib.h>
#include <stdio.h>

void guida(){
    printf("Inserimento non valido!\nInput atteso: ./F4Server righe colonne param1 param2\nN.B. il numero delle righe e delle colonne deve essere maggiore o uguale a 5\n");
}

void controlloInput(int argc, char * argv[]){//CONTROLLI INSERIMENTO DA BASH
    //controllo degli argomenti inseriti:
    //    - se argomenti diversi da 5 -> ERRORE
    //    - se n. righe o n. colonne minore di 5 -> ERRORE
    if(argc != 5 || atoi(argv[1]) < 5 || atoi(argv[2]) < 5){
        guida(); //->funzione che stampa la guida
        exit(1);
    }
}

int main(int argc, char * argv[]){
    int nRighe;
    int nColonne;
    char *param1, *param2;

    controlloInput(argc, argv); //funzione CONTROLLO INPUT 

    nRighe = atoi(argv[1]);
    nColonne = atoi(argv[2]);
    param1 = argv[3];
    param2 = argv[4];
    //printf("righe: %d\ncolonne: %d\nparam1: %s\nparam2: %s\n", nRighe, nColonne, param1, param2);
}