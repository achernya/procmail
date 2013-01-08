/*$Id: procmail.h,v 2.19 1992/04/23 16:46:41 berg Rel $*/

#include "includes.h"
#include "exopen.h"
#include "strpbrk.h"

typedef unsigned char uschar;	     /* sometimes uchar is already typedef'd */
#ifdef uchar
#undef uchar
#endif
#define uchar uschar

#ifdef console
#define vconsole (verbose=1,console)
#else
#define vconsole devnull
#endif

#ifndef DEFsendmail
#define DEFsendmail SENDMAIL
#endif

#ifndef SYSTEM_MBOX
#define SYSTEM_MBOX	SYSTEM_MAILBOX
#endif

#ifdef sMAILBOX_SEPARATOR
#define smboxseparator(fd)	\
 (tofolder?rwrite(fd,sMAILBOX_SEPARATOR,STRLEN(sMAILBOX_SEPARATOR)):0)
#define emboxseparator(fd)	\
 (tofolder?rwrite(fd,eMAILBOX_SEPARATOR,STRLEN(eMAILBOX_SEPARATOR)):0)
#else
#define smboxseparator(fd)
#define emboxseparator(fd)
#endif

#ifndef KERNEL_LOCKS
#define fdlock(fd)	0
#define fdunlock()	0
#else
#ifndef SYS_FILE_H_MISSING
#include <sys/file.h>
#endif
#endif

#define XTRAlinebuf	2	     /* surplus of LINEBUF (see readparse()) */
#define TMNATE		'\377'		     /* terminator (see readoarse()) */

#define PRDO	poutfd[0]
#define PWRO	poutfd[1]
#define PRDI	pinfd[0]
#define PWRI	pinfd[1]
#define PRDB	pbackfd[0]
#define PWRB	pbackfd[1]
#define LENoffset	(TABWIDTH*LENtSTOP)
#define MAXfoldlen	(LENoffset-STRLEN(sfolder)-1)
#define MCDIRSEP	(dirsep+STRLEN(dirsep)-1)      /* most common DIRSEP */

#define lck_LOCKFILE	1	  /* crosscheck the order of this with msg[] */
#define lck_ALLOCLIB	2		      /* in sterminate() in retint.c */
#define lck_MEMORY	4
#define lck_FORK	8
#define lck_FILDES	16
#define lck_KERNELL	32

struct varval{const char*const name;long val;};
#define locksleep	(strenvvar[0].val)
#define locktimeout	(strenvvar[1].val)
#define suspendv	(strenvvar[2].val)
#define noresretry	(strenvvar[3].val)
#define timeoutv	(strenvvar[4].val)
#define MAXvarvals	maxindex(strenvvar)

#ifndef MAIN
extern char*buf,*buf2,*globlock,*loclock,*tolock,*lastfolder;
extern const char shellflags[],shell[],lockext[],newline[],binsh[],
 unexpeof[],shellmetas[],*const*gargv,*sgetcp,*rcfile,dirsep[],msgprefix[],
 devnull[],executing[],oquote[],cquote[],whilstwfor[],procmailn[],Mail[];
extern struct varval strenvvar[];
extern long lastdump;
extern sh,pwait,retval,retvl2,lcking,locknext,verbose,linebuf,rc,tofolder,
 tofile,ignwerr,fakedelivery;
extern volatile nextexit;
extern volatile time_t alrmtime;
extern pid_t thepid,pidchild;
#endif

#ifdef NOmemmove
void*smemmove();
#endif
#ifdef strtol
#undef strtol
#define NOstrtol
long strtol();
#endif

void*tmalloc(),*trealloc(),*bregcomp(),srequeue(),slose(),sbounce(),
 stermchild(),ftimeout();
pid_t sfork();
long dump(),pipin(),renvint();
char*readdyn(),*fromprog(),*cat(),*tstrdup(),*cstr(),*pstrspn(),
 *bregexec(),*egrepin();
const char*tgetenv(),*hostname();
int sgetc(),getb();

/*
 *	External variables that are checked/changed by the signal handlers:
 *	volatile time_t alrmtime;
 *	pid_t pidfilt,pidchild;
 *	volatile int nextexit;
 *	int lcking;
 *	static volatile mailread;	in procmail.c
 */
