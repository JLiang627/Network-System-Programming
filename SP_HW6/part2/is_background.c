/*
 * is_background.c :  check for & at end
 */

#include <string.h> /* Added for strcmp */
#include "shell.h"

int is_background(char ** myArgv) {

  	if (*myArgv == NULL) /* Check if myArgv[0] is NULL */
    	return FALSE;

  	/* Look for "&" in myArgv, and process it.
  	 *
	 *	- Return TRUE if found.
	 *	- Return FALSE if not found.
	 *
	 * Fill in code.
	 */
	int i = 0;
	
	/* Find the end of the array */
	while (myArgv[i] != NULL) {
		i++;
	}

	/* i is now the index of the NULL terminator.
	 * The last argument is at index i-1.
	 * We know i > 0 because we checked for *myArgv == NULL.
	 */
	if (strcmp(myArgv[i-1], "&") == 0) {
		/* Found "&" at the end */
		myArgv[i-1] = NULL; /* Remove it from the argv array */
		return TRUE;
	}

  	return FALSE; /* Not found */
}