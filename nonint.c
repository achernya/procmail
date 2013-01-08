/************************************************************************
 *	Collection of routines that don't return int			*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	See the accompanying README file for more info.			*
 *									*
 *	Don't complain about the formatting, I know it's		*
 *	unconventional, but it's my standard format (I have my		*
 *	reasons).  If you don't like it, feed it through your		*
 *	favourite C beautifier.						*
 ************************************************************************/

#include "config.h"
#include "procmail.h"
#include "sysexits.h"

#ifdef NOmemmove
void*memmove(to,from,count)register void*to,*from;register t_buf count;{
 void*old;
 old=to;count++;--(char*)to;--(char*)from;
 if(to<=from){
   goto jiasc;
   do{
      *++(char*)to=*++(char*)from;
jiasc:;}
   while(--count);}
 else{
   (char*)to+=count;(char*)from+=count;
   goto jidesc;
   do{
      *--(char*)to=*--(char*)from;
jidesc:;}
   while(--count);}
 return old;}
#endif

void*tmalloc(len)t_buf len;{void*p;int i;	  /* this malloc can survive */
 if(p=malloc(len))		/* a temporary "out of swap space" condition */
   return p;
 for(i=renvint(iDEFnomemretry,nomemretry);i<0||i--;){
   sleep(SUSPENSION);	     /* problems?  don't panic, wait a few secs till */
   if(p=malloc(len))	     /* some other process has paniced (and died 8-) */
      return p;}
 nomemerr();}

void*trealloc(old,len)void*old;t_buf len;{int i; /* this realloc can survive */
 if(old=realloc(old,len))	/* a temporary "out of swap space" condition */
    return old;
 for(i=renvint(iDEFnomemretry,nomemretry);i<0||i--;){
   sleep(SUSPENSION);	     /* problems?  don't panic, wait a few secs till */
   if(old=realloc(old,len))  /* some other process has paniced (and died 8-) */
      return old;}
 nomemerr();}

#include "shell.h"

pid_t sfork(){pid_t i;		       /* if secur is set, it doesn't return */
 while((i=fork())&&secur&&i==-1)       /* until the fork was successfull     */
   sleep(SUSPENSION);
 return i;}

void sterminate(){
 if(!nextexit){
   log("Terminating prematurely");
   if(!lcking){
      log(newline);terminate();}}
 nextexit=1;}

void flagger(){					/* hey, we received a SIGHUP */
 flaggerd=1;}

void errgrandchild(){		       /* my grandchildren scream in despair */
 verrgrandchild=1;}

long dump(s,source,len)int s;char*source;long len;{int i;long ol;
 if(s>=0){
    ol=len;
    while(i=rwrite(s,source,BLKSIZ<len?BLKSIZ:(int)len)){
       if(i<0){
	  i=0;goto writefin;}
       len-=i;source+=i;}
    if(!len&&(ol<2||!(source[-1]=='\n'&&source[-2]=='\n')))
       rwrite(s,newline,1);	       /* message always ends with a newline */
writefin:
    if(fsync(s)&&errno!=EINVAL)
       len++;			       /* if there is a physical write error */
    rclose(s);return len-i;}
 return len?len:-1;}	   /* return an error even if nothing was to be sent */

long pipin(line,source,len)char*line,*source;long len;{pid_t pid;
 int poutfd[2];
 pipe(poutfd);
 if(!(pid=sfork())){
   rclose(PWRITE);redirect(PREAD);callnewprog(line);}
 rclose(PREAD);forkerr(pid,line);
 if(len=dump(PWRITE,source,len))
   writeerr(line);
 if(pwait&&waitfor(pid)!=EX_OK){
   progerr(line);len=1;}
 return len;}

char*readdyn(bf,filled)char*bf;long*filled;{int i;
 goto jumpin;
 do{
   *filled+=i;
jumpin:
   bf=realloc(bf,*filled+BLKSIZ);}			   /* dynamic adjust */
 while(0<(i=read(STDIN,bf+*filled,BLKSIZ)));
 if(!*filled)
   return realloc(bf,1);
 return realloc(bf,*filled);}			/* minimize the buffer space */

char*cat(a,b)char*a,*b;{
 return strcat(strcpy(buf,a),b);}

char*findel(start,end)register char*start,*end;{/* find the first empty line */
 while(start<end)
   if(*start++=='\n'&&start<end&&*start=='\n')
      return start+1;
 return end;}

char*tstrdup(a)char*a;{int i;
 i=strlen(a)+1;return tmemmove(malloc(i),a,i);}
