/*
 * convert.c : take a file in the form 
 * word1
 * multiline definition of word1
 * stretching over several lines, 
 * followed by a blank line
 * word2....etc
 * convert into a file of fixed-length records
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "dict.h"
#define BIGLINE 512

int main(int argc, char **argv) {
	FILE *in;
	FILE *out;        /* defaults */
	char line[BIGLINE];
	static Dictrec dr, blank;
	
	/* If args are supplied, argv[1] is for input, argv[2] for output */
	if (argc==3) {
		if ((in =fopen(argv[1],"r")) == NULL){DIE(argv[1]);}
		if ((out =fopen(argv[2],"w")) == NULL){DIE(argv[2]);}	
	}
	else{
		printf("Usage: convert [input file] [output file].\n");
		return -1;
	}

	/* Main reading loop : read word first, then definition into dr */

	/* Loop through the whole file. */
	/* We read the word line first */
	while (fgets(line, BIGLINE, in)) {
		
		/* Create and fill in a new blank record. */
		dr = blank; /* Clear the record */

		/* Read word and put in record.  Truncate at the end of the "word" field.
		 *
		 * Fill in code. */
		line[strcspn(line, "\n")] = 0; /* Remove trailing newline */
		strncpy(dr.word, line, WORD - 1); /* Copy word, leave space for null */
		dr.word[WORD - 1] = '\0'; /* Ensure null-termination */


		/* Read definition, line by line, and put in record.
		 *
		 * Fill in code. */
		char *text_ptr = dr.text;
		size_t space_left = TEXT - 1; /* Space for null terminator */

		while(fgets(line, BIGLINE, in)) {
			if (strcmp(line, "\n") == 0) {
				break; /* Blank line means end of definition */
			}

			size_t line_len = strlen(line);
			if (line_len >= space_left) {
				/* Definition is too long, truncate */
				strncpy(text_ptr, line, space_left);
				text_ptr += space_left;
				space_left = 0;

				/* Consume the rest of the oversized definition */
				while(fgets(line, BIGLINE, in) && strcmp(line, "\n") != 0) {
					/* consuming... */
				}
				break; /* Move to next word */
			}
			
			/* Append line to text field */
			strcpy(text_ptr, line);
			text_ptr += line_len;
			space_left -= line_len;
		}
		*text_ptr = '\0'; /* Null-terminate the text */


		/* Write record out to file.
		 *
		 * Fill in code. */
		if (fwrite(&dr, sizeof(Dictrec), 1, out) != 1) {
			perror("fwrite");
			exit(errno);
		}
	}

	fclose(in);
	fclose(out);
	return 0;
}