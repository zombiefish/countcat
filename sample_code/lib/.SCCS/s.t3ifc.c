h11059
s 00001/00001/00594
d D 1.19 90/02/14 18:18:59 acs 20 19
c Return new CANTOPENMSGQ when we can't open the MSGQ file.
e
s 00006/00001/00589
d D 1.18 88/10/21 00:04:18 root 19 18
c Make newlog() return more gracefully when it receives a negative retval from the daemon.
e
s 00004/00000/00586
d D 1.17 88/10/21 00:00:34 acs 18 16
c Add t3comm->size = 0 to newlog() and newinc().
e
s 00004/00002/00584
d R 1.17 88/10/20 23:32:21 acs 17 16
c Set errno to val if old errno was 0 in newlog() and newinc().
e
s 00046/00000/00540
d D 1.16 88/10/20 22:48:13 acs 16 15
c Add opendb() and resumedb() calls for LOGBOOK inc 3: Need quiesce and resume calls.
e
s 00006/00001/00534
d D 1.15 88/09/01 19:40:10 acs 15 14
c More of the same from the previous delta.
e
s 00008/00001/00527
d D 1.14 88/09/01 19:33:31 acs 14 13
c Opendb() is now more forgiving if you open several times and/or try to open when the daemon's not up.
e
s 00000/00001/00528
d D 1.13 88/09/01 11:21:17 acs 13 12
c Remove extra free(fn) in startup().
e
s 00086/00028/00443
d D 1.12 88/08/18 18:36:32 acs 12 11
c Ensure that we shutdown() when we get an error on domsgsnd(); fix nextsame() so it won't startup before it should.
e
s 00004/00005/00467
d D 1.11 88/08/18 14:15:16 acs 11 10
c Wasn't accounting for \0 in t3comm->size in some cases.
e
s 00007/00001/00465
d D 1.10 88/08/18 12:19:54 acs 10 9
c In newlog(), always set *logno + set errno (and return NULL) if t3comm->retval < 0.
e
s 00001/00000/00465
d D 1.9 88/08/17 15:27:26 acs 9 8
c Fix syntax error introduced by last update.
e
s 00003/00001/00462
d D 1.8 88/08/17 15:26:19 acs 8 7
c Newlog() now takes 1 parm: int *logno into which it will store the log entry number.
e
s 00082/00064/00381
d D 1.7 88/08/17 13:59:24 acs 7 6
c startup() and domsgsnd() now return values instead of issuing errmsg()s.  Removed errmsg().
e
s 00065/00009/00380
d D 1.6 88/08/11 18:15:24 acs 6 5
c Include dir now speced via -I, add firstword() and nextword() routines, remember last pos on every read and gen'l cleanup.
e
s 00014/00000/00375
d D 1.5 88/08/05 11:36:45 acs 5 4
c Support for new maxent() command.
e
s 00004/00005/00371
d D 1.4 88/08/04 14:16:08 acs 4 3
c Add lineno parm to addfile() routine.
e
s 00022/00000/00354
d D 1.3 88/08/03 14:44:27 acs 3 2
c Support for the ADDFILE command.
e
s 00108/00003/00246
d D 1.2 88/08/02 11:54:13 acs 2 1
c Support for NEXTSAME, FINDRE and NEXTRE.
e
s 00249/00000/00000
d D 1.1 88/07/26 16:59:10 acs 1 0
c date and time created 88/07/26 16:59:10 by acs
e
u
U
t
T
I 1
#ident "%W% %G% %U%"

/* %W% - last delta: %G% %U% gotten %H% %T% from %P%. */

/*   This routine provides the interface to the t3 daemon.  It contains
** code to support the following calls:
**
**  opendb()    addaword()  closedb()   newlog()    newinc()
**
**   See the t3db(3) man page for details of the calls.
**/

D 6
#include "../include/t3db.h"
#include "../include/t3comm.h"
I 2
#include "../include/t3dbfmt.h"
E 2
#include "../include/t3dberr.h"
E 6
I 6
#include "t3db.h"
#include "t3comm.h"
#include "t3dbfmt.h"
#include "t3dberr.h"
E 6
#include <varargs.h>

extern int errno, sys_nerr;

extern char *getenv(), *malloc(), *sys_errlist[], *cmdname;

static int retmsgqid = 0,           /* Msgqid for returned msgs         */
D 2
           dbisopen  = 0;           /* Indicator that the db is open    */
E 2
I 2
           dbisopen  = 0,           /* Indicator that the db is open    */
		   lastrelen = 0;           /* Wordlen of last re match         */
E 2

static struct t3comm *t3comm = NULL;

static char *dbname = NULL,
D 2
            *dbdir  = NULL;
E 2
I 2
            *dbdir  = NULL,
			*pattern = NULL;
E 2

static struct msgbuf *msgbuf;

I 2
D 6
static off_t lastrepos;             /* Position of last re match        */
E 6
I 6
static off_t lastrepos,             /* Position of last re match        */
             lastpos;               /* Position of last record read     */
E 6

E 2
D 7
static void errmsg();

E 7
opendb(fn, mode)
char    *fn;
int     mode;
{
D 7
	int     len;
E 7
I 7
	int     len, val;
E 7
	char    *cp;

	if(dbname != NULL)
D 14
		return CANTOPEN;
E 14
I 14
		if(strcmp(dbname, fn) != 0)
			return CANTOPEN;
		else
			return 0;
E 14

	len = strlen(fn);
	dbname = malloc(len + 1);
	strcpy(dbname, fn);
	
	for(cp = dbname + len; cp >= dbname && *cp != '/'; cp--)
		;
    if(cp < dbname)
		len = 0;
	else
		len = cp - dbname;
	dbdir = malloc(len + 1);
	strncpy(dbdir, dbname, len);
	*(dbdir + len) = '\0';
		
D 7
	startup();
E 7
I 7
D 15
	if( (val = startup()) != 0)
E 15
I 15
	if( (val = startup()) != 0) {
		free(dbname);
		dbname = NULL;
		free(dbdir);
		dbdir = NULL;
E 15
		return val;
I 15
	}
E 15
E 7

	t3comm->entry = mode;
D 11
	t3comm->size = strlen(fn);
E 11
I 11
	t3comm->size = strlen(fn) + 1;
E 11
	strcpy(t3comm->data, fn);

D 7
	domsgsnd(OPENDB, t3comm);
E 7
I 7
D 12
	if( (val = domsgsnd(OPENDB, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(OPENDB, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
I 14
		free(dbname);
		dbname = NULL;
		free(dbdir);
		dbdir = NULL;
E 14
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12

E 7
	if(t3comm->retval == 0)
		dbisopen = 1;
	errno = t3comm->errno;

	shutdown();

	return t3comm->retval;
}

I 16
quiescedb()
{
	int		val;

	if( (val = startup()) != 0)
		return val;

	t3comm->size = 0;

	if( (val = domsgsnd(QUIESCE, t3comm)) != 0) {
		int		sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	shutdown();
	errno = t3comm->errno;

	return t3comm->retval;
}

resumedb()
{
	int		val;

	if( (val = startup()) != 0)
		return val;

	t3comm->size = 0;

	if( (val = domsgsnd(RESUME, t3comm)) != 0) {
		int		sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	shutdown();
	errno = t3comm->errno;

	return t3comm->retval;
}

E 16
addword(wordentry, cnt)
register struct wordentry wordentry[];
int     cnt;
{
D 7
	int     i;
E 7
I 7
	int     i, val;
E 7

D 7
	startup();
E 7
I 7
	if( (val = startup()) != 0)
		return val;
E 7

	for(i = 0; i < cnt; i++) {
		t3comm->entry = wordentry[i].entry;
		strcpy(t3comm->data, wordentry[i].word);
D 11
		t3comm->size = strlen(t3comm->data);
E 11
I 11
		t3comm->size = strlen(t3comm->data) + 1;
E 11
	
D 7
		domsgsnd(ADDWORD, t3comm);
E 7
I 7
D 12
		if( (wordentry[i].retval = domsgsnd(ADDWORD, t3comm)) != 0)
E 12
I 12
		if( (wordentry[i].retval = domsgsnd(ADDWORD, t3comm)) != 0) {
			wordentry[i].errno = errno;
			shutdown();
E 12
			return i;
I 12
		}
E 12

E 7
		wordentry[i].errno = errno = t3comm->errno;
		wordentry[i].retval = t3comm->retval;
	}
	
	shutdown();

	return i;
}

closedb(fn)
char    *fn;
{
D 7
	startup();
E 7
I 7
	int     val;
E 7

I 7
	if( (val = startup()) != 0)
		return val;

E 7
	t3comm->size = 0;
	strcpy(t3comm->data, fn);

D 7
	domsgsnd(CLOSEDB, t3comm);
E 7
I 7
D 12
	if( (val = domsgsnd(CLOSEDB, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(CLOSEDB, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12

E 7
	if(t3comm->retval == 0) {
		free(dbname);
		dbname = NULL;
		free(dbdir);
		dbdir = NULL;
		dbisopen = 0;
	}
	errno = t3comm->errno;

	shutdown();

	return t3comm->retval;
}

char *
newinc() {
D 7
    startup();
E 7
I 7
	int     val;
E 7

D 7
	domsgsnd(NEWINC, t3comm);
E 7
I 7
	if( (val = startup()) != 0)
        return NULL;
E 7

I 18
	t3comm->size = 0;

E 18
I 7
D 12
	if( (val = domsgsnd(NEWINC, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(NEWINC, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return NULL;
I 12
	}
E 12

E 7
	shutdown();

D 11
	*(t3comm->data + t3comm->size) = '\0';
E 11
D 19
	return t3comm->data;
E 19
I 19
	if(t3comm->retval >= 0)
		return t3comm->data;
	else {
		errno = t3comm->errno;
		return NULL;
	}
E 19
}

char *
D 8
newlog() {
E 8
I 8
newlog(logno)
int     *logno;
I 9
{
E 9
E 8
D 7
    startup();
E 7
I 7
	int     val;
E 7

D 7
	domsgsnd(NEWLOG, t3comm);
E 7
I 7
    if( (val = startup()) != 0)
		return NULL;
I 18

	t3comm->size = 0;
E 18
E 7

I 7
D 12
	if( (val = domsgsnd(NEWLOG, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(NEWLOG, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return NULL;
I 12
	}
E 12

E 7
	shutdown();

I 8
	*logno = t3comm->retval;
E 8
D 10
	return t3comm->data;
E 10
I 10
	if(t3comm->retval >= 0)
		return t3comm->data;
	else {
		errno = t3comm->errno;
		return NULL;
	}
		
E 10
}

I 2
int
findre(pat, p)
char    *pat, **p;
{
	struct dbrechdr *dbrech;
I 7
	int     val;
E 7

I 7
	if( (val = startup()) != 0)
		return val;

E 7
	if(pattern != NULL)
		free(pattern);

	pattern = malloc(strlen(pat) + 1);
	strcpy(pattern, pat);

D 7
	startup();
E 7

	strcpy(t3comm->data, pattern);
D 11
	t3comm->size = strlen(pattern);
E 11
I 11
	t3comm->size = strlen(pattern) + 1;
E 11

D 7
	domsgsnd(FINDRE, t3comm);
E 7
I 7
D 12
	if( (val = domsgsnd(FINDRE, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(FINDRE, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12
E 7

	errno = t3comm->errno;
	if(t3comm->retval > 0) {
		*p = t3comm->data;
		dbrech = (struct dbrechdr *)t3comm->data;
		lastrelen = strlen(dbrech->word);
D 6
		lastrepos = t3comm->pos;
E 6
I 6
		lastrepos = lastpos = t3comm->pos;
E 6
	} else {
		lastrelen = -1;
D 6
		lastrepos = -1;
E 6
I 6
		lastrepos = lastpos = -1;
E 6
	}

	shutdown();

	return t3comm->retval;
}

int
nextre(p)
char    **p;
{
	struct dbrechdr *dbrech;
I 7
	int     val;
E 7

	if(pattern == NULL || lastrelen == -1)
		return NOWORD;

D 7
	startup();
E 7
I 7
	if( (val = startup()) != 0)
		return val;
E 7

	strcpy(t3comm->data, pattern);
D 11
	t3comm->size = strlen(pattern);
E 11
I 11
	t3comm->size = strlen(pattern) + 1;
E 11
	t3comm->entry = lastrelen;
	t3comm->pos = lastrepos;

D 7
	domsgsnd(NEXTRE, t3comm);
E 7
I 7
D 12
	if( (val = domsgsnd(NEXTRE, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(NEXTRE, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12
E 7

	errno = t3comm->errno;

	if(t3comm->retval > 0) {
		*p = t3comm->data;
		dbrech = (struct dbrechdr *)t3comm->data;
		lastrelen = strlen(dbrech->word);
D 6
		lastrepos = t3comm->pos;
E 6
I 6
		lastrepos = lastpos = t3comm->pos;
E 6
	}

	shutdown();
	
	return t3comm->retval;
}

int
I 6
firstword(dbrec)
register struct dbrec **dbrec;
{
D 7
	startup();
E 7
I 7
	int     val;
E 7

I 7
	if( (val = startup()) != 0)
		return val;

E 7
	t3comm->size = 0;

D 7
	domsgsnd(FIRSTWORD, t3comm);
E 7
I 7
D 12
	if( (val = domsgsnd(FIRSTWORD, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(FIRSTWORD, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12
E 7

	errno = t3comm->errno;

	if(t3comm->retval > 0) {
		*dbrec = (struct dbrec *)t3comm->data;
		lastpos = t3comm->pos;
	}

	shutdown();

	return t3comm->retval;
}

int
nextword(dbrec)
register struct dbrec **dbrec;
{
	struct dbrechdr *dbrh;
	struct dbrecbody *dbrb;
I 7
	int     val;
E 7

I 7
	if( (val = startup()) != 0)
		return val;

E 7
	dbrh = &((*dbrec)->dbrech);
	dbrb = (struct dbrecbody *)((dbrh->word)
							   + strlen(dbrh->word) + 1);

	t3comm->pos = lastpos;

D 7
	startup();

E 7
	t3comm->size = 0;

D 7
	domsgsnd(NEXTWORD, t3comm);
E 7
I 7
D 12
	if( (val = domsgsnd(NEXTWORD, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(NEXTWORD, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12
E 7

	errno = t3comm->errno;

	if(t3comm->retval > 0) {
		*dbrec = (struct dbrec *)t3comm->data;
		lastpos = t3comm->pos;
	}

	shutdown();

	return t3comm->retval;
}

int
E 6
nextsame(dbrec)
register struct dbrec **dbrec;
{
	struct dbrechdr *dbrh;
	struct dbrecbody *dbrb;
I 7
	int     val;
E 7

I 7
D 12
	if( (val = startup()) != 0)
		return val;

E 12
E 7
	dbrh = &((*dbrec)->dbrech);
	dbrb = (struct dbrecbody *)((dbrh->word)
							   + strlen(dbrh->word) + 1);

	if( (t3comm->pos = dbrb->nxtsame) == 0)
		return NOWORD;

D 7
	startup();

E 7
	t3comm->size = 0;

D 7
	domsgsnd(NEXTSAME, t3comm);
E 7
I 7
D 12
	if( (val = domsgsnd(NEXTSAME, t3comm)) != 0)
E 12
I 12
	if( (val = startup()) != 0)
E 12
		return val;
E 7

I 12
	if( (val = domsgsnd(NEXTSAME, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

E 12
	errno = t3comm->errno;

D 6
	if(t3comm->retval > 0)
E 6
I 6
	if(t3comm->retval > 0) {
E 6
		*dbrec = (struct dbrec *)t3comm->data;
I 6
		lastpos = t3comm->pos;
	}
E 6

	shutdown();

	return t3comm->retval;
}

I 3
int
D 4
addfile(fn, entry)
E 4
I 4
addfile(fn, entry, lineno)
E 4
char    *fn;
D 4
int     entry;
E 4
I 4
int     entry, *lineno;
E 4
{
I 7
	int     val;

	if( (val = startup()) != 0)
		return val;

E 7
	t3comm->size = strlen(fn) + 1;
	strcpy(t3comm->data, fn);
	t3comm->entry = entry;

D 7
	startup();
E 7
I 7
D 12
	if( (val = domsgsnd(ADDFILE, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(ADDFILE, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12
E 7

D 7
	domsgsnd(ADDFILE, t3comm);

E 7
	errno = t3comm->errno;
I 4
	*lineno = t3comm->entry;
E 4

D 4
	errno = t3comm->errno == 0;

E 4
	shutdown();

D 4
	return t3comm->retval == 0 ? t3comm->entry : t3comm->retval;
E 4
I 4
	return t3comm->retval;
E 4
}

I 5
int
maxent()
{
D 7
	t3comm->size = 0;
E 7
I 7
	int     val;
E 7

D 7
	startup();
E 7
I 7
	if( (val = startup()) != 0)
		return val;
E 7

D 7
	domsgsnd(MAXENT, t3comm);
E 7
I 7
	t3comm->size = 0;
E 7

I 7
D 12
	if( (val = domsgsnd(MAXENT, t3comm)) != 0)
E 12
I 12
	if( (val = domsgsnd(MAXENT, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
E 12
		return val;
I 12
	}
E 12

E 7
	shutdown();

	return t3comm->retval;
}

E 5
E 3
E 2
static int
startup() {
	char    *fn, seq = '\0';
	int     len, iterations = 0;
	key_t   msgkey;
	FILE    *fp;
	struct msgbuf t;

I 2
	if(dbname == NULL)
D 7
		errmsg(1, "No dbname - can't process request\n");
E 7
I 7
		return CANTOPEN;
E 7

E 2
D 12
	do {
		if(seq++ == 255) {
			if(iterations++ > 0)
				sleep(3);
				
			if(iterations > 5)
D 7
				errmsg(1, "Can't create a msg key after 5 tries\n");
E 7
I 7
				return -1;
E 7
		}

		if( (msgkey = ftok(dbname, seq++)) == (key_t)-1)
D 7
			errmsg(1, "Can't create a message key based on %s\n", dbname);
E 7
I 7
			return -1;
E 7
	} while( (retmsgqid = msgget(msgkey, 0666 | IPC_CREAT | IPC_EXCL)) < 0);

E 12
D 2
	fn = malloc(strlen(dbdir) + 1);
E 2
I 2
	fn = malloc(strlen(dbdir) + strlen(DEFMSGQ) + 2);
E 2
	strcpy(fn, dbdir);
	if(strlen(dbdir) > 0)
		strcat(fn, "/");
	strcat(fn, DEFMSGQ);

	if( (fp = fopen(fn, "r")) == NULL)
D 7
		errmsg(1, "Can't open msgqname %s: %s\n", fn, sys_errlist[errno]);
E 7
I 7
D 20
		return -1;
E 20
I 20
		return CANTOPENMSGQ;
E 20
E 7

    free(fn);

	if( (len = fread(&msgqid, 1, sizeof(msgqid), fp)) != sizeof(msgqid))
D 7
		errmsg(1, "Can't read %d bytes from %s, only got %d bytes\n",
			   sizeof(msgqid), fn, len);
E 7
I 7
		return -1;
E 7

	fclose(fp);
D 13
	free(fn);
E 13
I 12

	do {
		if(seq++ == 255) {
			if(iterations++ > 0)
				sleep(3);
				
			if(iterations > 5)
				return -1;
		}

		if( (msgkey = ftok(dbname, seq++)) == (key_t)-1)
			return -1;
	} while( (retmsgqid = msgget(msgkey, 0666 | IPC_CREAT | IPC_EXCL)) < 0);
E 12

	if(t3comm == NULL) {
		msgbuf = (struct msgbuf *)malloc(sizeof(struct t3comm)
										 + MAXMSGLEN
										 + sizeof(t.mtype));
		t3comm = (struct t3comm *)msgbuf->mtext;
		t3comm->msgqid = retmsgqid;
	}

}

static int
shutdown() {
	if(retmsgqid >= 0)
		msgctl(retmsgqid, IPC_RMID);

	retmsgqid = 0;
}

static int
domsgsnd(cmd, t3comm)
int     cmd;
struct t3comm *t3comm;
{
	int     val, iterations = 0;

	t3comm->msgqid = retmsgqid;
	msgbuf->mtype = cmd;

	while( (val = msgsnd(msgqid, msgbuf,
						 sizeof(struct t3comm) + t3comm->size, 0)) != 0) {
		if(iterations >= 5)
			break;
		if(iterations++ > 0)
			sleep(1);
	}

	if(val < 0)
D 7
		errmsg(1, "Can't msgsnd after %d tries: %s\n",
			   iterations, sys_errlist[errno]);
E 7
I 7
		return -1;
E 7

	msgbuf->mtype = 0;
	if( (val = msgrcv(retmsgqid, msgbuf, MAXMSGLEN, 0, 0)) < 0)
D 7
		errmsg(1, "Can't msgrcv: %s\n", sys_errlist[errno]);
}
E 7
I 7
		return -1;
E 7

D 7
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

	if(dbisopen)
		closedb(dbname);

	shutdown();

	exit(exit_code);
E 7
I 7
	return 0;
E 7
}
E 1
