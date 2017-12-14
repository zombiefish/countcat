SHELL = /bin/sh
#I = /usr/include

SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBSDIR=$(SRCDIR)/lib

LIBDIR=$(LIBSDIR)
INSDIR=../../../bin

CFLAGS=-g
LDFLAGS=

t3redump: t3redump.c $(LIBDIR)/t3ifc.o \
		$(INCDIR)/t3db.h $(INCDIR)/t3dbfmt.h
	$(CC) $(CFLAGS) -o t3redump -I$(INCDIR) t3redump.c $(LIBDIR)/t3ifc.o $(LDFLAGS)

$(LIBDIR)/t3ifc.o: $(LIBSDIR)/t3ifc.c
	sh -c "cd $(LIBSDIR); make -f t3ifc.mk install"

install: t3redump
	cp t3redump $(INSDIR)

clean:
clobber: clean
	-rm -f t3redump
