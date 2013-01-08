/************************************************************************
 *	Collection of routines that don't return int			*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 *	#include "STYLE"						*
 *									*
 ************************************************************************/
#ifdef	RCS
static char rcsid[]="$Id: nonint.c,v 2.0 1991/06/10 14:35:35 berg Rel $";
#endif
#include "config.h"
#include "procmail.h"

#define nomemretry	noresretry
#define noforkretry	noresretry

void*tmalloc(len)const size_t len;{void*p;int i;  /* this malloc can survive */
 if(p=malloc(len))		/* a temporary "out of swap space" condition */
   return p;
 if(p=malloc(1))
   free(p);			   /* works on some systems with latent free */
 for(lcking=2,i=nomemretry;i<0||i--;){
   suspend();		     /* problems?  don't panic, wait a few secs till */
   if(p=malloc(len)){	     /* some other process has paniced (and died 8-) */
      lcking=0;return p;}}
 nomemerr();}

void*trealloc(old,len)void*const old;const size_t len;{void*p;int i;
 if(p=realloc(old,len))
    return p;				    /* for comment see tmalloc above */
 if(p=malloc(1))
   free(p);
 for(lcking=2,i=nomemretry;i<0||i--;){
   suspend();
   if(p=realloc(old,len)){
      lcking=0;return p;}}
 nomemerr();}
		       /* line buffered to keep concurrent entries untangled */
log(new)const char*const new;{int lnew,i;static lold;static char*old;char*p;
 if(lnew=strlen(new)){						/* anything? */
   if(nextexit)
      goto direct;			      /* carefull, in terminate code */
   i=lold+lnew;
   if(p=lold?realloc(old,i):malloc(i)){
      memmove((old=p)+lold,new,(size_t)lnew);			   /* append */
      if(p[(lold=i)-1]=='\n'){					     /* EOL? */
	 rwrite(STDERR,p,i);lold=0;free(p);}}		/* flush the line(s) */
   else{					   /* no memory, force flush */
      if(lold){
	 rwrite(STDERR,old,i);lold=0;free(old);}
direct:
      rwrite(STDERR,new,lnew);}}}

#include "shell.h"

pid_t sfork(){pid_t i;int r;		/* this fork can survive a temporary */
 r=noforkretry;				   /* "process table full" condition */
 while((i=fork())==-1){
   lcking=3;
   if(!(r<0||r--))
      break;
   suspend();}
 lcking=0;return i;}

extern char*backblock;				/* see retint.c for comment */
extern long backlen;
pid_t pidfilt,pidchild;
extern pbackfd[2];

void sterminate(){static const char*const msg[]={newline,0,
 "memory\n","fork\n","file descriptor\n"};
 signal(SIGTERM,SIG_IGN);signal(SIGHUP,SIG_IGN);signal(SIGINT,SIG_IGN);
 if(pidchild)		    /* don't kill what is not ours, we might be root */
   kill(pidchild,SIGTERM);
 if(!nextexit){
   nextexit=1;log("Terminating prematurely");
   if(1!=lcking){
      if(1<lcking)
	 log(whilstwfor);
      log(msg[lcking]);terminate();}}}

void stermchild(){
 signal(SIGHUP,SIG_IGN);signal(SIGINT,SIG_IGN);signal(SIGQUIT,SIG_IGN);
 signal(SIGTERM,SIG_IGN);kill(pidfilt,SIGTERM);kill(thepid,SIGQUIT);
 log("Rescue of unfiltered data ");
 if(dump(PWRB,backblock,backlen)) /* pump back the data through the backpipe */
    log("failed\n");
 else
    log("succeeded\n");
 exit(EX_UNAVAILABLE);}

void flagger(){				       /* hey, we received a SIGQUIT */
 signal(SIGQUIT,flagger);flaggerd=1;}

long dump(s,source,len)const int s;const char*source;long len;{int i;
 if(s>=0){
    lastdump=len;
    while(i=rwrite(s,source,BLKSIZ<len?BLKSIZ:(int)len)){
       if(i<0){
	  i=0;goto writefin;}
       len-=i;source+=i;}
    if(!len&&(lastdump<2||!(source[-1]=='\n'&&source[-2]=='\n')))
       lastdump++,rwrite(s,newline,1); /* message always ends with a newline */
writefin:
    rclose(s);return len-i;}
 return len?len:-1;}	   /* return an error even if nothing was to be sent */

long pipin(line,source,len)char*const line;const char*source;long len;{
 pid_t pid;int poutfd[2];
 rpipe(poutfd);
 if(!(pid=sfork())){					    /* spawn program */
   rclose(PWRO);rclose(rc);getstdin(PRDO);callnewprog(line);}
 rclose(PRDO);forkerr(pid,line);
 if(len=dump(PWRO,source,len))			    /* dump mail in the pipe */
   writeerr(line);		       /* pipe was shut in our face, get mad */
 if(pwait&&waitfor(pid)!=EX_OK){	    /* optionally check the exitcode */
   progerr(line);len=1;}
 if(!sh){char*p;
   for(p=line;;)	    /* change back the \0's into blanks for printing */
      if(!*p++)
	 if(*p!=TMNATE)
	    p[-1]=' ';
	 else
	    break;}
 lastfolder=cstr(lastfolder,line);return len;}

char*readdyn(bf,filled)char*bf;long*const filled;{int i;long oldsize;
 oldsize= *filled;goto jumpin;
 do{
   *filled+=i;					/* change listed buffer size */
jumpin:
   bf=realloc(bf,*filled+BLKSIZ);      /* dynamically adjust the buffer size */
jumpback:;}
 while(0<(i=rread(STDIN,bf+*filled,BLKSIZ)));			/* read mail */
 switch(flaggerd){
   case 0:waitflagger();		    /* wait for the filter to finish */
   case 1:getstdin(PRDB);		       /* filter ready, get backpipe */
      if(1==rread(STDIN,buf,1)){		      /* backup pipe closed? */
	 bf=realloc(bf,(*filled=oldsize+1)+BLKSIZ);bf[oldsize]= *buf;flaggerd=2;
	 goto jumpback;}}		       /* filter goofed, rescue data */
 pidchild=0;					/* child must be gone by now */
 if(!*filled)
   return realloc(bf,1);		     /* +1 for housekeeping purposes */
 return realloc(bf,*filled+1);}			/* minimize the buffer space */

char*fromprog(name,dest)char*const name;char*dest;{int pinfd[2];long nls;
 rpipe(pinfd);
 if(!(pidchild=fork())){				    /* spawn program */
   rclose(STDIN);opena(devnull);rclose(PRDI);rclose(rc);rclose(STDOUT);
   rdup(PWRI);rclose(PWRI);callnewprog(name);}
 rclose(PWRI);nls=0;
 if(!forkerr(pidchild,name))
    while(0<rread(PRDI,dest,1))				    /* read its lips */
       if(*dest=='\n')				   /* carefull with newlines */
	  nls++;			/* trailing ones should be discarded */
       else{
	  if(nls)
	     for(dest[nls]= *dest;*dest++='\n',--nls;);	     /* fill them in */
	  dest++;}
 pidchild=0;rclose(PRDI);*dest='\0';return dest;}

char*cat(a,b)const char*const a,*const b;{
 return strcat(strcpy(buf,a),b);}

char*findnl(start,end)register const char*start,*end;{
 while(start<end)				   /* find the first newline */
   if(*start++=='\n')
      return(char*)start;
 return(char*)end;}

char*tstrdup(a)const char*const a;{int i;
 i=strlen(a)+1;return tmemmove(malloc(i),a,i);}

const char*tgetenv(a)const char*a;{const char*b;
 return(b=getenv(a))?b:"";}

char*cstr(a,b)const char*const a,*const b;{	/* dynamic buffer management */
 if(a)
   free(a);
 return tstrdup(b);}

long renvint(i,env)long i;const char*const env;{
 sscanf(tgetenv(env),"%d",&i);return i;}       /* parse it like a decimal nr */
