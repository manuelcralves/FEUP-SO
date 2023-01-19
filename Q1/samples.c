#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define NUM_ARGS 4

int main(int argc, char* argv[]) {
    // Verifica numero de argumentos.
    if (argc != NUM_ARGS) {
        fprintf(stderr, "usage: %s file numberfrags maxfragsize\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(argv[1],"r");
    // Verifica se o ficheiro existe.
    if(file == NULL) {
        fprintf(stderr, "%s: cannot open %s: %s\n", argv[0], argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Converte nfrags para inteiro e verifica se é válido.
    int nfrags = atoi(argv[2]);
    if(nfrags <= 0) {
        fprintf(stderr, "%s: invalid number of frags\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Converte maxfragsize para inteiro e verifica se é válido.
    int maxfragsize = atoi(argv[3]);
    if(maxfragsize <= 0) {
        fprintf(stderr, "%s: invalid max frag size\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Seed geradora dos números random.
    srandom(0);

    fseek(file, 0,SEEK_END);
    // Tamanho do ficheiro.
    int length = ftell(file) / sizeof(char);

    /* 
    *  Verifica se é possível ter todas as frags com tal tamanho,
    *  dado que tem de estar todas em posições diferentes.
    */
    if(nfrags + maxfragsize - 1 > length) {
        fprintf(stderr, "%s: length not enough\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    //Inicializa array de posições.
    int position[nfrags];
    for(int i = 0; i < nfrags; i++) {
        position[i] = random() % (length - maxfragsize + 1);
        /*
        * Se for qualquer outro termo que não o primeiro, verifica se a posicão já existe.
        * Se existir volta a gerar outra posição e a verificar outra vez,
        * até serem todas diferentes.
        */
        if(i > 0) {
            for(int j = 0; j < i; j++) {
                if(position[j] == position[i]) {
                    position[i] = random() % (length - maxfragsize + 1);
                    j = -1;
                }
            }
        }
        // Avança para a posição gerada e validada antes.
        fseek(file, position[i], SEEK_SET);
        // Aloca espaço para a string onde vamos escrever o conteúdo.
        char *frag = (char*)malloc(sizeof(char) * (maxfragsize+1));
        fread(frag, sizeof(char), maxfragsize, file);
        frag[maxfragsize] = '\0';
        for(int k = 0; k < maxfragsize; k++) {
            // Se encontrar um endline, substitui por espaço.
            if(frag[k] == '\n') frag[k] = ' ';
        }
        // Imprime string.
        printf(">%s<\n", frag);
        // Liberta espaço alocado.
        free(frag);
    }
    fclose(file);
    exit(EXIT_SUCCESS);
}