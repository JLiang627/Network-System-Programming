/*
 * builtin.c : check for shell built-in commands
 * structure of file is
 * 1. definition of builtin functions
 * 2. lookup-table
 * 3. definition of is_builtin and do_builtin
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/utsname.h>
#include <string.h> 
#include "shell.h"

/****************************************************************************/
/* builtin function definitions                                             */
/****************************************************************************/
static void bi_builtin(char ** argv);	/* "builtin" command tells whether a command is builtin or not. */
static void bi_cd(char **argv) ;		/* "cd" command. */
static void bi_echo(char **argv);		/* "echo" command.  Does not print final <CR> if "-n" encountered. */
static void bi_hostname(char ** argv);	/* "hostname" command. */
static void bi_id(char ** argv);		/* "id" command shows user and group of this process. */
static void bi_pwd(char ** argv);		/* "pwd" command. */
static void bi_quit(char **argv);		/* quit/exit/logout/bye command. */




/****************************************************************************/
/* lookup table                                                             */
/****************************************************************************/

static struct cmd {
  	char * keyword;					/* When this field is argv[0] ... */
  	void (* do_it)(char **);		/* ... this function is executed. */
} inbuilts[] = {
  	{ "builtin",    bi_builtin },   /* List of (argv[0], function) pairs. */

    /* Fill in code. */
    { "echo",       bi_echo },
    { "quit",       bi_quit },
    { "exit",       bi_quit },
    { "bye",        bi_quit },
    { "logout",     bi_quit },
    { "cd",         bi_cd },
    { "pwd",        bi_pwd },
    { "id",         bi_id },
    { "hostname",   bi_hostname },
    {  NULL,        NULL }          /* NULL terminated. */
};


static void bi_builtin(char ** argv) {
	/* Fill in code.*/
	if (argv[1] == NULL) {
		fprintf(stderr, "builtin: usage: builtin [command]\n");
		return;
	}
	
	if (is_builtin(argv[1])) {
		printf("%s is a builtin feature.\n", argv[1]);
	} else {
		printf("%s is NOT a builtin feature.\n", argv[1]);
	}
}

static void bi_cd(char **argv) {
	/* Fill in code.*/
	char *dir;
	if (argv[1] == NULL) {
		/* No directory specified, try HOME */
		dir = getenv("HOME");
		if (dir == NULL) {
			fprintf(stderr, "cd: HOME not set\n");
			return;
		}
	} else {
		dir = argv[1];
	}

	if (chdir(dir) != 0) {
		/* chdir failed */
		perror("cd");
	}
}

static void bi_echo(char **argv) {
	/* Fill in code. */
	int i = 1;
	/* Simple echo: print all arguments starting from argv[1] */
	while (argv[i] != NULL) {
		printf("%s ", argv[i]);
		i++;
	}
	printf("\n");
}

static void bi_hostname(char ** argv) {
	/* Fill in code.  */
	char host[HOST_NAME_MAX + 1]; /* Use HOST_NAME_MAX from limits.h */
	if (gethostname(host, sizeof(host)) == 0) {
		printf("hostname: %s\n", host);
	} else {
		perror("gethostname");
	}
}

static void bi_id(char ** argv) {
 	/* Fill in code.  */
	uid_t uid = getuid();
	gid_t gid = getgid();
	struct passwd *pw = getpwuid(uid);
	struct group *gr = getgrgid(gid);

	char *username = (pw) ? pw->pw_name : "unknown";
	char *groupname = (gr) ? gr->gr_name : "unknown";

	printf("UserID = %d (%s), GroupID = %d (%s)\n", uid, username, gid, groupname);
}

static void bi_pwd(char ** argv) {
	/* Fill in code.  */
	char *cwd = NULL;
	
	/* Use getcwd with dynamic allocation */
	cwd = getcwd(NULL, 0); 

	if (cwd != NULL) {
		printf("%s\n", cwd);
		free(cwd); /* Free the memory allocated by getcwd */
	} else {
		perror("pwd"); /* Report error */
	}
}

static void bi_quit(char **argv) {
	exit(0);
}


/****************************************************************************/
/* is_builtin and do_builtin                                                */
/****************************************************************************/

static struct cmd * this; /* close coupling between is_builtin & do_builtin */

/* Check to see if command is in the inbuilts table above.
Hold handle to it if it is. */
int is_builtin(char *cmd) {
	struct cmd *tableCommand;

  	for (tableCommand = inbuilts ; tableCommand->keyword != NULL; tableCommand++)
    	if (strcmp(tableCommand->keyword,cmd) == 0) {
      		this = tableCommand;
      		return 1;
    	}
  return 0;
}


/* Execute the function corresponding to the builtin cmd found by is_builtin. */
int do_builtin(char **argv) {
	this->do_it(argv);
	return 0; /* Return value for consistency, though not strictly used in shell.c */
}