#ident "@(#)dumpdb.c	1.5 7/7/92 16:43:17"

/* @(#)dumpdb.c	1.5 - last delta: 7/7/92 16:43:17 gotten 7/7/92 16:43:18 from /usr/local/src/db/src/tools/.SCCS/s.dumpdb.c. */

/*		vi:set tabstop=4:		*/

/*   Dump a t3 database without using the daemon.  This routine will also
** consistency check the database insofar as it ensures that all bytes are
** read and that no byte is read twice.  It also checks that all I/O
** operations (lseek, read, etc.) complete successfully.  Any noted
** anomalies are reported.
**/

#include <stdio.h>
#include <fcntl.h>
#include <varargs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "t3db.h"
#include "t3dbfmt.h"

extern char *malloc(), *memchr(), *getenv(), *optarg;

char cmdname[12];
extern int optind, errno;

extern char *sys_errlist[];

void usage(), errmsg();

char okchars[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00 - 0x0f */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10 - 0x1f */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x20 - 0x2f */
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, /* 0x30 - 0x3f */
	-1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, /* 0x40 - 0x4f */
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, /* 0x50 - 0x5f */
	-1, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, /* 0x60 - 0x6f */
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, -1, -1, -1, -1, -1, /* 0x70 - 0x7f */
};

struct idbifc {
	int     fd;
	long        pos;
	struct dbrechdr *hdrp;
	struct dbrecbody *bodyp;
};
			
long bytes = 0,
	 usedlen = 0;

char *used = NULL;

main(ac, av)
int ac;
char **av;
{
	int     i, j, c, size, expsize;
	char    *fn = NULL;
	long    pos, pos1, pos2;
	struct dbhdr dbhdr;
	struct dblethd dblethd;
	struct dbrec dbrec;
	struct idbifc idb;
	struct stat stbuf;
	char *p;
	long missed = 0;
	int     map = 0;

	strcpy(cmdname, "dumpdb");

	while( (i = getopt(ac, av, "md:")) != EOF) {
		switch(i) {
		case 'd':
		    fn = malloc(strlen(optarg)+1);
		    strcpy(fn, optarg);
		    break;
		case 'm':
		    map = 1;
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

    if( (idb.fd = open(fn, O_RDONLY)) <= 0)
        errmsg(1, "Can't open %s: %s\n", fn, sys_errlist[errno]);

	expsize = sizeof(struct dbhdr);
    if( (size = read(idb.fd, &dbhdr, expsize)) != expsize)
		errmsg(1, "Short read of dbhdr in %s!  Only got %d bytes, expected %d\n",
				fn, size, expsize);

	updbytes(0L, size);

	for(i = 0; i < NUMLETS; i++) {
		for(c = 0; c < 128; c++)
			if(i == okchars[c])
				break;
		if( (pos = dbhdr.lethdpos[i]) == 0) {
			continue;
		}
		lseek(idb.fd, pos, 0);
		expsize = sizeof(struct dblethd);
		if( (size = read(idb.fd, &dblethd, expsize)) != expsize)
			errmsg(1, "Short read of dblethd in %s!  Only got %d bytes, expected %d\n",
					fn, size, expsize);

		updbytes(pos, expsize);

		for(j = 0; j < MAXDIRECTWORDSIZE-1; j++) {
			if( (pos1 = dblethd.sizes[j]) == 0)
				continue;

			idb.pos = pos1;
			getdbrec(&idb);
			printentry(&idb);

			do {
				long opos2 = pos2;
				pos2 = idb.bodyp->nxtsame;
				if( (pos2) ) {
					struct idbifc nidb;
	
					nidb.fd = idb.fd;
					nidb.hdrp = NULL;
					do {
						if(nidb.hdrp != NULL)
							free(nidb.hdrp);
						nidb.pos = pos2;
						getdbrec(&nidb);
						printentry(&nidb);
					} while( (pos2 = nidb.bodyp->nxtsame) );
					free(nidb.hdrp);
				}
	
				if( (pos2 = idb.bodyp->nxtwrd) ) {
					free(idb.hdrp);
					idb.pos = pos2;
					getdbrec(&idb);
					printentry(&idb);
				}
			} while(pos2);
			free(idb.hdrp);
		}
	}
	for(p = used, pos = usedlen; pos > 0; pos--)
		if(*(p++) != 0)
			bytes++;

	fstat(idb.fd, &stbuf);

	if(bytes != stbuf.st_size)
		printf("Error! Bytes read: %ld, expected: %ld\n",
			   bytes, stbuf.st_size);

	if(usedlen != stbuf.st_size)
		printf("Error! Recorded %ld (0x%08lx) bytes but file is %ld (0x%08lx) bytes\n",
				usedlen, usedlen, stbuf.st_size, stbuf.st_size);

	if(memchr(used, '\0', usedlen) != NULL) {
		char *p1;
		long lenleft;

		printf("Missed reading the following bytes:\n");
		p = used; lenleft = usedlen;
		while( (p = memchr(p, '\0', lenleft)) != NULL) {
			lenleft = usedlen - (p - used);
			p1 = p;
			while((lenleft > 0) && (*(++p1) == '\0'))
				lenleft--;

			printf("    0x%08lx-0x%08lx\n", p-used, p1-used);
			p = p1;
		}
		if(map)
			mapused();
	}
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

olddbrec(wordlen, nentries, idbifc)
int wordlen, nentries;
struct idbifc *idbifc;
{
	int size, sizehdr, sizebody;
	struct dbrechdr *hdr;
	struct dbrecbody *body;

	sizehdr = ((((sizeof(struct dbrechdr) + wordlen + 1)) + 3) /4) * 4;
	sizebody = (sizeof(struct dbrecbody) + sizeof(body->entry[0]) * (nentries - NUMDBRENTRIES));

	size = sizeof(struct dbrec) + wordlen + 1;
	size += (sizeof(body->entry[0]) * (nentries - NUMDBRENTRIES));
	hdr = idbifc->hdrp = (struct dbrechdr *)malloc(sizehdr);
	
	//	idbifc->bodyp = (struct dbrecbody *)((char *)hdr
	//			+ sizeof(struct dbrechdr) + wordlen + 1);
	idbifc->bodyp = (struct dbrecbody *)malloc(sizebody);

	
	return size;
}

int
getdbrec(idbifc)
struct idbifc *idbifc;
{
	struct dbrechdr dbrechdr;
	struct idbifc idb;
	int     rcdsize;
	int     hdrsize;

	idb = *idbifc;

	lseek(idbifc->fd, idbifc->pos, 0);

	read(idbifc->fd, &dbrechdr, sizeof(struct dbrechdr));
	
	rcdsize = olddbrec(dbrechdr.wordsz, dbrechdr.entries, &idb);
	hdrsize = dbrechdr.wordsz + 1;
	read(idbifc->fd, idb.hdrp->word, hdrsize);

	idbifc->hdrp = idb.hdrp; idbifc->bodyp = idb.bodyp;

	read(idbifc->fd, idb.bodyp, sizeof(struct dbrecbody));
	
	*(idbifc->hdrp) = dbrechdr;

	updbytes(idb.pos, rcdsize);

	return rcdsize;
}

printentry(idb)
struct idbifc *idb;
{
	int i, j;

	for(i = 0; i < idb->hdrp->used_entries; i++) {
	  j = idb->bodyp->entry[i];
		printf("%s %d\n", idb->hdrp->word, j);
	}
}

updbytes(pos, len)
long pos;
int len;
{

	long ousedlen;
	char *oused, *p;

	if(used == NULL) {
		usedlen = pos+len;
		used = malloc(usedlen);
		memset(used, 1, usedlen);
	} else if(usedlen < pos+len) {
		ousedlen = usedlen;
		usedlen = pos+len;
		oused = used;
		used = malloc(usedlen);
		memset(used, 0, usedlen);
		memcpy(used, oused, ousedlen);

		ousedlen = len;
		p = used + pos;
		while(ousedlen-- >= 0 && *(p++) == '\0')
			;

		if(ousedlen > 0) {
			printf("Error! Overlap of location 0x%08lx for ",
				   (p - oused));
			ousedlen = 0;
			while(*(p++) == '\0')
				ousedlen++;
			printf("%ld bytes\n",
					ousedlen);
		}

		memset(used+pos, 1, len);
		free(oused);
	} else {
		ousedlen = len;
		oused = used;
		p = oused + pos;
		while(ousedlen-- >= 0 && *(p++) == '\0')
			;

		if(ousedlen > 0) {
			printf("Error! Overlap of location 0x%08lx for ",
				   (p - oused));
			ousedlen = 0;
			while(*(p++) == '\0')
				ousedlen++;
			printf("%ld bytes\n",
					ousedlen);
		}

		memset(used+pos, 1, len);
	}
}

mapused() {
	char *p = used;
	long len = usedlen;
	char *endp = p + len;
	int     i;

	if(len > 0)
		do {
			printf("Offset: 0x%08lx ", p - used);
			for(i = 0; i < 32 && p < endp; i++) {
				if(i == 15)
					putchar(' ');
				if(*(p++) == 0)
					putchar('-');
            	else
                	putchar('U');
			}
			putchar('\n');
		} while( (len -= 32) >= 0);
}

/*** Local Variables: ***/
/*** tab-stops:(4 8 12 16 20 24 28 32 36 40 44 48 52 56 60 64 68 72 80) ***/
/*** tab-width:4 ***/
/*** End: ***/
