# @(#)checkdb.mk	1.2 - last delta: 11/9/90 09:53:32 gotten 11/9/90 09:53:33 from /usr/local/src/db/src/tools/.SCCS/s.checkdb.mk.
# Make getall

SRCDIR=../../../db
INCDIR=$(SRCDIR)/include
LIBDIR=$(SRCDIR)/lib
INSDIR=/export/home/hubble/logbook/bin

CFLAGS=-g -K
LDFLAGS=-la
HDRS=$(INCDIR)/t3db.h $(INCDIR)/t3dbfmt.h

all:
	pwd
	for i in *.mk; do \
		make -f $$i; \
	done
clean:
	rm *.o
clobber:
	rm -f checkdb dumpdb formatdb quiescedb resumedb
	rm -f t3manyre t3plogf t3redump t3seqdump
