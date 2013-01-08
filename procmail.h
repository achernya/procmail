#include "includes.h"

#ifdef	NOsync			/* If you don't want syncs at all define */
# define fsync(fd) 0		/* NOsync.  Only recommended if procmail */
# define sync() 0		/* isn't used in a networked environment */
#else
# ifdef NOfsync			/* If you don't have fsync, define NOfsync */
#  define fsync(fd) 0		/* sync will be used instead.  Is a bit	   */
# endif				/* slower, but works nevertheless	   */
#endif

#define PREAD	(poutfd[0])
#define PWRITE	(poutfd[1])
#define tscrc(a,b)	(0<fscanf(rc,a,b))
#define scrc(a,b)	fscanf(rc,a,b)

#ifndef MAIN
extern char buf[],buf2[],maildir[],defaultf[],logfile[],lockfile[],grep[],
 host[],locksleep[],orgmail[],eumask[],shellmetas[],shellflags[],shell[],
 sendmail[],lockext[],locktimeout[],devnull[],newline[],binsh[],home[],
 tmp[],user[],nomemretry[],*rcfile,**gargv,*globlock,*loclock,*tolock;
extern retval,flaggerd,verrgrandchild,sh,pwait,secur,lcking,nextexit,locknext;
extern pid_t mother;
extern FILE*rc;
#endif

#ifdef NOmemmove
void*memmove();
#endif

void*tmalloc(),*trealloc();
pid_t sfork();
void sterminate(),flagger(),errgrandchild();
long dump(),pipin();
char*readdyn(),*cat(),*findel(),*tstrdup();
