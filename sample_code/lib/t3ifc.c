#ident "@(#)t3ifc.c	1.19 2/14/90 18:18:59"
#define	__USE_GNU	1
/* @(#)t3ifc.c	1.19 - last delta: 2/14/90 18:18:59 gotten 2/14/90 18:18:59 from /usr/local/src/db/lib/.SCCS/s.t3ifc.c. */

/*   This routine provides the interface to the t3 daemon.  It contains
** code to support the following calls:
**
**  opendb()    addaword()  closedb()   newlog()    newinc()
**
**   See the t3db(3) man page for details of the calls.
**/
#include <sys/msg.h>
#include "t3db.h"
#include "t3comm.h"
#include "t3dbfmt.h"
#include "t3dberr.h"
#include <varargs.h>

int  mpm_errno;
int  mpm_msgrcv;      
int  mpm_msgsnd;
int  mpm_val;
int  mpm_findre;
char  mpm_dbname[256];
int  mpm_opendb_1;
int  mpm_opendb_2;
int  mpm_opendb_3;
int  mpm_opendb_4;
int  mpm_opendb_5;
int  mpm_opendb_6;
int  mpm_opendb_7;
int  mpm_opendb_8;
int  mpm_opendb_9;

extern char *sys_errlist[];
/* extern char *dbname; */       


static int domsgsnd();
static int shutdown();
static int startup();
extern int errno, sys_nerr;

extern char *getenv(), *malloc(),  *cmdname;

static int retmsgqid = 0,           /* Msgqid for returned msgs         */
           dbisopen  = 0,           /* Indicator that the db is open    */
		   lastrelen = 0;           /* Wordlen of last re match         */

static struct t3comm *t3comm = NULL;

static char *dbname = NULL;
static  char  *dbdir  = NULL,
		*pattern = NULL;

static struct msgbuf *imsgbuf;

static off_t lastrepos,             /* Position of last re match        */
             lastpos;               /* Position of last record read     */

opendb(fn, mode)
char    *fn;
int     mode;
{
	int     len, val;
	char    *cp;


	if(dbname != NULL)
		if(strcmp(dbname, fn) != 0)
		{
		    free(dbname);
		   /* 
		   int dblen = strlen(dbname);
		   int fnlen = strlen(fn);
		   debugmsg("dbname: %s	    fn: %s\n",dbname, fn);  
		   strncpy(mpm_dbname, dbname, dblen);         
		   strncpy(mpm_dbname+dblen+1, fn, fnlen);
		    return CANTOPEN1;
		   */ 
		}	
		else
			return 0;

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
		
	if( (val = startup()) != 0) {
		free(dbname);
		dbname = NULL;
		free(dbdir);
		dbdir = NULL;
		debugmsg("startup() returned %d\n", val);     
		return val;
	}

	t3comm->entry = mode;
	t3comm->size = strlen(fn) + 1;
	strcpy(t3comm->data, fn);

	if( (val = domsgsnd(OPENDB, t3comm)) != 0) {
		int sverrno = errno;
		 mpm_opendb_4 = val;
		 mpm_opendb_5 = errno;

		shutdown();
		free(dbname);
		dbname = NULL;
		free(dbdir);
		dbdir = NULL;
		errno = sverrno;
		return val;
	}

	if(t3comm->retval == 0)
		dbisopen = 1;
	errno = t3comm->errnum;
		 mpm_opendb_6 = errno;

	shutdown();

	return t3comm->retval;
}

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
	errno = t3comm->errnum;

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
	errno = t3comm->errnum;

	return t3comm->retval;
}

addword(wordentry, cnt)
register struct wordentry wordentry[];
int     cnt;
{
	int     i, val;

	if( (val = startup()) != 0)
		return val;

	for(i = 0; i < cnt; i++) {
		t3comm->entry = wordentry[i].entry;
		strcpy(t3comm->data, wordentry[i].word);
		t3comm->size = strlen(t3comm->data) + 1;
	
		if( (wordentry[i].retval = domsgsnd(ADDWORD, t3comm)) != 0) {
			wordentry[i].errnum = errno;
			shutdown();
			return i;
		}

		wordentry[i].errnum = errno = t3comm->errnum;
		wordentry[i].retval = t3comm->retval;
	}
	
	shutdown();

	return i;
}

closedb(fn)
char    *fn;
{
	int     val;

	if( (val = startup()) != 0)
		return val;

	t3comm->size = 0;
	strcpy(t3comm->data, fn);

	if( (val = domsgsnd(CLOSEDB, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	if(t3comm->retval == 0) {
		free(dbname);
		dbname = NULL;
		free(dbdir);
		dbdir = NULL;
		dbisopen = 0;
	}
	errno = t3comm->errnum;

	shutdown();

	return t3comm->retval;
}

char *
newinc() {
	int     val;

	if( (val = startup()) != 0)
        return NULL;

	t3comm->size = 0;

	if( (val = domsgsnd(NEWINC, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return NULL;
	}

	shutdown();

	if(t3comm->retval >= 0)
		return t3comm->data;
	else {
		errno = t3comm->errnum;
		return NULL;
	}
}

char *
newlog(logno)
int     *logno;
{
	int     val;

    if( (val = startup()) != 0)
		return NULL;

	t3comm->size = 0;

	if( (val = domsgsnd(NEWLOG, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return NULL;
	}

	shutdown();

	*logno = t3comm->retval;
	if(t3comm->retval >= 0)
		return t3comm->data;
	else {
		errno = t3comm->errnum;
		return NULL;
	}
		
}

int
 findre(pat, p)
char    *pat, **p;
{
	struct dbrechdr *dbrech;
	int     val;

	mpm_findre = 1;

	if( (val = startup()) != 0)
	{
		mpm_findre = val;
		return val;
	}	

	if(pattern != NULL)
		free(pattern);

	pattern = malloc(strlen(pat) + 1);
	strcpy(pattern, pat);


	strcpy(t3comm->data, pattern);
	t3comm->size = strlen(pattern) + 1;

	if( (val = domsgsnd(FINDRE, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	errno = t3comm->errnum;
	if(t3comm->retval > 0) {
		*p = t3comm->data;
		dbrech = (struct dbrechdr *)t3comm->data;
		lastrelen = strlen(dbrech->word);
		lastrepos = lastpos = t3comm->pos;
	} else {
		lastrelen = -1;
		lastrepos = lastpos = -1;
	}

	shutdown();

	return t3comm->retval;
}

int
nextre(p)
char    **p;
{
	struct dbrechdr *dbrech;
	int     val;

	if(pattern == NULL || lastrelen == -1)
		return NOWORD;

	if( (val = startup()) != 0)
		return val;

	strcpy(t3comm->data, pattern);
	t3comm->size = strlen(pattern) + 1;
	t3comm->entry = lastrelen;
	t3comm->pos = lastrepos;

	if( (val = domsgsnd(NEXTRE, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	errno = t3comm->errnum;

	if(t3comm->retval > 0) {
		*p = t3comm->data;
		dbrech = (struct dbrechdr *)t3comm->data;
		lastrelen = strlen(dbrech->word);
		lastrepos = lastpos = t3comm->pos;
	}

	shutdown();
	
	return t3comm->retval;
}

int
firstword(dbrec)
register struct dbrec **dbrec;
{
	int     val;

	if( (val = startup()) != 0)
		return val;

	t3comm->size = 0;

	if( (val = domsgsnd(FIRSTWORD, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	errno = t3comm->errnum;

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
	int     val;

	if( (val = startup()) != 0)
		return val;

	dbrh = &((*dbrec)->dbrech);
	/* dbrb = (struct dbrecbody *)((dbrh->word)
						   + strlen(dbrh->word) + 1);
				
	*/

	dbrb = (struct dbrecbody *)((char *)(dbrh->word) + (((strlen(dbrh->word) + 4) /4)*4));  /* align for solaris */


	t3comm->pos = lastpos;

	t3comm->size = 0;

	if( (val = domsgsnd(NEXTWORD, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	errno = t3comm->errnum;

	if(t3comm->retval > 0) {
		*dbrec = (struct dbrec *)t3comm->data;
		lastpos = t3comm->pos;
	}

	shutdown();

	return t3comm->retval;
}

int
nextsame(dbrec)
register struct dbrec **dbrec;
{
	struct dbrechdr *dbrh;
	struct dbrecbody *dbrb;
	int     val;

	dbrh = &((*dbrec)->dbrech);
	/* dbrb = (struct dbrecbody *)((dbrh->word)
							   + strlen(dbrh->word) + 1);
	*/

	dbrb = (struct dbrecbody *)((char *)(dbrh->word) + (((strlen(dbrh->word) + 4) /4)*4));  /* align for solaris */


	
	if( (t3comm->pos = dbrb->nxtsame) == 0)
		return NOWORD;

	t3comm->size = 0;

	if( (val = startup()) != 0)
		return val;

	if( (val = domsgsnd(NEXTSAME, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	errno = t3comm->errnum;

	if(t3comm->retval > 0) {
		*dbrec = (struct dbrec *)t3comm->data;
		lastpos = t3comm->pos;
	}

	shutdown();

	return t3comm->retval;
}

int
addfile(fn, entry, lineno)
char    *fn;
int     entry, *lineno;
{
	int     val;

	if( (val = startup()) != 0)
		return val;

	t3comm->size = strlen(fn) + 1;
	strcpy(t3comm->data, fn);
	t3comm->entry = entry;

	if( (val = domsgsnd(ADDFILE, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	errno = t3comm->errnum;
	*lineno = t3comm->entry;

	shutdown();

	return t3comm->retval;
}

int
maxent()
{
	int     val;

	if( (val = startup()) != 0)
		return val;

	t3comm->size = 0;

	if( (val = domsgsnd(MAXENT, t3comm)) != 0) {
		int sverrno = errno;

		shutdown();
		errno = sverrno;
		return val;
	}

	shutdown();

	return t3comm->retval;
}

static int startup()
{
	char    *fn;
	unsigned char seq = '\0';
	int     len, iterations = 0;
	key_t   msgkey;
	FILE    *fp;
	struct msgbuf t;
	
	strcpy(mpm_dbname, "fred");
	
	if(dbname == NULL)
	{
		strcpy(mpm_dbname, "Mike");
		return CANTOPEN;
	}

	
	fn = malloc(strlen(dbdir) + strlen(DEFMSGQ) + 2);
	strcpy(fn, dbdir);
	if(strlen(dbdir) > 0)
		strcat(fn, "/");
	strcat(fn, DEFMSGQ);
	if( (fp = fopen(fn, "r")) == NULL)
	{
		return CANTOPENMSGQ;
	}

        free(fn);

	if( (len = fread(&msgqid, 1, sizeof(msgqid), fp)) != sizeof(msgqid))
	    {
		debugmsg("fread failure in opendb: filename: %s\n", fn);
		return -1;
	    }	

	fclose(fp);

	do {
		if(seq++ == 255) {
			if(iterations++ > 0)
				sleep(3);
				
			if(iterations > 5)
			{
			    debugmsg("Iteration count exceeded"); 
				return -1;
			}	
		}

		if( (msgkey = ftok(dbname, seq++)) == (key_t)-1)
		    {
			strcpy(mpm_dbname,"joe \0");                             
			return -1;
		    }	
	} while( (retmsgqid = msgget(msgkey, 0666 | IPC_CREAT | IPC_EXCL)) < 0);


	if(t3comm == NULL) {
		imsgbuf = (struct msgbuf *)malloc(sizeof(struct t3comm)
										 + MAXMSGLEN
										 + sizeof(t.mtype));
		t3comm = (struct t3comm *)imsgbuf->mtext;
		t3comm->msgqid = retmsgqid;
	}

	return 0;

}

static int shutdown()
 {
	if(retmsgqid >= 0)
		msgctl(retmsgqid, IPC_RMID, 0);

	retmsgqid = 0;
}

static int domsgsnd(cmd, t3comm)
int     cmd;
struct t3comm *t3comm;
{
	int     val, iterations = 0;



	t3comm->msgqid = retmsgqid;
	imsgbuf->mtype = cmd;

	while( (val = msgsnd(msgqid, imsgbuf,
	 					 sizeof(struct t3comm) + t3comm->size, 0)) != 0) {
		if(iterations >= 5)
			break;
		if(iterations++ > 0)
			sleep(1);
			
	}

	if(val < 0)
	{
		mpm_msgrcv = 0;
		mpm_msgsnd = 1;
		mpm_errno = errno;
		mpm_val = val;
		return -1;
	}	

	imsgbuf->mtype = 0;
	if( (val = msgrcv(retmsgqid, imsgbuf, MAXMSGLEN, 0, 0)) < 0)
	{
		mpm_msgrcv = 1;
		mpm_msgsnd = 0;
		mpm_errno = errno;
		mpm_val = val;
		return -2;
	}	

	return 0;
}
