/************************************************************************
 *	Collection of NFS secure exclusive open routines		*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 *	#include "STYLE"						*
 *									*
 ************************************************************************/
#ifdef	RCS
static char rcsid[]="$Id: exopen.c,v 2.0 1991/06/10 14:35:35 berg Rel $";
#endif
#include "config.h"
#include "includes.h"
#include "exopen.h"

const char*hostname();
extern pid_t thepid;

const char*hostname(){static char name[HOSTNAMElen+1];
#ifdef	NOuname
 gethostname(name,HOSTNAMElen+1);
#else
 struct utsname names;
 uname(&names);strncpy(name,names.nodename,HOSTNAMElen);
#endif
 name[HOSTNAMElen]='\0';return name;}

ultoan(val,dest)unsigned long val;char*dest;{register i;       /* convert to */
 do{				    /* a number within the set [0-9A-Za-z-_] */
    i=val&0x3f;
    *dest++=i+(i<10?'0':i<10+26?'A'-10:i<10+26+26?'a'-10-26:
       i==10+26+26?'-'-10-26-26:'_'-10-26-27);}
 while(val>>=6);
 *dest='\0';}

unique(full,p,mode)const char*const full;char*const p;const mode_t mode;{
 unsigned long retry=3;int i;			  /* create unique file name */
 do{
   ultoan(SERIALmask&(retry<<16)+(unsigned long)thepid,p+1);
   *p='_';strcat(p,hostname());}
 while(0>(i=ropen(full,O_WRONLY|O_CREAT|O_EXCL|O_SYNC,mode))&&errno==EEXIST
   &&retry--);	    /* casually check if it already exists (highly unlikely) */
 if(i<0){
   writeerr(full);return 0;}
 rclose(i);return 1;}
				     /* rename MUST fail if already existent */
myrename(old,new)const char*const old,*const new;{int i,serrno;
 i=link(old,new);serrno=errno;unlink(old);errno=serrno;return i;}
