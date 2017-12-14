SHELL = /bin/sh
I = /usr/include

SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBSDIR=$(SRCDIR)/lib

LIBDIR=$(LIBSDIR)
INSDIR=../../../bin

CFLAGS=-g
LDFLAGS=

t3seqdump: t3seqdump.c $(LIBDIR)/t3ifc.o \
		$(INCDIR)/t3dbfmt.h $(INCDIR)/t3db.h
	$(CC) $(CFLAGS) -o t3seqdump -I$(INCDIR) t3seqdump.c $(LIBDIR)/t3ifc.o $(LDFLAGS)

$(LIBDIR)/t3ifc.o: $(LIBSDIR)/t3ifc.c
	sh -c "cd $(LIBSDIR); make -f t3ifc.mk install"

install: t3seqdump
	cp t3seqdump $(HOME)/bin

clean:
clobber: clean
	-rm -f t3seqdump
