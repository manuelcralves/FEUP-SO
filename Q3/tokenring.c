#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define MAX_LENGTH 20
#define NUM_ARGS 4
#define WRITE 1
#define READ 0

int main (int argc, char *argv[]) {
    // Verifica numero de argumentos.
    if(argc != NUM_ARGS) {
        fprintf(stderr, "usage: %s number_processes probability lock_seconds\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Converte número de processos para inteiro e verifica se é válido
    int numProc = atoi(argv[1]);
    if(numProc <= 1) {
        fprintf(stderr, "%s: invalid number of processes (must be more than 1)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Converte probabilidade para float, passa para percentagem e verifica se é válida
    int prob = atof(argv[2])*100;
    if(prob <= 0 || prob >= 100) {
        fprintf(stderr, "%s: invalid probability (between 0 and 1)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Converte número de segundos para inteiro e verifica se é válido
    int seconds = atoi(argv[3]);
    if(seconds <= 0) {
        fprintf(stderr, "%s: invalid number of seconds (must be more than 0)\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Cria todos os pipes
    for(int i = 1; i <= numProc; i++) {
        char *pipe = (char*)malloc(sizeof(char) * MAX_LENGTH);
        // Nome do pipe
        if(i == numProc) {
            sprintf(pipe, "pipe%dto1", i);
        } else {
            sprintf(pipe, "pipe%dto%d", i, i+1);
        }
        // Cria fifo com permissões read e write
        if((mkfifo(pipe, 0666)) < 0) {
            fprintf(stderr, "%s: mkfifo error: %s", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        }
        free(pipe);
    }

    // Inicializa o token
    int token = 0;
    
    // Cria os processos
    pid_t pids[numProc];
    for(int i = 0; i < numProc; i++) {
        char *readPipe = (char*)malloc(sizeof(char) * MAX_LENGTH);
        char *writePipe = (char*)malloc(sizeof(char) * MAX_LENGTH);
        
        if((pids[i] = fork()) < 0) {
            fprintf(stderr, "%s: fork error: %s\n", argv[0], strerror(errno));
            exit(EXIT_FAILURE);
        } else if(pids[i] == 0) {
            // Nomes dos pipes de leitura e de escrita
            if(i == 0) {
                sprintf(writePipe, "pipe1to2");
                sprintf(readPipe, "pipe%dto1", numProc);
            } else if(i == numProc-1) {
                sprintf(writePipe, "pipe%dto1", numProc);
                sprintf(readPipe, "pipe%dto%d", numProc-1, numProc);
            } else {
                sprintf(writePipe, "pipe%dto%d", i+1, i+2);
                sprintf(readPipe, "pipe%dto%d", i, i+1);
            }

            int fd[2];

            // Seed geradora para a probabilidade
            srandom(time(NULL) - i);

            // Se for o primeiro, como não precisa de ler, apenas escreve para o proximo
            if(i == 0 && token == 0) {
                // Abre o pipe de escrita
                if((fd[WRITE] = open(writePipe, O_WRONLY)) < 0) {
                    fprintf(stderr, "%s: open pipe (%s) error: %s\n", argv[0], writePipe,strerror(errno));
                    exit(EXIT_FAILURE);
                }

                token++;
                
                // Escreve o valor to token para o pipe
                if(write(fd[WRITE], &token, sizeof(int)) < 0) {
                    fprintf(stderr, "%s: write error: %s\n", argv[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }

                close(fd[WRITE]);
            }

            while(1) {
                // Lê o valor do processo anterior
                if((fd[READ] = open(readPipe, O_RDONLY)) < 0) {
                    fprintf(stderr, "%s: open pipe (%s) error: %s\n", argv[0], readPipe, strerror(errno));
                    exit(EXIT_FAILURE);
                }

                if(read(fd[READ], &token, sizeof(int)) < 0) {
                    fprintf(stderr, "%s: read error: %s\n", argv[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }

                close(fd[READ]);

                token++;

                // Probabilidade de dar lock no token
                int rand = (random() % 100) + 1;
                if(rand <= prob) {
                    printf("[p%d] lock on token (val = %d)\n", i+1, token);
                    sleep(seconds);
                    printf("[p%d] unlock token\n", i+1);
                }

                // Escreve o valor do token, já incrementado, para o processo seguinte
                if((fd[WRITE] = open(writePipe, O_WRONLY)) < 0) {
                    fprintf(stderr, "%s: open pipe (%s) error: %s\n", argv[0], writePipe, strerror(errno));
                    exit(EXIT_FAILURE);
                }
                
                if(write(fd[WRITE], &token, sizeof(int)) < 0) {
                    fprintf(stderr, "%s: write error: %s\n", argv[0], strerror(errno));
                    exit(EXIT_FAILURE);
                }

                close(fd[WRITE]);
            }

            exit(EXIT_SUCCESS);
        }
        free(writePipe);
        free(readPipe);
    }


    // Processo pai espera que os processos filhos terminem
    for(int i = 0; i < numProc; i++) {
        if(waitpid(pids[i], NULL, 0) < 0) {
            fprintf(stderr, "%s: waitpid (%d) error: %s\n", argv[0], pids[i], strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    exit(EXIT_SUCCESS);
}