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
	FILE *out;        
	char line[BIGLINE];
	static Dictrec dr, blank;
	
	if (argc==3) {
		if ((in =fopen(argv[1],"r")) == NULL){DIE(argv[1]);}
		if ((out =fopen(argv[2],"w")) == NULL){DIE(argv[2]);}	
	}
	else{
		printf("Usage: convert [input file] [output file].\n");
		return -1;
	}

	while (fgets(line, BIGLINE, in)) { //讀取單字
		
		/* Create and fill in a new blank record. */
		dr = blank;

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

		while(fgets(line, BIGLINE, in)) { //讀取單字定義
			if (strcmp(line, "\n") == 0) {
				break;
			}

			size_t line_len = strlen(line);
			if (line_len >= space_left) {
				strncpy(text_ptr, line, space_left);
				text_ptr += space_left;
				space_left = 0;

				while(fgets(line, BIGLINE, in) && strcmp(line, "\n") != 0) { //處理過長的定義
				
				}
				break;
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