/*
 * parse.c : use whitespace to tokenise a line
 * Initialise a vector big enough
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"

/* Parse a commandline string into an argv array. */
char ** parse(char *line) {

  	static char delim[] = " \t\n"; /* SPACE or TAB or NL */
  	int count = 0;
  	char * token;
  	char **newArgv;

  	/* Nothing entered. */
  	if (line == NULL || strcmp(line,"\n")==0) {
    	return NULL;
  	}

  	/* Init strtok with commandline, then get first token.
     * Return NULL if no tokens in line.
	 *
	 * Fill in code.
     */
	token = strtok(line, delim);
	if (token == NULL) {
		return NULL;
	}

  	/* Create array with room for first token.
  	 *
	 * Fill in code.
	 */
	/* Allocate space for one token pointer */
	newArgv = (char **)malloc(sizeof(char *));
	if (newArgv == NULL) {
		perror("malloc");
		exit(1);
	}
	newArgv[count] = strdup(token); /* Copy token */
	count++;


  	/* While there are more tokens...
	 *
	 * - Get next token.
	 *	- Resize array.
	 * - Give token its own memory, then install it.
	 * * Fill in code.
	 */
	while ((token = strtok(NULL, delim)) != NULL) {
		/* Resize array for one more token pointer */
		newArgv = (char **)realloc(newArgv, (count + 1) * sizeof(char *));
		if (newArgv == NULL) {
			perror("realloc");
			exit(1);
		}
		newArgv[count] = strdup(token); /* Copy token */
		count++;
	}


  	/* Null terminate the array and return it.
	 *
  	 * Fill in code.
	 */
	/* Resize array for the final NULL pointer */
	newArgv = (char **)realloc(newArgv, (count + 1) * sizeof(char *));
	if (newArgv == NULL) {
		perror("realloc");
		exit(1);
	}
	newArgv[count] = NULL; /* Add NULL terminator */


  	return newArgv;
}


/*
 * Free memory associated with argv array passed in.
 * Argv array is assumed created with parse() above.
 */
void free_argv(char **oldArgv) {

	int i = 0;

	/* Free each string hanging off the array.
	 * Free the oldArgv array itself.
	 *
	 * Fill in code.
	 */
	
	/* Free each string (created by strdup) */
	while (oldArgv[i] != NULL) {
		free(oldArgv[i]);
		i++;
	}

	/* Free the array of pointers */
	free(oldArgv);
}