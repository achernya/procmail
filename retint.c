/************************************************************************
 *	Collection of routines that return an int (sort of anyway :-)	*
 *									*
 *	Copyright (c) 1990-1992, S.R. van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: retint.c,v 2.26 1992/04/09 16:16:41 berg Rel $";
#endif
#include "config.h"
#include "procmail.h"
#include "shell.h"

setdef(name,contents)const char*const name,*const contents;
{ strcat(strcat(strcpy((char*)(sgetcp=buf2),name),"="),contents);
  readparse(buf,sgetc,2);sputenv(buf);
}

char*lastexec,*backblock;
long backlen;		       /* length of backblock, filter recovery block */
pid_t pidfilt;
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
	rclose(PWRO),stermchild();
     if(dump(PWRO,source,len))		  /* send in the text to be filtered */
	writeerr(line),stermchild();
     if(pwait&&waitfor(pidfilt)!=EX_OK)	 /* check the exitcode of the filter */
      { if(!(pwait&2))				  /* do we put it on report? */
	   progerr(line);
	stermchild();
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
  while(pid!=(j=wait(&i))||WIFSTOPPED(i))
     if(-1==j)
	return -1;
  return WIFEXITED(i)?WEXITSTATUS(i):-1;
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
  return ropen(a,O_WRONLY|O_APPEND|O_CREAT,NORMperm);
#else
 {int fd;
  return(fd=ropen(a,O_WRONLY,0))<0?creat(a,NORMperm):fd;
 }
#endif
}

yell(a,b)const char*const a,*const b;		     /* log if -d option set */
{ if(verbose)
     log(a),logqnl(b);
}

unlock(lockp)char**const lockp;
{ lcking|=lck_LOCKFILE;
  if(*lockp)
   { yell("Unlocking",*lockp);
     if(unlink(*lockp))
	log("Couldn't unlock"),logqnl(*lockp);
     if(!nextexit)			   /* if not inside a signal handler */
	free(*lockp);
     *lockp=0;
   }
  lcking&=~lck_LOCKFILE;
  if(nextexit==1)	    /* make sure we are not inside terminate already */
     log(newline),terminate();
}

nomemerr()		  /* set nextexit to prevent log from using malloc() */
{ nextexit=2;log("Out of memory\n");
  if(buf2)
   { buf[linebuf-1]=buf2[linebuf-1]='\0';log("buffer 0:");logqnl(buf);
     log("buffer 1:");logqnl(buf2);
   }
  if(retval!=EX_TEMPFAIL)
     retval=EX_OSERR;
  terminate();
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

rclose(fd)const int fd;		      /* a SysV secure close (signal immune) */
{ int i;
  while((i=close(fd))&&errno==EINTR);
  return i;
}

rwrite(fd,a,len)const int fd,len;void*const a;	      /* a SysV secure write */
{ int i;
  while(0>(i=write(fd,a,(size_t)len))&&errno==EINTR);
  return i;
}

rread(fd,a,len)const int fd,len;void*const a;	       /* a SysV secure read */
{ int i;
  while(0>(i=read(fd,a,(size_t)len))&&errno==EINTR);
  return i;
}

ropen(name,mode,mask)const char*const name;const int mode;const mode_t mask;
{ int i,r;					       /* a SysV secure open */
  for(r=noresretry,lcking|=lck_FILDES;0>(i=open(name,mode,mask));)
     if(errno!=EINTR&&!((errno==EMFILE||errno==ENFILE)&&(r<0||r--)))
	break;		 /* survives a temporary "file table full" condition */
  lcking&=~lck_FILDES;return i;
}

rdup(p)const int p;
{ int i,r;					  /* catch "file table full" */
  for(r=noresretry,lcking|=lck_FILDES;0>(i=dup(p));)
     if(!((errno==EMFILE||errno==ENFILE)&&(r<0||r--)))
	break;
  lcking&=~lck_FILDES;return i;
}

rpipe(fd)int fd[2];
{ int i,r;					  /* catch "file table full" */
  for(r=noresretry,lcking|=lck_FILDES;0>(i=pipe(fd));)
     if(!((errno==EMFILE||errno==ENFILE)&&(r<0||r--)))
      { *fd=fd[1]= -1;break;
      }
  lcking&=~lck_FILDES;return i;
}

lockit(name,lockp)char*name;const char**const lockp;
{ int i,permanent=2,triedforce=0;struct stat stbuf;time_t t;
  if(*lockp)
   { if(!strcmp(name,*lockp))	/* compare the previous lockfile to this one */
	return;			 /* they're equal, save yourself some effort */
     unlock(lockp);		       /* unlock any previous lockfile FIRST */
   }				  /* to prevent deadlocks (I hate deadlocks) */
  if(!*name)
     return;
  name=tstrdup(name); /* allocate now, so we won't hang on memory *and* lock */
  for(lcking|=lck_LOCKFILE;;)
   { yell("Locking",name);		/* in order to cater for clock skew: */
     if(!NFSxopen(name,LOCKperm,&t))	       /* get time t from filesystem */
      { *lockp=name;break;			   /* lock acquired, hurray! */
      }
     switch(errno)
      { case EEXIST:		   /* check if it's time for a lock override */
	   if(!lstat(name,&stbuf)&&stbuf.st_size<=MAX_LOCK_SIZE&&locktimeout
	    &&!lstat(name,&stbuf)&&locktimeout<t-stbuf.st_mtime)
	     /*
	      * stat() till unlink() should be atomic, but can't guarantee that
	      */
	    { if(triedforce)			/* already tried, not trying */
		 goto faillock;					    /* again */
	      if(S_ISDIR(stbuf.st_mode)||unlink(name))
		 triedforce=1,log("Forced unlock denied on"),logqnl(name);
	      else
	       { log("Forcing lock on");logqnl(name);suspend();goto ce;
	       }
	    }
	   else
	      triedforce=0;		 /* legitimate iteration, clear flag */
	   break;
	default:	       /* maybe filename too long, shorten and retry */
	   if(0<(i=strlen(name)-1)&&!strchr(dirsep,name[i-1]))
	    { log("Truncating");logqnl(name);log(" and retrying lock\n");
	      name[i]='\0';continue;
	    }
faillock:  log("Lock failure on");logqnl(name);goto term;
	case ENOENT:case ENOTDIR:case EIO:case EACCES:
	   if(!--permanent)
	      goto faillock;
	case ENOSPC:;
#ifdef EDQUOT
	case EDQUOT:;
#endif
      }
     sleep((unsigned)locksleep);
ce:  if(nextexit)
term: { free(name);break;		     /* drop the preallocated buffer */
      }
   }
  lcking&=~lck_LOCKFILE;
  if(nextexit)
   { log(whilstwfor);log("lockfile");logqnl(name);terminate();
   }
}

lcllock()				   /* lock a local file (if need be) */
{ if(locknext)
     if(tolock)
	lockit(tolock,&loclock);
     else
	lockit(strcat(buf2,tgetenv(lockext)),&loclock);
}

sterminate()
{ static const char*const msg[]={"memory","fork",	  /* crosscheck with */
   "a file descriptor","a kernel lock"};	  /* lck_ defs in procmail.h */
  ignoreterm();
  if(pidchild>0)	    /* don't kill what is not ours, we might be root */
     kill(pidchild,SIGTERM);
  if(!nextexit)
   { nextexit=1;log("Terminating prematurely");
     if(!(lcking&lck_LOCKFILE))
      { register unsigned i,j;
	if(i=(lcking&~(lck_ALLOCLIB|lck_LOCKFILE))>>1)
	 { log(whilstwfor);
	   for(j=0;(i>>=1)&1;++j);
	   log(msg[j]);
	 }
	log(newline);terminate();
      }
   }
}

terminate()
{ ignoreterm();
  if(retvl2!=EX_OK)
     fakedelivery=0,retval=retvl2;
  if(getpid()==thepid)
   { if(retval!=EX_OK)
      { lastfolder=fakedelivery?"**Lost**":		/* don't free() here */
	 retval==EX_TEMPFAIL?"**Requeued**":"**Bounced**";
      }
     logabstract();nextexit=2;unlock(&loclock);unlock(&globlock);fdunlock();
   }
  exit(fakedelivery==2?EX_OK:retval);
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
  return rc=ropen(name,O_RDONLY,0);
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
     rcbufend=rcbuf+rread(rc,rcbufp=rcbuf,STDBUF);		   /* refill */
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

#ifdef KERNEL_LOCKS
static oldfdlock;				    /* the fd we locked last */
#ifdef F_SETLKW
static struct flock flck;		/* why can't it be a local variable? */

fdlock(fd)					   /* the POSIX-fcntl() lock */
{ flck.l_type=F_WRLCK;flck.l_whence=SEEK_SET;flck.l_len=0;
  flck.l_start=tell(fd);lcking|=lck_KERNELL;
  fd=fcntl(oldfdlock=fd,F_SETLKW,&flck);lcking&=~lck_KERNELL;return fd;
}

fdunlock()
{ flck.l_type=F_UNLCK;return fcntl(oldfdlock,F_SETLK,&flck);
}
#else /* F_SETLKW */
#ifdef F_LOCK
static long oldlockoffset;

fdlock(fd)						 /* the SysV-lockf() */
{ oldlockoffset=tell(fd);lcking|=lck_KERNELL;fd=lockf(oldfdlock=fd,F_LOCK,0L);
  lcking&=~lck_KERNELL;return fd;
}

fdunlock()
{ lseek(oldfdlock,oldlockoffset,SEEK_SET);return lockf(oldfdlock,F_ULOCK,0L);
}
#else /* F_LOCK */
#ifdef LOCK_EX
fdlock(fd)						  /* the BSD-flock() */
{ lcking|=lck_KERNELL;fd=flock(oldfdlock=fd,LOCK_EX);lcking&=~lck_KERNELL;
  return fd;
}

fdunlock()
{ return flock(oldfdlock,LOCK_UN);
}
#endif /* LOCK_EX */
#endif /* F_LOCK */
#endif /* F_SETLKW */
#endif /* KERNEL_LOCKS */
					/* an NFS secure exclusive file open */
NFSxopen(name,mode,tim)char*name;const mode_t mode;time_t*const tim;
{ char*p;int j= -2,i;struct stat stbuf;
  i=lastdirsep(name)-name;strncpy(p=malloc(i+UNIQnamelen),name,i);
  if(unique(p,p+i,mode))	       /* try and rename the unique filename */
   { stat(p,&stbuf);
     if(tim)
	*tim=stbuf.st_mtime;	 /* return the filesystem time to the caller */
     j=myrename(p,name);
   }
  free(p);return j;
}
