SHELL = /bin/sh
#I = /usr/include
CC = gcc
SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBSDIR=$(SRCDIR)/lib

LIBDIR=$(LIBSDIR)
INSDIR=../../../bin

CFLAGS=-g

quiescedb: quiescedb.c $(LIBDIR)/t3ifc.o \
		$(INCDIR)/t3dberr.h $(INCDIR)/t3db.h
	$(CC) $(CFLAGS) -o quiescedb -I$(INCDIR) quiescedb.c $(LIBDIR)/t3ifc.o $(LDFLAGS)

$(LIBDIR)/t3ifc.o: $(LIBSDIR)/t3ifc.c
	sh -c "cd $(LIBSDIR); make -f t3ifc.mk install"

install: quiescedb
	cp quiescedb $(INSDIR)
	chmod u+s $(INSDIR)/quiescedb

clean:
clobber: clean
	-rm -f quiescedb
