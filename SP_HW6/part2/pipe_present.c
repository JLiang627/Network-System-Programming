/*
 *  pipe_present.c :  check for |
 */

#include <string.h>
#include "shell.h"

/*
 * Return index offset into argv of where "|" is,
 * -1 if in an illegal position (first or last index in the array),
 * or 0 if not present.
 */
int pipe_present(char ** myCurrentArgv) {
	int index = 0;

  	/* Search through myCurrentArgv for a match on "|". */
	while (myCurrentArgv[index] != NULL) {
		if (strcmp(myCurrentArgv[index], "|") == 0) {
			break; /* Found it */
		}
		index++;
	}

  	if (myCurrentArgv[index] == NULL) {
    	/* Off the end (not found) */
    	return 0;

  	} else if (index == 0 || myCurrentArgv[index+1] == NULL) {
		/* At the beginning or at the end (illegal) */
    	return -1;

  	} else {
    	/* In the middle. */
    	return index;
  	}
}
