/*
 * redirect_in.c  :  check for <
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "shell.h"
#define STD_OUTPUT 1
#define STD_INPUT  0

/*
 * Look for "<" in myArgv, then redirect input to the file.
 * Returns 0 on success, sets errno and returns -1 on error.
 */
int redirect_in(char ** myArgv) {
  	int i = 0;
  	int fd;

  	/* search forward for <
  	 *
	 * Fill in code. */
	while (myArgv[i] != NULL) {
		if (strcmp(myArgv[i], "<") == 0) {
			break; /* Found "<" */
		}
		i++;
	}

  	if (myArgv[i]) {	/* found "<" in vector. */

    	//  * 1) Open file.
		fd = open(myArgv[i+1], O_RDONLY);
		if (fd < 0) {
			perror("open"); /* Report error */
			return -1; /* errno is set by open() */
		}

     	//  * 2) Redirect stdin to use file for input.
		if (dup2(fd, STD_INPUT) < 0) {
			perror("dup2");
			close(fd); /* Clean up file descriptor */
			return -1; /* errno is set by dup2() */
		}
   		
		//  * 3) Cleanup / close unneeded file descriptors.
		close(fd);

   		//  * 4) Remove the "<" and the filename from myArgv.
		myArgv[i] = NULL;
   		//  * Fill in code. 
  	}
  	return 0;
}
