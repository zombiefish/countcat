#include "t3db.h"
#include "t3dbfmt.h"
#include "t3daemon.h"
#include "t3comm.h"
#include "t3dberr.h"
#include <time.h>

extern char *sys_errlist[];

main(argc, argv)
int argc;
char **argv;
{

    char    *dbdir="/uts/vpputs/usr/logbook/logs5/B-EPM5b";
    key_t   msgkey;
    int    	retmsgqid; 
    msgkey =  *(char *)msgkey;		/* create core dump */
    if( (msgkey = ftok(dbdir, '2')) == (key_t)-1)
           printf("Can't create a message key based on %s error %s\n", dbdir, sys_errlist[errno]);

    printf("msgkey for %s is 0x%08x\n",dbdir, msgkey);

    return 0;
    
}
						    
