BASENAME = /global              # change to your home directory if need be
BINDIR   = ${BASENAME}/bin
MANDIR   = ${BASENAME}/man/man1

OCFLAGS  = -O
OLDFLAGS = -s

# Uncomment appropiately (for sysV and the like):
CFLAGS  = ${OCFLAGS} #-DSYSV -DNOfsync -DNO_ANSI_PROT -Dvoid=char
LDFLAGS = ${OLDFLAGS} #-lbsd 

CC = cc
INSTALL= install -m 755
RM= rm -f

all:    procmail lockfile

procmail: procmail.o
	${CC} ${CFLAGS} -o procmail procmail.o ${LDFLAGS}

procmail.o: procmail.c
	${CC} ${CFLAGS} -c procmail.c

lockfile: lockfile.o
	${CC} ${CFLAGS} -o lockfile lockfile.o ${LDFLAGS}

lockfile.o: lockfile.c
	${CC} ${CFLAGS} -c lockfile.c

install: all
	${INSTALL} procmail lockfile ${BINDIR}
	chmod 644 procmail.1 lockfile.1
	cp procmail.1 lockfile.1 ${MANDIR}

clean:
	${RM} procmail.o lockfile.o procmail lockfile
