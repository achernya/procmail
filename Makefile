#$Id: Makefile,v 2.3 1991/06/20 09:54:14 berg Rel $

# change BASENAME to your home directory if need be
BASENAME = /usr/local

BINDIR	 = $(BASENAME)/bin
MANSUFFIX= 1
MANDIR	 = $(BASENAME)/man/man$(MANSUFFIX)

########################################################################
# Only edit below this line if you *think* you know what you are doing #
########################################################################

# Directory for the standard include files
USRINCLUDE = /usr/include

OCFLAGS	 = -O
OLDFLAGS = -s

CFLAGS	= $(OCFLAGS) -I$(USRINCLUDE) -I./include
LDFLAGS = $(OLDFLAGS)

CC = cc
O = o
RM= rm -f

BINS=procmail lockfile formail

OBJ=procmail.$(O) nonint.$(O) goodies.$(O)

DEP=shell.h procmail.h config.h

all:	autoconf.h $(BINS)

procmail: $(OBJ) exopen.$(O) common.$(O) retint.$(O)
	$(CC) $(CFLAGS) -o procmail $(OBJ) exopen.$(O) common.$(O) \
retint.$(O) $(LDFLAGS)

lockfile: lockfile.$(O) exopen.$(O)
	$(CC) $(CFLAGS) -o lockfile lockfile.$(O) exopen.$(O) ${LDFLAGS}

formail: formail.$(O) common.$(O)
	$(CC) $(CFLAGS) -o formail formail.$(O) common.$(O) ${LDFLAGS}

_autotst: _autotst.$(O)
	$(CC) $(CFLAGS) -o _autotst _autotst.$(O) $(LDFLAGS)

autoconf.h: autoconf Makefile
	/bin/sh autoconf $(O)

Makefile:

$(OBJ): $(DEP)

retint.$(O): $(DEP) exopen.h

exopen.$(O): config.h includes.h exopen.h

formail.$(O): config.h includes.h shell.h

lockfile.$(O): config.h includes.h

common.$(O): includes.h shell.h

procmail.h: includes.h
	touch procmail.h

includes.h: autoconf.h
	touch includes.h

.c.$(O):
	$(CC) $(CFLAGS) -c $*.c

install: all
	chmod 0755 $(BINS)
	cp $(BINS) $(BINDIR)
	chmod 0644 man/procmail.$(MANSUFFIX) man/lockfile.$(MANSUFFIX) \
man/formail.$(MANSUFFIX)
	cp man/procmail.$(MANSUFFIX) man/lockfile.$(MANSUFFIX) \
man/formail.$(MANSUFFIX) $(MANDIR)

again: all

clean:
	$(RM) $(OBJ) common.$(O) lockfile.$(O) exopen.$(O) retint.$(O) \
formail.$(O) $(BINS) autoconf.h _autotst* grepfor
