SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBDIR=$(SRCDIR)/lib
INSDIR=../../../bin
CC= gcc

CFLAGS=-g
LDFLAGS=
HDRS=$(INCDIR)/t3db.h $(INCDIR)/t3dbfmt.h

checkdb: checkdb.c $(HDRS)
	$(CC) $(CFLAGS) -o checkdb -I$(INCDIR) checkdb.c $(LDFLAGS)

install: checkdb
	-cp checkdb $(INSDIR)

