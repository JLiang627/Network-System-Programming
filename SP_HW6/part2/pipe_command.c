/* * pipe_command.c  :  deal with pipes
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h> /* <-- 為了 waitpid 需要加入 */

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

    	case 0:	/* No pipe found. Base case. */
      		execvp(myArgv[0], myArgv);
			perror("execvp (base case)");
  			exit(errno);

    	default:	/* Pipe in the middle. */

      		/* Split arg vector. */
			left_argv = myArgv;
			right_argv = &myArgv[pipe_argv_index + 1];
			myArgv[pipe_argv_index] = NULL;

      		/* Create a pipe. */
			if (pipe(pipefds) < 0) {
				perror("pipe");
				exit(errno);
			}

      		/* Fork a new process. */
      		switch(pid = fork()) {

        		case -1 : /* Fork error */
	  				perror("fork");
	  				exit(errno);

        		/* ---------------------------------------------------- */
        		/* ** 子程序 (Child) ** : 執行 *左側* 命令 (cat mess) */
        		/* ---------------------------------------------------- */
        		case 0 :
	  				/* Redirect output to the pipe's write end */
					if (dup2(pipefds[1], STD_OUTPUT) < 0) {
						perror("dup2 (child output)");
						exit(errno);
					}
	 				close(pipefds[0]); /* Don't need read end */
					close(pipefds[1]); /* Don't need write end (after dup) */
	 				
					/* * 執行 *左側* 命令。
					 * 注意：這裡 *不是* 遞迴呼叫，
					 * 因為左側不需要再處理管道。
					 */
					execvp(left_argv[0], left_argv);
					perror("execvp (left side)");
	  				exit(errno);

        		/* ---------------------------------------------------- */
        		/* ** 父程序 (Parent) ** : 執行 *右側* 命令 (sort -u) */
        		/* ---------------------------------------------------- */
        		default :
	  				/* Redirect input from the pipe's read end */
					if (dup2(pipefds[0], STD_INPUT) < 0) {
						perror("dup2 (parent input)");
						exit(errno);
					}
					close(pipefds[0]); /* Don't need read end (after dup) */
					close(pipefds[1]); /* Don't need write end */

					/* *
					 * 等待 *左側* (子程序) 執行完畢。
					 * 這樣 `cat mess` 會先結束。
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