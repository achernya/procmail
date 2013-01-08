#$Id: Makefile,v 2.21 1992/04/23 16:46:41 berg Rel $

# change BASENAME to your home directory if need be
BASENAME = /usr/local

# You can predefine ARCHITECTURE to a bin directory suffix
#ARCHITECTURE=.sun4

BINDIR	  = $(BASENAME)/bin$(ARCHITECTURE)
MANDIR	  = $(BASENAME)/man
MAN1SUFFIX= 1
MAN5SUFFIX= 5
MAN1DIR	  = $(MANDIR)/man$(MAN1SUFFIX)
MAN5DIR	  = $(MANDIR)/man$(MAN5SUFFIX)

# Things that can be made are:

# procmail formail lockfile		These are the three programs contained
#					in this package

# all			Makes all three binaries and the man pages
# install.man		Installs the man pages to $(MAN1DIR) and $(MAN5DIR)
# install		Is a "make all" followed by copying all the binaries
#			and man pages to $(BINDIR), $(MAN1DIR) and $(MAN5DIR)
#			respectively
# recommend		Show some recommended suid/sgid modes
# suid			Install the by-'make recommend'-shown modes
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

CFLAGS	= $(OCFLAGS) #-D_POSIX_SOURCE
LDFLAGS = $(OLDFLAGS) #-lcposix

CC	= cc # gcc
MAKE	= make
SHELL	= /bin/sh
O	= o
RM	= /bin/rm -f
INSTALL = cp
DEVNULL = /dev/null

BINS=procmail lockfile formail

MANS=man/procmail.1 man/procmailrc.5 man/procmailex.5 man/formail.1 \
	man/lockfile.1

MANS1=procmail.$(MAN1SUFFIX) formail.$(MAN1SUFFIX) lockfile.$(MAN1SUFFIX)
MANS5=procmailrc.$(MAN5SUFFIX) procmailex.$(MAN5SUFFIX)

OBJ=nonint.$(O) goodies.$(O) regexp.$(O)

DEP=shell.h procmail.h config.h

all:	everything recommend

everything: autoconf.h $(BINS) $(MANS)

procmail: procmail.$(O) $(OBJ) exopen.$(O) common.$(O) retint.$(O) strpbrk.$(O)
	$(CC) $(CFLAGS) -o procmail procmail.$(O) $(OBJ) exopen.$(O) \
	 common.$(O) retint.$(O) strpbrk.$(O) $(LDFLAGS)

lockfile: lockfile.$(O) exopen.$(O) strpbrk.$(O)
	$(CC) $(CFLAGS) -o lockfile lockfile.$(O) exopen.$(O) strpbrk.$(O) \
	 ${LDFLAGS}

formail: formail.$(O) common.$(O) strpbrk.$(O)
	$(CC) $(CFLAGS) -o formail formail.$(O) common.$(O) strpbrk.$(O) \
	 ${LDFLAGS}

_autotst: _autotst.$(O)
	$(CC) $(CFLAGS) -o _autotst _autotst.$(O) $(LDFLAGS)

autoconf.h: autoconf Makefile
	$(SHELL) ./autoconf $(O) "$(MAKE)" autoconf.h "$(SHELL)" "$(RM)" \
$(USRINCLUDE)

Makefile: Manifest

Manifest: config.h
	@touch Manifest
	@-if fgrep -n -e '`' config.h $(DEVNULL) | fgrep -v -e EOFName ; then \
	 echo;echo '	^^^^^^^^^^^^^^^^^^^^ WARNING ^^^^^^^^^^^^^^^^^^^^^';\
	      echo '	* Having backquotes in there could be unhealthy! *';\
	 echo;fi;exit 0

$(OBJ): $(DEP)

retint.$(O): $(DEP)

procmail.$(O): $(DEP) patchlevel.h

exopen.$(O): config.h includes.h exopen.h strpbrk.h

formail.$(O): config.h includes.h shell.h strpbrk.h

lockfile.$(O): config.h includes.h exopen.h strpbrk.h

common.$(O): includes.h shell.h

recommend.$(O): config.h includes.h strpbrk.h
	@$(CC) -c $(CFLAGS) recommend.c

strpbrk.$(O): strpbrk.h includes.h

procmail.h: includes.h exopen.h strpbrk.h
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
	$(SHELL) man/mansed man/procmail.man man/procmail.1 $(SHELL)

man/procmailrc.5: man/man.sed man/procmailrc.man man/mansed
	$(SHELL) man/mansed man/procmailrc.man man/procmailrc.5 $(SHELL)

man/procmailex.5: man/man.sed man/procmailex.man man/mansed
	$(SHELL) man/mansed man/procmailex.man man/procmailex.5 $(SHELL)

man/formail.1: man/man.sed man/formail.man man/mansed
	$(SHELL) man/mansed man/formail.man man/formail.1 $(SHELL)

man/lockfile.1: man/man.sed man/lockfile.man man/mansed
	$(SHELL) man/mansed man/lockfile.man man/lockfile.1 $(SHELL)

recommend: recommend.$(O) strpbrk.$(O)
	@echo ----------------------------------------------------------------\
---------------
	@echo If you are a system administrator you should consider \
integrating procmail
	@echo into the mail-delivery system -- for advanced functionality \
AND SECURITY --.
	@echo For more information about this topic you should look in the \
examples/advanced
	@echo file.
	@echo
	@echo "Also, HIGLY RECOMMENDED (type 'make suid' to execute it):"
	@echo
	@$(CC) $(CFLAGS) -o _autotst recommend.$(O) strpbrk.$(O) ${LDFLAGS}
	@./_autotst $(BINDIR)/procmail $(BINDIR)/lockfile >suid.sh
	@./_autotst $(BINDIR)/procmail $(BINDIR)/lockfile
	@$(RM) _autotst
	@echo ----------------------------------------------------------------\
---------------

suid.sh: recommend

suid:	suid.sh install.bin
	@cat suid.sh
	@$(SHELL) ./suid.sh
	@cd $(BINDIR); echo Installed in $(BINDIR); ls -l $(BINS)

install.man: $(MANS)
	@-mkdir $(MANDIR) 2>$(DEVNULL); exit 0
	@-mkdir $(MAN1DIR) 2>$(DEVNULL); exit 0
	@-mkdir $(MAN5DIR) 2>$(DEVNULL); exit 0
	@chmod 0644 man/*.1 man/*.5
	$(INSTALL) man/procmail.1 $(MAN1DIR)/procmail.$(MAN1SUFFIX)
	$(INSTALL) man/procmailrc.5 $(MAN5DIR)/procmailrc.$(MAN5SUFFIX)
	$(INSTALL) man/procmailex.5 $(MAN5DIR)/procmailex.$(MAN5SUFFIX)
	$(INSTALL) man/lockfile.1 $(MAN1DIR)/lockfile.$(MAN1SUFFIX)
	$(INSTALL) man/formail.1 $(MAN1DIR)/formail.$(MAN1SUFFIX)

install.bin: everything
	@-mkdir $(BINDIR) 2>$(DEVNULL); exit 0
	@chmod 0755 $(BINS)
	$(INSTALL) $(BINS) $(BINDIR)

install: everything install.man install.bin
	@echo
	@cd $(BINDIR); echo Installed in $(BINDIR); ls -l $(BINS)
	@cd $(MAN1DIR); echo Installed in $(MAN1DIR); ls -l $(MANS1)
	@cd $(MAN5DIR); echo Installed in $(MAN5DIR); ls -l $(MANS5)
	@$(MAKE) recommend

deinstall:
	cd $(BINDIR); $(RM) $(BINS); ls -l $(BINS)
	cd $(MAN1DIR); $(RM) $(MANS1); ls -l $(MANS1)
	cd $(MAN5DIR); $(RM) $(MANS5); ls -l $(MANS5)

clean:
	$(RM) $(OBJ) common.$(O) lockfile.$(O) exopen.$(O) retint.$(O) \
	 strpbrk.$(O) formail.$(O) procmail.$(O) $(BINS) autoconf.h _autotst* \
	 lookfor grepfor $(MANS) man/man.sed recommend.$(O) suid.sh
