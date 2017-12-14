h09864
s 00025/00004/00004
d D 1.2 88/08/12 15:28:48 acs 2 1
c Included install:, clean: and clobber: rules and included dependencies on /usr/include.
e
s 00008/00000/00000
d D 1.1 88/07/27 16:09:00 acs 1 0
c date and time created 88/07/27 16:09:00 by acs
e
u
U
t
T
I 2
# %W% - last delta: %G% %U% gotten %H% %T% from %P%.
# Make t3ifc.o

SHELL = /bin/sh
INS = /etc/install
BIN = /usr/local/lib/db

I = /usr/include
SRCDIR=/usr/local/src/db
INCDIR=$(SRCDIR)/include

E 2
I 1
CFLAGS=-g
LDFLAGS=-la
D 2
LIBDIR=../lib
INCDIR=../include
E 2
HDRS=$(INCDIR)/t3dbfmt.h $(INCDIR)/t3db.h $(INCDIR)/t3daemon.h $(INCDIR)/t3dberr.h

D 2
t3ifc.o: t3ifc.c $(HDRS) $(INCDIR)/t3comm.h
	$(CC) $(CFLAGS) -c t3ifc.c
E 2
I 2
t3ifc.o: t3ifc.c  $(INCDIR)/t3db.h $I/stdio.h \
		$I/sys/types.h $I/sys/ipc.h $I/sys/msg.h $I/signal.h \
		$I/sys/signal.h $(INCDIR)/t3comm.h \
		$(INCDIR)/t3dbfmt.h \
		$(INCDIR)/t3dberr.h $I/varargs.h 
	$(CC) $(CFLAGS) -I$(INCDIR) -c t3ifc.c

install: t3ifc.o
	rm -f $(BIN)/t3ifc.o
	/etc/install -m 0444 -f $(BIN) t3ifc.o

clean:
clobber: clean
	-rm -f t3ifc.o
E 2
E 1
