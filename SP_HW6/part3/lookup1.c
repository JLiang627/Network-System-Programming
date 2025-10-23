/*
 * lookup1 : straight linear search through a local file
 * of fixed length records. The file name is passed
 *	         as resource.
 */
#include <string.h>
#include "dict.h" 

int lookup(Dictrec * sought, const char * resource) {
	Dictrec dr;
	static FILE * in;
	static int first_time = 1;

	if (first_time) { 
		first_time = 0;
		/* open up the file
		 *
		 * Fill in code. */
		if ((in = fopen(resource, "r")) == NULL) {
			/* Let main() handle the error reporting */
			return UNAVAIL;
		}
	}

	/* read from top of file, looking for match
	 *
	 * Fill in code. */
	rewind(in); /* Start from the beginning of the file */
	while(fread(&dr, sizeof(Dictrec), 1, in) == 1) {
		/* Fill in code. */
		if (strcmp(dr.word, sought->word) == 0) {
			/* Found it! Copy the definition back. */
			strcpy(sought->text, dr.text);
			return FOUND;
		}
	}

	return NOTFOUND;
}