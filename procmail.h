/*$Id: procmail.h,v 2.13 1992/01/31 11:32:45 berg Rel $*/

#include "includes.h"

typedef unsigned char uschar;	     /* sometimes uchar is already typedef'd */
#ifdef uchar
#undef uchar
#endif
#define uchar uschar

#ifndef console
#define console devnull
#endif

#ifndef DEFsendmail
#define DEFsendmail SENDMAIL
#endif

#ifndef SYSTEM_MBOX
#define SYSTEM_MBOX	SYSTEM_MAILBOX
#endif

#ifdef MAILBOX_SEPARATOR
#define mboxseparator(fd)	\
 (tofolder?rwrite(fd,MAILBOX_SEPARATOR,STRLEN(MAILBOX_SEPARATOR)):0)
#else
#define mboxseparator(fd)
#endif

#ifndef KERNEL_LOCKS
#define lockfd(fd)	0
#define unlockfd()	0
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
extern sh,pwait,retval,lcking,locknext,verbose,linebuf,rc,tofolder,tofile,
 ignwerr,fakedelivery;
extern volatile nextexit;
extern volatile time_t alrmtime;
extern pid_t thepid;
#endif

#ifdef NOmemmove
void*smemmove();
#endif
#ifdef strtol
#undef strtol
#define NOstrtol
long strtol();
#endif
#ifdef NOstrpbrk
char*strpbrk();
#endif

void*tmalloc(),*trealloc(),*bregcomp(),srequeue(),slose(),sbounce(),
 stermchild(),ftimeout();
pid_t sfork();
long dump(),pipin(),renvint();
char*readdyn(),*fromprog(),*cat(),*tstrdup(),*cstr(),*pstrspn(),
 *bregexec(),*egrepin(),*lastdirsep();
const char*tgetenv(),*hostname();
int sgetc(),getb();

/*
 *	External variables that are checked/changed by the signal handlers:
 *	volatile time_t alrmtime;
 *	pid_t pidfilt,pidchild;
 *	volatile int nextexit;
 *	int lcking;
 */
