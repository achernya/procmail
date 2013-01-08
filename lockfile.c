/************************************************************************
 *	lockfile.c	a conditional semaphore-file creator		*
 *									*
 *	Is relatively bug free.						*
 *									*
 *	Created by S.R.van den Berg, The Netherlands			*
 *	This file can be freely copied for any use.			*
 ************************************************************************/
#ifdef	RCS
static char rcsid[]="$Id: lockfile.c,v 2.1 1991/06/11 14:00:41 berg Rel $";
#endif
static char rcsdate[]="$Date: 1991/06/11 14:00:41 $";
#include "config.h"		       /* overkill, I know, only need DIRSEP */
#include "includes.h"

volatile int exitflag;
pid_t thepid;
const char dirsep[]=DIRSEP;

void failure(){
 exitflag=1;}

main(argc,argv)const int argc;const char*argv[];{const char**p,*cp;
 int sleepsec,retries,i,invert,force,suspend,retval=0;
 static char usage[]=
   "Usage: lockfile -nnn | -rnnn | -! | -lnnn | -snnn | file ...\n";
 sleepsec=8;force=retries=invert=0;suspend=16;thepid=getpid();
 if(argc<2){
   putse(usage);return EX_USAGE;}
again:
 p=argv+1;signal(SIGHUP,failure);signal(SIGINT,failure);
 signal(SIGQUIT,failure);signal(SIGTERM,failure);
 while(*p)
    if(*(cp= *p++)=='-')
       switch(cp[1]){
       case '!':invert=1;break;
       case 'r':retries=strtol(cp+2,(char**)0,10);break;
       case 'l':force=strtol(cp+2,(char**)0,10);break;
       case 's':suspend=strtol(cp+2,(char**)0,10);break;
       default:
	  if(cp[1]-'0'>(unsigned)9){
	     putse(usage);retval=EX_USAGE;goto failure;}
	  if(sleepsec>=0)
	     sleepsec=strtol(cp+1,(char**)0,10);}
    else
      if(sleepsec<0)
	 unlink(cp);
      else{
	 while(0>NFSxopen(cp)){struct stat buf;time_t t;
	    if(exitflag||retries==1){
failure:       sleepsec= -1;p[-1]=0;goto again;}
	    if(force&&(t=time((time_t*)0),!stat(cp,&buf))&&
	       force<t-buf.st_mtime){
	       unlink(cp);putse("Forcing lock on \"");putse(cp);putse("\"\n");
	       sleep(suspend);}
	    else
	       sleep(sleepsec);
	    if(retries)
	       retries--;}}
return retval?retval:invert^(sleepsec<0)?EX_CANTCREAT:EX_OK;}

putse(a)char*a;{char*b;
 b=a-1;
 while(*++b);
 write(STDERR,a,b-a);}

#include "exopen.h"

NFSxopen(name)char*name;{char*p,*q;int j= -1,i;
 for(q=name;p=strpbrk(q,dirsep);q=p+1);
 i=q-name;
 if(!(p=malloc(i+UNIQnamelen)))
    return exitflag=1;
 strncpy(p,name,i);
 if(unique(p,p+i,0))
   j=myrename(p,name);
 free(p);return j;}

void*tmalloc(len)const size_t len;{				     /* stub */
 return malloc(len);}

ropen(name,mode,mask)const char*const name;const mode_t mask;{	     /* stub */
 return open(name,mode,mask);}

rclose(fd)const int fd;{					     /* stub */
 return close(fd);}

writeerr(a)const char*const a;{}				     /* stub */
