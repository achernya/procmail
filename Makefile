#$Id: Makefile,v 2.13 1992/01/31 11:32:45 berg Rel $

# change BASENAME to your home directory if need be
BASENAME = /usr/local

# You can predefine ARCHITECTURE to a bin directory suffix
BINDIR	 = $(BASENAME)/bin$(ARCHITECTURE)
MANSUFFIX= 1
MANDIR	 = $(BASENAME)/man/man$(MANSUFFIX)

# Things that can be made are:

# procmail formail lockfile		These are the three programs contained
#					in this package

# all			Makes all three binaries and the man pages
# install.man		Installs the man pages to $(MANDIR)
# install		Is a "make all" followed by copying all the binaries
#			and man pages to $(BINDIR) and $(MANDIR) respectively
# clean			Restores the package to pre-make state
# deinstall		Removes the previously installed binaries and man
#			pages by careful surgery

########################################################################
# Only edit below this line if you *think* you know what you are doing #
########################################################################

# Directory for the standard include files
USRINCLUDE = /usr/include

OCFLAGS = -O #-ansi -pedantic -Wid-clash-6
OLDFLAGS= -s

CFLAGS	= $(OCFLAGS) -I$(USRINCLUDE) -I./include #-D_POSIX_SOURCE
LDFLAGS = $(OLDFLAGS)

CC	= cc # gcc
MAKE	= make
SHELL	= /bin/sh
O	= o
RM	= rm -f

BINS=procmail lockfile formail

MANS=man/procmail.1 man/formail.1

MANSO=procmail.$(MANSUFFIX) formail.$(MANSUFFIX) lockfile.$(MANSUFFIX)

OBJ=nonint.$(O) goodies.$(O) regexp.$(O)

DEP=shell.h procmail.h config.h

all:	autoconf.h $(BINS) $(MANS)

procmail: procmail.$(O) $(OBJ) exopen.$(O) common.$(O) retint.$(O)
	$(CC) $(CFLAGS) -o procmail procmail.$(O) $(OBJ) exopen.$(O) \
common.$(O) retint.$(O) $(LDFLAGS)

lockfile: lockfile.$(O) exopen.$(O)
	$(CC) $(CFLAGS) -o lockfile lockfile.$(O) exopen.$(O) ${LDFLAGS}

formail: formail.$(O) common.$(O)
	$(CC) $(CFLAGS) -o formail formail.$(O) common.$(O) ${LDFLAGS}

_autotst: _autotst.$(O)
	$(CC) $(CFLAGS) -o _autotst _autotst.$(O) $(LDFLAGS)

autoconf.h: autoconf Makefile
	/bin/sh autoconf $(O) $(MAKE) autoconf.h

Makefile:

$(OBJ): $(DEP)

retint.$(O): $(DEP) exopen.h

procmail.$(O): $(DEP) patchlevel.h

exopen.$(O): config.h includes.h exopen.h

formail.$(O): config.h includes.h shell.h

lockfile.$(O): config.h includes.h

common.$(O): includes.h shell.h

procmail.h: includes.h
	touch procmail.h

includes.h: autoconf.h
	touch includes.h

.c.$(O):
	$(CC) -c $(CFLAGS) $*.c

man/man.sed: man/manconf.c config.h procmail.h
	$(CC) $(CFLAGS) -o man/manconf man/manconf.c ${LDFLAGS}
	man/manconf >man/man.sed
	rm -f man/manconf

man/procmail.1: man/man.sed man/procmail.man man/mansed
	/bin/sh man/mansed man/procmail.man man/procmail.1

man/formail.1: man/man.sed man/formail.man man/mansed
	/bin/sh man/mansed man/formail.man man/formail.1

install.man: $(MANS)
	chmod 0644 man/*.1
	cp man/procmail.1 $(MANDIR)/procmail.$(MANSUFFIX)
	cp man/lockfile.1 $(MANDIR)/lockfile.$(MANSUFFIX)
	cp man/formail.1 $(MANDIR)/formail.$(MANSUFFIX)

install: all install.man
	@chmod 0755 $(BINS)
	@cp $(BINS) $(BINDIR) || {( cd $(BINDIR); rm -f $(BINS); );\
cp $(BINS) $(BINDIR); }
	@echo
	@cd $(BINDIR); echo Installed in $(BINDIR); ls -l $(BINS)
	@cd $(MANDIR); echo Installed in $(MANDIR); ls -l $(MANSO)
	@echo ----------------------------------------------------------------\
---------------
	@echo If you are a system administrator, you should consider \
installing procmail
	@echo suid-root and/or integrating procmail into the mail-delivery \
system -- for
	@echo advanced functionality --. " " For more information about this \
topic you should
	@echo look in the examples/advanced file.
	@echo ----------------------------------------------------------------\
---------------

deinstall:
	cd $(BINDIR); $(RM) $(BINS); ls -l $(BINS)
	cd $(MANDIR); $(RM) $(MANSO); ls -l $(MANSO)

clean:
	$(RM) $(OBJ) common.$(O) lockfile.$(O) exopen.$(O) retint.$(O) \
formail.$(O) procmail.$(O) $(BINS) autoconf.h _autotst* lookfor grepfor \
$(MANS) man/man.sed
