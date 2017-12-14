#ident "@(#)logbook.h	1.19 1/29/92 23:27:42"

/* @(#)logbook.h	1.19 - last delta: 1/29/92 23:27:42 gotten 1/29/92 23:27:50 from /usr/local/src/logbook/.SCCS/s.logbook.h. */

/* define NOEXTERN to eliminate the extern definitions */

#define null ((void *)0)

#define MAINpanel 0
#define AEpanel   1
#define ICpanel   2
#define SELpanel  3
#define Qpanel    4
#define Vpanel    5
#define CKpanel   6

#define MAXCMD  22          /* Number of commands - 1 (UO, LR, LS, etc) */
#define NO_CMD  0
#define BAD_CMD 1
#define UO     0x00000010
#define LR     0x00000020
#define LS     0x00000040
#define LD     0x00000080
#define LC     0x00000100
#define HE     0x00000200
#define FC     0x00000400
#define IC     0x00000800
#define EN     0x00001000
#define LF     0x00002000
#define IX     0x00004000
#define CP     0x00008000
#define OTJ    0x00010000
#define OTL    0x00020000
#define EOL    0x00040000
#define OFF    0x00080000
#define PGUP   0x00100000
#define PGDN   0x00200000
#define QS     0x00400000
#define CK     0x00800000
#define CANCEL 0x01000000
#define MS     0x02000000
#define DOC    0x04000000

#define VIEWMAX 2000

#define RC_HDR_LEN   34
#define RESCT      20
#define RESCHARS 1024
#define U_DAT_FN  "/.logudat"
#define NED "/usr/openwin/bin/xterm"
#define RONED "/usr/openwin/bin/xterm"
#define ROLESS "/usr/local/gpl/bin/less"
#define VI "vi"
#define XTERM "xterm"
#define XTERMCMD "-e"
struct user_data_struct
	{
	int  magic;
	int  ReservedInt[RESCT];
	short user_flags;
	short ReservedShort[RESCT];
	char product[50];
	char dbname[100];
	char ReservedChar[RESCHARS];
	};
#define MAGIC      0x19520420
#define WRITE_LOG  1      /* off=rdonly */
#define PCC        2      /* off=field */
#define SPY        4
#define ONEPROD    8

struct cmd_tbl_struct {
	int value;
	int len;
	char *name; };

struct sel_struct {
	char **ptr;
	char *title;
	int  num; };
	
struct in_s
{
	int     ccol,
			crow,
			hot;
	char    cdate[9],
			ctime[6],
			udate[9],
			utime[6],
			sev[2],
			workq[7],
			rptdby[21],
			prno[11],
			status[3],
			joined[2],
			comp[80],
			prob[80],
			fix[80],
			emsg[80];
};

struct p1_s
{
	char	ddate[11],
	dtime[9],
	cmd [41],
	prod[15],
	emsg[80],
	info[80],
	inc_no[20],
	log_no[20];
};

#ifndef NOEXTERN

extern char rc_hdr[];
extern struct user_data_struct user_data;

extern int DEBUG;
extern FILE *debug;

extern char *home;

extern char savedlogid[80];
extern int savedlog;

extern char cmd_text[80];
extern int cur_panel,cmd_no,prev_panel;

extern struct cmd_tbl_struct cmdtbl[];
extern struct sel_struct SEL;

extern char dbname[];
extern char *prod_tab[500];
extern char *prod_db[500];
extern int pt_count;

extern char *pfkey[25];
extern char *pfk_list[3];

extern char *cw[];
extern int cw_count;

extern struct in_s in;

extern struct p1_s p1;

extern char loginwork[256];

#define VIEWMAX 2000
extern char *viewport[];
extern char *viewbuf;
extern int view,viewmax,viewportsize;

#endif

void debugmsg();
void spy();
void allspy();
char	*panel_name(int v);
void	set_panel(int p);
char	*panel_sufx(int v);
void	fixcmd(char *c, int q, int ok);
char	*cmd_name(int v);
void	pop_panel();
void	IC_time(char *dp, char *tp);
