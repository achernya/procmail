/************************************************************************
 *	Collection of NFS secure exclusive open routines		*
 *									*
 *	Copyright (c) 1990-1992, S.R. van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: exopen.c,v 2.7 1992/01/21 17:27:04 berg Rel $";
#endif
#include "config.h"
#include "includes.h"
#include "exopen.h"

const char*hostname();
extern pid_t thepid;

const char*hostname()
{ static char name[HOSTNAMElen+1];
#ifdef	NOuname
  gethostname(name,HOSTNAMElen+1);
#else
  struct utsname names;
  uname(&names);strncpy(name,names.nodename,HOSTNAMElen);
#endif
  name[HOSTNAMElen]='\0';return name;
}

ultoan(val,dest)unsigned long val;char*dest;	      /* convert to a number */
{ register i;				     /* within the set [0-9A-Za-z-_] */
  do
   { i=val&0x3f;
     *dest++=i+(i<10?'0':i<10+26?'A'-10:i<10+26+26?'a'-10-26:
      i==10+26+26?'-'-10-26-26:'_'-10-26-27);
   }
  while(val>>=6);
  *dest='\0';
}

unique(full,p,mode)const char*const full;char*const p;const mode_t mode;
{ unsigned long retry=3;int i;			  /* create unique file name */
  do
   { ultoan(SERIALmask&(retry<<16)+(unsigned long)thepid,p+1);
     *p=UNIQ_PREFIX;strcat(p,hostname());
   }
#ifndef O_CREAT
#define ropen(path,type,mode)	creat(path,mode)
#endif
  while(0>(i=ropen(full,O_WRONLY|O_CREAT|O_EXCL|O_SYNC,mode))&&errno==EEXIST&&
   retry--);	    /* casually check if it already exists (highly unlikely) */
  if(i<0)
   { writeerr(full);return 0;
   }
  rclose(i);return 1;
}
				     /* rename MUST fail if already existent */
myrename(old,new)const char*const old,*const new;
{ int i,serrno;struct stat stbuf;
  link(old,new);serrno=errno;i=stat(old,&stbuf);unlink(old);errno=serrno;
  return stbuf.st_nlink==2?i:-1;
}

#ifdef NOstrpbrk
char*strpbrk(st,del)const char*const st,*del;
{ const char*f=0,*t;
  for(f=0;*del;)
     if((t=strchr(st,*del++))&&(!f||t<f))
	f=t;
  return(char*)f;
}
#endif
