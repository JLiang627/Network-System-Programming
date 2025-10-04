/*
 * run_command.c :    do the fork, exec stuff, call other functions
 */


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h> // 加入這行以使用 execlp 函數
#include "shell.h"
#include <string.h> // 加入這行以使用 strcmp 函數

void run_command(char **myArgv) {
    pid_t pid;
    int stat;

    /* Create a new child process. */
    pid = fork();

    switch (pid) {

        /* Error. */
        case -1 :
            perror("fork");
            exit(errno);

        /* Parent. */
        default :
            /* Wait for child to terminate. */
            if (waitpid(pid, &stat, 0) == -1) {
                perror("waitpid");
                exit(errno);
            }

            /* Display exit status with detailed checks. */
            if (WIFSIGNALED(stat)) {
                printf("Child terminated by signal %d.\n", WTERMSIG(stat));
            } else if (WIFEXITED(stat)) {
                printf("Child exited with status %d.\n", WEXITSTATUS(stat));
            } else if (WIFSTOPPED(stat)) {
                printf("Child stopped by signal %d.\n", WSTOPSIG(stat));
            }

            return;

        /* Child. */
        case 0 :
            /* Print myArgv in child process. */
            for (int i = 0; myArgv[i] != NULL; i++) {
                printf("[%d] : %s\n", i, myArgv[i]);
            }
            
            /* Run command in child process. */
            if (strcmp(myArgv[0], "ls") == 0)
                execlp("ls", "ls", "-l", (char *)NULL);
            perror("execlp");
            /* Handle error return from exec */
            exit(errno);
    }
}
