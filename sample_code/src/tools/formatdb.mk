SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBDIR=$(SRCDIR)/lib

IGNSRC = ../daemon/IGNORE

INSDIR = ../../../bin

CFLAGS=-g
LDFLAGS=
HDRS=$(INCDIR)/t3dbfmt.h $(INCDIR)/t3db.h $(INCDIR)/t3daemon.h

dname.o: dname.c
	$(CC) $(CFLAGS) -c -o dname.o -I$(INCDIR) dname.c

formatdb: formatdb.c $(HDRS)
	$(CC) $(CFLAGS) -o formatdb -I$(INCDIR) -DIGNSRC=\"$(IGNSRC)\" formatdb.c dname.o $(LDFLAGS)


install: formatdb
	cp formatdb $(INSDIR)
	chmod u+s $(INSDIR)/formatdb

clean:
	-rm -f formatdb.o
	-rm -f dname.o

clobber: clean
	-rm -f formatdb
