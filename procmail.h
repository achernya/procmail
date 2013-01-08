/*$Id: procmail.h,v 2.0 1991/06/10 14:35:35 berg Rel $*/

#include "includes.h"

typedef unsigned char uchar;

#ifndef console
#define console devnull
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
#define MAXvarvals	maxindex(strenvvar)

#ifndef MAIN
extern char*buf,*buf2,*globlock,*loclock,*tolock,*lastfolder;
extern const char grep[],shellflags[],shell[],lockext[],newline[],binsh[],
 unexpeof[],shellmetas[],*const*gargv,*sgetcp,*rcfile,dirsep[],msgprefix[],
 devnull[],executing[],oquote[],cquote[],whilstwfor[];
extern struct varval strenvvar[];
extern long lastdump;
extern sh,pwait,retval,lcking,locknext,verbose,linebuf,rc;
extern volatile flaggerd,nextexit;
extern pid_t thepid;
#endif

#ifdef NOmemmove
void*memmove();
#endif

void*tmalloc(),*trealloc();
pid_t sfork();
void sterminate(),stermchild(),flagger(),errgrandchild();
long dump(),pipin(),renvint();
char*readdyn(),*fromprog(),*cat(),*findnl(),*tstrdup(),*cstr();
const char*tgetenv(),*hostname();
int sgetc(),getb();
