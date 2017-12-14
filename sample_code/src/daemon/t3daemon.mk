# %W% - last delta: %G% %U% gotten %H% %T% from %P%.
# Make the t3 daemon

SHELL = /bin/sh
I = /usr/include

SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
INSDIR=/usr/local/bin

#   Use the following to compile for UTS 2.0:
CFLAGS=-g -DUTS20 -eftu
#   Use the following to compile for UTS 1.2:
#CFLAGS=-g
LDFLAGS=
LIBS= 

OBJS=t3daemon.o t3access.o misc.o

TOLB = /usr/local/bin/tolb

t3daemon: $(OBJS) $(TOLB)
	$(CC) ${LDFLAGS} -o t3daemon $(OBJS) ${LIBS}

t3daemon.o: t3daemon.c \
	$(INCDIR)/t3db.h $(INCDIR)/t3dbfmt.h $(INCDIR)/t3daemon.h \
	$(INCDIR)/t3comm.h $(INCDIR)/t3dberr.h  \
	$I/stdio.h $I/sys/types.h $I/sys/ipc.h $I/sys/msg.h $I/signal.h \
	$I/sys/signal.h $I/sys/stat.h \
	$I/fcntl.h $I/errno.h $I/sys/errno.h 
	$(CC) ${CFLAGS} -I$(INCDIR) -c -DTOLB=\"$(TOLB)\" t3daemon.c

t3access.o: t3access.c \
	$(INCDIR)/t3db.h $(INCDIR)/t3dbfmt.h $(INCDIR)/t3dberr.h  \
	$I/stdio.h $I/fcntl.h $I/sys/types.h $I/sys/stat.h \
	$I/memory.h $I/string.h\
	$I/sys/ipc.h \
	$I/sys/msg.h $I/signal.h $I/sys/signal.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c t3access.c

install: t3daemon
	FORCE=yes /etc/install -k -u logbook -g logbook -f $(INSDIR) -m ug+s -o t3daemon

clean:
	-rm -f $(OBJS)
clobber: clean
	-rm -f t3daemon
