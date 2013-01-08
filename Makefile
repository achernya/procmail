# change BASENAME to your home directory if need be
BASENAME = /global

BINDIR	 = ${BASENAME}/bin
MANDIR	 = ${BASENAME}/man/man1

OCFLAGS	 = -O
OLDFLAGS = -s

# Uncomment appropiately (for sysV and the like):
CFLAGS	= ${OCFLAGS} #-DSYSV
LDFLAGS = ${OLDFLAGS} #-lbsd 

CC = cc
INSTALL= mv
RM= rm -f

OBJ=procmail.o nonint.o retint.o

all:	procmail lockfile

procmail: ${OBJ}
	${CC} ${CFLAGS} -o procmail ${OBJ} ${LDFLAGS}

lockfile: lockfile.o
	${CC} ${CFLAGS} -o lockfile lockfile.o ${LDFLAGS}

.c.o:
	${CC} ${CFLAGS} -c $*.c

install: all
	chmod 755 procmail lockfile
	${INSTALL} procmail lockfile ${BINDIR}
	chmod 644 procmail.1 lockfile.1
	cp procmail.1 lockfile.1 ${MANDIR}

clean:
	${RM} ${OBJ} lockfile.o procmail lockfile
