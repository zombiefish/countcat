#ident "@(#)t3redump.c	1.4 3/13/92 17:16:20"

/*	vi:set tabstop=4:	*/

/* @(#)t3redump.c	1.4 - last delta: 3/13/92 17:16:20 gotten 3/13/92 17:16:22 from /usr/local/src/db/src/tools/.SCCS/s.t3redump.c. */

/*   Dump a t3 database by using r.e. matches.  Useful to compare this
** output to that of t3seqdump which uses firstword() and nextword() (a
** sequential dump, sort of).
**/

#include <stdio.h>
#include <fcntl.h>
#include <varargs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "t3db.h"
#include "t3dbfmt.h"

extern char *malloc(), *memchr(), *getenv(),/*  *cmdname,*/ *optarg;

char cmdname[14];

extern int optind, errno;

extern char *sys_errlist[];

void usage(), errmsg();

main(ac, av)
int ac;
char **av;
{
	int     i, val;
	char    *fn = NULL, pattern[4];
	struct dbrec *dbrec;

	strcpy(cmdname, "t3redump");

	while( (i = getopt(ac, av, "d:")) != EOF) {
		switch(i) {
		case 'd':
		    fn = malloc(strlen(optarg)+1);
		    strcpy(fn, optarg);
		    break;
		case '?':
		    usage();
		}
	}
	if(ac != optind)
		usage();

	if(fn == NULL) {
		char    *p, *p1;
		if( (p = getenv("DBDIR")) == NULL)
			p = DEFDBDIR;
		if( (p1 = getenv("DBNAME")) == NULL)
			p1 = DEFDBNAME;

		fn = malloc(strlen(p) + strlen(p1) + 2);
		strcpy(fn, p);
		strcat(fn, "/");
		strcat(fn, p1);
	}

    if( (val = opendb(fn, O_RDONLY)) < 0) {
		if(val == -1)
			errmsg(1, "Can't opendb %s: %s\n", fn, sys_errlist[errno]);
		else
			errmsg(1, "Can't opendb %s: %d\n", fn, val);
	}

	for(i = '0'; i <= 'z'; i++) {
		if(i > '9' && i < 'a')
			i = 'a';

		sprintf(pattern, "%c.*", i);

		if(findre(pattern, &dbrec) > 0)
			do {
				do {
						printentry(dbrec);
				} while( (val = nextsame(&dbrec)) > 0);
			} while( (val = nextre(&dbrec)) > 0);
	}
	exit(0);
}

void
usage() {
	fprintf(stderr, "%s: Usage: %s [-d dbname]\n",
		cmdname, cmdname);
	exit(2);
}

/*   The errmsg routine takes a varargs-style parm list.  The first parm
** is the exit code, the 2nd is the filename, the 3rd is the file desc
** and the remaining parms are printf-style args.
**/
/*VARARGS*/
void
errmsg(va_alist)
va_dcl
{
	int         exit_code;
	char        *fmt;
	va_list     arg_ptr;

	va_start(arg_ptr);
	exit_code = va_arg( arg_ptr, int);

	fmt = va_arg(arg_ptr, char *);
	fprintf(stderr, "%s: ", cmdname);
	vfprintf(stderr, fmt, arg_ptr);

	exit(exit_code);
}

printentry(dbrec)
struct dbrec *dbrec;
{
	int i;
	struct dbrecbody *dbrecb;

	dbrecb = (struct dbrecbody *)(((char *)&(dbrec->dbrecb)) + strlen(dbrec->dbrech.word) + 1);

	for(i = 0; i < dbrec->dbrech.used_entries; i++)
		printf("%s %d\n", dbrec->dbrech.word, dbrecb->entry[i]);
}

