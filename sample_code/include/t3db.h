#ifndef DB

#ident "@(#)t3db.h	1.5 9/20/88 09:58:15"

/* @(#)t3db.h	1.5 - last delta: 9/20/88 09:58:15 gotten 9/20/88 09:58:16 from /usr/local/src/db/include/.SCCS/s.t3db.h. */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#define KEYLEN          6
#define MAXMSGLEN       16384
#define NUMDBRENTRIES   50

#define DEFDBDIR        "/database"
#define DEFDBNAME       "t3db"
#define DEFTEXTDIR      "text"
#define DEFMSGQ         "MSGQ"
#define INCIDENTS       "INCIDENTS"
#define LOGINFO         "LOGINFO"

key_t   ftok(),
		msgkey;             /* Key to be used for IPC comm          */

int     msgqid;             /* Msg q id for our comm port           */

struct wordentry {
	char    *word;          /* Pointer to the null-terminated word  */
	int     entry;          /* Value to associate with it           */
	int     retval;         /* Return value from add of this pair   */
	int     errnum;          /* Errno from add of this pair          */
};

struct dbrechdr {
	short       entries;        /* Number of elems in entry[]   */
	short       wordsz;         /* Size of word if > 15 chars else ?    */
	int         used_entries;   /* Number of entries present    */
    char        word[0];         /* The word                     */
};

struct dbrecbody {
    off_t       nxtwrd;         /* Pointer to next word         */
    off_t       nxtsame;        /* Pointer to more entries of same word */
    int         entry[NUMDBRENTRIES];   /* The array of entries         */
};

struct dbrec {
	struct dbrechdr dbrech;
    struct dbrecbody dbrecb;
};

struct t3dbifc {
    int     retval;                     /* Return value from function   */
	int     errnum;                      /* Errno if applicable          */
	off_t   pos;                        /* Internal use                 */
    int     size;                       /* Size of data[]               */
	union {
		char data[0];                    /* The returned data or...      */
        struct dbrec;                   /* ...a db record.              */
	} t3dbdata;
};

#define DB
#endif
