#$Id: Makefile,v 2.0 1991/06/10 14:39:08 berg Rel $

# change BASENAME to your home directory if need be
BASENAME = /usr/local

BINDIR	 = $(BASENAME)/bin
MANDIR	 = $(BASENAME)/man/man1

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

_autotst: _autotst.c
	$(CC) $(CFLAGS) -o _autotst _autotst.c $(LDFLAGS)

autoconf.h: autoconf Makefile
	/bin/sh autoconf

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
	chmod 755 $(BINS)
	cp $(BINS) $(BINDIR)
	chmod 644 man/procmail.1 man/lockfile.1 man/formail.1
	cp man/procmail.1 man/lockfile.1 man/formail.1 $(MANDIR)

clean:
	$(RM) $(OBJ) common.$(O) lockfile.$(O) exopen.$(O) retint.$(O) \
formail.$(O) $(BINS) autoconf.h _autotst*
