#ifndef DBCOMM

#ident "@(#)t3comm.h	1.8 10/20/88 22:47:30"

/* @(#)t3comm.h	1.8 - last delta: 10/20/88 22:47:30 gotten 10/20/88 22:47:30 from /usr/local/src/db/include/.SCCS/s.t3comm.h. */

/*   Supported commands.  These belong in msgbuf.mtype.             */
#define OPENDB      1
#define FIRSTWORD   2
#define NEXTWORD    3
#define ADDWORD     4
#define NEXTSAME    5
#define CLOSEDB     6
#define NEWLOG      7
#define NEWINC      8
#define FINDRE      9
#define NEXTRE     10
#define ADDFILE    11
#define MAXENT     12
#define QUIESCE    13
#define RESUME     14
#define MAXCMD     14

/*   Structure definition for communication with the t3db daemon    */
struct t3comm {
	int     msgqid;         /* Msg q to respond to                  */
    int     retval;         /* Return value from function           */
	int     errnum;          /* Errno if applicable                  */
    off_t   pos;            /* Used internally - don't touch!       */
	int     entry;          /*   Value           function           */
							/* mode              OPENDB             */
							/* entry             ADDWORD            */
							/* entry             ADDFILE            */
							/* lineno on bad return from ADDFILE    */
							/* otherwise, this field is unused      */
    int     size;           /* Size of following data               */
    char    data[0];         /* Whatever is to be communicated       */
};

#define DBCOMM

#endif
