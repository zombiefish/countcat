#ident "%W% %G% %U%"

/* %W% - last delta: %G% %U% gotten %H% %T% from %P%. */
/*   This is the code for the daemon that updates a database.  There is
** one daemon for each database that is active.  It operates as follows:
**    1)  Determine if the database exists.  If it does then skip to
**        step 3 below.
**    2)  Create the database by touching it.
**    3)  Determine if the sequence file exists.  If it does then skip
**        to step 5 below.
**    4)  Create the sequence file by writing a string of length KEYLEN
**        zeroes to it.
**    5)  Create a message port through which interface routines can
**        request service.
**    6)  Announce that we're up.
**    7)  Wait for a message or a signal.  When one is received then
**        handle it appropriately.
** See the individual routines for detailed information.
**/

#include "t3db.h"
#include "t3dbfmt.h"
#include "t3daemon.h"
#include "t3comm.h"
#include "t3dberr.h"
#include <time.h>

extern char *sys_errlist[];

static void gennames();
static void checknames();
static void get_msg_port();
static void dieint();
static void diequit();
static void dieterm();
static void diehup();
static void debug();
static void usage();
static void errmsg();
static void checkout();
static int openit(register struct t3comm *t3comm);
static int wait_for_msg();
static int frstwrd(register struct t3comm *t3comm);
static int nxtwrd(struct t3comm *t3comm);
static int addwrd(register struct t3comm *t3comm);
static int nxtsame(struct t3comm *t3comm);
static int closeit(register struct t3comm *t3comm);
static int newinc(register struct t3comm *t3comm);
static int newlog(register struct t3comm *t3comm);
static int fndre(register struct t3comm *t3comm);
static int nxtre(register struct t3comm *t3comm);
static int addfile(struct t3comm *t3comm);
static int mxent(struct t3comm *t3comm);
static int quiesce(struct t3comm *t3comm);
static int resume(struct t3comm *t3comm);
static int sendmsg(struct t3comm *t3comm);
static char * resolve(char *fn);
static void chown(char *fn);
//struct msgbuf
//  {
//    long int mtype;             /* type of received/sent message */
//    char mtext[1];              /* text of the message */
//  };

extern char *getenv(), *malloc(), *getcwd(), cmdname[], *optarg;

#define QNAME "/QUIESCE"

char *mymalloc();

extern int	  errno, sys_nerr, optind, opterr;
extern char  *Lomem, *Himem;

extern int	  maxbuf;

char *dbdir     = NULL,
	 *dbname    = NULL,
	 *dbpath    = NULL,
	 *textdir   = NULL,
	 *textpath  = NULL,
	 *incidents = NULL,
	 *loginfo   = NULL,
	 *incseqfn  = NULL,
	 *logseqfn  = NULL,
	 *msgqname  = NULL,
	 *mtext     = NULL;
	 


static int checkfile();

char *resolve();

#define RWX     (S_IREAD | S_IWRITE | S_IEXEC)
#define RW      (S_IREAD | S_IWRITE)

struct filetypes {
	char **fn;                 /* Pointer to filename                   */
	int  perm;                 /* Permissions needed                    */
	char filedir;              /* Whether it's a file or a dir          */
} myfiles[] = {
{   &dbdir,      RWX | RWX >> 3,    'd'},
{   &dbpath,     RW  | RW  >> 3,    'f'},
{   &textpath,   RWX | RWX >> 3,    'd'},
{   &incidents,  RW  | RW  >> 3,    'f'},
{   &loginfo,    RW  | RW  >> 3,    'f'},
{   &incseqfn,   RW  | RW  >> 3,    'f'},
{   &logseqfn,   RW  | RW  >> 3,    'f'},
{   &msgqname,   RW  | RW  >> 3,    'f'},
{   NULL,       0,             '\0'} };

struct msgtypes {
	long    command;            /* Command requested                    */
    int		ok_when_quiesced;	/* Command may run while quiesced		*/
	int     (*func)();          /* Function to invoke                   */
} msgtypes[] = {
{   OPENDB,         1, openit},
{   FIRSTWORD,      1, frstwrd},
{   NEXTWORD,       1, nxtwrd},
{   ADDWORD,        0, addwrd},
{   NEXTSAME,       1, nxtsame},
{   CLOSEDB,        1, closeit},
{   NEWLOG,         0, newlog},
{   NEWINC,         0, newinc},
{   FINDRE,         1, fndre},
{   NEXTRE,         1, nxtre},
{   ADDFILE,        0, addfile},
{   MAXENT,         1, mxent},
{   QUIESCE,        1, quiesce},
{   RESUME,         1, resume},
{   MAXCMD+1,       1, NULL} };

int  incseqfd, logseqfd, quiesced = 0;

char incseq[KEYLEN + 1],
     logseq[KEYLEN + 1];

int debug_lvl = 0,            /* Debugging level requested              */
	dyingint = 0,             /* Received SIGINT  to go away            */
	dyingquit= 0,             /* Received SIGQUIT to go away            */
	dyingterm= 0,             /* Received SIGTERM to go away            */
	dyinghup= 0,              /* Received SIGHUP  to go away            */
	inclock_held = 0,         /* flock()ed the inc sequence file        */
	loglock_held = 0,         /* flock()ed the log sequence file        */
	got_msgqfile = 0,         /* Created the MSGQ file                  */
	pid = 0;                  /* PID of the running process             */

struct msgbuf *msgbuf;

struct dbifc *dbifc = NULL;

FILE    *errfile;

main(argc, argv)
int argc;
char **argv;
{
	int     i, val;
	int     redirect = 0,
			background = 0;

	struct t3comm *t3comm;
	long    command;
	char    *prevHimem = NULL;
	errfile = stderr;
	strcpy(cmdname, argv[0]);
	while( (i = getopt(argc, argv, "c:D:drb")) != EOF) {
		switch(i) {
		case 'D':
			dbdir = malloc(strlen(optarg) + 1);
			strcpy(dbdir, optarg);
			break;
		case 'd':
			debug_lvl++;
			break;
		case 'r':
			redirect = 1;
			break;
		case 'b':
			background = 1;
			break;
		case 'c':
		{
			int cachesz = 0;
			int i;
			char c;

			for(i = 0; c = optarg[i]; i++) {
				if(c >= '0' && c <= '9')
					cachesz = (cachesz * 10) + (c - '0');
				else if(c == 'M' || c == 'm') {
					if(cachesz <= 0)
						usage();
					cachesz *= 0x100000;
					i++;
					break;
				} else if(c == 'K' || c == 'k') {
					if(cachesz <= 0)
						usage();
					cachesz *= 0x1000;
					i++;
					break;
				} else
					usage();
			}

			if(optarg[i])
				usage();

			if(cachesz > 0xa00000 || cachesz < 0xa000) {
					fprintf(stderr, "Cache size must be <= 10 meg. and >= 10K\n");
					exit(2);
			}

			maxbuf = cachesz;
			break;
		}
		case '?':
			usage();
			break;
		}
	}

	if(optind != argc)
		usage();

	gennames();
		
/* 	if(wd_flock(incseqfn, 0) < 0 && access(incseqfn, 0) == 0)
		errmsg(1, "Can't lock %s...another daemon already up?\n", incseqfn);
	wd_unflock(incseqfn);
*/

	if(background) {
		redirect = 1;
		for(i = 0; i < 10 && (pid = fork()) == -1; i++) {
			if(errno != EAGAIN)
				errmsg(1, "Can't become a background task: %s\n", sys_errlist[errno]);
			sleep(1);
		}
		if(pid != 0)
			exit(0);
		signal(SIGHUP, SIG_IGN);
		setpgrp();
	}

	pid = getpid();

	if(redirect) {
		long    clock = time(0);
		extern char *ctime();
		char    *dt = ctime(&clock), *cp, *pfx = "/Log.";
		int     lendbdir = strlen(dbdir), lenpfx = strlen(pfx);

		cp = malloc(lendbdir + lenpfx + 6);
		strcpy(cp, dbdir);
		strcat(cp, pfx);
		strncat(cp, dt+4, 3);
		strncat(cp, dt+8, 2);
		if(cp[strlen(cp) - 2] == ' ')
			cp[strlen(cp) - 2] = '0';

		if( (errfile = fopen(cp, "a")) == NULL) {
			errfile = stderr;
			errmsg(1, "Can't open %s for error msg output\n", cp);
		}
		free(cp);
	}
	
	debug(1, "Names are:\n");
	debug(1, "dbdir    = %s\n", dbdir);
	debug(1, "dbname   = %s\n", dbname);
	debug(1, "dbpath   = %s\n", dbpath);
	debug(1, "textdir  = %s\n", textdir);
	debug(1, "textpath = %s\n", textpath);
	debug(1, "incseq   = %s\n", incseqfn);
	debug(1, "logseq   = %s\n", logseqfn);
	debug(1, "msgqname = %s\n", msgqname);
	debug(1, "Cachesize= %d (0x%08x)\n", maxbuf, maxbuf);
	debug(1, "Names done:\n");

	checknames();

	msgbuf = (struct msgbuf *)malloc(sizeof(struct msgbuf) + MAXMSGLEN);

	signal(SIGINT, dieint);
	signal(SIGQUIT, diequit);
	signal(SIGTERM, dieterm);
	if(!background)
		signal(SIGHUP, diehup);

	dbifc = (struct dbifc *)malloc(sizeof(struct dbifc) + MAXMSGLEN);
	dbifc->fd = -1;

	dbifc->fn = dbpath;
	debug(1, "Calling opendb:\n");
	if( (val = opendb(dbifc, O_RDWR)) < 0)
		if(val == -1)
			errmsg(1, "Can't open db %s in read/write: %s\n",
					   dbname, sys_errlist[errno]);
		else
			errmsg(1, "Exitting because of above errors!\n");

	debug(1, "Calling  get_msg_port:\n");
	get_msg_port();
	debug(1, "Returned from  get_msg_port:\n");
	debug(1, "%s: Up, ready and waiting\n", cmdname);

	while(dyingint + dyingquit + dyingterm + dyinghup == 0) {
		if( (val = wait_for_msg()) < 0) {
			if(errno != EINTR)
				/* errmsg(1, "Bad return from msgrcv: %s\n", sys_errlist[errno]); */
				debug(1, "Bad return from msgrcv: %s\n", sys_errlist[errno]);
			continue;
		}

		t3comm = (struct t3comm *)(msgbuf->mtext);
		command = msgbuf->mtype;

		for(i = 0; msgtypes[i].command < command; i++)
			;

		if(msgtypes[i].command == command) {
			if((msgtypes[i].ok_when_quiesced == 0) && quiesced) {
				t3comm->retval = QUIESCED;
				sendmsg(t3comm);
			} else {
				(*msgtypes[i].func)(t3comm);
			}
		} else {
			t3comm->retval = BADCMD;
			sendmsg(t3comm);
		}
		if(prevHimem == NULL)
			prevHimem = Himem;
		else {
			if(Himem != prevHimem) {
				debug(15, "prevHimem = 0x%08lx, Himem now 0x%08lx\n",
					  prevHimem, Himem);
				prevHimem = Himem;
			}
		}
	}
	if(dyingint)
		debug(0, "Caught SIGINT - exitting\n");
	if(dyingquit)
		debug(0, "Caught SIGQUIT - exitting\n");
	if(dyingterm)
		debug(0, "Caught SIGTERM - exitting\n");
	if(dyinghup)
		debug(0, "Caught SIGHUP - exitting\n");
	checkout();
	exit(0);
}

/*   Gennames() will generate the names of all database-related files that
** are used.  These are:
**      dbdir   - the directory in which all other files reside
**      dbname  - the name of the database sans .dir/.pag
**      dbpath  - the juxtaposition of dbdir and dbname
**      textdir - the directory in which the text files reside
**      textpath- the juxtaposition of dbdir and textdir
**      sequence- the sequence number file
**/
static void gennames()
{
	char *cp;
	int  len;

	if(dbdir == NULL) {
		if( (cp = getenv("DBDIR")) == NULL)
			cp = DEFDBDIR;

		len = strlen(cp);
		dbdir = malloc(len + 1);
		strcpy(dbdir, cp);
	} else
		len = strlen(dbdir) + 1;

	if( (cp = getenv("DBNAME")) == NULL)
		cp = DEFDBNAME;

	len += strlen(cp) + 1;
	dbname = malloc(strlen(cp) + 1);
	strcpy(dbname, cp);
	dbpath = malloc(len + 1);
	strcpy(dbpath, dbdir);
	strcat(dbpath, "/");
	strcat(dbpath, dbname);

	if( (cp = getenv("INCSEQ")) == NULL)
		cp = DEFINCSEQ;
	incseqfn = malloc(len + strlen(cp) + 1);
	strcpy(incseqfn, dbdir);
	strcat(incseqfn, "/");
	strcat(incseqfn, cp);

	if( (cp = getenv("LOGSEQ")) == NULL)
		cp = DEFLOGSEQ;
	logseqfn = malloc(len + strlen(cp) + 1);
	strcpy(logseqfn, dbdir);
	strcat(logseqfn, "/");
	strcat(logseqfn, cp);

	cp = DEFMSGQ;
	msgqname = malloc(len + strlen(cp) + 1);
	strcpy(msgqname, dbdir);
	strcat(msgqname, "/");
	strcat(msgqname, cp);

	if( (cp = getenv("TEXTDIR")) == NULL)
		cp = DEFTEXTDIR;

	len = strlen(cp);
	textdir = malloc(len + 1);
	strcpy(textdir, cp);
	len += strlen(dbdir);
	textpath = malloc(len + 1);
	strcpy(textpath, dbdir);
	strcat(textpath, "/");
	strcat(textpath, textdir);

	incidents = malloc(strlen(dbdir) + 1 + strlen(INCIDENTS) + 1);
	strcpy(incidents, dbdir);
	strcat(incidents, "/");
	strcat(incidents, INCIDENTS);

	loginfo = malloc(strlen(dbdir) + 1 + strlen(LOGINFO) + 1);
	strcpy(loginfo, dbdir);
	strcat(loginfo, "/");
	strcat(loginfo, LOGINFO);
}

/*   Checknames() will verify the existance of all necessary files.  It
** will create those files that it can and issue a warning message.  If it
** can't create a file it will issue an error message and exit.
**/
static void checknames()
{
	int     i, bytes, fd;
	struct filetypes *ftp;

    for(ftp = myfiles; ftp->fn; ftp++) {
		if( (fd = checkfile(ftp->fn, ftp->perm, ftp->filedir)) < 0)
			errmsg(1, "Can't create/open %s: %s\n", *(ftp->fn), sys_errlist[errno]);
		if(ftp->filedir == 'f')
			close(fd);
	}

	debug(1, "Calling wd_flock for INCSEQ:\n");
	if(wd_flock(incseqfn, 1) < 0) {
		debug(1, "Back from (error)  wd_flock for INCSEQ:\n");
		errmsg(1, "Can't lock %s...another daemon already up?\n", incseqfn);
	}	
	debug(1, "Back from (OK) wd_flock for INCSEQ:\n");
	inclock_held = 1;

	incseqfd = open(incseqfn, O_RDWR | O_SYNC);
	lseek(incseqfd, 0L, 0);
	debug(1, "Done open and lseek for INCSEQ:\n");
	if( (bytes = read(incseqfd, incseq, KEYLEN)) == 0) {
		/* Create the file */
		debug(1, "Creating the file for INCSEQ:\n");
		for(i = 0; i < KEYLEN; i++) {
			incseq[i] = '0';
			incseq[i+1] = '\0';
		}
		lseek(incseqfd, 0L, 0);
		write(incseqfd, incseq, KEYLEN);
		lseek(incseqfd, 0L, 0);
	} else if(bytes < KEYLEN) {
		debug(1, "Bytes read zero for INCSEQ:\n");
		errmsg(1, "%s is corrupted - short read!\n", incseqfn);
	}

	debug(1, "Calling wd_flock for LOGSEQ:\n");
	if(wd_flock(logseqfn, 1) < 0)
		errmsg(1, "Can't lock %s...another daemon already up?\n", logseqfn);
	loglock_held = 1;

	logseqfd = open(logseqfn, O_RDWR | O_SYNC);
	lseek(logseqfd, 0L, 0);
	if( (bytes = read(logseqfd, logseq, KEYLEN)) == 0) {
		/* Create the file */
		for(i = 0; i < KEYLEN; i++) {
			logseq[i] = '0';
			logseq[i+1] = '\0';
		}
		lseek(logseqfd, 0L, 0);
		write(logseqfd, logseq, KEYLEN);
		lseek(logseqfd, 0L, 0);
	} else if(bytes < KEYLEN)
		errmsg(1, "%s is corrupted - short read!\n", logseqfn);
}

/*   Get_msg_port() will create a message port that is accessible by
** everyone.  If the msg port already exists it will issue an error msg
** and exit.
**/
static void get_msg_port()
{
	FILE    *fp;
	int     val;

	if( (msgkey =  ftok(dbdir, '3')) == (key_t)-1)
		errmsg(1, "Can't create a message key based on %s\n", dbdir);

	debug(1, "msgkey from ftok() = (0x%08x)\n", msgkey);

	debug(1, "Calling msgget");
	msgqid = msgget(msgkey, 0666 | IPC_CREAT | IPC_EXCL);
	debug(1, "Returned from msgget");

	/* if( (msgqid = msgget(msgkey, 0666 | IPC_CREAT | IPC_EXCL)) < 0) { */
	if (msgqid  < 0) {
		debug(1,"Can't create msg q for %s: %s\n", dbdir, sys_errlist[errno]);
		errmsg(1, "Can't create msg q for %s: %s\n", dbdir, sys_errlist[errno]);
	}

		
	debug(1, "msgqid from msgget() = (0x%08x)\n", msgqid);


	if( (fp = fopen(msgqname, "w")) == NULL)
		errmsg(1, "Can't open %s for writing\n", msgqname);
	if( (val = fwrite(&msgqid, 1, sizeof(msgqid), fp)) != sizeof(msgqid)) {
		fclose(fp);
		unlink(msgqname);
		errmsg(1, "Tried to write %d bytes to %s but only wrote %d: %s\n",
			   sizeof(msgqid), val, msgqname, sys_errlist[errno]);
	}
	got_msgqfile = 1;
	fclose(fp);
}

/*   A die*() routine is invoked whenever one of SIGINT, SIGHUP, SIGQUIT
** or SIGTERM is received.  It sets the "dying" flag and exits */
static void dieint()
{
	dyingint = 1;
	signal(SIGINT, dieint);
}

static void diequit()
{
	dyingquit = 1;
	signal(SIGQUIT, diequit);
}

static void dieterm()
{
	dyingterm = 1;
	signal(SIGTERM, dieterm);
}

static void diehup()
{
	dyinghup = 1;
	signal(SIGHUP, diehup);
}

/*   The debug routine takes a varargs-style parm list.  The first parm
** is the level at which this message should be produced and the remaining
** parms are printf-style args.  If the debug level is higher than the
** passed level no message is issued.
**/
/*VARARGS*/
static void debug(va_alist)
va_dcl
{
	int         level;
	char        *fmt;
	va_list     arg_ptr;
#ifdef UTS20
	struct tm *t;
#else
	struct tm t;
#endif /* UTS20 */
	long        clock;

	va_start(arg_ptr);
	level = va_arg( arg_ptr, int);
	if(level > debug_lvl)
		return;

	fmt = va_arg(arg_ptr, char *);
	if(!isatty(fileno(errfile)))
		fseek(errfile, 0L, 2);

	clock=time((long*)0);
	t = localtime((long*)&clock);
		
	fprintf(errfile, "PID %d ", pid);
#ifdef UTS20
	fprintf(errfile, "%0.2d/%0.2d/%0.2d ", t->tm_mon+1, t->tm_mday,
					 t->tm_year);
	fprintf(errfile, "%0.2d:%0.2d:%0.2d: ", t->tm_hour, t->tm_min, t->tm_sec);
#else
	fprintf(errfile, "%0.2d/%0.2d/%0.2d ", t.tm_mon+1, t.tm_mday,
					 t.tm_year);
	fprintf(errfile, "%0.2d:%0.2d:%0.2d: ", t.tm_hour, t.tm_min, t.tm_sec);
#endif /* UTS20 */
	vfprintf(errfile, fmt, arg_ptr);
	fflush(errfile);
}

static void usage()
{
	fprintf(stderr, "%s: Usage: %s [-c cache-size[MmKk] [-D dbdir] [-d...] [-r] [-b]\n", cmdname, cmdname);
	exit(2);
}

/*   The errmsg routine takes a varargs-style parm list.  The first parm
** is the exit code and the remaining parms are printf-style args.
**/
/*VARARGS*/
static void errmsg(va_alist)
va_dcl
{
	int         exit_code;
	char        *fmt;
	va_list     arg_ptr;
#ifdef UTS20
	struct tm *t;
#else
	struct tm t;
#endif /* UTS20 */
	long        clock;
    
	va_start(arg_ptr);
	exit_code = va_arg(arg_ptr, int);

	fmt = va_arg(arg_ptr, char *);

	if(!isatty(fileno(errfile))) {
		clock=time((long*)0);
		t = localtime((long*)&clock);
	
		fseek(errfile, 0L, 2);

		fprintf(errfile, "PID %d ", pid);
#ifdef UTS20
		fprintf(errfile, "%0.2d/%0.2d/%0.2d ", t->tm_mon+1, t->tm_mday,
						 t->tm_year);
		fprintf(errfile, "%0.2d:%0.2d:%0.2d: ", t->tm_hour, t->tm_min, t->tm_sec);
#else
		fprintf(errfile, "%0.2d/%0.2d/%0.2d ", t.tm_mon+1, t.tm_mday,
						 t.tm_year);
		fprintf(errfile, "%0.2d:%0.2d:%0.2d: ", t.tm_hour, t.tm_min, t.tm_sec);
#endif /* UTS20 */
	}

	vfprintf(errfile, fmt, arg_ptr);

	if(exit_code == 0)
		return;

	checkout();
	exit(exit_code);
}

/*   Checkout() will get us out of the system cleanly. */
static void checkout()
{
	int temp = msgqid;
	char    *qfile;

	if(quiesced) {
		quiesced = 0;
		qfile = mymalloc(strlen(dbdir) + strlen(QNAME) + 1);
		strcpy(qfile, dbdir);
		strcat(qfile, QNAME);
		if(unlink(qfile) < 0)
			debug("Can't remove quiesce file %s: %s\n", qfile,
				sys_errlist[errno]);
		else
			debug(1, "Database resumed\n");
		myfree(qfile);
	}

	msgqid = 0;
	if(temp && (msgctl(temp, IPC_RMID, 0) < 0) )
		errmsg(1, "Can't remove msg q: %s\n", sys_errlist[errno]);

	if(inclock_held)
		wd_unflock(incseqfn);

	if(loglock_held)
		wd_unflock(logseqfn);

	if(dbifc != NULL && dbifc->fd >= 0) {
		dbifc->fn = dbpath;
		closedb(dbifc);
	}
	if(got_msgqfile)
		unlink(msgqname);
}

/*   Wait_for_msg() will wait for a message to be received and return
** whatever was returned by msgrcv().
**/
static int wait_for_msg()
{
	errno = 0;
	return(msgrcv(msgqid, msgbuf, MAXMSGLEN, 0L, 0));
}

/*   Checkfile() will ensure that a file of the proper type exists.  It is
** passed the filename, the permissions that the file must have if it must
** be created and a flag which indicates whether it's a directory or a
** file.  If there is an error and the target is a directory then this
** routine will issue error messages and exit.  If the target is a file
** then this routine will return the file descriptor regardless of whether
** an error occurred or not...it is the responsibility of the caller to
** issue appropriate error messages or close the file descriptor.
**/
static int
checkfile(fn, perm, type)
char **fn, type;
int perm;
{
	struct stat stbuf;
	char *temp, temp1[6];
	int fd;
	char *tempfile = *fn;

	char *mkdir = "mkdir ";
	char *chgrp = "chgrp logbook ";
	int lenmkdir = strlen(mkdir);
	int lenchgrp = strlen(chgrp);
    
	if(stat(*fn, &stbuf) < 0) {
		if(errno != ENOENT) {
			if(type == 'd')
				errmsg(1, "Can't stat %s: %s", *fn, sys_errlist[errno]);
			else
				return -1;
		}
		if(type == 'd') {
			temp = malloc(strlen(*fn) + lenmkdir + 1);
			strcpy(temp, mkdir);
			strcat(temp, *fn);
			if(system(temp) != 0)
				errmsg(1, "Can't create %s\n", *fn);
			free(temp);
			chown(*fn);
			if(chmod(*fn, perm) != 0)
				errmsg(1, "Can't chmod %s: %s\n", *fn, sys_errlist[errno]);
			return 0;
		} else {
			if( (fd = creat(*fn, perm)) > 0)
				debug(1, "Created %s\n", *fn);
			else
				debug(1, "Couldn't create %s: %s\n", *fn, sys_errlist[errno]);
			temp = malloc(strlen(*fn) + lenchgrp + 1);
			strcpy(temp, chgrp);
			strcat(temp, *fn);
			if(system(temp) != 0)
				errmsg(1, "Can't chgrp %s\n", *fn);
			if(chmod(*fn, perm) != 0)
				errmsg(1, "Can't chmod %s: %s\n", *fn, sys_errlist[errno]);
			return fd;
		}
	} else {
		if(type == 'd') {
			if( (stbuf.st_mode & S_IFDIR) == 0)
				errmsg(1, "%s is not a directory\n", *fn);
			else
				chown(*fn);
		} else {
			chown(*fn);
			return(fd = open(*fn, O_RDWR));
		}
	}
}

/*   Open the database.  Verify that the db that is to be opened is the
** db that we are servicing.  If it isn't then return CANTOPEN.
**/
static int openit(register struct t3comm *t3comm)
{
	int     val;
	char    *name;

	debug(10, "Opendb with dbname = %s\n", t3comm->data);

	if( (name = resolve(t3comm->data)) == NULL) {
		debug(2, "Returning CANTOPEN - can't resolve %s\n",
			  t3comm->data);
		return(t3comm->retval = CANTOPEN);
	}
		
	if(strcmp(name, dbpath) == 0) {
		dbifc->fn = t3comm->data;
		dbifc->errnum = t3comm->errnum = 0;
		dbifc->retval = t3comm->retval = 0;
		
		t3comm->pos = 0;
		val = t3comm->retval = 0;
	} else {
		debug(2, "Returning CANTOPEN - dbname \"%s\" differs from mine\n",
			  name);
		val = t3comm->retval = CANTOPEN;
	}

	free(name);
	sendmsg(t3comm);
	return(val);
}

/*   Retrieve the first word in the database.  Pretty simple, actually. */
static int frstwrd(register struct t3comm *t3comm)
{
	int     i;

	if(dbifc->fd < 0) {
		dbifc->fn = dbname;
		if( (i = opendb(dbifc)) < 0) {
			debug(1, "addwrd: Can't opendb %s, got %d\n", dbname, i);
			t3comm->retval = CANTOPEN;
			sendmsg(t3comm);
			return t3comm->retval;
		}
	}


	if( (dbifc->retval = firstword(dbifc)) < 0)
		debug(2, "Got %d on firstword, errno %d\n",
               dbifc->retval, dbifc->errnum);

	t3comm->pos = dbifc->pos;
	t3comm->errnum = dbifc->errnum;
	t3comm->retval = dbifc->retval;
	
	if( (t3comm->retval = dbifc->retval) > 0) {
		t3comm->size = dbifc->size;
		memcpy(t3comm->data, dbifc->data, t3comm->size);
	} else
		t3comm->size = 0;

	sendmsg(t3comm);

	return(dbifc->retval);
}

static int nxtwrd(struct t3comm *t3comm)
{
	int     val;

	if(dbifc->fd < 0) {
		dbifc->fn = dbname;
		if( (val = opendb(dbifc)) < 0) {
			debug(1, "nxtwrd: Can't opendb %s, got %d\n", dbname, val);
			t3comm->retval = CANTOPEN;
			sendmsg(t3comm);
			return t3comm->retval;
		}
	}

	dbifc->pos = t3comm->pos;


	if(( (dbifc->retval = nextword(dbifc)) < NOWORD)
	|| (dbifc->retval == -1))
		debug(2, "Got %d on nextword with pos 0x%08lx, errno %d\n",
			  dbifc->retval, dbifc->pos, dbifc->errnum);

	t3comm->pos = dbifc->pos;
	t3comm->errnum = dbifc->errnum;
	t3comm->retval = dbifc->retval;
	
	if( (t3comm->retval = dbifc->retval) > 0) {
		t3comm->size = dbifc->size;
		memcpy(t3comm->data, dbifc->data, t3comm->size);
	} else
		t3comm->size = 0;

	sendmsg(t3comm);

	return(dbifc->retval);
}

/*   Add a new word to the database.  We're passed the word to add in
** t3comm->data, the size of the word in t3comm->size and the value to
** associate with the word in t3comm->entry.  Pass these to the daemon.
**/
static int addwrd(register struct t3comm *t3comm)
{
	char    *p;
	int     i;

	if(dbifc->fd < 0) {
		dbifc->fn = dbname;
		if( (i = opendb(dbifc)) < 0) {
			debug(1, "addwrd: Can't opendb %s, got %d\n", dbname, i);
			t3comm->retval = CANTOPEN;
			sendmsg(t3comm);
			return t3comm->retval;
		}
	}

	dbifc->entry = t3comm->entry;
	dbifc->pos = t3comm->pos;
	p = dbifc->data;
	strcpy(p, t3comm->data);


	dbifc->size = t3comm->size;
	dbifc->errnum = t3comm->errnum = 0;

	if( (dbifc->retval = addword(dbifc)) < 0)
		debug(2, "Got %d on addword with word \"%s\" and value %d, errno %d\n",
               dbifc->retval, dbifc->data, dbifc->entry, dbifc->errnum);

	t3comm->pos = dbifc->pos;
	t3comm->errnum = dbifc->errnum;
	t3comm->retval = dbifc->retval;

	sendmsg(t3comm);
	return(dbifc->retval);
}

/*   Find the next entry for the same word.  We're given the current
** position in the file and that's just what the daemon needs to find the
** next one.  Pass this value to it.
**/
static int nxtsame(struct t3comm *t3comm)
{
	int     i;

	if(dbifc->fd < 0) {
		dbifc->fn = dbname;
		if( (i = opendb(dbifc)) < 0) {
			debug(1, "nxtsame: Can't opendb %s, got %d\n", dbname, i);
			t3comm->retval = CANTOPEN;
			sendmsg(t3comm);
			return t3comm->retval;
		}
	}

	dbifc->pos = t3comm->pos;


	dbifc->errnum = t3comm->errnum = 0;

	if(dbifc->pos > 0) {
		if(( (dbifc->retval = nextsame(dbifc)) < NOWORD)
		|| (dbifc->retval == -1))
			debug(2, "Got %d on nextsame with pos 0x%08lx, errno %d\n",
                   dbifc->retval, dbifc->pos, dbifc->errnum);
				   
	} else
		dbifc->retval = NOWORD;

	t3comm->pos = dbifc->pos;
	t3comm->errnum = dbifc->errnum;
	t3comm->retval = dbifc->retval;
	
	if( (t3comm->retval = dbifc->retval) > 0) {
		t3comm->size = dbifc->size;
		memcpy(t3comm->data, dbifc->data, t3comm->size);
	} else
		t3comm->size = 0;

	sendmsg(t3comm);

	return(dbifc->retval);
}

/*   Close the database.  Don't inform the daemon of this since other
** folks may still be using it.  This call is just here for completeness.
**/
static int closeit(register struct t3comm *t3comm)
{
	int     val;

	dbifc->errnum = t3comm->errnum = 0;
	dbifc->retval = t3comm->retval = 0;

	t3comm->pos = 0;
	t3comm->errnum = 0;
	val = t3comm->retval = 0;

	sendmsg(t3comm);
	return(val);
}

/*   Create a new incident.  This is nothing more than incrementing the
** incident number counter that is stored in incseq.  No need to call the
** daemon for this, just pass back the next number after re-writing the
** file.
**/
static int newinc(register struct t3comm *t3comm)
{
	int     seq;

	seq = atoi(incseq) + 1;
	sprintf(incseq, "%06d", seq);
	
	lseek(incseqfd, 0L, 0);
	write(incseqfd, incseq, KEYLEN);

	strncpy(t3comm->data, incseq, KEYLEN);

	t3comm->size = strlen(t3comm->data) + 1;
	sendmsg(t3comm);
}

/*   Create a file for a new logbook entry.  No need to call the daemon
** for this since we're creating a file in the directory defined by
** textpath.  All filenames in this dir are 6-byte ASCII numbers.  Find
** the next highest unused one, create it and pass the name back to the
** requester.  Pass back the new number, as an integer, in t3comm->retval.
**/
static int newlog(register struct t3comm *t3comm)
{
	int     seq, fd;
	char    *p;
	struct stat stbuf;

	debug(1, "newlog() entered:\n");

	p = malloc(strlen(textpath) + KEYLEN + 2);
	seq = atoi(logseq);
	do {
		sprintf(logseq, "%06d", ++seq);
		sprintf(p, "%s/%06d", textpath, seq);
	} while(stat(p, &stbuf) >= 0);

	if(errno == ENOENT) {
		fd = checkfile(&p, RW | RW >> 3, 'f');
		close(fd);
		
		lseek(logseqfd, 0L, 0);
		write(logseqfd, logseq, KEYLEN);
	
		strcpy(t3comm->data, p);
		t3comm->size = strlen(t3comm->data) + 1;
		t3comm->retval = seq;
	} else {
		t3comm->size = 0;
		t3comm->retval = CANTOPEN;
		*(t3comm->data) = '\0';
	}

	debug(1, "newlog() sending message back:\n");
	sendmsg(t3comm);
	free(p);
	debug(1, "newlog() exited:\n");
}

/*   Find an occurrence of a regular expression.  We're passed the regexp
** in t3comm->data and the size of the regexp in t3comm->size.  Return
** the first record that matched.
**/
static int fndre(register struct t3comm *t3comm)
{
	char    *p;
	int     i;

	if(dbifc->fd < 0) {
		dbifc->fn = dbname;
		if( (i = opendb(dbifc)) < 0) {
			debug(1, "fndre: Can't opendb %s, got %d\n", dbname, i);
			t3comm->retval = CANTOPEN;
			sendmsg(t3comm);
			return t3comm->retval;
		}
	}

	dbifc->entry = t3comm->entry;
	dbifc->pos = t3comm->pos;
	p = dbifc->data;
	strcpy(p, t3comm->data);


	dbifc->size = t3comm->size;
	dbifc->errnum = t3comm->errnum = 0;

	if( (dbifc->retval = findre(dbifc)) < NOWORD)
		debug(2, "Got %d on findre with word \"%s\", errno %d\n",
               dbifc->retval, dbifc->data, dbifc->errnum);

	t3comm->pos = dbifc->pos;
	t3comm->errnum = dbifc->errnum;
	t3comm->retval = dbifc->retval;
	
	if( (t3comm->retval = dbifc->retval) > 0) {
		t3comm->size = dbifc->size;
		memcpy(t3comm->data, dbifc->data, t3comm->size);
	} else
		t3comm->size = 0;

	sendmsg(t3comm);
	return(dbifc->retval);
}

/*   Find the next occurrence of a regular expression.  We're passed the
** position of the current record in t3comm->pos, the size of the last
** matching word in t3comm->entry, the regexp in t3comm->data and the
** size of the regexp in t3comm->size.  Return the next matching record.
**/
static int nxtre(register struct t3comm *t3comm)
{
	char    *p;

	dbifc->entry = t3comm->entry;
	dbifc->pos = t3comm->pos;
	p = dbifc->data;
	strcpy(p, t3comm->data);


	dbifc->size = t3comm->size;
	dbifc->errnum = t3comm->errnum = 0;

	if( (dbifc->retval = nextre(dbifc)) < NOWORD)
		debug(2, "Got %d on nextre with word \"%s\", errno %d\n",
               dbifc->retval, dbifc->data, dbifc->errnum);

	t3comm->pos = dbifc->pos;
	t3comm->errnum = dbifc->errnum;
	t3comm->retval = dbifc->retval;

	if( (t3comm->retval = dbifc->retval) > 0) {
		t3comm->size = dbifc->size;
		memcpy(t3comm->data, dbifc->data, t3comm->size);
	} else
		t3comm->size = 0;

	sendmsg(t3comm);
	return(dbifc->retval);
}

/*   Add all of the words in a specified file.  The file must contain one
** word per line.  The filename is specified in t3comm->data, the length
** of the filename in t3comm->size and the entry that is to be associated
** with each word is in t3comm->entry.  Any errors are passed back in
** t3comm->retval and processing is halted.  In this case, t3comm->entry
** is set to the line number in the file where the error occurred.
**/
static int addfile(struct t3comm *t3comm)
{
	int     i, val = 1, lineno = 0;
	FILE    *fp;
	char    buf[513];


	if( (fp = fopen(t3comm->data, "r")) == NULL) {
		debug(2, "Can't open %s for reading in routine addfile: %s\n",
			  t3comm->data,sys_errlist[errno]);
		t3comm->retval = CANTOPEN;
		t3comm->errnum = errno;
		t3comm->entry = -1;
		sendmsg(t3comm);
		return CANTOPEN;
	}
		
	if(dbifc->fd < 0) {
		dbifc->fn = dbname;
		if( (i = opendb(dbifc)) < 0) {
			debug(1, "addfile: Can't opendb %s, got %d\n", dbname, i);
			t3comm->retval = CANTOPEN;
			t3comm->errnum = errno;
			t3comm->entry = -1;
			sendmsg(t3comm);
			return CANTOPEN;
		}
	}

	dbifc->entry = t3comm->entry;

	while(!feof(fp) && val > 0) {
		if(fgets(buf, sizeof(buf)-1, fp) == NULL)
			if(ferror(fp)) {
				val = -1;
				dbifc->errnum = errno;
				break;
			}
			else
				continue;

		lineno++;

		for(i = 0; buf[i]; i++)
			if(buf[i] == '\n')
				buf[i] = '\0';

		dbifc->size = strlen(buf);
		strcpy(dbifc->data, buf);
		dbifc->errnum = t3comm->errnum = 0;
	
		if( (val = addword(dbifc)) < 0) {
			debug(2, "Got %d on addword from addfile with word \"%s\" and value %d, errno %d\n",
				   val, dbifc->data, dbifc->entry, dbifc->errnum);
			break;
		}
	}
	fclose(fp);

	if(val >= 0) {
		val = 0;
		t3comm->entry = -1;
	} else
		t3comm->entry = lineno;

	t3comm->pos = dbifc->pos;
	t3comm->errnum = dbifc->errnum;
	t3comm->retval = dbifc->retval = val;
	sendmsg(t3comm);
	return(dbifc->retval);
}
	
/*   Return the integer corresponding to the highest log created thus far.
**/
static int mxent(struct t3comm *t3comm)
{
	t3comm->retval = dbifc->retval = maxent(dbifc);

	t3comm->size = 0;

	sendmsg(t3comm);

	return t3comm->retval;
}

/*  Quiesce the database, disallowing any update requests.
**/
static int quiesce(struct t3comm *t3comm)
{
	int     fd;
	char    *qfile;

	t3comm->retval = 0;

	t3comm->size = 0;

	if(quiesced == 0) {
		qfile = mymalloc(strlen(dbdir) + strlen(QNAME) + 1);
		strcpy(qfile, dbdir);
		strcat(qfile, QNAME);
		if( (fd = creat(qfile, 0)) <= 0) {
			debug("Can't create quiesce file %s: %s\n", qfile,
				sys_errlist[errno]);
			t3comm->retval = -1;
		} else {
			close(fd);
			debug(1, "Database quiesced\n");
		}
		myfree(qfile);
	}
	if(t3comm->retval == 0)
		quiesced++;

	sendmsg(t3comm);

	return t3comm->retval;
}

/*  Resume processing update requests.
**/
static int resume(struct t3comm *t3comm)
{
	char    *qfile;

	t3comm->retval = 0;

	t3comm->size = 0;

	if(--quiesced <= 0) {
		quiesced = 0;
		qfile = mymalloc(strlen(dbdir) + strlen(QNAME) + 1);
		strcpy(qfile, dbdir);
		strcat(qfile, QNAME);
		if(unlink(qfile) < 0) {
			debug("Can't remove quiesce file %s: %s\n", qfile,
				sys_errlist[errno]);
			t3comm->retval = -1;
		} else
			debug(1, "Database resumed\n");
		myfree(qfile);
	}

	sendmsg(t3comm);

	return t3comm->retval;
}

static int sendmsg(struct t3comm *t3comm)
{
	int     val;

	if( (val = msgsnd(t3comm->msgqid,
					  msgbuf,
					  sizeof(struct t3comm) + t3comm->size,
					  0)) != 0)
		debug(1, "Msgsnd to q 0x%08x in response to cmd %ld got %d, errno = %d\n",
			t3comm->msgqid, msgbuf->mtype, val, errno);

	return val;
}

/*   Resolve the passed fn to its true file name by doing away with ".."
** and "." entries.
**/
static char * resolve(char *fn)
{
   register char *p = fn,
				 *p1;
   char    *lastslash = NULL,
		   *cdir;

   if(!*p)
	   return NULL;

   if(*p == '/') {
	   cdir = NULL;
	   p = p1 = malloc(strlen(fn) + 1);
	   strcpy(p, fn);
   } else {
	   if( (cdir = getcwd((char *)NULL, 256)) == NULL)
		   return NULL;
	   p = malloc(strlen(cdir) + strlen(fn) + 2);
	   strcpy(p, cdir);
	   strcat(p, "/");
	   p1 = p + strlen(cdir);
	   free(cdir);
	   strcat(p, fn);
   }

   for( ;*p1; p1++) {
	   if(strncmp(p1, "/..", 3) == 0 && (*(p1+3) == '/' || *(p1+3) == '\0')) {
		   for(lastslash = p1-1; lastslash >= p && *lastslash != '/'; lastslash--)
			   ;
		   if(lastslash < p) {
			   free(p);
			   return NULL;
		   }
		   strcpy(lastslash, p1+3);
		   p1 = lastslash - 1;
		   continue;
	   }
	   if(strncmp(p1, "/.", 2) == 0 && (*(p1+2) == '/' || *(p1+2) == '\0')) {
		   strcpy(p1, p1+2);
		   p1--;
		   continue;
	   }
   }
   return(p);
}

char *
mymalloc(size)
int     size;
{
	char *p;

	p = malloc(size);

	debug(99, "Mymalloc: size 0x%08x, p = %08x\n",
          size, p);

	return(p);
}

myfree(p)
char    *p;
{
	debug(99, "Myfree: p = %08x\n", p);

	free(p);
}

static void chown(char *fn)
{
	char *temp;
	char *tolb = TOLB;
	int lentolb = strlen(tolb);

	temp = malloc(strlen(fn) + lentolb + 2);
	strcpy(temp, tolb);
	strcat(temp, " ");
	strcat(temp, fn);
	if(system(temp) != 0)
		errmsg(1, "Can't change owner of %s to logbook\n", fn);
	free(temp);
}
