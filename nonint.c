/************************************************************************
 *	Collection of routines that don't return int			*
 *									*
 *	Copyright (c) 1990-1992, S.R. van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: nonint.c,v 2.16 1992/01/31 12:35:17 berg Rel $";
#endif
#include "config.h"
#include "procmail.h"

#define nomemretry	noresretry
#define noforkretry	noresretry

void*tmalloc(len)const size_t len;    /* this malloc can survive a temporary */
{ void*p;int i;				    /* "out of swap space" condition */
  if(p=malloc(len))
     return p;
  if(p=malloc(1))
     free(p);			   /* works on some systems with latent free */
  for(lcking=2,i=nomemretry;i<0||i--;)
   { suspend();		     /* problems?  don't panic, wait a few secs till */
     if(p=malloc(len))	     /* some other process has paniced (and died 8-) */
      { lcking=0;return p;
      }
   }
  nomemerr();
}

void*trealloc(old,len)void*const old;const size_t len;
{ void*p;int i;
  if(p=realloc(old,len))
     return p;				    /* for comment see tmalloc above */
  if(p=malloc(1))
    free(p);
  for(lcking=2,i=nomemretry;i<0||i--;)
   { suspend();
     if(p=realloc(old,len))
      { lcking=0;return p;
      }
   }
  nomemerr();
}
		       /* line buffered to keep concurrent entries untangled */
log(new)const char*const new;
{ int lnew,i;static lold;static char*old;char*p;
  if(lnew=strlen(new))						/* anything? */
   {
#ifndef O_CREAT
     lseek(STDERR,0L,SEEK_END);		  /* locking should be done actually */
#endif
     if(nextexit)
	goto direct;			       /* careful, in terminate code */
     i=lold+lnew;
     if(p=lold?realloc(old,i):malloc(i))		 /* unshelled malloc */
      { memmove((old=p)+lold,new,(size_t)lnew);			   /* append */
	if(p[(lold=i)-1]=='\n')					     /* EOL? */
	 { rwrite(STDERR,p,i);lold=0;free(p);		/* flush the line(s) */
	 }
      }
     else					   /* no memory, force flush */
      { if(lold)
	 { rwrite(STDERR,old,i);lold=0;free(old);
	 }
direct: rwrite(STDERR,new,lnew);
      }
   }
}

#include "shell.h"

pid_t sfork()				/* this fork can survive a temporary */
{ pid_t i;int r;			   /* "process table full" condition */
  r=noforkretry;
  while((i=fork())==-1)
   { lcking=3;
     if(!(r<0||r--))
	break;
     suspend();
   }
  lcking=0;return i;
}

void srequeue()
{ retval=EX_TEMPFAIL;sterminate();
}

void slose()
{ fakedelivery=2;sterminate();
}

void sbounce()
{ retval=EX_CANTCREAT;sterminate();
}

extern char*lastexec,*backblock;		/* see retint.c for comment */
extern long backlen;
extern pid_t pidfilt,pidchild;
extern pbackfd[2];

void stermchild()
{ if(pidfilt>0)		    /* don't kill what is not ours, we might be root */
     kill(pidfilt,SIGTERM);
  log("Rescue of unfiltered data ");
  if(dump(PWRB,backblock,backlen))    /* pump back the data via the backpipe */
     log("failed\n");
  else
     log("succeeded\n");
  exit(EX_UNAVAILABLE);
}

void ftimeout()
{ alarm(0);alrmtime=0;
  if(pidchild>0&&!kill(pidchild,SIGTERM))	   /* careful, killing again */
      { log("Timeout, terminating");logqnl(lastexec);
      }
  signal(SIGALRM,ftimeout);
}

long dump(s,source,len)const int s;const char*source;long len;
{ int i;
  if(s>=0)
   { fdlock(s);lastdump=len;mboxseparator(s);  /* prepend optional separator */
#ifndef O_CREAT
     lseek(s,0L,SEEK_END);
#endif
     if(len&&tofile)		       /* if it is a file, trick NFS into an */
      { --len;rwrite(s,source++,1);sleep(1);		    /* a_time<m_time */
      }
     while(i=rwrite(s,source,BLKSIZ<len?BLKSIZ:(int)len))
      { if(i<0)
	 { i=0;goto writefin;
	 }
	len-=i;source+=i;
      }
     if(!len&&(lastdump<2||!(source[-1]=='\n'&&source[-2]=='\n')))
	lastdump++,rwrite(s,newline,1);	       /* message always ends with a */
     mboxseparator(s);		 /* newline and an optional custom separator */
writefin:
     fdunlock();rclose(s);return ignwerr?(ignwerr=0):len-i;
   }
  return len?len:-1;	   /* return an error even if nothing was to be sent */
}

long pipin(line,source,len)char*const line;char*source;long len;
{ int poutfd[2];
  rpipe(poutfd);
  if(!(pidchild=sfork()))				    /* spawn program */
   { rclose(PWRO);rclose(rc);getstdin(PRDO);callnewprog(line);
   }
  rclose(PRDO);
  if(forkerr(pidchild,line))
     return 1;
  if(len=dump(PWRO,source,len))			    /* dump mail in the pipe */
     writeerr(line);		       /* pipe was shut in our face, get mad */
  if(pwait&&waitfor(pidchild)!=EX_OK)	    /* optionally check the exitcode */
   { progerr(line);len=1;
   }
  pidchild=0;
  if(!sh)
     concatenate(line);
  lastfolder=cstr(lastfolder,line);return len;
}

char*readdyn(bf,filled)char*bf;long*const filled;
{ int i;long oldsize;
  oldsize= *filled;goto jumpin;
  do
   { *filled+=i;				/* change listed buffer size */
jumpin:
#ifdef SMALLHEAP
     if((size_t)*filled>=(size_t)(*filled+BLKSIZ))
	lcking=2,nomemerr();
#endif
     bf=realloc(bf,*filled+BLKSIZ);    /* dynamically adjust the buffer size */
jumpback:;
   }
  while(0<(i=rread(STDIN,bf+*filled,BLKSIZ)));			/* read mail */
  if(pidchild>0)
   { pidchild=0;getstdin(PRDB);		       /* filter ready, get backpipe */
     if(1==rread(STDIN,buf,1))			      /* backup pipe closed? */
      { bf=realloc(bf,(*filled=oldsize+1)+BLKSIZ);bf[oldsize]= *buf;
	 goto jumpback;			       /* filter goofed, rescue data */
      }
   }
  pidchild=0;					/* child must be gone by now */
  if(!*filled)
     return realloc(bf,1);		     /* +1 for housekeeping purposes */
  return realloc(bf,*filled+1);			/* minimise the buffer space */
}

char*fromprog(name,dest)char*const name;char*dest;
{ int pinfd[2];long nls;
  rpipe(pinfd);inittmout(name);
  if(!(pidchild=sfork()))				    /* spawn program */
   { rclose(STDIN);opena(devnull);rclose(PRDI);rclose(rc);rclose(STDOUT);
     rdup(PWRI);rclose(PWRI);callnewprog(name);
   }
  rclose(PWRI);nls=0;
  if(!forkerr(pidchild,name))
   { while(0<rread(PRDI,dest,1))			    /* read its lips */
	if(*dest=='\n')				    /* careful with newlines */
	   nls++;		    /* trailing newlines should be discarded */
	else
	 { if(nls)
	      for(dest[nls]= *dest;*dest++='\n',--nls;);     /* fill them in */
	   dest++;
	 }
     waitfor(pidchild);
   }
  pidchild=0;rclose(PRDI);*dest='\0';return dest;
}

char*cat(a,b)const char*const a,*const b;
{ return strcat(strcpy(buf,a),b);
}

char*tstrdup(a)const char*const a;
{ int i;
  i=strlen(a)+1;return tmemmove(malloc(i),a,i);
}

const char*tgetenv(a)const char*const a;
{ const char*b;
  return(b=getenv(a))?b:"";
}

char*cstr(a,b)const char*const a,*const b;	/* dynamic buffer management */
{ if(a)
     free(a);
  return tstrdup(b);
}

long renvint(i,env)const long i;const char*const env;
{ const char*p;long t;
  t=strtol(env,&p,10);return p==env?i:t;       /* parse it like a decimal nr */
}

char*egrepin(expr,source,len,casesens)const char*expr,*source;
 const long len;
{ source=bregexec(expr=bregcomp(expr,!casesens),
   source,len>0?(size_t)len:(size_t)0,!casesens);
  free(expr);return(char*)source;
}

char*lastdirsep(filename)const char*filename;	 /* finds the next character */
{ const char*p;					/* following the last DIRSEP */
  while(p=strpbrk(filename,dirsep))
     filename=p+1;
  return(char*)filename;
}
