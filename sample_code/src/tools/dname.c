#ident "@(#)/usr/src/amdahl/lib/liba/str/dname.c	1.5.1.1 5/30/90 17:49:30 - (c) Copyright 1988, 1989, 1990 Amdahl Corporation"
/* */
#include <stdio.h>

#define SLASH           '/'
#define EOS             '\0'

static  char    dot[] = ".";

char *
dname(pathname)
char    *pathname;
{
	register char   *nullbyte = NULL;
	register char   *c = pathname;

	if (*c == SLASH)
		nullbyte = ++c;
	while (*c) {
		if (*c == SLASH)
			nullbyte = c;
		c++;
	}
	if (nullbyte == NULL)
		strcpy(pathname, dot);
	else
                *nullbyte = EOS;
	return(pathname);
}
