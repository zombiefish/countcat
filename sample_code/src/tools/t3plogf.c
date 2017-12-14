#ident "@(#)t3plogf.c	1.12 7/7/92 16:40:37"
  
/* @(#)t3plogf.c	1.12 - last delta: 7/7/92 16:40:37 gotten 7/7/92 16:40:38 from /usr/local/src/db/src/tools/.SCCS/s.t3plogf.c. */
 
/*	vi:set tabstop=4:	*/

/*   Process a logfile (add its words to a t3 database) using the t3
** daemon.
**/

#include "t3db.h"
#include "t3dberr.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <memory.h>
#include <string.h>
#include <varargs.h>

extern char *getenv(),
            *malloc(),
			*getcwd(),
            *tmpnam(),
			*sname(),
			*dname(),
			*optarg;

char  cmdname[14];

extern int      errno, sys_nerr, optind, opterr;
extern char     *sys_errlist[];

static void usage(),
			errmsg(),
			debug();
static int proc_word(char *start);
static int ignore_this_word(char *word);
static int save_word(char *word);
 
static verbose = 0;

static char *dbname,
            *ignfile;


struct wordlist {
	struct wordlist *next;
	char    word[1];
} *ignore_words = NULL, *good_words = NULL;

static FILE *errfp = stderr;

static char badchars[255];

int main(int ac, char **av)
{
	int     i, j, len, val, logno;
	char    buf[512], *cp, *ignfn, *infn, *dbdir, *efn = NULL;
	FILE    *ignfp;
	struct wordlist *nign, *oign;

	strcpy(cmdname, "t3plogf");
	dbname = NULL;

	while( (i = getopt(ac, av, "vl:e:d:")) != EOF) {
		switch(i) {
		case 'v':
			verbose++;
			break;
		case 'l':
			for(cp = optarg; *cp; cp++)
				if(*cp >= '0' && *cp <= '9')
					continue;
				else
					usage();
			logno = atoi(optarg);
		    break;
		case 'd':
			dbname = malloc(strlen(optarg)+1);
			strcpy(dbname, optarg);
			dbdir = malloc(strlen(optarg) + 1);
			strcpy(dbdir, optarg);
			dbdir = dname(dbdir);
		    break;
		case 'e':
			efn = malloc(strlen(optarg)+1);
			strcpy(efn, optarg);
		    break;
		case '?':
		    usage();
		}
	}
	if(ac != (optind + 1))
		usage();

	/*   Save the input filename */
	infn = av[optind];

	/*   If we have an error-file name then try to get to it */
	if(efn) {
        if( (errfp = fopen(efn, "w+")) == NULL) {
			errfp = stderr;
			errmsg(1, "Can't create %s: %s\n", cmdname, efn, sys_errlist[errno]);
		}
	} else
		errfp = stderr;

	/*   If we have a dbname then use it */
	if(dbname == NULL) {
		char    *p;

		if( (dbdir = getenv("DBDIR")) == NULL)
			dbdir = DEFDBDIR;

		if( (p = getenv("DBNAME")) == NULL)
			p = DEFDBNAME;
	
		len = strlen(dbdir) + strlen(p) + 2;
		dbname = malloc(len);
		strcpy(dbname, dbdir);
		strcat(dbname, "/");
		strcat(dbname, p);
	}

	/*   Get the dbdir, open the database then create the name of the
	** ignore file */
    if( (val = opendb(dbname, O_RDWR)) < 0)
		if(val == -1)
			errmsg(1, "Can't opendb(%s): %s\n", dbname, sys_errlist[errno]);
		else
			errmsg(1, "Got %d on opendb(%s)\n", val, dbname);

	len = strlen(dbdir) + strlen("/IGNORE") + 1;
	ignfn = malloc(len);
	strcpy(ignfn, dbdir);
	strcat(ignfn, "/IGNORE");
	
	/*   Read in the ignore file */
	if( (ignfp = fopen(ignfn, "r")) != NULL) {
		oign = ignore_words = (struct wordlist *)malloc(sizeof(struct wordlist) + 1);
		oign->next = NULL; oign->word[0] = '\0';
		while(!feof(ignfp)) {
			if(fgets(buf, sizeof(buf), ignfp) == NULL)
				if(ferror(ignfp))
					break;
				else
					continue;
			len = strlen(buf) - 1;
			buf[len] = '\0';
			nign = (struct wordlist *)malloc(sizeof(struct wordlist) + len + 1);
			oign->next = nign;
			strcpy(nign->word, buf);
			nign->next = 0;
			oign = nign;
		}
	}
	fclose(ignfp);
	free(ignfn);

	/*   Initialize badchars.  The only non-bad chars are A-Z, a-z and
	** 0-9.  For all other characters (0 excepted) set the element to the
	** character value. */
	for(j = 0, i = 1; i < sizeof(badchars); i++) {
		badchars[i] = '\0';
		if( (i >= 'A' && i <= 'Z')
		 || (i >= 'a' && i <= 'z')
		 || (i >= '0' && i <= '9') )
			;
		else
			badchars[j++] = i;
	}
		  
	process_log(infn, logno);
}

/*   Process a log file.  We do this by reading it, breaking each line
** into tokens, passing each token to proc_word() to break up and write
** to a temporary file, making all letters lowercase and removing and
** duplicate words.
**/
int process_log(char *fn, int entry)
{
	FILE    *fp, *outfp;
	char    buf[512],
			*start,
			outfn[L_tmpnam];
	int     lineno, inword, i, c, val;
	struct wordlist *wl;

	/*   Allocate and open the file from which addfile will read */
	tmpnam(outfn);
	if( (outfp = fopen(outfn, "w")) == NULL)
		errmsg(1, "Can't open %s to hold pre-processed words: %s\n",
			   outfn, sys_errlist[errno]);

	/*   Process the log file */
    if( (fp = fopen(fn, "r")) == NULL)
		errmsg(1, "Can't open %s for reading: %s\n", fn, sys_errlist[errno]);

	for(i = inword = 0, start = buf; !feof(fp); ) {
		if( (c = getc(fp)) == EOF)
			if(ferror(fp))
				errmsg(1, "Can't read from %s: %s\n",
					   fn, sys_errlist[errno]);
			else
				continue;
		if(c == '\b') {
			if(i > 0)
				i--;
			continue;
		}

		/*   Whitespace makes us process a word if we're in a word */
		if(c == ' ' || c == '\t' || c == '\n') {
			buf[i] = '\0';
			if(inword)
				proc_word(buf);
			i = inword = 0;
			continue;
		}

		/*   By now we now that we've made it into a word.  Save the char
		** after making it lowercase and set our flag */
		if(c >= 'A' && c <= 'Z')
			c |= 0x20;

		buf[i++] = c;

		inword = 1;
	}

	if( (wl = good_words) != NULL) {
		for( ; wl; wl = wl->next) {
			if(wl->word[0] == '\0')
				continue;
			fputs(wl->word, outfp);
			fputc( '\n', outfp);
		}
		fclose(outfp);
		if( (val = addfile(outfn, entry, &lineno)) == -1)
			errmsg(1, "Can't add words from %s: %s", outfn, sys_errlist[errno]);
		else if(val != 0)
			errmsg(1, "Got %d when trying to add words from %s, lineno: %d, errno: %d\n",
				   val, outfn, lineno, errno);
	} else
		fclose(outfp);

	unlink(outfn);
}

/*   Process a word.  Strip and "bad" characters from the beginning of the
** word and then write all words longer than 1 character that may be
** formed from the string.  If the word passed in is "/usr/src/foo.c"
** then we need to write out 9 words:
**      usr
**      usr/src
**      usr/src/foo
**      usr/src/foo.c
**      src
**      src/foo
**      src/foo.c
**      foo
**      foo.c
**   Note that we don't change the word that's passed in to us.  Instead
** we make a copy of our own that we can munge and place a pointer to it
** in mycopy.  We make another copy of the word here as well and place its
** pointer into cp1.  This is the copy that we'll really mess with by
** using it as an argument to strtok().
**/
static int proc_word(char *start)
{
	int     words = 0, len, mylen, i;
	char    *cp,
			*cp1,
			*mycopy,
			*nextstart = NULL;

	/*   Skip any leading characters that we don't recognize */
	start += strspn(start, badchars);

	/*   Allocate space for our copy of the word (sans leading trash) and
	** copy the word to it. */
	mycopy = malloc(mylen = strlen(start) + 1);
	memcpy(mycopy, start, mylen);

	if(strlen(mycopy) < 2) {
		free(mycopy);
		return;
	}

	/*   If this is a word that we're supposed to ignore then skip it */
	if(ignore_this_word(mycopy)) {
		free(mycopy);
		return;
	}

	/*   Make another copy of the word (sans leading trash) that strtok()
	** can munge. */
	cp1 = malloc(strlen(mycopy));
	strcpy(cp1, mycopy);

	/*   Skip through this word writing out each sub-word. */
	for(cp = cp1; (cp = strtok(cp, badchars)) != NULL; cp = NULL) {
		if(ignore_this_word(cp))
			continue;

		/*   If we've just processed the first word then remember where
		** the 2nd word starts so we can call ourself to process the
		** remaining words */
		if(words++ == 1)
			nextstart = mycopy + (cp - cp1);

		if(strlen(cp) < 2)
			continue;
	
		/*   Write out the sub-word */
		save_word(cp);
	}

	/*   If we have 2 or more words then call ourself to process the
	** subwords */
    if(words > 1)
		proc_word(nextstart);

	/*   Write out all leading words */
	cp = cp1;
	for(i = 0; i < (words-1); i++) {
		len = strlen(cp);
		*(cp + len) = *(start + len);
		if(strlen(cp) < 2 || ignore_this_word(cp))
			continue;
		save_word(cp);
	}

	save_word(mycopy);

	free(mycopy);
	free(cp1);
}

/*   Return 1 if this word SHOULD be ignored, else return 0. */
static int ignore_this_word(char *word)
{
	struct wordlist *ign;
	int     val;

    for(ign = ignore_words; ign; ign = ign->next)
		if( (val = strcmp(ign->word, word)) == 0)
			return 1;
		else if(val > 0)
			break;

	return 0;
}

/*   Save the passed word into the good_word wordlist */
static int save_word(char *word)
{
	struct wordlist *cgw, *ngw, *pgw;
	int     val;

	if( (pgw = good_words) == NULL) {
		good_words = (struct wordlist *)malloc((2 * sizeof(struct wordlist)) + strlen(word) + 2);
		pgw = good_words + 1;
		good_words->next = pgw;
		good_words->word[0] = '\0';
		pgw->next = NULL;
		strcpy(pgw->word, word);
		return;
	} else {
		for(cgw = pgw->next; cgw; pgw = cgw, cgw = pgw->next) {
			if( (val = strcmp(cgw->word, word)) == 0)
				return;
			else if(val < 0)
				continue;

			/*   We need to place the new word into the list */
			break;
		}
	}
	/*   If we've gotten here we're ready to place the new word after
	** pgw */
    ngw = (struct wordlist *)malloc(sizeof(struct wordlist) + strlen(word) + 1);
    ngw->next = cgw;
    pgw->next = ngw;
    strcpy(ngw->word, word);
}
			
static void usage()
{
	fprintf(errfp, "%s: Usage: %s [-v] [-d dbname] [-e errfile] -l logno file\n",
		cmdname, cmdname);
	exit(2);
}

/*   The errmsg routine takes a varargs-style parm list.  The first parm
** is the exit code and the remaining parms are printf-style args.
**/
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
	if(errfp == stderr)
		fprintf(errfp, "%s: ", cmdname);
	vfprintf(errfp, fmt, arg_ptr);

	if(exit_code > 0)
		exit(exit_code);
}

/*   The debug routine takes a varargs-style parm list.  The first parm
** is the level at which this message should be produced and the remaining
** parms are printf-style args.  If the debug level is higher than the
** passed level no message is issued.
**/
/*VARARGS*/
static void
debug(va_alist)
va_dcl
{
	int         level;
	char        *fmt;
	va_list     arg_ptr;
    
	if(verbose == 0)
		return;

	va_start(arg_ptr);
	level = va_arg( arg_ptr, int);
	if(level > verbose)
		return;

	fmt = va_arg(arg_ptr, char *);
	fprintf(errfp, "%s: ", cmdname);
	vfprintf(errfp, fmt, arg_ptr);
}

/*** Local Variables: ***/
/*** tab-stops:(4 8 12 16 20 24 28 32 36 40 44 48 52 56 60 64 68 72 80) ***/
/*** tab-width:4 ***/
/*** End: ***/
