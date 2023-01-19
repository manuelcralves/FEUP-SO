#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define EPUB ".epub\0"
#define SIZE_ARGS 4
#define MIN_ARGS_SIZE 2

int main(int argc, char* argv[]) {
    // Verifica numero de argumentos.
    if (argc < MIN_ARGS_SIZE) {
        fprintf(stderr, "usage: %s [files]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Cria lista onde posteriormente são guardados os nomes dos ficheiros .epub
    int numArgs = argc - 1;
    char *epubsList[numArgs];

    for(int i = 1; i <= numArgs; i++) {
        // Processo para a partir do nome do ficheiro .txt mudar para .epub e guardar na lista
        epubsList[i-1] = (char*)malloc(sizeof(char)*(strlen(argv[i]) + 2));
        strcpy(epubsList[i-1], argv[i]);
        epubsList[i-1][strlen(epubsList[i-1]) - 4] = '\0';
        strcat(epubsList[i-1], EPUB);
    }

    pid_t pidepub[numArgs];
    for(int i = 0; i < numArgs; i++) {
        // Cria processos filho
        if((pidepub[i] = fork()) < 0) {
            fprintf(stderr, "%s: fork error: %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
        else if(pidepub[i] == 0) {
            // Aplica o pandoc no respetivo ficheiro
            printf("[pid%d] converting %s...\n", getpid(), epubsList[i]);

            execlp("pandoc", "pandoc", argv[i+1], "-o", epubsList[i], "--quiet", NULL);

            fprintf(stderr, "%s: couldn't convert %s to epub: %s\n", argv[0], argv[i+1], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    // Processo pai espera que os processos filhos terminem
    for(int i = 0; i < numArgs; i++) {
        if(waitpid(pidepub[i], NULL, 0) < 0) {
            fprintf(stderr, "%s: waitpid (%d) error: %s\n", argv[0], pidepub[i], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    printf("compressing files...\n");

    // Cria array de argumentos que será usado posteriormente na chamada execvp
    char *args[numArgs+SIZE_ARGS];

    args[0] = "zip";
    args[1] = "ebooks.zip";
    for(int i = 0; i < numArgs; i++) {
        args[i+2] = malloc(sizeof(char)*(strlen(epubsList[i])+1));
        strcpy(args[i+2], epubsList[i]);
    }
    args[numArgs+2] = "--quiet";
    args[numArgs+3] = (char*)NULL;

    // Cria processo filho para dar zip dos ficheiros .epub
    pid_t pidzip;
    if((pidzip = fork()) < 0) {
        fprintf(stderr, "%s: fork error: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    } else if(pidzip == 0) {
        execvp("zip", args);

        fprintf(stderr, "%s: couldn't compress epub files: %s\n", argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        if(waitpid(pidzip, NULL, 0) < 0) {
            fprintf(stderr, "%s: waitpid (%d) error %s\n", argv[0], pidzip, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    printf("compressed all files sucessfully (ebooks.zip)\n");

    // Liberta espaço alocado
    for(int i = 0; i < numArgs; i++) {
        free(epubsList[i]);
        free(args[i+2]);
    }

    exit(EXIT_SUCCESS);
}