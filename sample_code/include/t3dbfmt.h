#ifndef DBFMT

#ident "@(#)t3dbfmt.h	1.5 8/5/88 11:38:34"

/* @(#)t3dbfmt.h	1.5 - last delta: 8/5/88 11:38:34 gotten 8/5/88 14:12:46 from /usr/local/src/db/include/.SCCS/s.t3dbfmt.h. */


/* Structure defines for the format of our database */

#define MAXDIRECTWORDSIZE   15
#define NUMLETS         36

struct dbifc {
    int     fd;                         /* File desc for the db         */
	char    *fn;                        /* Pointer to db file name      */
    int     retval;                     /* Return value from function   */
	int     errnum;                      /* Errno if applicable          */
	off_t   pos;                        /* Internal use                 */
	int     entry;                      /* On ADDWORD - value to add    */
										/* on NEXTRE  - length of word  */
	int     size;                       /* Size of following data       */
	char    data[0];                     /* Data, if applicable.  Note that
										** this may be returned as well */
};

/*   The dbhdr record occurs once in a database file...at the beginning */
struct dbhdr {
    off_t       used;           /* Amount of space in use               */
    int         maxent;         /* Maximum entry recorded so far        */
    off_t       firstfree;      /* Byte position of first free spot     */
    off_t       lethdpos[NUMLETS];  /* Position of each letter in file  */
};
 
/*   The dblethd record occurs once for each possible starting letter (in
** this context, a letter is a legitimate starting character).  It is
** pointed to by dbhdr.lethdpos[i].  Words of size 2-MAXDIRECTWORDSIZE are
** pointed to directly while words of larger than that are all grouped
** together.  Note that the phrase "pointed to" actually means that the
** pointer is to the word's dbrec structure.
**/
struct dblethd {
    off_t       sizes[MAXDIRECTWORDSIZE-1];     /* Position of words of size 2-MAXDIRECTWORDSIZE    */
												/* and 1 for words > MAXDIRECTWORDSIZE              */
};

#define DBFMT
#endif





