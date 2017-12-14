#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <values.h>
#include <string.h>
#include <ctype.h>
#define _XOPEN_SOURCE
#include <time.h>
#include <varargs.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>
#include <sys/file.h>

extern char *sys_errlist[];


#define F_ULOCK	0
#define	F_LOCK	1
#define	F_TLOCK	2
#define	F_TEST	3
#define	LOCK_S	1
#define	LOCK_EX	2
#define	LOCK_NB	4
#define	LOCK_UN	8

char	cmdname[256];
char	Himem[100];

int	sindex(char *str, char *srch)
{
	char	*result;
	int	index;

	result = strstr(str, srch);
	if(result == NULL) {
		return -1;
	}
	return (*result - *str);
}

int	wd_flock(char *filename, int bits)
{
	char	flockfn[1024];
	int	fd, rc;

	strcpy(flockfn, filename);
	strcat(flockfn, ".lk");
	fprintf(stderr, "About to do link\n");
	link(filename, flockfn);
	fprintf(stderr, "About to do open\n");
	if((fd = open(flockfn, O_CREAT | O_WRONLY)) < 0) {
		return fd;
	}
	fprintf(stderr, "About to do link\n");
	if (rc = lockf(fd, F_TLOCK) < 0)
	    fprintf(stderr, "Can't lock %s errno is: %s\n",flockfn, sys_errlist[errno]);
	close(fd);
	return(rc);
}

int	wd_unflock(char *filename)
{
	char	flockfn[1024];
	int	fd, rc ;

	strcpy(flockfn, filename);
	strcat(flockfn, ".lk");
	if((fd = open(flockfn, O_RDWR)) < 0) {
		return fd;
	}
	rc = lockf(fd, F_ULOCK);
	close(fd);
	unlink(flockfn);
}

