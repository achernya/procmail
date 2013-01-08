/************************************************************************
 *	A some common routines for procmail and formail			*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 *	#include "STYLE"						*
 *									*
 ************************************************************************/
#ifdef	RCS
static char rcsid[]="$Id: common.c,v 2.2 1991/07/17 14:58:38 berg Rel $";
#endif
#include "includes.h"

void*tmalloc();
extern const char binsh[];

#ifdef NOmemmove
void*memmove(To,From,count)void*To,*From;register size_t count;{
#ifdef NObcopy
 register char*to=To,*from=From;/*void*old;*/	  /* silly compromise, throw */
 /*old=to;*/count++;--to;--from;   /* away space to be syntactically correct */
 if(to<=from){
   goto jiasc;
   do{
      *++to= *++from;					  /* copy from above */
jiasc:;}
   while(--count);}
 else{
   to+=count;from+=count;
   goto jidesc;
   do{
      *--to= *--from;					  /* copy from below */
jidesc:;}
   while(--count);}
 return To/*old*/;}
#else
 bcopy(From,To,count);return To;}
#endif
#endif

#include "shell.h"

shexec(argv)const char*const*argv;{int i;const char**newargv,**p;
#ifdef SIGXCPU
 signal(SIGXCPU,SIG_DFL);signal(SIGXFSZ,SIG_DFL);
#endif
 signal(SIGPIPE,SIG_DFL);execvp(*argv,argv); /* -- or is it a shell script ? */
 for(p=(const char**)argv,i=1;i++,*p++;);	      /* count the arguments */
 newargv=malloc(i*sizeof*p);
 for(*(p=newargv)=binsh;*++p= *++argv;);
 execve(*newargv,newargv,environ);	      /* no shell script? -> trouble */
 log("Failed to execute");logqnl(*argv);exit(EX_UNAVAILABLE);}
