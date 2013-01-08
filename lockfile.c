/************************************************************************
 *	lockfile.c	a conditional semaphore-file creator		*
 *									*
 *	It has been designed to be able to be run suid/sgid root or	*
 *	any id you see fit (in case your mail spool area is *not*	*
 *	world writeable), without creating security holes.		*
 *									*
 *	Seems to be perfect.						*
 *									*
 *	Created by S.R. van den Berg, The Netherlands			*
 *	This file can be freely copied for any use.			*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: lockfile.c,v 2.15 1992/06/30 16:42:26 berg Rel $";
#endif
static char rcsdate[]="$Date: 1992/06/30 16:42:26 $";
#include "config.h"
#include "includes.h"
#include "exopen.h"
#include "strpbrk.h"

#ifndef SYSTEM_MBOX
#define SYSTEM_MBOX	SYSTEM_MAILBOX
#endif

volatile int exitflag;
pid_t thepid;
char systm_mbox[]=SYSTEM_MBOX;
const char dirsep[]=DIRSEP,lockext[]=DEFlockext,nameprefix[]="lockfile: ";

void failure()
{ exitflag=1;
}

main(argc,argv)const char*const argv[];
{ const char*const*p,*cp;uid_t uid;
  int sleepsec,retries,invert,force,suspend,retval=0,virgin=0;
  static const char usage[]=
  "Usage: lockfile -nnn | -rnnn | -! | -lnnn | -snnn | -ml | -mu | file ...\n";
  sleepsec=8;force=retries=invert=0;suspend=16;thepid=getpid();uid=getuid();
  *lastdirsep(systm_mbox)='\0';
  if(--argc<=0)
   { putse(usage);return EX_USAGE;
   }
again:
  p=argv;signal(SIGHUP,(void(*)())failure);signal(SIGINT,(void(*)())failure);
  signal(SIGQUIT,(void(*)())failure);signal(SIGTERM,(void(*)())failure);
  while(argc--)
     if(*(cp= *++p)=='-')
	switch(cp[1])
	 { case '!':invert=1;break;
	   case 'r':retries=strtol(cp+2,(char**)0,10);break;
	   case 'l':force=strtol(cp+2,(char**)0,10);break;
	   case 's':suspend=strtol(cp+2,(char**)0,10);break;
	   case 'm':
	    { struct passwd*pass;char*ma;
	      if(virgin||!(pass=getpwuid(uid))||
	       !(ma=malloc(strlen(systm_mbox)+strlen(pass->pw_name)+
	       STRLEN(lockext)+1)))
		 goto eusg;
	      strcpy(ma,systm_mbox);strcat(ma,pass->pw_name);
	      strcat(ma,lockext);
	      if(cp[2]=='u')
	       { unlink(ma);break;
	       }
	      if(cp[2]=='l')
	       { cp=ma;goto stilv;
	       }
	      goto eusg;
	    }
	   case HELPOPT1:case HELPOPT2:putse(usage);
	      putse(
 "\t-nnn\twait nnn seconds (default 8) between locking attempts\
\n\t-rnnn\tmake nnn retries before giving up on a lock\
\n\t-!\tinvert the exit code of lockfile\
\n\t-lnnn\tset locktimeout to nnn seconds\
\n\t-snnn\tsuspend nnn seconds (default 16) after a locktimeout occurred\
\n\t-ml\tlock your system mail-spool file\
\n\t-mu\tunlock your system mail-spool file\n");goto xusg;
	   default:
	      if(cp[1]-'0'>(unsigned)9)
eusg:	       { putse(usage);
xusg:		 retval=EX_USAGE;goto lfailure;
	       }
	      if(sleepsec>=0)
		 sleepsec=strtol(cp+1,(char**)0,10);
	 }
     else if(sleepsec<0)
	unlink(cp);
     else
      { time_t t;
	setgid(getgid());setuid(uid);
stilv:	virgin=1;
	while(0>NFSxopen(cp,&t))
	 { struct stat buf;
	   if(exitflag||retries==1)
	    { putse(nameprefix);putse("Sorry, giving up\n");
lfailure:     sleepsec= -1;argc=p-argv-1;goto again;
	    }
	   if(force&&!stat(cp,&buf)&&force<t-buf.st_mtime)
	    { unlink(cp);putse(nameprefix);putse("Forcing lock on \"");
	      putse(cp);putse("\"\n");sleep(suspend);
	    }
	   else
	      sleep(sleepsec);
	   if(retries)
	      retries--;
	 }
      }
  return retval?retval:invert^(sleepsec<0)?EX_CANTCREAT:EX_OK;
}

putse(a)char*a;
{ char*b;
  b=a-1;
  while(*++b);
  write(STDERR,a,(size_t)(b-a));
}

NFSxopen(name,tim)char*name;time_t*const tim;
{ char*p,*q;int j= -1,i;struct stat stbuf;
  for(q=name;p=strpbrk(q,dirsep);q=p+1);
  i=q-name;
  if(!(p=malloc(i+UNIQnamelen)))
     return exitflag=1;
  strncpy(p,name,i);
  if(unique(p,p+i,0))
     stat(p,&stbuf),*tim=stbuf.st_mtime,j=myrename(p,name);
  free(p);return j;
}

void*tmalloc(len)const size_t len;				     /* stub */
{ return malloc(len);
}

ropen(name,mode,mask)const char*const name;const int mode;const mode_t mask;
{ return open(name,mode,mask);					     /* stub */
}

rclose(fd)const int fd;						     /* stub */
{ return close(fd);
}

writeerr(a)const char*const a;					     /* stub */
{
}
