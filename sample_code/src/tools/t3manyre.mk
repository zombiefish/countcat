SHELL = /bin/sh
#I = /usr/include

SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBSDIR=$(SRCDIR)/lib

LIBDIR=$(LIBSDIR)
INSDIR=../../../bin

CFLAGS=-g
LDFLAGS=

t3manyre: t3manyre.c $(LIBDIR)/t3ifc.o \
		$(INCDIR)/t3db.h $(INCDIR)/t3dbfmt.h $(INCDIR)/t3dberr.h \
		$(INCDIR)/t3daemon.h
	$(CC) $(CFLAGS) -I$(INCDIR) -o t3manyre t3manyre.c $(LIBDIR)/t3ifc.o $(LDFLAGS)

$(LIBDIR)/t3ifc.o: $(LIBSDIR)/t3ifc.c
	$(SHELL) -c "cd $(LIBSDIR); make -f t3ifc.mk install"

install: t3manyre
	cp t3manyre $(INSDIR)

clean:
clobber: clean
	-rm -f t3manyre
