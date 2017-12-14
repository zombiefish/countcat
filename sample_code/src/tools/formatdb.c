#ident "@(#)formatdb.c	1.9 9/21/88 16:37:52"

/* @(#)formatdb.c	1.9 - last delta: 9/21/88 16:37:52 gotten 9/21/88 16:37:53 from /usr/local/src/db/src/tools/.SCCS/s.formatdb.c. */

/*   Format a t3 database.  If no parms specified then take the name of
** the database from the env vars DBDIR and DBNAME.  If they don't exist
** use the defaults supplied in t3daemon.h.
**/

#include "t3db.h"
#include "t3daemon.h"
#include "t3dbfmt.h"
#include <memory.h>
#include <string.h>
#include <pwd.h>

#define BLKSIZ  4096

extern char *getenv(), *malloc(),/* *cmdname,*/ *optarg;
char   cmdname[12];

extern char *dname();

extern struct passwd *getpwnam();

extern int	  errno, sys_nerr, optind, opterr;
extern char	 *sys_errlist[];

void usage(),
	 errmsg();

static char *dbname;
	 
main(ac, av)
int ac;
char **av;
{
	int i, len;
	char *fn = NULL, *cp;

	strcpy(cmdname, "formatdb");
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
	if(optind != ac)
		usage();

	if(fn == NULL) {
		char    *p;

		if( (p = getenv("DBNAME")) == NULL)
			p = DEFDBNAME;

		fn = malloc(strlen(p) + 1);
		strcpy(fn, p);
		if( (cp = getenv("DBDIR")) == NULL)
			cp = DEFDBDIR;
		len = strlen(fn) + strlen(cp) + 2;
		dbname = malloc(len);
		strcpy(dbname, cp);
		strcat(dbname, "/");
		strcat(dbname, fn);
	} else
		dbname = fn;

	format(dbname);
}

format(fn)
char *fn;
{
	struct stat stbuf;
	int     fd, c, size, dirlen;
	struct dbhdr dbhdr;
	char    *dbdir, *cp;

	if(stat(fn, &stbuf) >= 0) {
		fprintf(stderr, "%s: Are you sure you want to re-format %s? ",
				cmdname, fn);
		c = getchar();
		if(c != 'y' && c != 'Y')
			errmsg(1, "Format of %s cancelled!\n", fn);
	}
	if( (fd = creat(fn, S_IREAD | S_IWRITE)) < 0)
		errmsg(1, "Can't create %s: %s\n", fn, sys_errlist[errno]);
		
	if(flock(fn, 1) < 0)
		errmsg(1, "Can't lock %s, format cancelled\n", fn);

	memset(&dbhdr, '\0', sizeof(struct dbhdr));
	dbhdr.used = 0;
	dbhdr.maxent = -1;
	dbhdr.firstfree = sizeof(struct dbhdr);
	
	if( (size = write(fd, &dbhdr, sizeof(dbhdr))) != sizeof(dbhdr))
		errmsg(1, "Tried to write %d bytes, only wrote %d: %s!\n",
				sizeof(dbhdr), size, sys_errlist[errno]);
	if(unflock(fn) < 0)
		errmsg(1, "Can't unlock %s, format completed\n", fn);

	close(fd);

	dbdir = malloc(strlen(fn) + 1);
	strcpy(dbdir, fn);
	dirlen = strlen(dbdir);
	dname(dbdir);
	cp = malloc(strlen("rm -f ") + dirlen + strlen("/text/*") + 1);
	strcpy(cp, "rm -f ");
	strcat(cp, dbdir);
	strcat(cp, "/text/*");
	if(system(cp) != 0)
		errmsg(0, "Removal of text files incomplete\n");

	free(cp);
	cp = malloc(dirlen + strlen("/LOGSEQ") + 1);
	strcpy(cp, dbdir);
	strcat(cp, "/LOGSEQ");
	if(unlink(cp) != 0)
		errmsg(0, "Can't remove %s: %s\n", cp, sys_errlist[errno]);
	free(cp);

	cp = malloc(dirlen + strlen("/INCSEQ") + 1);
	strcpy(cp, dbdir);
	strcat(cp, "/INCSEQ");
	if(unlink(cp) != 0)
		errmsg(0, "Can't remove %s: %s\n", cp, sys_errlist[errno]);
	free(cp);
	
	cp = malloc(dirlen + strlen("/INCIDENTS") + 1);
	strcpy(cp, dbdir);
	strcat(cp, "/INCIDENTS");
	if(unlink(cp) != 0)
		errmsg(0, "Can't remove %s: %s\n", cp, sys_errlist[errno]);
	free(cp);

	cp = malloc(dirlen + strlen("/LOGINFO") + 1);
	strcpy(cp, dbdir);
	strcat(cp, "/LOGINFO");
	if(unlink(cp) != 0)
		errmsg(0, "Can't remove %s: %s\n", cp, sys_errlist[errno]);
	free(cp);

	cp = malloc(dirlen + strlen("/IGNORE") + 1);
	strcpy(cp, dbdir);
	strcat(cp, "/IGNORE");
	if(unlink(cp) != 0)
		errmsg(0, "Can't remove %s: %s\n", cp, sys_errlist[errno]);
	free(cp);

	cp = malloc(strlen("cp ") + strlen(IGNSRC) + 1 + dirlen + strlen("/IGNORE") + 1);
	strcpy(cp, "cp ");
	strcat(cp, IGNSRC);
	strcat(cp, " ");
	strcat(cp, dbdir);
	strcat(cp, "/IGNORE");
	if(system(cp) != 0)
		errmsg(0, "Can't copy IGNORE file to %s\n", dbdir);
	free(cp);

	cp = malloc(dirlen + 1 + strlen("/IGNORE"));
	strcpy(cp, dbdir);
	strcat(cp, "/IGNORE");
	if(mychown(cp) != 0)
		errmsg(0, "Can't chown %s to logbook\n", cp);
	if(chmod(cp, 0440) != 0)
		errmsg(0, "Can't chown %s to logbook\n", cp);
	free(cp);

	errmsg(0, "Format of %s complete\n", fn);
}

void
usage() {
	fprintf(stderr, "%s: Usage: %s -d dbname\n",
		cmdname, cmdname);
	exit(2);
}

/*   The errmsg routine takes a varargs-style parm list.  The first parm
** is the exit code and the remaining parms are printf-style args.
**/
/*VARARGS*/
void
errmsg(va_alist)
va_dcl
{
	int     exit_code;
	char    *fmt;
	va_list arg_ptr;

	va_start(arg_ptr);
	exit_code = va_arg( arg_ptr, int);

	fmt = va_arg(arg_ptr, char *);
	fprintf(stderr, "%s: ", cmdname);
	vfprintf(stderr, fmt, arg_ptr);
	if(exit_code != 0)
		exit(exit_code);
}

mychown(file)
char    *file;
{
	struct passwd *pwd;

	if( (pwd = getpwnam("logbook")) == NULL) {
		errmsg(0, "Can't get passwd entry for logbook\n");
		return 1;
	}

	if(chown(file, pwd->pw_uid, pwd->pw_gid) != 0)
		return -1;

	return 0;
}
