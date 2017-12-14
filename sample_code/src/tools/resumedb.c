#ident "@(#)resumedb.c	1.2 3/13/92 17:16:19"

/*	vi:set tabstop=4:	*/

/* @(#)resumedb.c	1.2 - last delta: 3/13/92 17:16:19 gotten 3/13/92 17:16:22 from /usr/local/src/db/src/tools/.SCCS/s.resumedb.c. */

/*   Resume a quiesced database.  Calls resumedb() to cause the daemon
** for the database to resume processing (i.e. allow update requests).
** Command format:
**        resumedb -D path-to-database
**/

#include <stdio.h>
#include <fcntl.h>
#include "t3db.h"
#include "t3dberr.h"
#include <string.h>

extern int errno, optind, opterr;

extern char /**cmdname,*/ *optarg, *malloc(), *sys_errlist[];

char cmdname[14];

main(argc, argv)
int		argc;
char	**argv;
{
	int     i, val;
	char	*dbname = NULL;

	strcpy(cmdname, "resumedb");
	while( (i = getopt(argc, argv, "D:")) != EOF) {
		switch(i) {
		case 'D':
			dbname = malloc(strlen(optarg) + 1);
			strcpy(dbname, optarg);
			break;
		  case '?':
			usage();
			break;
		}
	}

	if(optind != argc || dbname == NULL)
		usage();

	if( (val = opendb(dbname, O_RDONLY)) != 0) {
		if(val == -1)
			fprintf(stderr, "Can't open database %s: %s\n", dbname, sys_errlist[errno]);
		else
			fprintf(stderr, "Can't open database %s: %d\n", dbname, val);
		exit(1);
	}

    if( (val = resumedb()) != 0) {
		fprintf(stderr, "Can't resume db: got %d\n", val);
		exit(1);
	}

	if( (val = closedb()) != 0) {
		fprintf(stderr, "Can't close database %s, got %d\n", dbname, val);
		exit(1);
	}

	exit(0);
}

usage() {
	fprintf(stderr, "%s: Usage: %s -D path-to-database\n", cmdname, cmdname);
	exit(1);
}
