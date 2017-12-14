# @(#)t3ifc.mk	1.2 - last delta: 8/12/88 15:28:48 gotten 8/12/88 15:28:49 from /usr/local/src/db/lib/.SCCS/s.t3ifc.mk.
# Make t3ifc.o

SHELL = /bin/sh
INS = /etc/install
BIN = /usr/local/lib/db

I = /usr/include
SRCDIR=../../db
INCDIR=$(SRCDIR)/include

CFLAGS=-g
LDFLAGS=-la
HDRS=$(INCDIR)/t3dbfmt.h $(INCDIR)/t3db.h $(INCDIR)/t3daemon.h $(INCDIR)/t3dberr.h

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
