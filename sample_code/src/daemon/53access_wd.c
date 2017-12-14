#ident "%W% %G% %U%"

/* %W% - last delta: %G% %U% gotten %H% %T% from %P%. */

/*   This is the code that the daemon uses to actually manipulate the
** database.  The following routines are currently supported:
**
**    opendb(dbifc, mode) - open the database using the passed mode.
**    closedb(dbifc)      - close the database.
**    addword(dbifc)      - Add a word to the database.  Entry in dbifc is
**                          the value that's added for the word that is in
**                          dbifc->data.
**    findword(dbifc)     - Find the value for the word in dbifc->data.
**    nextword(dbifc)     - Retrieve the next word.  Uses dbifc->pos as a
**                          starting point.
**    findre(dbifc)       - Find a word that matches the specified regular
**                          expression.
**    nextre(dbifc)       - Find the next word that matches the specified
**                          regular expression.
**
**   Additionally, the following internal routines are present:
**
**    makegoodword(wp)    - Turn the word pointed to by wp into a good
**                          word by removing any leading trash.  Trash is
**                          defined as and non-alphanumeric.
**    lethdndx(c)         - Return the index into dblethd[] for a given
**                          character.
**    newdbrec(wordlen, idbifc) - Allocate storage for a new dbrec struct.
**    newdbrec(wordlen, nentries, idbifc) - Allocate storage for an
**                          existing dbrec structure.
**    getdbrec(idbifc)    - Read in a dbrec structure.
**    putdbrec(idbifc, size) - Write out a dbrec structure of size size.
**    findwrd(wp, idbifc) - The real work part of findword().
**    nextsame(idbifc)    - Read in the record pointed to by
**                          idbifc->bodyp->nxtsame.
**    fndre(idbifc, wordlen) - Find the next word that matches the
**                          specified regular expression.
**    addmembuf(idbifc, size) - Add the dbrec to the memory buffer.
**    getmembuf(idbifc)   - Retrieve a dbrec from the memory buffer.
**    makemembuf()        - Create the memory buffer.
**    errmsg(va_alist)    - Write an error msg to stderr and, if exitcode
**                          is non-zero, exit otherwise return.
**/

#include <stdio.h>
#include <fcntl.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory.h>
#include <string.h>
#include <varargs.h>
#include "t3db.h"
#include "t3dbfmt.h"
#include "t3dberr.h"


#define NULL    0

#define BAD         255

#define WORDSIZE(len)   ((len > MAXDIRECTWORDSIZE) ? MAXDIRECTWORDSIZE-2 : len - 2)

#define MINWORDSIZE 2

#define DEFMAXBUF      0x00100000

int	maxbuf = DEFMAXBUF;

/*   Routines available external to this module                         */
int     firstword(),        /* Retrieves first record matching word     */
		nextword(),         /* Retrieves record pointed to by nxtwrd    */
		nextsame();         /* Retrieves record pointed to by nxtsame   */

extern int      errno, sys_nerr, optind, opterr;
extern char     *cmdname, *sys_errlist[];

extern char *calloc(), *malloc(), *mymalloc();

static void errmsg();

unsigned char okchars[256] = {
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0x00 - 0x0f */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0x10 - 0x1f */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0x20 - 0x2f */
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,BAD,BAD,BAD,BAD,BAD,BAD, /* 0x30 - 0x3f */
   BAD, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, /* 0x40 - 0x4f */
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,BAD,BAD,BAD,BAD,BAD, /* 0x50 - 0x5f */
   BAD, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, /* 0x60 - 0x6f */
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,BAD,BAD,BAD,BAD,BAD, /* 0x70 - 0x7f */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0x80 - 0x8f */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0x90 - 0x9f */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0xa0 - 0xaf */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0xb0 - 0xbf */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0xc0 - 0xcf */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0xd0 - 0xdf */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0xe0 - 0xef */
   BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD,BAD, /* 0xf0 - 0xff */
};

static struct dbhdr dbhdr;
static struct dblethd *dblethd[NUMLETS];

static char firstletter;


/*   Internal interface structure                                       */
struct idbifc {
	int     fd;                 /* File descriptor to use               */
	int     size;               /* Size of returned record              */
	off_t   pos;                /* Position of record in file           */
	struct dbrechdr *hdrp;      /* Pointer to the dbrechdr structure    */
	struct dbrecbody *bodyp;    /* Pointer to the dbrecbody structure   */
};
			
/*   Structure used to buffer records internally                        */
struct membuf {
	struct membuf *next;        /* Pointer to next record               */
	off_t       pos;            /* Position of record in file           */
	int         length;         /* Size of the following record         */
	char        data[0];         /* The record                           */
};

static struct membuf *bufhead = NULL,   /* Start of membuf      */
                     *lastbp = NULL,    /* Last used membuf     */
					 *endbp;            /* addr of end of allocated membufs */

static char *theword;           /* Pointer to the "in-process" word     */

static int getmembuf(struct idbifc *idbifc);
static int findwrd(char *wp, struct idbifc *idbifc);
static int makemembuf();
static int makegoodword(unsigned char *wp);
static int putdbrec(struct idbifc *idbifc, int size);
static int lethdndx(unsigned char c);
static int newdbrec(int wordlen, struct idbifc *idbifc);
static int fndre(struct idbifc *idbifc, int wordlen, char *pat);
static int getdbrec(struct idbifc *idbifc);
static int addmembuf(struct idbifc *idbifc,int  rcdsize);



/*   Open the database                                                  */
opendb(dbifc, mode)
struct dbifc *dbifc;
int     mode;
{
	struct stat stbuf;
	int         i, size;
	char        *fn = dbifc->fn;

	if(stat(fn, &stbuf) < 0)
		return CANTOPEN;

	if(wd_flock(fn, 1) < 0) {
		errmsg(0, "Can't lock %s\n", fn);
		return CANTLOCK;
	}

	if(mode & O_RDWR)
		mode |= O_SYNC;
	if( (dbifc->fd = open(fn, mode)) < 0)
		return(dbifc->fd);

	lseek(dbifc->fd, 0L, 0);

	if( (size = read(dbifc->fd, &dbhdr, sizeof(dbhdr))) != sizeof(dbhdr)) {
		errmsg(0, "%s corrupt - can't read header!\n", fn);
		return BADHDR;
	}

	for(i = 0; i < NUMLETS; i++) {
		dblethd[i] = (struct dblethd *)calloc(1, sizeof(struct dblethd));
		if(dbhdr.lethdpos[i]) {
			lseek(dbifc->fd, dbhdr.lethdpos[i], 0);
			if( (size = read(dbifc->fd, dblethd[i], sizeof(struct dblethd))) != sizeof(struct dblethd)) {
				int j;

				if(size == -1)
					errmsg(0, "%s corrupt - can't read letter header! %s\n",
							fn, sys_errlist[errno]);
				else
					errmsg(0, "%s corrupt - can't read letter header!\nExpected %d bytes, got %d\n",
							fn, sizeof(struct dblethd), size);
				for(j = 0; j <= i; j++)
					free(dblethd[j]);
				return BADLETHD;
			}
		}
	}
	
	return(dbifc->fd);
}

/*   Close the database                                                 */
closedb(dbifc)
struct dbifc *dbifc;
{
	wd_unflock(dbifc->fn);
		
	if(dbifc->fd < 0)
		return;

	return(close(dbifc->fd));
}

/*   Add a word to the database                                         */
int
addword(dbifc)
struct dbifc *dbifc;
{
	struct idbifc idb;
	int     size, wordlen, entry = dbifc->entry;
	char    *wp = dbifc->data;
	
	if( (wordlen = makegoodword(wp)) < 0)
		return wordlen;

	wp = theword;               /* This is set in makegoodword()    */

	idb.fd = dbifc->fd;

	if( (size = findwrd(wp, &idb)) == BADWORD)
		return BADWORD;
	else if(size == NOLETTER) {
		/*   We have no entry that starts with this letter so we must
		** allocate and write new dblethd and dbrec structures and
		** re-write the dbhdr structure. */

		int     rcdsize;
		off_t   opos, pos;

		opos = pos = dbhdr.firstfree;
		
		rcdsize = newdbrec(wordlen, &idb);
		strcpy(idb.hdrp->word, wp);
		idb.bodyp->nxtwrd = 0;
		idb.bodyp->nxtsame = 0;
		idb.bodyp->entry[0] = entry;
		idb.hdrp->used_entries = 1;
		idb.pos = pos;
		
		if( (size = putdbrec(&idb, rcdsize)) != rcdsize) {
			myfree(idb.hdrp);
			dbifc->errnum = errno;
			return BADWRITE;
		}

		dblethd[firstletter]->sizes[WORDSIZE(wordlen)] = pos;

		pos += sizeof(struct dbrec) + wordlen + 1;

		write(dbifc->fd, dblethd[firstletter], sizeof(struct dblethd));
		dbhdr.lethdpos[firstletter] = pos;
		
		pos += sizeof(struct dblethd);
		
		dbhdr.firstfree = pos;
		dbhdr.used += (pos - opos);
		if(entry > dbhdr.maxent)
			dbhdr.maxent = entry;

		lseek(dbifc->fd, 0L, 0);
		write(dbifc->fd, &dbhdr, sizeof(struct dbhdr));

		myfree(idb.hdrp);
	} else if(size == NOSIZE) {
		/*   We know that we don't have any record of this word size
		** so we can simply allocate a new dbrec structure and write
		** it then re-write the dblethd and dbhdr structures. */

		off_t   pos;
		int     rcdsize;

		/*   Seek to the end of the file, allocate a new record, initialize
		** it, write the record and free it */
		pos = dbhdr.firstfree;
		lseek(dbifc->fd, pos, 0);

		rcdsize = newdbrec(wordlen, &idb);
		strcpy(idb.hdrp->word, wp);
		idb.bodyp->nxtwrd = 0;
		idb.bodyp->nxtsame = 0;
		idb.bodyp->entry[0] = entry;
		idb.hdrp->used_entries = 1;
		idb.pos = pos;
		
		if( (size = putdbrec(&idb, rcdsize)) != rcdsize) {
			dbifc->errnum = errno;
			myfree(idb.hdrp);
			return BADWRITE;
		}
		myfree(idb.hdrp);
 
		/*   Update the dblethd structure, seek to its position in the
		** file and re-write it.  Currently, pos points to where the dbrec
		** was written. */
		dblethd[firstletter]->sizes[WORDSIZE(wordlen)] = pos;
		pos += rcdsize;     /* Can now update it to point beyond the rec */
 
		lseek(dbifc->fd, dbhdr.lethdpos[firstletter], 0);
		write(dbifc->fd, dblethd[firstletter], sizeof(struct dblethd));
		
		dbhdr.firstfree = pos;
		if(entry > dbhdr.maxent)
			dbhdr.maxent = entry;
 
		lseek(dbifc->fd, 0L, 0);
		write(dbifc->fd, &dbhdr, sizeof(struct dbhdr));
	} else if(size == NOWORD) {
		/*   We have words of this size but this word doesn't exist in the
		** database.  Simply create a record and link it in */

		struct idbifc nidb;
		off_t   npos, pos;
		int     nrcdsize, cmpval;

		nidb.fd = idb.fd;

		npos = pos = dbhdr.firstfree;
 
		nrcdsize = newdbrec(wordlen, &nidb);
 
		strcpy(nidb.hdrp->word, wp);
		nidb.bodyp->nxtwrd = 0;
		nidb.bodyp->nxtsame = 0;
		nidb.bodyp->nxtwrd = 0;
		nidb.hdrp->used_entries = 1;
		nidb.bodyp->entry[0] = entry;
		nidb.pos = npos;
		
		if( (cmpval = strcmp(idb.hdrp->word, wp)) > 0) {
			/*   Must link this word in at the head of the chain to
			** maintain ascending collating sequence */

			dblethd[firstletter]->sizes[WORDSIZE(wordlen)] = npos;
			nidb.bodyp->nxtwrd = idb.pos;
		} else
			/*   Link this word in after the word that firstword() got.
			** Will take care of idb links below. */

			nidb.bodyp->nxtwrd = idb.bodyp->nxtwrd;
 
		if( (size = putdbrec(&nidb, nrcdsize)) != nrcdsize) {
			dbifc->errnum = errno;
			myfree(idb.hdrp);
			myfree(nidb.hdrp);
			return BADWRITE;
		}

		pos += nrcdsize;
		dbhdr.firstfree = pos;       /* First free block is at end   */

		myfree(nidb.hdrp);

		if(cmpval < 0) {
			/*   Calculate size of OLD record, seek to it and re-write it to
			** update the nxtwrd field.  Follow the nxtsame chain and do the
			** same for every record on it. */

			size = sizeof(struct dbrec) + idb.hdrp->wordsz + 1;
			size += (sizeof(idb.bodyp->entry[0]) * (idb.hdrp->entries - NUMDBRENTRIES));
			do {
				/*   Make the idb record's nxtwrd field point to the newly
				** added record. */
				idb.bodyp->nxtwrd = npos;
				if( (nrcdsize = putdbrec(&idb, size)) != size) {
					dbifc->errnum = errno;
					myfree(idb.hdrp);
					return BADWRITE;
				}
				if( (idb.pos = idb.bodyp->nxtsame) == 0)
					break;
				myfree(idb.hdrp);
			} while( (size = getdbrec(&idb)) > 0);
		}
 
		myfree(idb.hdrp);         /* Lose the old record */

		if(entry > dbhdr.maxent)
			dbhdr.maxent = entry;

		/*   Now, go back and re-write the dbhdr record */
		lseek(dbifc->fd, 0L, 0);
		
		/* Exposure!!!  If the system crashes after the previous write
		** but before the next write we will waste space. */
		
		write(dbifc->fd, &dbhdr, sizeof(struct dbhdr));
 
		if(cmpval > 0) {
			/*   Added a new record at the head of the chain so we must
			** re-write the dblethd structure. */

			lseek(dbifc->fd, dbhdr.lethdpos[firstletter], 0);
			write(dbifc->fd, dblethd[firstletter], sizeof(struct dblethd));
		}
	} else {
		/*   A record for this word already exists in the database. If the
		** specified entry is already present then we're done, else see
		** if it has room for another entry.  If so just update the record
		** and write it out.  If not then allocate a new record, initialize
		** it, write it and link it into the nxtsame structure */

		int     rcdsize,
				i,
				haveent = 0,
				nxtsize = size;
		off_t   opos, pos;

		do {
			for(i = 0; i < idb.hdrp->used_entries; i++) {
				if(idb.bodyp->entry[i] == entry) {
					haveent = 1;    /* Entry already present */
					break;
				}
			}

			if(haveent || idb.hdrp->used_entries < idb.hdrp->entries)
				break;
			if(idb.bodyp->nxtsame == 0) {
				nxtsize = 0;
				break;
			}

			idb.pos = idb.bodyp->nxtsame;
			myfree(idb.hdrp);
			nxtsize = getdbrec(&idb);
		} while(1);

		if(haveent) {
			/* Entry already present: simply return the size of the
			** current record (nxtsize) after freeing the record's memory.
			**/
			myfree(idb.hdrp);
			return nxtsize;
		} else if(nxtsize > 0) {
			/* No entry present.  Simply add the new entry to the existing
			** record. */
			idb.bodyp->entry[idb.hdrp->used_entries++] = entry;
			if( (size = putdbrec(&idb, nxtsize)) != nxtsize) {
				dbifc->errnum = errno;
				nxtsize = BADWRITE;
			}
			myfree(idb.hdrp);
			return nxtsize;
		} else {
			/*   No room at the inn.  Guess we'll have to write a new
			** record and link it into the previous */

			struct idbifc nidb;

			nidb.fd = idb.fd;

			opos = pos = dbhdr.firstfree;

			rcdsize = newdbrec(wordlen, &nidb);
			nidb.hdrp->used_entries = 1;
			nidb.bodyp->entry[0] = entry;
			nidb.bodyp->nxtsame = 0;
			nidb.bodyp->nxtwrd = idb.bodyp->nxtwrd;
			nidb.pos = pos;
			strcpy(nidb.hdrp->word, wp);

			if( (size = putdbrec(&nidb, rcdsize)) != rcdsize) {
				dbifc->errnum = errno;
				myfree(nidb.hdrp);
				myfree(idb.hdrp);
				return BADWRITE;
			}

			myfree(nidb.hdrp);

			pos += rcdsize;
			dbhdr.firstfree = pos;
			if(entry > dbhdr.maxent)
				dbhdr.maxent = entry;

			lseek(dbifc->fd, 0L, 0);
			write(dbifc->fd, &dbhdr, sizeof(dbhdr));

            idb.bodyp->nxtsame = opos;
			if( (rcdsize = putdbrec(&idb, size)) != size) {
				myfree(idb.hdrp);
				return BADWRITE;
			}

			myfree(idb.hdrp);
		}
	}

	return size;
}

/*   Find a word.  Dbifc->data is the word that we're looking for.
**/
int
findword(dbifc)
struct dbifc *dbifc;
{
	char    *wp;
	int     size = dbifc->size;
	struct idbifc idb;

	if(size <= 0)
		return NOWORD;

	idb.fd = dbifc->fd;

	wp = mymalloc(size+1);
	memset(wp, 0, size+1);
	strncpy(wp, dbifc->data, size);

	if(makegoodword(wp) < 0)
		return NOWORD;

	wp = theword;
	dbifc->size = size = findwrd(wp, &idb);

	dbifc->pos = idb.pos;

	if(size > 0)
		memcpy(dbifc->data, idb.hdrp, size);

	myfree(wp);
	myfree(idb.hdrp);

	return size;
}

/*   Get the first word in the database. */
int
firstword(dbifc)
struct dbifc *dbifc;
{
	struct idbifc idb;
	int     i,j , size;

	idb.fd = dbifc->fd; idb.pos = 0;
	idb.hdrp = NULL; idb.bodyp = NULL;
	for(i = 0; i < NUMLETS && idb.pos == 0; i++) {
		if(dbhdr.lethdpos[i] == 0)
			continue;
		for(j = WORDSIZE(MINWORDSIZE); j <= WORDSIZE(MAXDIRECTWORDSIZE); j++) {
			if( (idb.pos = dblethd[i]->sizes[j]) != 0)
				break;
		}
	}
	if(idb.pos == 0)
		return NOWORD;

	size = getdbrec(&idb);

	dbifc->pos = idb.pos;
	if( (dbifc->size = size) > 0) {
		memcpy(dbifc->data, idb.hdrp, size);
		myfree(idb.hdrp);
	}

	return(dbifc->retval = size);
}

/*   Get the next word.  Use idbifc.bodyp->nxtwrd to get the position of
** the next one.  If there is no next word then look at the next length
** or letter if necessary.  Only return NOWORD when we've reached the end
** of the whole list.
**/
int
nextword(dbifc)
struct dbifc *dbifc;
{
	struct idbifc idb;
	char    *wp = dbifc->data;
	int     i, j, size;

	idb.fd = dbifc->fd;
	idb.pos = dbifc->pos;
	idb.hdrp = NULL; idb.bodyp = NULL;

	size = getdbrec(&idb);

	if( (idb.pos = idb.bodyp->nxtwrd) != 0) {
		/*   This is an easy one!  We simply read the record pointed to
		** by idb.pos.
		**/
		myfree(idb.hdrp);
		size = getdbrec(&idb);
	} else {
		/*   No next word for this size and/or letter.
		**/
		if(strlen(idb.hdrp->word) >= MAXDIRECTWORDSIZE) {
			/*   We know we've gotta go the the next letter since all
			** words longer than MAXDIRECTWORDSIZE are chained together.
			**/
			for(i = lethdndx(*(idb.hdrp->word)) + 1; i < NUMLETS; i++) {
				if(dbhdr.lethdpos[i] != 0)
					for(j = WORDSIZE(MINWORDSIZE); j <= WORDSIZE(MAXDIRECTWORDSIZE); j++)
						if( (idb.pos = dblethd[i]->sizes[j]) != 0)
							break;
				if(idb.pos != 0)
					break;
			}
		} else {
			/*   We've still got sizes left for this letter.  Continue
			** through the sizes till we either 1) find a word or 2) run
			** out of sizes to check.  If 1 then we're ready to get the
			** record.  If 2 then we need to continue through the letters
			** till we find a letter that has a word.
			**/

			int     starti = lethdndx(*(idb.hdrp->word)),
					startj = strlen(idb.hdrp->word) + 1;

			for(i = starti; i < NUMLETS; i++) {
				if(dbhdr.lethdpos[i] != 0)
					for(j = WORDSIZE(startj); j <= WORDSIZE(MAXDIRECTWORDSIZE); j++)
						if( (idb.pos = dblethd[i]->sizes[j]) != 0)
							break;
				if(idb.pos != 0)
					break;
				startj = MINWORDSIZE;
			}
		}

		/*   Now we've either 1) found a word to read in (idb.pos != 0) or
		** 2) we don't.  In either case, free the old record then, if 1 we
		** set up to return NOWORD.  If 2 we read in the record.
		**/
		myfree(idb.hdrp);
		if(idb.pos == 0)
			size = NOWORD;
		else
			size = getdbrec(&idb);
	}

	dbifc->pos = idb.pos;
	if( (dbifc->size = size) > 0) {
		memcpy(dbifc->data, idb.hdrp, size);
		myfree(idb.hdrp);
	}

	return(dbifc->retval = size);
}

/*   Find the first word that matches the specified regular expression. */
int
findre(dbifc)
struct dbifc *dbifc;
{
	char    *pat,
			*wp = dbifc->data;
	int     wordlen, size;
	struct idbifc idb;

	if( (wordlen = makegoodword(wp)) < 0)
		return wordlen;
	wp = theword;

	firstletter = lethdndx(*wp);
    if(dbhdr.lethdpos[firstletter] == 0)
		return NOLETTER;

	if( (pat = regcmp(dbifc->data, (char *)0)) == NULL)
		return NOWORD;

	idb.fd = dbifc->fd;
	idb.pos = 0;

	size = fndre(&idb, WORDSIZE(MINWORDSIZE), pat);
	free(pat);

	dbifc->pos = idb.pos;
	dbifc->errnum = errno;
	if(size > 0) {
		dbifc->size = size;
		memcpy(dbifc->data, idb.hdrp, size);
		myfree(idb.hdrp);
	} else
		dbifc->size = 0;

	return(dbifc->retval = size);
}

/*   Find the next word that matches the specified regular expression. */
int
nextre(dbifc)
struct dbifc *dbifc;
{
	char    *pat,
			*wp = dbifc->data;
	int     size,
			wordlen;
	struct idbifc idb;

	if( (wordlen = makegoodword(wp)) < 0)
		return wordlen;
	wp = theword;

	firstletter = lethdndx(*wp);
	if(dbhdr.lethdpos[firstletter] == 0)
		return NOLETTER;
	
	idb.fd = dbifc->fd;
	idb.pos = dbifc->pos;
	
	if( (size = getdbrec(&idb)) <= 0)
		return size;

	wordlen = dbifc->entry;
	idb.pos = idb.bodyp->nxtwrd;
	myfree(idb.hdrp);

	if( (idb.pos == 0)
	 && (WORDSIZE(wordlen) == WORDSIZE(wordlen + 1)) )
		return NOWORD;
			
	wordlen++;

	if( (pat = regcmp(dbifc->data, (char *)0)) == NULL)
		return NOWORD;

	size = fndre(&idb, WORDSIZE(wordlen), pat);
	free(pat);

	dbifc->pos = idb.pos;
	dbifc->errnum = errno;
	if(size > 0) {
		dbifc->size = size;
		memcpy(dbifc->data, idb.hdrp, size);
		myfree(idb.hdrp);
	} else
		dbifc->size = 0;

	return(dbifc->retval = size);
}

/*   Find the next record that is linked along the nxtsame chain.  If none
** found don't update idbifc, simply return 0.  If found, update all info
** in the passed idbifc structure */
int
nextsame(dbifc)
struct dbifc *dbifc;
{
	struct idbifc idb;
	int     size;

	if( (idb.pos = dbifc->pos) <= 0)
		return NOWORD;

	idb.fd = dbifc->fd;
	idb.hdrp = (struct dbrechdr *)dbifc->data;
	idb.bodyp = (struct dbrecbody *)((char *)idb.hdrp +
									 strlen(idb.hdrp->word) + 1);

	size = getdbrec(&idb);

	if(size > 0) {
		memcpy(dbifc->data, idb.hdrp, size);
		myfree(idb.hdrp);
	}

	return(dbifc->size = size);
}

/*   Return the highest entry recorded so far                           */
int
maxent(dbifc)
struct dbifc *dbifc;
{
    return(dbifc->retval = dbhdr.maxent);
}

/*                Internal routines                                     */

/*   Make a "good" word from the passed parameter.  Storage for the word
** will be allocated and all "bad" characters stripped from the passed
** parameter as they are copied to the new storage area.
**/
static int makegoodword(unsigned char *wp)
{
	int     wordlen;

	while(*wp && okchars[*wp] == BAD)
		wp++;

	if( (wordlen = strlen(wp)) <= 0)
		return BADWORD;

	theword = wp;

	if(wordlen < MINWORDSIZE)
		return BADWORD;

	return wordlen;
}

/*   Return the index into dblethd[] for the given character */
static int lethdndx(unsigned char c)
{
	return(okchars[c < 0x80 ? c : (c - 0x80)]);
}

/*   Allocate a new dbrec structure, returning its length.  Initialize
** it appropriately.
**
**   Called passing the length of the word, a pointer to the new dbrec
** structure (which we will initialize) and a pointer that has been
** incremented by wordlen (which we will provide).  The first pointer the
** caller can use to refer to everything in the structure before the word
** itself.  Since the word is variable in length we will also provide the
** 2nd pointer which allows the caller to reference items in the structure
** beyond the word and be pointing at the proper memory location.
**/
static int newdbrec(int wordlen, struct idbifc *idbifc)
{
	int size;
	struct idbifc ret;
	struct dbrechdr *hdr;

	size = sizeof(struct dbrec) + wordlen + 1;
	idbifc->hdrp = hdr = (struct dbrechdr *)mymalloc(size);
	memset(hdr, '\0', size);
	
	hdr->wordsz = wordlen;
	hdr->entries = NUMDBRENTRIES;
	hdr->used_entries = 0;
	
	idbifc->bodyp = (struct dbrecbody *)((char *)hdr + sizeof(struct dbrechdr) + wordlen + 1);

	return(idbifc->size = size);
}

/*   Allocate a structure for an existing dbrec record.
**
**   Called passing the length of the word, a pointer to the new dbrec
** structure (which we will initialize) and a pointer that has been
** incremented by wordlen (which we will provide).  The first pointer the
** caller can use to refer to everything in the structure before the word
** itself.  Since the word is variable in length we will also provide the
** 2nd pointer which allows the caller to reference items in the structure
** beyond the word and be pointing at the proper memory location.
**/
static int
olddbrec(wordlen, nentries, idbifc)
int wordlen, nentries;
struct idbifc *idbifc;
{
	int size, sizehdr, sizebody ;
	struct dbrechdr *hdr;
	struct dbrecbody *body;

	sizehdr = ((((sizeof(struct dbrechdr) + wordlen + 1)) + 3) /4) * 4;
	sizebody = (sizeof(struct dbrecbody) + sizeof(body->entry[0]) * (nentries - NUMDBRENTRIES));
	
	size = sizeof(struct dbrec) + wordlen + 1;
	size += (sizeof(body->entry[0]) * (nentries - NUMDBRENTRIES));

	hdr = idbifc->hdrp = (struct dbrechdr *)mymalloc(sizehdr + sizebody);
	memset(hdr, 0, sizehdr+sizebody);
/*	
	idbifc->bodyp = (struct dbrecbody *)((char *)hdr
					+ sizeof(struct dbrechdr) + wordlen + 1);
*/	

	idbifc->bodyp = (struct dbrecbody *)((char *)hdr+sizehdr);       
	

	return(idbifc->size = (sizehdr +sizebody));
}

/*   Read in a dbrec.  Account for the variable-length nature of it */
static int getdbrec(struct idbifc *idbifc)
{
	struct dbrechdr dbrechdr;
	struct idbifc idb;
	int     rcdsize, size, hdrsize;

	idb = *idbifc;

	if(getmembuf(&idb) < 0) {
		lseek(idbifc->fd, idbifc->pos, 0);
	
		size = read(idbifc->fd, &dbrechdr, sizeof(struct dbrechdr));
		
		rcdsize = olddbrec(dbrechdr.wordsz, dbrechdr.entries, &idb);
		hdrsize = dbrechdr.wordsz + 1;
		size += read(idbifc->fd, idb.hdrp->word, hdrsize);
		
		idbifc->hdrp = idb.hdrp; idbifc->bodyp = idb.bodyp;
/*	
		size += read(idbifc->fd, idb.hdrp->word,
					 rcdsize - sizeof(struct dbrechdr));
*/

		size += read(idbifc->fd, idb.bodyp, sizeof(struct dbrecbody));


		*(idbifc->hdrp) = dbrechdr;
		idbifc->size = rcdsize;

		addmembuf(&idb, rcdsize);
	} else
		*idbifc = idb;

	return idbifc->size;
}

static int putdbrec(struct idbifc *idbifc, int size)
{
	lseek(idbifc->fd, idbifc->pos, 0);
	if( (idbifc->size = write(idbifc->fd, idbifc->hdrp, size)) == size)
		addmembuf(idbifc, idbifc->size);

	return size;
}

/*   Find the first record whose word matches the passed parm.  If no
** match return the record previous to the one requested.  If there is a
** match then return the matched record. */
static int findwrd(char *wp, struct idbifc *idbifc)
{
	struct idbifc idb, oidb;
	struct dbrechdr *orech;
	int size, wordlen, cmpval;

	idb.fd = idbifc->fd; idb.pos = 0;
	idbifc->hdrp = NULL; idbifc->bodyp = NULL;

	if( (wordlen = strlen(wp)) <= 0)
		return BADWORD;
	
	firstletter = lethdndx(*wp);        /* Get the index for this word  */

    if(dbhdr.lethdpos[firstletter] == 0)
		return NOLETTER;

	idb.pos = dblethd[firstletter]->sizes[WORDSIZE(wordlen)];
	if(idb.pos == 0)
		return NOSIZE;
	
	size = getdbrec(&idb);
	oidb.hdrp = orech = NULL;
	while( (cmpval = strcmp(idb.hdrp->word, wp)) < 0) {
		if(orech)
			myfree(orech);
		orech = oidb.hdrp;
		oidb = idb;
		if( (idb.pos = idb.bodyp->nxtwrd) == 0)
			break;
		size = getdbrec(&idb);
	}
	if(orech)               /* We *never* need this one */
		myfree(orech);

	if(cmpval == 0) {       /* Got an exact match       */
		if(oidb.hdrp)
			myfree(oidb.hdrp);
		*idbifc = idb;
		return size;
	}

	/*   We've broken out because we've either 1) run out of records or
	** 2) hit a record whose word is greater than wp.  If 1 then we
	** need to return the current idb.  If 2) then we need to return the
	** old idb (if we have one...if we don't then return the current one).
	** Note that for case 1 the parenthetical on case 2 can NOT be true
	** since we must've made one pass through the while loop. */

    if(idb.pos == 0)            /* Case 1 */
		*idbifc = oidb;
	else {                      /* Case 2 */
		if(oidb.hdrp) {
			myfree(idb.hdrp);
			*idbifc = oidb;
		} else
			*idbifc = idb;
	}

	return NOWORD;
}

/*   Find the next word that matches the specified regular expression */
static int fndre(struct idbifc *idbifc, int wordlen, char *pat)
{
	int     i, size;
	register char *cmpval;
	extern char *__loc1;
	struct idbifc idb;

	idb = *idbifc;

	for(i = wordlen; i <= WORDSIZE(MAXDIRECTWORDSIZE); i++) {
		if(idb.pos == 0)
			if( (idb.pos = dblethd[firstletter]->sizes[i]) == 0)
				continue;
	  
		size = getdbrec(&idb);
		/*   Search the database.  Only return a match if we match at the
		** beginning of the word.  */
		while( ((cmpval = regex(pat, idb.hdrp->word)) == NULL)
		  ||   ((__loc1 - idb.hdrp->word) > 0) ) {
			cmpval = NULL;
			if( (idb.pos = idb.bodyp->nxtwrd) == 0)
				break;
			myfree(idb.hdrp);
			size = getdbrec(&idb);
		}
	
		if(cmpval != NULL) {    /* Got a match              */
			*idbifc = idb;
			return size;
		}

		/*  We've broken out because we've run out of records that are
		** the same size as wp.  Iterate till we exhaust all word
		** lengths of words that start with this letter.
		**/
	   myfree(idb.hdrp);
	}
	return NOWORD;
}

/*   Add a new record to the memory buffer. */
static int addmembuf(struct idbifc *idbifc,int  rcdsize)
{
	struct membuf *nbp,         /* @ the 1st empty entry    */
				  *endnbp,		/* @ end of a new entry		*/
				  *cbp = NULL,  /* @ entry whose position == requested or NULL */
				  *pbp;         /* @ entry before cbp       */
	static struct membuf *bp;       /* To remember last pos in membuf   */
	register struct membuf *wbp,    /* Work buf pointer         */
						   *obp;    /* @ entry before wbp       */
	int matched = 0;

#ifdef NO_BUFFER
	return;
#endif

	if(bufhead == NULL) {
		makemembuf();
		bp = bufhead;
	}
	/*  bp only set if this routine gets the bufhead structure by calling
	**  makemembuf, but getdbrec calls getmembuf first, it therefore 
	**  seems that bp will always been NULL
	*/

	if (bp == NULL)
	    bp = bufhead;

	/*  If current position in membuf, "bp", is above where the requested
	** record could be then set bp to the beginning of the membuf pool.
	**/

	if(bp > lastbp || bp->pos > idbifc->pos)
		bp = bufhead;

	/*   Search the membuf entries until we either A) find one whose
	** position == the requested one or B) we have looked at all of our
	** entries.                                                         */
	for(obp = wbp = bp; wbp && wbp <= lastbp; wbp = wbp->next) {
		if(wbp->pos >= idbifc->pos) {
			/*   Found a match! */
			pbp = obp;      /* Location of block previous to this one */
			cbp = wbp;      /* Location of current block */
			if(wbp->pos == idbifc->pos)
				matched = 1;    /* Got a match| */
			break;
		}
		obp = wbp;
	}

	bp = wbp;

	/*   Compute address of a new entry in case we need it              */
	nbp = (struct membuf *)((char *)lastbp + sizeof(struct membuf)
		  + lastbp->length);
	endnbp = (struct membuf *)((char *)nbp + sizeof(struct membuf)
			 + rcdsize);

	if(matched) {   /*  We found a match on position in our for loop */
		if(cbp->length == rcdsize) {
			memcpy(cbp->data, idbifc->hdrp, rcdsize);
			return;
		} else if(endnbp < endbp) {    /* there's room in the membuf pool */

			/*   This is a strange consequence which is only included
			** because I'm a coward.  We have an entry whose position ==
			** the requested position but whose size doesn't == the passed
			** record size.  We'll discard the "old" version (the one
			** pointed to by cbp) and replace it with the new entry.   Is
			** this the right thing to do?  I dunno but it seems like I
			** gotta do *something*!
			**/

			nbp->next = cbp->next;
			nbp->pos = idbifc->pos;
			nbp->length = rcdsize;
			memcpy(nbp->data, idbifc->hdrp, rcdsize);
			
			pbp->next = nbp;
			/* pbp->pos = 0; */
		} else          /* no room at the inn, do nothing */
			return;
	} else if(endnbp < endbp) {    /* there's room in the membuf pool */
		nbp->next = obp->next;
		obp->next = nbp;
		nbp->length = rcdsize;
		nbp->pos = idbifc->pos;
		memcpy(nbp->data, idbifc->hdrp, rcdsize);
	} else						/* No room in the membuf pool		*/
		return;

	/*   Since we only fall through to here if we've used nbp we can
	** safely update lastbp.
	**/

	lastbp = nbp;
}

/*   Get a record whose contents is in the memory buffer.  Determine if
** the record is in membuf by keying on idbifc->pos.  If it's present then
** allocate storage for the record, copy the memory copy to the storage
** and return the length of it.  If it's not present then return -1. */
static int getmembuf(struct idbifc *idbifc)
{
	int     rcdsize;
	struct dbrechdr *hdr;
	static struct membuf *lbp;      /* remember last pos in membuf */
	register struct membuf *bp;     /* Work reg                    */

#ifdef NO_BUFFER
	return -1;
#endif
	
	if(bufhead == NULL) {
		makemembuf();
		lbp = bufhead;
		return -1;
	}

	/*  If current position in membuf, "lbp", is above where the requested
	** record could be then set lbp to the beginning of the membuf pool.
	**/

	if(lbp > lastbp || lbp->pos > idbifc->pos)
		lbp = bufhead;

	/*   Search membuf pool for an entry that matches the requested
	** position.
	**/
	for(bp = lbp; bp && bp <= lastbp && bp->pos < idbifc->pos; bp = bp->next)
		;

	if(bp->pos != idbifc->pos)  /* No entry found! */
		return -1;

	lbp = bp;

	/*   bp points at the entry in the membuf pool whose position matches
	** the requested position!!  Copy the data into the area pointed to by
	** the passed idbifc pointer.
    **/

	rcdsize = bp->length;
	hdr = (struct dbrechdr *)mymalloc(rcdsize);
	memcpy(hdr, bp->data, rcdsize);
	idbifc->bodyp = (struct dbrecbody *)((char *)hdr
					+ sizeof(struct dbrechdr) + hdr->wordsz + 1);
	idbifc->hdrp = hdr;

    return(idbifc->size = rcdsize);
}

/*   Create the memory buffer if it isn't already present. */
static int makemembuf()
{

#ifdef NO_BUFFER
	return;
#endif

	if(bufhead == NULL) {
		lastbp = bufhead = (struct membuf *)mymalloc(maxbuf);
		memset(bufhead, '\0', maxbuf);
		bufhead->pos = 0;
		bufhead->length = 0;
		bufhead->next = NULL;
		endbp = (struct membuf *)((char *)bufhead + (maxbuf-1));
	}
}

/*VARARGS*/
static void
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
	fflush(stderr);
	if(exit_code == 0)
		return;

	exit(exit_code);
}
