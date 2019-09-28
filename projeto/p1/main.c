#include <stdio.h>

int main(){
    FILE *inFile, *outFile;
    char commando, *input;

    fgets(input, 500 , stdin);
    
    inFile = fopen("", 'r');
    while(1){
        switch(commando){
            case 'd':

                break;
            case 'c':

                break;
            case 'l':

                break;
            default:
                printf("Comando %c inexistente", commando);
                break;
        }
    }
    exit(0);
}