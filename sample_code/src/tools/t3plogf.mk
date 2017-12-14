SHELL = /bin/sh
I = /usr/include

SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
INSDIR=../../../bin
LIBDIR=$(SRCDIR)/lib
LIBSDIR=$(SRCDIR)/lib

CFLAGS=-g
LDFLAGS=

t3plogf: t3plogf.c dname.o $(LIBDIR)/t3ifc.o \
		$(INCDIR)/t3db.h
	$(CC) $(CFLAGS) -I$(INCDIR) -o t3plogf t3plogf.c dname.o $(LIBDIR)/t3ifc.o $(LDFLAGS)

$(LIBDIR)/t3ifc.o: $(LIBSDIR)/t3ifc.c
	sh -c "cd $(LIBSDIR); make -f t3ifc.mk install"

dname.o: dname.c
	$(CC) $(CFLAGS) -o dname.o dname.c

install: t3plogf
	cp t3plogf $(INSDIR)

clean:
clobber: clean
	-rm -f t3plogf
