#ident "@(#)checkdb.c	1.3 7/7/92 16:35:20"

/* @(#)checkdb.c	1.3 - last delta: 7/7/92 16:35:20 gotten 7/7/92 16:35:22 from /usr/local/src/db/src/tools/.SCCS/s.checkdb.c.                   */

/*	vi:set tabstop=4:						*/

/*   Verify the integrity of a database.                                */

#include <stdio.h>
#include <fcntl.h>
#include <varargs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <t3db.h>
#include "t3dbfmt.h"

char cmdname[10];
extern char *malloc(), *memchr(), *getenv(), *optarg;

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

long filesize;

main(ac, av)
int ac;
char **av;
{
	int     i, j, c, size, expsize, exit_code = 0;
	char    *fn = NULL;
	long    pos, pos1, pos2;
	struct dbhdr dbhdr;
	struct dblethd dblethd;
	struct dbrec dbrec;
	struct idbifc idb;
	struct stat stbuf;
	char *p;
	long missed = 0;

	strcpy(cmdname, "checkdb");
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
	if(ac != optind || fn == NULL)
		usage();

    if( (idb.fd = open(fn, O_RDONLY)) <= 0)
        errmsg(1, "Can't open %s: %s\n", fn, sys_errlist[errno]);
fprintf(stderr, "open successful for %s\n", fn);
	fstat(idb.fd, &stbuf);
	filesize = stbuf.st_size;
			
	expsize = sizeof(struct dbhdr);
    if( (size = read(idb.fd, &dbhdr, expsize)) != expsize)
		errmsg(1, "Short read of dbhdr in %s!  Only got %d bytes, expected %d\n",
				fn, size, expsize);
fprintf(stderr, "read header successful for %s\n", fn);

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

			if(pos1 > filesize)
				errmsg(1, "sizes: seek to %lx from sizes[%d]\n", pos1, j);

			idb.pos = pos1;
			getdbrec(&idb);
			do {
				if( (pos2 = idb.bodyp->nxtsame) ) {
					struct idbifc nidb;
	
					nidb.fd = idb.fd;
					nidb.hdrp = NULL;
					nidb.pos = idb.pos;
					do {
						if(nidb.hdrp != NULL)
							free(nidb.hdrp);
						if(pos2 > filesize)
							errmsg(1, "nxtsame to pos2 (%ld) from (BAD) %ld invalid, filesize %d\n",
									   pos2, nidb.pos, filesize);
					
						nidb.pos = pos2;
						getdbrec(&nidb);
					} while( (pos2 = nidb.bodyp->nxtsame) );
					free(nidb.hdrp);
				}
	
				if( (pos2 = idb.bodyp->nxtwrd) ) {
					free(idb.hdrp);
					if(pos2 > filesize)
						errmsg(1, "nxtwrd to pos2 (%lx) from (BAD) %lx invalid\n",
								   pos2, idb.pos);
			
					idb.pos = pos2;
					getdbrec(&idb);
				}
			} while(pos2);
			free(idb.hdrp);
		}
	}

fprintf(stderr, "bigloop successful");
	for(p = used, pos = usedlen; pos > 0; pos--)
		if(*(p++) != 0)
			bytes++;

	fstat(idb.fd, &stbuf);

    if(bytes != stbuf.st_size)
		errmsg(1, "Error! Only accounted for %ld bytes, expected %ld bytes\n",
			   bytes, stbuf.st_size);

	if(usedlen != stbuf.st_size)
		errmsg(1, "Error! Recorded %ld (0x%08lx) bytes but file size is %ld (0x%08lx) bytes\n",
				usedlen, usedlen, stbuf.st_size, stbuf.st_size);

	if(memchr(used, '\0', usedlen) != NULL) {
		char *p1;
		long lenleft;

		doprintf("Missed reading the following bytes:\n");
		p = used; lenleft = usedlen;
		while( (p = memchr(p, '\0', lenleft)) != NULL) {
			lenleft = usedlen - (p - used);
			p1 = p;
			while((lenleft > 0) && (*(++p1) == '\0'))
				lenleft--;

			doprintf("    0x%08lx-0x%08lx\n", p-used, p1-used);
			p = p1;
		}
		exit_code = 1;
	}

	exit(exit_code);
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
  //	int size;
  //	struct dbrechdr *hdr;
  //	struct dbrecbody *body;
  //
  //size = sizeof(struct dbrec) + wordlen + 1;
  //size += (sizeof(body->entry[0]) * (nentries - NUMDBRENTRIES));
  //hdr = idbifc->hdrp = (struct dbrechdr *)malloc(size);
  //
  //idbifc->bodyp = (struct dbrecbody *)((char *)hdr
  //				+ sizeof(struct dbrechdr) + wordlen + 1);
  //
  //return size;
  int size, sizehdr, sizebody;
  struct dbrechdr *hdr;
	struct dbrecbody *body;

	sizehdr = ((((sizeof(struct dbrechdr) + wordlen + 1)) + 3) /4) * 4;
	sizebody = (sizeof(struct dbrecbody) + sizeof(body->entry[0]) * (nentries - NUMDBRENTRIES));

	size = sizeof(struct dbrec) + wordlen + 1;
	size += (sizeof(body->entry[0]) * (nentries - NUMDBRENTRIES));
	hdr = idbifc->hdrp = (struct dbrechdr *)malloc(sizehdr+sizebody);
	
	//	idbifc->bodyp = (struct dbrecbody *)((char *)hdr
	//			+ sizeof(struct dbrechdr) + wordlen + 1);
	//  idbifc->bodyp = (struct dbrecbody *)malloc(sizebody);

	idbifc->bodyp = (struct dbrecbody *)((char *)hdr+sizehdr);

	
	return size;
}

int
getdbrec(idbifc)
struct idbifc *idbifc;
{
	struct dbrechdr dbrechdr;
	struct idbifc idb;
	int     rcdsize, hdrsize;

	idb = *idbifc;

	if(idbifc->pos > filesize)
		errmsg(1, "Position %lx larger than file size\n", idbifc->pos);

	lseek(idbifc->fd, idbifc->pos, 0);
	
	/* read in the static portion of the database entry header */
	read(idbifc->fd, &dbrechdr, sizeof(struct dbrechdr));

	hdrsize = dbrechdr.wordsz + 1;
	
	if(dbrechdr.entries > NUMDBRENTRIES)
		errmsg(1, "Num entries INVALID at 0x%lx\n", idbifc->pos);

	rcdsize = olddbrec(dbrechdr.wordsz, dbrechdr.entries, &idb);
	idbifc->hdrp = idb.hdrp; idbifc->bodyp = idb.bodyp;


	/* now read in the variable portion of the dbrechdr */       
	read(idbifc->fd, idb.hdrp->word, hdrsize);

	/* read(idbifc->fd, idb.hdrp->word, rcdsize - sizeof(struct dbrechdr)); */
	read(idbifc->fd, &idb.bodyp->nxtwrd, sizeof(struct dbrecbody)); 
	
	*(idbifc->hdrp) = dbrechdr;

	updbytes(idb.pos, rcdsize);

	return rcdsize;
}

updbytes(pos, len)
long pos;
int len;
{

	long ousedlen, maxval, i, incr;
	char *oused, *p;

	maxval = 0xffffff;	/* memset() and memcpy() can't do more than this */

	if(pos + len > filesize)
		errmsg(1, "More bytes referenced than used; pos 0x%lx, len 0x%x\n",
				  pos, len);

	if(used == NULL) {
		usedlen = pos+len;
		p = used = malloc(usedlen);

		/*   Have to do the following instead of a simple
		** memset(used, 1, usedlen);
		** because memset() doesn't properly handle setting more than
		** 16M-1 bytes in one shot.  Note that bzero() has the same problem
		** while bcopy() doesn't.
		**/
		incr = usedlen < maxval ? usedlen : maxval;

		for(i = usedlen; (i-incr) > 0; i -= incr) {
			memset(p, 1, incr);
			p += incr;
		}
		if(i > 0)
			memset(p, 1, i);
	} else if(usedlen < pos+len) {
		ousedlen = usedlen;
		usedlen = pos+len;
		oused = used;
		used = malloc(usedlen);

		/*   Have to do the following instead of a simple
		** memset(used, 0, usedlen);
		** because memset() doesn't properly handle setting more than
		** 16M-1 bytes in one shot.  Note that bzero() has the same problem
		** while bcopy() doesn't.
		**/
		incr = usedlen < maxval ? usedlen : maxval;

		for(p = used, i = usedlen; (i-incr) > 0; i -= incr) {
			memset(p, 0, incr);
			p += incr;
		}
		if(i > 0)
			memset(p, 0, i);

		bcopy(oused, used, ousedlen);

		ousedlen = len;
		p = used + pos;
		while(ousedlen-- >= 0 && *(p++) == '\0')
			;

		if(ousedlen > 0) {
			doprintf("Error! Overlap of location 0x%08lx for ",
				   (p - oused));
			ousedlen = 0;
			while(*(p++) == '\0')
				ousedlen++;
			doprintf("%ld bytes\n",
					ousedlen);
			exit(1);
		}

		/*   Have to do the following instead of a simple
		** memset(used+pos, 1, len);
		** because memset() doesn't properly handle setting more than
		** 16M-1 bytes in one shot.  Note that bzero() has the same problem
		** while bcopy() doesn't.
		**/
		incr = len < maxval ? len : maxval;
		p = used + pos;

		for(i = len; (i-incr) > 0; i -= incr) {
			memset(p, 1, incr);
			p += incr;
		}
		if(i > 0)
			memset(p, 1, i);
		free(oused);
	} else {
		ousedlen = len;
		oused = used;
		p = oused + pos;
		while(ousedlen-- >= 0 && *(p++) == '\0')
			;

		if(ousedlen > 0) {
			doprintf("Error! Overlap of location 0x%08lx for ",
				   (p - oused));
			ousedlen = 0;
			while(*(p++) == '\0')
				ousedlen++;
			doprintf("%ld bytes\n",
					ousedlen);
			exit(1);
		}

		/*   Have to do the following instead of a simple
		** memset(used+pos, 1, len);
		** because memset() doesn't properly handle setting more than
		** 16M-1 bytes in one shot.  Note that bzero() has the same problem
		** while bcopy() doesn't.
		**/
		incr = len < maxval ? len : maxval;
		p = used + pos;

		for(i = len; (i-incr) > 0; i -= incr) {
			memset(p, 1, incr);
			p += incr;
		}
		if(i > 0)
			memset(p, 1, i);
	}
}

doprintf(va_alist)
va_dcl
{
    char        *fmt;
    va_list     arg_ptr;

    va_start(arg_ptr);
    fmt = va_arg(arg_ptr, char *);
    vprintf(fmt, arg_ptr);
	fflush(stdout);
}

/*** Local Variables: ***/
/*** tab-stops:(4 8 12 16 20 24 28 32 36 40 44 48 52 56 60 64 68 72 80) ***/
/*** tab-width:4 ***/
/*** End: ***/
