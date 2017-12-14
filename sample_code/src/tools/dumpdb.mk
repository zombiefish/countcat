SHELL = /bin/sh
#I = /usr/include

SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBSDIR=$(SRCDIR)/lib

INSDIR=../../../bin
LIBDIR=$(LIBSDIR)

CFLAGS=-g
LDFLAGS=

dumpdb: dumpdb.c $(LIBDIR)/t3ifc.o \
		$(INCDIR)/t3db.h $(INCDIR)/t3dbfmt.h
	$(CC) $(CFLAGS) -I$(INCDIR) -o dumpdb dumpdb.c $(LIBDIR)/t3ifc.o $(LDFLAGS)

$(LIBDIR)/t3ifc.o: $(LIBSDIR)/t3ifc.c
	sh -c "cd $(LIBSDIR); make -f t3ifc.mk install"

install: dumpdb
	cp dumpdb $(INSDIR)
	chmod u+s $(INSDIR)/dumpdb

clean:
clobber: clean
	-rm -f dumpdb
