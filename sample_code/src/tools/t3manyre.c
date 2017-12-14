#ident "@(#)t3manyre.c	1.9 3/13/92 17:16:20"

/*	vi:set tabstop=4:	*/

/* @(#)t3manyre.c	1.9 - last delta: 3/13/92 17:16:20 gotten 3/13/92 17:16:22 from /usr/local/src/db/src/tools/.SCCS/s.t3manyre.c. */

/*   Find a word in the database providing regular expression capability */

#include "t3db.h"
#include "t3daemon.h"
#include "t3dbfmt.h"
#include "t3dberr.h"
#include <memory.h>
#include <string.h>

#define AND 1
#define OR  2
#define NOT 3

#define EQ(x, y)	(strcmp(x, y)==0)

extern char *getenv(), *malloc(), *calloc(),/* *cmdname, */ *optarg;
char cmdname[14];

extern int      errno, sys_nerr, optind, opterr;
extern char     *sys_errlist[];

extern char *newinc();

void usage(),
	 errmsg(),
	 debug();

struct anode {
	int (*F)();
	struct anode *L, *R;
} Node[100];
int Nn;  /* number of nodes */
	 
int     Randlast = 0;
int     Complement = 0;
int     strikes = 0;

char   *nxtarg();

struct anode *exp(),
			 *e1(),
			 *e2(),
			 *e3(),
			 *mk();
	 
int verbose = 0,
    MaxEnt;                     /* Largest entry stored in the db       */

static char *dbname = NULL,
            *vector = NULL;

static int  veclen = 0;         /* Length of vector in bytes            */

char **Argv;
int  Argc, Ai;

main(ac, av)
int ac;
char **av;
{
	int     i;
	char    *fn = NULL,
			*infn = NULL,
			*expr = NULL,
			*cp;
	struct anode *exlist;

	strcpy(cmdname, "t3manyre");
	if(fn == NULL) {
		char    *p;

		if( (p = getenv("DBNAME")) == NULL)
			p = DEFDBNAME;

		fn = malloc(strlen(p) + 1);
		strcpy(fn, p);
	}

	if( (cp = getenv("DBDIR")) == NULL)
		cp = DEFDBDIR;
	dbname = malloc(strlen(fn) + strlen(cp) + 2);
	strcpy(dbname, cp);
	strcat(dbname, "/");
	strcat(dbname, fn);
	
    if( (i = opendb(dbname, O_RDWR)) < 0) {
		if(i == -1)
			errmsg(1, "Can't open database %s: %s\n",
				   dbname, sys_errlist[errno]);
		else
			errmsg(1, "Can't open database %s: %d\n",
				   dbname, i);
	}

	Argc = ac; Argv = av;
	Ai = 1;

	MaxEnt = maxent();

	if( (Argc = ac) > 1) {
		if(!(exlist = exp())) { /* parse and compile the arguments */
			errmsg(1, "Parsing error\n");
			exit(1);
		}

		if(Ai < Argc)
			errmsg(1, "Missing operator\n");
	
		if( (*exlist->F)(exlist, &vector, &veclen) )
			printmatch();
		else
			printf("No match\n");
	} else {
		int     tty = isatty(0);

		while(getline(tty) != EOF) {
			if(Argc <= 1)
				continue;
	
			Ai = 1;
			Randlast = 0;
			strikes = 0;
	
			if(!(exlist = exp()))   /* parse and compile the arguments */
				errmsg(0, "Parsing error\n");
	
			if(Ai < Argc)
				errmsg(0, "Missing operator\n");
	
			if( (*exlist->F)(exlist, &vector, &veclen) )
				printmatch();
			else
				printf("No match\n");
	
			for(i = 1; i < Argc; i++)
				free(Argv[i]);
	
			free(Argv);
	
			if(vector) {
				free(vector);
				vector = NULL;
				veclen = 0;
			}
		}
	}
}

int
getline(tty)
int     tty;
{
	int     c, i, j, wc = 0, inword = 0;
	char    buf[512];
	char    *cp = buf;

	if(feof(stdin))
		return EOF;

	if(tty)
		printf("Enter search arguments or EOF to terminate:\n");

    for(i = 0; i < 511; i++) {
		c = getchar();

		if(c == EOF || c == '\n') {
			if(inword)
				wc++;
			break;
		}

		buf[i] = c;

		if(c == '\b')
			buf[i--] = '\0';
		else if(c == ' ' || c == '\t') {
			if(!inword) {
				i--;
				continue;
			}
			buf[i] = '\0';
			inword = 0;
			wc++;
		} else
			inword = 1;
	}
	buf[i] = '\0';

	Argc = wc + 1;
	Argv = (char **)calloc(Argc, sizeof(char *));

	for(wc = 1, j = 0; j < i; j++, wc++) {
		int len = strlen(buf+j);

		Argv[wc] = malloc(len+1);
		strcpy(Argv[wc], buf+j);

		j += len;
	}
}

int
printmatch()
{
	int     val, i, j, bit;

	printf("Matched on ");

	for(val = 0, i = 0; i < veclen && val < MaxEnt; i++) {
		if(vector[i] == 0)
			continue;

		for(j = 0, bit = 0x80; bit; bit >>= 1, j++) {
			if((vector[i] & bit) == 0)
				continue;

			if( (val = (i * 8) + j) > MaxEnt)
				break;

			printf("%d ", val);
		}
	}
	putchar('\n');
}

int
fndre(p, vec, len)
register struct { int f; char *pat; } *p;
char    **vec;
int     *len;
{
	int     val, i, byte, biggest, entry;
	char    bit, *cp;
	struct dbrec *dbrec;
	struct dbrechdr *dbrech;
	struct dbrecbody *dbrecb;
	
	if( (val = findre(p->pat, &dbrec)) <= 0)
		return 0;

	biggest = 0;
	do {
		do {
			dbrech = &(dbrec->dbrech);
			dbrecb = (struct dbrecbody *)((char *)(&(dbrec->dbrecb)) +
					 dbrech->wordsz + 1);

			/*   Find the largest entry so we can allocate the vector */
			for(i = 0; i < dbrech->used_entries; i++)
				if(biggest < dbrecb->entry[i])
					biggest = dbrecb->entry[i];

			byte = (biggest >> 3) + 1;
			bit = biggest & 7;
			bit = 0x80 >> bit;

			newvec(vec, len, byte);
			cp = *vec;

			/*   Update the new vector with the entries */
			for(i = 0; i < dbrech->used_entries; i++) {
				entry = dbrecb->entry[i];
				byte = entry >> 3;
				bit = entry & 7;
				bit = 0x80 >> bit;
				cp[byte] |= bit;
			}
		} while( (val = nextsame(&dbrec)) > 0);
	} while( (val = nextre(&dbrec)) > 0);
	return 1;
}

int
newvec(ovector, osize, newsize)
char    **ovector;
int     *osize, newsize;
{
	char *vec;

	if(*osize >= newsize)
		return;

    vec = malloc(newsize);
	memset(vec, 0, newsize);
    if(*ovector != NULL && *osize > 0) {
        memcpy(vec, *ovector, *osize);
        free(*ovector);
    }
    *ovector = vec;
    *osize = newsize;
}

/* parse ALTERNATION (OR) */
struct anode *
exp()
{
	int or();
	register struct anode * p1;

	p1 = e1();
	if( EQ(nxtarg(), "|") ) {
		Randlast--;
		return(mk(or, p1, exp()));
	}
	else if(Ai <= Argc)
		--Ai;
	return(p1);
}

/* parse CONCATENATION (AND) */
struct anode *
e1()
{
	int and();
	register struct anode * p1;
	register char *a;

	p1 = e2();
	a = nxtarg();
	if( EQ(a, "&") ) {
And:
		Randlast--;
		return(mk(and, p1, e1()));
	} else if( EQ(a, "(") || EQ(a, "!" ) || (*a=='-' && !EQ(a, "|")) ) {
		--Ai;
/*      goto And; */
	} else if(Ai <= Argc)
		--Ai;
	return(p1);
}

/* parse NOT (!) */
struct anode *
e2()
{
	int     not();

	if(Randlast)
		errmsg(1, "Operand follows operand\n");

	Randlast++;

	if( EQ(nxtarg(), "!") )
		return(mk(not, e3(), (struct anode *)0));
	else if(Ai <= Argc)
		--Ai;
	return(e3());
}

/* parse parens and predicates */
struct anode *
e3()
{
	int     fndre();
	struct anode *p1;
	register char *a;

	a = nxtarg();
	if( EQ(a, "(") ) {
		Randlast--;
		p1 = exp();
		a = nxtarg();
		if( !EQ(a, ")") )
			goto err;
		return(p1);
	} else if( *a == '&' || *a == '!' || *a == '|')
		errmsg(1, "Operator follows operator\n");
	else
		return(mk(fndre, (struct anode *)a, (struct anode *)0));
err:
	errmsg(1, "Missing ')'\n");
}
struct anode *
mk(f, l, r)
int (*f)();
struct anode *l, *r;
{
	Node[Nn].F = f;
	Node[Nn].L = l;
	Node[Nn].R = r;
	return(&(Node[Nn++]));
}

/* get next arg from command line */
char *
nxtarg()
{

	if(strikes==3)
		errmsg(1, "Incomplete statement\n");
	if(Ai>=Argc) {
		strikes++;
		Ai = Argc + 1;
		return("");
	}
	return(Argv[Ai++]);
}

/* execution time functions */
and(p, vec, len)
register struct anode *p;
char    **vec;
int     *len;
{
	int     ret, veclena = 0, veclenb = 0;
	char    *veca = NULL, *vecb = NULL;

    if( (ret = (*p->L->F)(p->L, &veca, &veclena)) == 0)
		return 0;
    if( (ret *= (*p->R->F)(p->R, &vecb, &veclenb)) == 0) {
		free(veca);
		return 0;
	}
	ret *= doand(vec, len, veca, veclena, vecb, veclenb);
	return ret;
}

or(p, vec, len)
register struct anode *p;
char    **vec;
int     *len;
{
	int     ret, veclena = 0, veclenb = 0;
	char    *veca = NULL, *vecb = NULL;

    ret = (*p->L->F)(p->L, &veca, &veclena);
    ret += (*p->R->F)(p->R, &vecb, &veclenb);
	ret += door(vec, len, veca, veclena, vecb, veclenb);
    return ret;
}

not(p, vec, len)
register struct anode *p;
char    **vec;
int     *len;
{
	int     ret, veclena = 0;
	char    *veca = NULL;

    (*p->L->F)(p->L, &veca, &veclena);
    ret = donot(vec, len, veca, veclena);
    return ret;
}

int
doand(vec, len, veca, lena, vecb, lenb)
char    **vec, *veca, *vecb;
int     *len, lena, lenb;
{
	int     i,
			minlen = (lena < lenb) ? lena : lenb,
			maxlen = (lena > lenb) ? lena : lenb,
			ret = 0;
	char    *maxvec = (lena > lenb) ? veca : vecb;

	newvec(vec, len, maxlen);

	for(i = 0; i < minlen; i++)
		if( ((*vec)[i] = veca[i] & vecb[i]))
			ret = 1;

	if(Complement)
		for(i = minlen; i < maxlen; i++)
			if( ((*vec)[i] &= maxvec[i]) )
				ret = 1;

	free(veca);
	free(vecb);

	return ret;
}

int
door(vec, len, veca, lena, vecb, lenb)
char    **vec, *veca, *vecb;
int     *len, lena, lenb;
{
	int     i,
			maxlen = (lena > lenb) ? lena : lenb,
			minlen = (lena < lenb) ? lena : lenb,
			ret = 0;
	char    *maxvec = (lena > lenb) ? veca : vecb;

	newvec(vec, len, maxlen);

	for(i = 0; i < minlen; i++)
		if( ((*vec)[i] = veca[i] | vecb[i]))
			ret = 1;

	for(i = minlen; i < maxlen; i++)
		if( ((*vec)[i] = maxvec[i]) )
			ret = 1;

	free(veca);
	free(vecb);

	return ret;
}

int
donot(vec, len, veca, lena)
char    **vec, *veca;
int     *len, lena;
{
	int     i,
			ret = 0;

	newvec(vec, len, lena);

	for(i = 0; i < *len; i++)
		if( ((*vec)[i] = ~ veca[i]))
			ret = 1;

	Complement = 1;

	free(veca);

	return ret;
}

void
usage() {
	fprintf(stderr, "%s: Usage: %s [-d dbname] -e regexp\n",
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

	if(exit_code > 0)
		exit(exit_code);
}

/*   The debug routine takes a varargs-style parm list.  The first parm
** is the level at which this message should be produced and the remaining
** parms are printf-style args.  If the debug level is higher than the
** passed level no message is issued.
**/
/*VARARGS*/
void
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
	fprintf(stderr, "%s: ", cmdname);
	vfprintf(stderr, fmt, arg_ptr);
}
