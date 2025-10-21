/* * pipe_command.c  :  deal with pipes
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h> /* <-- waitpid  */

#include "shell.h"

#define STD_OUTPUT 1
#define STD_INPUT  0

void pipe_and_exec(char **myArgv) {
  	int pipe_argv_index = pipe_present(myArgv);
  	int pipefds[2];
	char **left_argv;
	char **right_argv;
	pid_t pid;

  	switch (pipe_argv_index) {

    	case -1:	/* Pipe at beginning or at end of argv. */
      		fputs ("Missing command next to pipe in commandline.\n", stderr);
      		errno = EINVAL;
			perror("pipe_present");
  			exit(errno);

    	case 0:	/* No pipe found. */
      		execvp(myArgv[0], myArgv);
			perror("execvp");
  			exit(errno);

    	default:	/* Pipe in the middle. */

			left_argv = myArgv;
			right_argv = &myArgv[pipe_argv_index + 1];
			myArgv[pipe_argv_index] = NULL;

			if (pipe(pipefds) < 0) {
				perror("pipe");
				exit(errno);
			}

      		switch(pid = fork()) {

        		case -1 :
	  				perror("fork");
	  				exit(errno);

        		/* (cat mess) */
        		case 0 :
	  				/* Redirect output to the pipe's write end */
					if (dup2(pipefds[1], STD_OUTPUT) < 0) {
						perror("dup2 (child output)");
						exit(errno);
					}
	 				close(pipefds[0]); /* Don't need read end */
					close(pipefds[1]); /* Don't need write end (after dup) */
	 				
					execvp(left_argv[0], left_argv);
					perror("execvp (left side)");
	  				exit(errno);

        		/* (sort -u) */
        		default :
	  				/* Redirect input from the pipe's read end */
					if (dup2(pipefds[0], STD_INPUT) < 0) {
						perror("dup2 (parent input)");
						exit(errno);
					}
					close(pipefds[0]); /* Don't need read end (after dup) */
					close(pipefds[1]); /* Don't need write end */

					/* *
					 * 等待左側(子程序)執行完畢。
					 * cat mess 會先結束。
					 */
					waitpid(pid, NULL, 0);
          			pipe_and_exec(right_argv);
					exit(errno); 
			}
	}
	
	/* Safety net */
	perror("pipe_and_exec: unreachable code reached");
  	exit(errno);
}