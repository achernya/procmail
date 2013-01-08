/************************************************************************
 *	Collection of routines that return an int (sort of anyway :-)	*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: retint.c,v 2.12 1991/10/22 15:31:26 berg Rel $";
#endif
#include "config.h"
#include "procmail.h"
#include "shell.h"

setdef(name,contents)const char*const name,*const contents;
{ strcat(strcat(strcpy(sgetcp=buf2,name),"="),contents);
  readparse(buf,sgetc,0);sputenv(buf);
}

char*lastexec,*backblock;
long backlen;		       /* length of backblock, filter recovery block */
pid_t pidfilt,pidchild;
int pbackfd[2];				       /* the emergency backpipe :-) */

pipthrough(line,source,len)char*line,*source;const long len;
{ int pinfd[2],poutfd[2];
  rpipe(pbackfd);rpipe(pinfd);				 /* main pipes setup */
  if(!(pidchild=sfork()))			/* create a sending procmail */
   { backblock=source;backlen=len;signal(SIGTERM,stermchild);
     signal(SIGINT,stermchild);signal(SIGHUP,stermchild);
     signal(SIGQUIT,stermchild);rclose(rc);rclose(PRDI);rclose(PRDB);
     rpipe(poutfd);rclose(STDOUT);
     if(!(pidfilt=sfork()))				/* create the filter */
      { rclose(PWRO);rclose(PWRB);rdup(PWRI);rclose(PWRI);getstdin(PRDO);
	callnewprog(line);
      }
     rclose(PWRI);rclose(PRDO);
     if(forkerr(pidfilt,line))
      { rclose(PWRO);stermchild();
      }
     if(dump(PWRO,source,len))		  /* send in the text to be filtered */
      { writeerr(line);stermchild();
      }
     if(pwait&&waitfor(pidfilt)!=EX_OK)	 /* check the exitcode of the filter */
      { progerr(line);stermchild();
      }
     rclose(PWRB);exit(EX_OK);			  /* allow parent to proceed */
   }
  rclose(PWRI);rclose(PWRB);getstdin(PRDI);
  if(forkerr(pidchild,procmailn))
     return 1;
  return 0;		    /* we stay behind to read back the filtered text */
}

waitfor(pid)const pid_t pid;		      /* wait for a specific process */
{ int i;pid_t j;
  while(pid!=(j=wait(&i))||(i&127)==127)
     if(-1==j)
	return EX_UNAVAILABLE;
  return i>>8&255;
}

getstdin(pip)const int pip;
{ rclose(STDIN);rdup(pip);rclose(pip);
}

callnewprog(newname)const char*const newname;
{ if(sh)					 /* should we start a shell? */
   { const char*newargv[4];
     yell(executing,newname);newargv[3]=0;newargv[2]=newname;
     newargv[1]=tgetenv(shellflags);*newargv=tgetenv(shell);shexec(newargv);
   }
 {register const char*p;int argc;const char**newargv;
  argc=1;p=newname;	     /* If no shell, chop up the arguments ourselves */
  if(verbose)
   { log(executing);log(oquote);goto no_1st_comma;
   }
  do					     /* show chopped up command line */
   { if(verbose)
      { log(",");
no_1st_comma:
	log(p);
      }
     while(*p++);
   }
  while(argc++,*p!=TMNATE);
  if(verbose)
     log(cquote);
  newargv=malloc(argc*sizeof*newargv);p=newname;argc=0;	 /* alloc argv array */
  do
   { newargv[argc++]=p;
     while(*p++);
   }
  while(*p!=TMNATE);
  newargv[argc]=0;shexec(newargv);
 }
}

writeerr(line)const char*const line;
{ log("Error while writing to");logqnl(line);
}

forkerr(pid,a)const pid_t pid;const char*const a;
{ if(pid==-1)
   { log("Failed forking");logqnl(a);return 1;
   }
  return 0;
}

progerr(line)const char*const line;
{ log("Program failure of");logqnl(line);
}

opena(a)const char*const a;
{ lastfolder=cstr(lastfolder,a);yell("Opening",a);
#ifdef O_CREAT
  return ropen(a,O_WRONLY|O_APPEND|O_CREAT,0666);
#else
 {int fd;
  return(fd=ropen(a,O_WRONLY))<0?creat(a,0666):fd;
 }
#endif
}

yell(a,b)const char*const a,*const b;		     /* log if -d option set */
{ if(verbose)
   { log(a);logqnl(b);
   }
}

unlock(lockp)const char**const lockp;
{ lcking=1;
  if(*lockp)
   { yell("Unlocking",*lockp);
     if(unlink(*lockp))
      { log("Couldn't unlock");logqnl(*lockp);
      }
     free(*lockp);*lockp=0;
   }
  lcking=0;
  if(nextexit==1)	    /* make sure we are not inside terminate already */
   { log(newline);terminate();
   }
}

nomemerr()
{ log("Out of memory\nbuffer 0: \"");buf[linebuf-1]=buf2[linebuf-1]='\0';
  log(buf);log("\"\nbuffer 1:");logqnl(buf2);retval=EX_OSERR;terminate();
}

logqnl(a)const char*const a;
{ log(oquote);log(a);log(cquote);
}

nextrcfile()			/* next rcfile specified on the command line */
{ const char*p;
  while(p= *gargv)
   { gargv++;
     if(!strchr(p,'='))
      { rcfile=p;return 1;
      }
   }
  return 0;
}

rclose(fd)const int fd;		      /* a sysV secure close (signal immune) */
{ int i;
  while((i=close(fd))&&errno==EINTR);
  return i;
}

rwrite(fd,a,len)const int fd,len;void*const a;	      /* a sysV secure write */
{ int i;
  while(0>(i=write(fd,a,(size_t)len))&&errno==EINTR);
  return i;
}

rread(fd,a,len)const int fd,len;void*const a;	       /* a sysV secure read */
{ int i;
  while(0>(i=read(fd,a,(size_t)len))&&errno==EINTR);
  return i;
}

ropen(name,mode,mask)const char*const name;const int mode;const mode_t mask;
{ int i,r;					       /* a sysV secure open */
  for(lcking=4,r=noresretry;0>(i=open(name,mode,mask));)
     if(errno!=EINTR&&!((errno==EMFILE||errno==ENFILE)&&(r<0||r--)))
	break;		 /* survives a temporary "file table full" condition */
  lcking=0;return i;
}

rdup(p)const int p;
{ int i,r;
  for(lcking=4,r=noresretry;0>(i=dup(p));)	  /* catch "file table full" */
     if(!((errno==EMFILE||errno==ENFILE)&&(r<0||r--)))
	break;
  lcking=0;return i;
}

rpipe(fd)int fd[2];
{ int i,r;
  for(lcking=4,r=noresretry;0>(i=pipe(fd));)	  /* catch "file table full" */
     if(!((errno==EMFILE||errno==ENFILE)&&(r<0||r--)))
      { *fd=fd[1]= -1;break;
      }
  lcking=0;return i;
}

lockit(name,lockp)char*name;const char**const lockp;
{ int i;struct stat stbuf;
  unlock(lockp);		       /* unlock any previous lockfile FIRST */
  if(!*name)
     return;
  for(lcking=1;;)		  /* to prevent deadlocks (I hate deadlocks) */
   { yell("Locking",name);
     if(!NFSxopen(name))
      { *lockp=tstrdup(name);			   /* lock acquired, hurray! */
term:	lcking=0;
	if(nextexit)
	 { log(whilstwfor);log("lockfile");logqnl(name);terminate();
	 }
	return;			   /* check if it's time for a lock override */
      }
     if(errno==EEXIST&&!stat(name,&stbuf)&&!stbuf.st_size)
      { time_t t;
	if(locktimeout&&(t=time((time_t*)0),!stat(name,&stbuf)))     /* stat */
	   if(locktimeout<t-stbuf.st_mtime)  /* till unlink should be atomic */
	    { if(unlink(name))			 /* but can't guarantee that */
	       { log("Forced unlock denied on");logqnl(name);
	       }
	      else
	       { log("Forcing lock on");logqnl(name);suspend();
	       }
	    }
      }
     else		       /* maybe filename too long, shorten and retry */
      { if(errno!=ENOTDIR&&0<(i=strlen(name)-1)&&!strchr(dirsep,name[i-1]))
	 { name[i]='\0';continue;
	 }
	log("Lockfailure on");logqnl(name);return;
      }
     sleep((unsigned)locksleep);
     if(nextexit)
	goto term;
   }
}

lcllock()				   /* lock a local file (if need be) */
{ if(locknext)
     if(tolock)
	lockit(tolock,&loclock);
     else
	lockit(strcat(buf2,tgetenv(lockext)),&loclock);
}

terminate()
{ nextexit=2;			/* prevent multiple invocations of terminate */
  if(getpid()==thepid)
   { if(retval!=EX_OK)
      { log(Mail);
	log(fakedelivery?"lost\n":
	 retval==EX_TEMPFAIL?"requeued\n":"bounced\n");
      }
     unlock(&loclock);unlock(&globlock);unlockfd();
   }
  exit(retval);
}

ignoreterm()
{ signal(SIGTERM,SIG_IGN);signal(SIGHUP,SIG_IGN);signal(SIGINT,SIG_IGN);
  signal(SIGQUIT,SIG_IGN);
}

suspend()
{ long t;
  sleep((unsigned)suspendv);
  if(alrmtime)
     if((t=alrmtime-time((time_t*)0))<=1)	  /* if less than 1s timeout */
	ftimeout();				  /* activate it by hand now */
     else		    /* set it manually again, to avoid problems with */
	alarm((unsigned)t);	/* badly implemented sleep library functions */
}

inittmout(progname)const char*const progname;
{ lastexec=cstr(lastexec,progname);
  alrmtime=timeoutv?time((time_t*)0)+(unsigned)timeoutv:0;
  alarm((unsigned)timeoutv);
}

skipspace()
{ while(testb(' ')||testb('\t'));
}

sgetc()						/* a fake fgetc for a string */
{ return *sgetcp?*(uchar*)sgetcp++:EOF;
}

skipped(x)const char*const x;
{ log("Skipped");logqnl(x);
}

concatenate(old)char*const old;
{ register char*p=old;
  while(*p!=TMNATE)			  /* concatenate all other arguments */
   { while(*p++);
     p[-1]=' ';
   }
  *p=p[-1]='\0';return*old;
}

detab(p)char*p;
{ while(p=strchr(p,'\t'))
     *p=' ';						/* take out all tabs */
}

static uchar rcbuf[STDBUF],*rcbufp,*rcbufend;	 /* buffers for custom stdio */
static ungetb;						 /* pushed back char */

bopen(name)const char*const name;				 /* my fopen */
{ rcbufp=rcbufend=0;ungetb= -1;yell("Rcfile:",name);
  return rc=ropen(name,O_RDONLY);
}

getbl(p)char*p;							  /* my gets */
{ int i;char*q;
  for(q=p;;)
   { switch(i=getb())
      { case '\n':case EOF:
	   *p='\0';return p!=q;		     /* did we read anything at all? */
      }
     *p++=i;
   }
}

getb()								 /* my fgetc */
{ if(ungetb>=0)					    /* anything pushed back? */
   { int i;
     i=ungetb;ungetb= -1;return i;
   }
  if(rcbufp==rcbufend)
   { rcbufend=rcbuf+rread(rc,rcbufp=rcbuf,STDBUF);		   /* refill */
   }
  return rcbufp<rcbufend?*rcbufp++:EOF;
}

testb(x)const int x;		   /* fgetc that only succeeds if it matches */
{ int i;
  if((i=getb())==x)
     return 1;
  ungetb=i;return 0;
}

alphanum(c)const int c;
{ return c>='0'&&c<='9'||c>='a'&&c<='z'||c>='A'&&c<='Z'||c=='_';
}
				       /* open file or new file in directory */
deliver(boxname)char*const boxname;
{ struct stat stbuf;
  strcpy(buf,boxname);			 /* boxname can be found back in buf */
  return stat(buf,&stbuf)||!S_ISDIR(stbuf.st_mode)?
   (tofolder=1,opena(buf)):dirmail();
}

#ifndef lockfd
static oldlockfd;				    /* the fd we locked last */
#ifdef F_SETLKW
static struct flock flck;		/* why can't it be a local variable? */

lockfd(fd)					   /* the POSIX-fcntl() lock */
{ flck.l_type=F_WRLCK;flck.l_whence=SEEK_SET;flck.l_len=0;
  flck.l_start=tell(fd);lcking=5;fd=fcntl(oldlockfd=fd,F_SETLKW,&flck);
  lcking=0;return fd;
}

unlockfd()
{ flck.l_type=F_UNLCK;return fcntl(oldlockfd,F_SETLK,&flck);
}
#else
#ifdef F_LOCK
static long oldlockoffset;

lockfd(fd)						 /* the sysV-lockf() */
{ oldlockoffset=tell(fd);lcking=5;fd=lockf(oldlockfd=fd,F_LOCK,0L);lcking=0;
  return fd;
}

unlockfd()
{ lseek(oldlockfd,oldlockoffset,SEEK_SET);return lockf(oldlockfd,F_ULOCK,0L);
}
#else
#ifdef LOCK_EX
lockfd(fd)						  /* the BSD-flock() */
{ lcking=5;fd=flock(oldlockfd=fd,LOCK_EX);lcking=0;return fd;
}

unlockfd()
{ return flock(oldlockfd,LOCK_UN);
}
#endif
#endif
#endif
#endif

#include "exopen.h"

NFSxopen(name)char*name;		/* an NFS secure exclusive file open */
{char*p,*q;int j= -2,i;
  for(q=name;p=strpbrk(q,dirsep);q=p+1);		 /* find last DIRSEP */
  i=q-name;strncpy(p=malloc(i+UNIQnamelen),name,i);
  if(unique(p,p+i,0))
     j=myrename(p,name);	 /* try and rename it, fails if nonexclusive */
  free(p);return j;
}
