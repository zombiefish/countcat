SHELL = /bin/sh
#I = /usr/include
CC = gcc
SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBSDIR=$(SRCDIR)/lib

LIBDIR=$(LIBSDIR)
INSDIR=../../../bin

CFLAGS=-g

resumedb: resumedb.c $(LIBDIR)/t3ifc.o \
		$(INCDIR)/t3dberr.h $(INCDIR)/t3db.h
	$(CC) $(CFLAGS) -o resumedb -I$(INCDIR) resumedb.c $(LIBDIR)/t3ifc.o $(LDFLAGS)

$(LIBDIR)/t3ifc.o: $(LIBSDIR)/t3ifc.c
	sh -c "cd $(LIBSDIR); make -f t3ifc.mk install"

install: resumedb
	cp resumedb $(INSDIR)
	chmod u+s $(INSDIR)/resumedb

clean:
clobber: clean
	-rm -f resumedb
