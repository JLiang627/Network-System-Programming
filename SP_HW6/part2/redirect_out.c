/*
 * redirect_out.c   :   check for >
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>  
#include <string.h> /* Added for strcmp */
#include "shell.h"
#define STD_OUTPUT 1
#define STD_INPUT  0

/*
 * Look for ">" in myArgv, then redirect output to the file.
 * Returns 0 on success, sets errno and returns -1 on error.
 */
int redirect_out(char ** myArgv) {
	int i = 0;
  	int fd;

  	/* search forward for >
  	 * Fill in code. */
	while (myArgv[i] != NULL) {
		if (strcmp(myArgv[i], ">") == 0) {
			break; /* Found ">" */
		}
		i++;
	}

  	if (myArgv[i]) {	/* found ">" in vector. */

    	// Open file.
    	fd = open(myArgv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if (fd < 0) {
			perror("open");
			return -1; 
		}
    	
		// Redirect to use it for output.
		if (dup2(fd, STD_OUTPUT) < 0) {
			perror("dup2");
			close(fd); 
			return -1; 
		}

		close(fd);

		myArgv[i] = NULL;

    	// * Fill in code. */
  	}
  	return 0;
}
