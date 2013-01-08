/************************************************************************
 *	procmail.c	an autonomous mail processor			*
 *									*
 *	It has been designed to be able to be run suid root and (in	*
 *	case your mail spool area is *not* world writeable) sgid	*
 *	mail (or daemon), without creating security holes.		*
 *									*
 *	Seems to be perfect.						*
 *									*
 *	Copyright (c) 1990-1992, S.R. van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: procmail.c,v 2.37 1992/07/01 12:47:38 berg Rel $";
#endif
#include "config.h"
#define MAIN
#include "procmail.h"
#include "shell.h"
#include "patchlevel.h"

char*buf,*buf2,*globlock,*loclock,*tolock,*lastfolder;
const char shellflags[]="SHELLFLAGS",shell[]="SHELL",
 shellmetas[]="SHELLMETAS",lockext[]="LOCKEXT",newline[]="\n",binsh[]=BinSh,
 unexpeof[]="Unexpected EOL\n",*const*gargv,*sgetcp,*rcfile=PROCMAILRC,
 dirsep[]=DIRSEP,msgprefix[]="MSGPREFIX",devnull[]=DevNull,
 executing[]="Executing",oquote[]=" \"",cquote[]="\"\n",procmailn[]="procmail",
 whilstwfor[]=" whilst waiting for ",sdelivered[]="DELIVERED";
static const char slinebuf[]="LINEBUF",tokey[]=TOkey,eumask[]="UMASK",
 tosubstitute[]=TOsubstitute,lockfile[]="LOCKFILE",defaultf[]="DEFAULT",
 maildir[]="MAILDIR",couldnread[]="Couldn't read",logfile[]="LOGFILE",
 orgmail[]="ORGMAIL",user[]="USER",tmp[]=Tmp,home[]="HOME",sfolder[]=FOLDER,
 sendmail[]="SENDMAIL",host[]="HOST",Log[]="LOG",From[]=FROM,
 exflags[]=RECFLAGS,systm_mbox[]=SYSTEM_MBOX,cldchd[]="Couldn't chdir to",
 pmusage[]=PM_USAGE;
struct varval strenvvar[]={{"LOCKSLEEP",DEFlocksleep},
 {"LOCKTIMEOUT",DEFlocktimeout},{"SUSPEND",DEFsuspend},
 {"NORESRETRY",DEFnoresretry},{"TIMEOUT",DEFtimeout}};
long lastdump;			 /* the no. of bytes dumped (saved) recently */
int retval=EX_CANTCREAT,retvl2=EX_OK,sh,pwait,lcking,locknext,verbose,rc= -2,
 tofolder,tofile,ignwerr,fakedelivery,
 linebuf=mx(DEFlinebuf,STRLEN(systm_mbox)<<1);
volatile int nextexit;			       /* if termination is imminent */
volatile time_t alrmtime;
pid_t thepid,pidchild;
static volatile mailread;	/* if the mail is completely read in already */
static long filled;				   /* the length of the mail */
static char*themail,*thebody;		    /* the head and body of the mail */

main(argc,argv)const char*const argv[];
{ static char flags[maxindex(exflags)];
  static const char*const keepenv[]=KEEPENV,*const prestenv[]=PRESTENV,
   *const trusted_ids[]=TRUSTED_IDS;
  char*chp,*startchar,*chp2,*fromwhom=0;long tobesent;
  int i,lastcond,succeed;uid_t uid;gid_t gid;
#define Deliverymode	lastcond
#define Presenviron	i
#define Privileged	succeed
  Deliverymode=strcmp(lastdirsep(argv[0]),procmailn);
  for(Presenviron=argc=0;(chp=(char*)argv[++argc])&&*chp=='-';)
     for(;;)					       /* processing options */
      { switch(*++chp)
	 { case VERSIONOPT:log(VERSION);return EX_OK;
	   case HELPOPT1:case HELPOPT2:log(pmusage);log(PM_HELP);
	      log(PM_QREFERENCE);return EX_USAGE;
	   case PRESERVOPT:Presenviron=1;continue;
	   case TEMPFAILOPT:retval=EX_TEMPFAIL;continue;
	   case FROMWHOPT:case ALTFROMWHOPT:
	      if(*++chp)
		 fromwhom=chp;
	      else if(chp=(char*)argv[argc+1])
		 ++argc,fromwhom=chp;
	      else
		 log("Missing name\n");
	      break;
	   case DELIVEROPT:Deliverymode=1;++chp;goto last_option;
	   default:log("Unrecognised options:");logqnl(chp);
	      log(pmusage);log("Processing continued\n");
	   case '\0':;
	 }
	break;
      }
last_option:
  if(!Presenviron)				     /* drop the environment */
   { const char**emax=(const char**)environ,*const*ep,*const*kp;
     for(kp=keepenv;*kp;++kp)			     /* preserve a happy few */
	for(i=strlen(*kp),ep=emax;chp2=(char*)*ep;++ep)
	   if(!strncmp(*kp,chp2,i)&&chp2[i]=='=')
	    { *emax++=chp2;break;
	    }
     *emax=0;						    /* drop the rest */
   }
#ifdef LD_ENV_FIX
 {const char**emax=(const char**)environ,**ep;static const char ld_[]="LD_";
  for(ep=emax;*emax;++emax);		  /* find the end of the environment */
  while(*ep)
     if(!strncmp(ld_,*ep++,STRLEN(ld_)))	       /* it starts with LD_ */
	*--ep= *--emax,*emax=0;				/* copy from the end */
 }
#endif /* LD_ENV_FIX */
  if(Deliverymode&&(!chp||(!*chp&&!(chp=(char*)argv[++argc]))))
     Deliverymode=0,log("Missing recipient\n");
 {struct passwd*pass,*passinvk;
  passinvk=getpwuid(uid=getuid());Privileged=1;
  if(*trusted_ids&&uid!=geteuid())
   { struct group*grp;const char*const*kp;
     if(passinvk)			      /* check out the invoker's uid */
	for(chp2=passinvk->pw_name,kp=trusted_ids;*kp;)
	   if(!strcmp(chp2,*kp++))	      /* is it among the privileged? */
	    { endpwent();goto privileged;
	    }
     endpwent();
     if(grp=getgrgid(getgid()))		      /* check out the invoker's gid */
       for(chp2=grp->gr_name,kp=trusted_ids;*kp;)
	  if(!strcmp(chp2,*kp++))	      /* is it among the privileged? */
	   { endgrent();goto privileged;
	   }
    endgrent();Privileged=0;
    if(Deliverymode)
       fromwhom=0;
  }
privileged:
  umask(INIT_UMASK);fclose(stdout);fclose(stderr);rclose(STDOUT);
  rclose(STDERR);
  if(0>opena(devnull)||0>opena(vconsole)&&0>opena(devnull))
     return EX_OSFILE;			  /* couldn't open stdout and stderr */
  setbuf(stdin,(char*)0);buf=malloc(linebuf);buf2=malloc(linebuf);
  thepid=getpid();
#ifdef SIGXCPU
  signal(SIGXCPU,SIG_IGN);signal(SIGXFSZ,SIG_IGN);
#endif
  signal(SIGPIPE,SIG_IGN);signal(SIGTERM,srequeue);signal(SIGINT,sbounce);
  signal(SIGHUP,sbounce);signal(SIGQUIT,slose);signal(SIGALRM,ftimeout);
  ultstr(0,(unsigned long)uid,buf);
  chp2=fromwhom?fromwhom:!passinvk||!*passinvk->pw_name?buf:passinvk->pw_name;
  {time_t t;
  t=time((time_t*)0);startchar=ctime(&t);		 /* the current time */
  }
  strncpy(buf2,chp2,i=linebuf-strlen(startchar)-2);buf2[i]='\0';
  strcat(strcat(buf2," "),startchar);
  thebody=themail=malloc((tobesent=STRLEN(From)+strlen(buf2))+STRLEN(From));
  filled=0;
  if(Deliverymode||fromwhom)	 /* do we need to peek for a leading From_ ? */
   { int r;
     while(1==(r=rread(STDIN,themail,1))&&*themail=='\n');   /* skip garbage */
     if(STRLEN(From)-1==(i=rread(STDIN,themail+1,STRLEN(From)-1))&&
      !strncmp(From,themail,STRLEN(From)))	      /* is it a From_ line? */
      { if(fromwhom||!Privileged)
	 { char a;
	   while(1==rread(STDIN,&a,1)&&a!='\n');   /* discard the From_ line */
	   i=0;goto Frominserted;
	 }
	filled=STRLEN(From);		       /* leave the From_ line alone */
      }
     else		   /* move the read-ahead text beyond our From_ line */
      { tmemmove(themail+tobesent,themail,i=r<=0?0:i>0?i+1:1);
	strcpy(themail,From);			  /* insert From_ of our own */
Frominserted:
	tmemmove(themail+STRLEN(From),buf2,tobesent-STRLEN(From));
	filled=tobesent+i;
      }
   }
  readmail(0,0L);chdir(tmp);		      /* read in the mail completely */
  if(Deliverymode)
     do
      { chp2=chp;
#ifndef NO_USER_TO_LOWERCASE_HACK
	for(;*chp;chp++)
	   if(*chp>='A'&&*chp<='Z')	  /* kludge recipient into lowercase */
	      *chp+='a'-'A';	 /* because getpwnam might be case sensitive */
#endif
	if(argv[++argc])			  /* more than one recipient */
	   if(pidchild=sfork())
	    { if(forkerr(pidchild,procmailn)||waitfor(pidchild)!=EX_OK)
		 retvl2=retval;
	      pidchild=0;		      /* loop for the next recipient */
	    }
	   else
	    { thepid=getpid();
	      while(argv[++argc]);	    /* skip till end of command line */
	    }
      }
     while(chp=(char*)argv[argc]);
  gargv=argv+argc;				 /* save it for nextrcfile() */
  if(geteuid()==ROOT_uid&&Deliverymode&&(pass=getpwnam(chp2))||
   (pass=passinvk))
    /*
     *	set preferred uid to the intended recipient
     */
   { gid=pass->pw_gid;uid=pass->pw_uid;setdef(home,pass->pw_dir);
     chdir(pass->pw_dir);setdef(user,*pass->pw_name?pass->pw_name:buf);
     setdef(shell,pass->pw_shell);
   }
  else			 /* user could not be found, set reasonable defaults */
    /*
     *	set preferred uid to nobody, in case we are running as root
     */
   { setdef(home,tmp);setdef(user,buf);setdef(shell,binsh);
     setgid(gid=NOBODY_gid);setuid(uid=NOBODY_uid);
   }
  endpwent();
 }
  setdef(orgmail,systm_mbox);setdef(shellmetas,DEFshellmetas);
  setdef(shellflags,DEFshellflags);setdef(maildir,DEFmaildir);
  setdef(defaultf,DEFdefault);setdef(sendmail,DEFsendmail);
  setdef(lockext,DEFlockext);setdef(msgprefix,DEFmsgprefix);
 {const char*const*kp;
  for(kp=prestenv;*kp;)	    /* preset or wipe selected environment variables */
     strcpy((char*)(sgetcp=buf2),*kp++),readparse(buf,sgetc,2),sputenv(buf);
 }
  if(chdir(chp=(char*)getenv(maildir)))
     log(cldchd),logqnl(chp);
 /*
  *	check if the original/default mailbox of the recipient exists, if
  *	does, perform some security checks on it (check if it's a regular
  *	file, check if it's owned by the recipient), if something is wrong
  *	try and move the bogus mailbox out of the way,	create the
  *	original/default mailbox file, and chown it to the recipient
  */
  chp=(char*)getenv(orgmail);strncpy(buf,chp,i=lastdirsep(chp)-chp);
 {struct stat stbuf; /* check if the recipient's system mailbox is a link */
  if(!lstat(chp,&stbuf))
     if(!(stbuf.st_mode&S_IWUSR)||S_ISLNK(stbuf.st_mode)||
      (S_ISDIR(stbuf.st_mode)?!(stbuf.st_mode&S_IXUSR):stbuf.st_nlink!=1))
	goto bogusbox;		 /* we only deliver to real files (security) */
     else if(stbuf.st_uid!=uid)		     /* the recipient doesn't own it */
bogusbox:					     /* bogus mailbox found! */
      { ultoan((unsigned long)stbuf.st_ino,		  /* i-node numbered */
	 strchr(strcpy(buf+i,BOGUSprefix),'\0'));
	if(rename(chp,buf))		   /* try and move it out of the way */
	   goto fishy;		 /* couldn't rename, something is fishy here */
      }
     else
	goto notfishy;				       /* everything is fine */
  buf[i]='\0';
  if(!stat(buf,&stbuf)&&
   (stbuf.st_mode&(S_IWGRP|S_IXGRP|S_IWOTH))==(S_IWGRP|S_IXGRP)&&
   stbuf.st_gid==(gid_t)(tobesent=getegid()))
     umask(INIT_UMASK&~S_IRWXG);			    /* keep the gid? */
  else
     tobesent=gid;
 /*
  *	try and create the file, check if it can be chowned to the recipient
  *	if not, then we're either not root or accessing a secure NFS-partition
  *	in the latter case, the created file is owned by nobody, not good, so
  *	we unlink it again, set our uid to the recipient and try again
  */
  if(NFSxopen(chp,NORMperm,(time_t*)0))
     goto fishy;
  if(chown(chp,uid,(gid_t)tobesent)&&
   (unlink(chp),setgid(gid),setuid(uid),NFSxopen(chp,NORMperm,(time_t*)0)))
fishy:
     sputenv(orgmail),sputenv(defaultf);
  umask(INIT_UMASK);
notfishy:
  cat(chp,getenv(lockext));			   /* remove bogus lockfiles */
  if(!lstat(strcpy(buf2,buf),&stbuf)&&stbuf.st_uid!=uid)
   { ultoan((unsigned long)stbuf.st_ino,		  /* i-node numbered */
      strchr(strcpy(buf+i,BOGUSprefix),'\0'));
     rename(buf2,buf);			   /* try and move it out of the way */
   }
 }
  if(!Deliverymode)			       /* not explicit delivery mode */
    /*
     *	really change the uid now, since we are not in explicit
     *	delivery mode
     */
     setgid(gid),setuid(uid),nextrcfile();
  do					     /* main rcfile interpreter loop */
   { alarm((unsigned)(alrmtime=0));			    /* reset timeout */
     while(chp=(char*)argv[argc])      /* interpret command line specs first */
      { argc++;
likearg:
	strcpy(buf,chp);
	if(chp=strchr(buf,'='))
	 { strcpy((char*)(sgetcp=buf2),++chp);readparse(chp,sgetc,2);
	   goto argenv;
	 }
      }
     if(rc<0)						 /* open new rc file */
      { struct stat stbuf;
       /*
	*	if we happen to be still running as root, and the rcfile
	*	is mounted on a secure NFS-partition, we might not be able
	*	to access it, so check if we can stat it, if yes, drop
	*	all rights and set uid to the recipient beforehand
	*/
	while(*buf='\0',stat(strcat(
	 strchr(dirsep,*rcfile)?buf:cat(tgetenv(home),MCDIRSEP),rcfile),
	 &stbuf)||!(stbuf.st_mode&S_IRUSR)?0:(setgid(gid),setuid(uid)),
	 0>bopen(buf))
fake_rc: { log(couldnread);logqnl(buf);
	   if(!nextrcfile())		      /* not available? try the next */
	    { bopen(devnull);goto nomore_rc;
	    }
	 }
       /*
	*	OK, so now we have opened an rcfile, but for security reasons
	*	we only accept it if it is owned by the recipient or if the
	*	the directory it is in, is not world writeable
	*/
	i= *(chp=lastdirsep(buf));
	if(lstat(buf,&stbuf)||
	 (stbuf.st_uid!=uid&&(*chp='\0',stat(buf,&stbuf)||
	 (stbuf.st_mode&(S_IWOTH|S_IXOTH))==(S_IWOTH|S_IXOTH))))
	 { rclose(rc);log("Suspicious rcfile\n");*chp=i;goto fake_rc;
	 }
       /*
	*	set uid back to recipient in any case, since we might just
	*	have opened his/her .procmailrc (don't remove these, since
	*	the rcfile might have been created after the first stat)
	*/
	succeed=lastcond=0;setgid(gid);setuid(uid);
      }
     unlock(&loclock);				/* unlock any local lockfile */
     do skipspace();					  /* skip whitespace */
     while(testb('\n'));
     if(testb(':'))				       /* check for a recipe */
      { readparse(buf,getb,0);sh=strtol(buf,&chp,10);
	if(chp==buf)					 /* no number parsed */
	   sh= -1;
	if(tolock)		 /* clear temporary buffer for lockfile name */
	   free(tolock);
	for(i=maxindex(flags);flags[i]=0,i--;);		  /* clear the flags */
	for(tolock=0,locknext=0;;)
	 { switch(i= *chp++)
	    { default:
		 if(!(chp2=strchr(exflags,i)))	   /* check for a valid flag */
		  { --chp;break;
		  }
		 flags[chp2-exflags]=1;			     /* set the flag */
	      case ' ':case '\t':continue;
	      case '\0':
		 if(*chp!=TMNATE)		/* if not the real end, skip */
		    continue;
		 break;
	      case ':':locknext=1;	    /* yep, local lockfile specified */
		 if(*chp||*++chp!=TMNATE)
		    tolock=tstrdup(chp),chp=strchr(chp,'\0')+1;
	    }
	   if(concatenate(chp))
	      skipped(chp);			    /* display any leftovers */
	   break;
	 }
	if(sh<0)	  /* assume the appropriate default nr of conditions */
	   sh=!flags[ALSO_NEXT_RECIPE]&&!flags[ALSO_N_IF_SUCC];
	startchar=themail;tobesent=thebody-themail;
	if(flags[BODY_GREP])		       /* what needs to be egrepped? */
	   if(flags[HEAD_GREP])
	      tobesent=filled;
	   else
	      startchar=thebody,tobesent=filled-tobesent;
	i=flags[ALSO_NEXT_RECIPE]?lastcond:1;		  /* init test value */
	if(flags[ALSO_N_IF_SUCC])
	   i=lastcond&&succeed;		/* only if the last recipe succeeded */
	while(sh--)				    /* any conditions (left) */
	 { skipspace();getbl(buf2);
	   if(!strncmp(buf2,tokey,STRLEN(tokey)))	     /* magic TOkey? */
	      cat(tosubstitute,buf2+STRLEN(tokey));
	   else if(*buf=='!'&&!strncmp(buf2+1,tokey,STRLEN(tokey)))  /* yes! */
	      strcat(cat("!",tosubstitute),buf2+1+STRLEN(tokey));
	   else
	      strcpy(buf,buf2);
	   if(i)				 /* check out all conditions */
	    { chp=buf+1;
substituted:  strcpy((char*)(sgetcp=buf2),buf);
	      switch(*buf)
	       { default:--chp;		     /* no special character, backup */
		 case '!':case '\\':
		    i=!!egrepin(chp,startchar,tobesent,		  /* grep it */
		     flags[DISTINGUISH_CASE])^*buf=='!';       /* invert it? */
		    break;
		 case '$':*buf2='"';readparse(buf,sgetc,2);goto substituted;
		 case '>':case '<':readparse(buf,sgetc,2);
		    i=strtol(buf+1,&chp,10);
		    i='<'==*buf?filled<i:filled>i;	   /* compare length */
		    while(*chp==' ')
		       ++chp;
		    if(*chp)				    /* any leftover? */
		       skipped(chp);
	       }
	      if(verbose)
		 log(i?"M":"No m"),log("atch on"),logqnl(buf);
	    }
	 }
	if(!flags[ALSO_NEXT_RECIPE]&&!flags[ALSO_N_IF_SUCC])
	   lastcond=i;			   /* save the outcome for posterity */
	startchar=themail;tobesent=filled;	    /* body, header or both? */
	if(flags[PASS_HEAD])
	 { if(!flags[PASS_BODY])
	      tobesent=thebody-themail;
	 }
	else if(flags[PASS_BODY])
	   tobesent-=(startchar=thebody)-themail;
	chp=strchr(strcpy(buf,tgetenv(sendmail)),'\0');succeed=sh=0;
	pwait=flags[WAIT_EXIT]|flags[WAIT_EXIT_QUIET]<<1;
	ignwerr=flags[IGNORE_WRITERR];skipspace();
	if(testb('!'))					 /* forward the mail */
	 { readparse(chp+1,getb,0);
	   if(i)
	      goto forward;
	 }
	else if(testb('|'))				    /* pipe the mail */
	 { getbl(buf2);
	   for(chp=buf2;*(chp=strchr(chp,'\0')-1)=='\\'&&getbl(chp););
	   if(i)
	    { if(sh=!!strpbrk(buf2,tgetenv(shellmetas)))
		 strcpy(buf,buf2);	 /* copy literally, shell will parse */
	      else
		 sgetcp=buf2,readparse(buf,sgetc,0);	/* parse it yourself */
forward:      if(!tolock)	   /* an explicit lockfile specified already */
	       { chp=buf;*buf2='\0';
		 while(i= *chp)	    /* find the implicit lockfile ('>>name') */
		    if(chp++,i=='>'&&*chp=='>')
		     { chp=pstrspn(chp+1," \t");
		       tmemmove(buf2,chp,i=strcspn(chp,EOFName));buf2[i]='\0';
		       if(sh)		 /* expand any environment variables */
			{ chp=tstrdup(buf);sgetcp=buf2;readparse(buf,sgetc,0);
			  strcpy(buf2,buf);strcpy(buf,chp);free(chp);
			}
		       break;
		     }
	       }
	      lcllock();inittmout(buf);
	      if(flags[FILTER])
	       { if(startchar==themail&&tobesent!=filled)     /* if only 'h' */
		  { if(!pipthrough(buf,startchar,tobesent))
		       succeed=1,readmail(1,tobesent);
		  }
		 else if(!pipthrough(buf,startchar,tobesent))
		    succeed=1,filled=startchar-themail,readmail(0,0L);
	       }
	      else if(!pipin(buf,startchar,tobesent)&&
	       (succeed=1,!flags[CONTINUE]))
		 goto mailed;
	    }
	 }
	else		   /* dump the mail into a mailbox file or directory */
	 { readparse(buf,getb,0);
	   if(concatenate(chp=strchr(buf,'\0')+1))
	      skipped(chp);			     /* report any leftovers */
	   if(i)
	    { strcpy(buf2,buf);lcllock();strcpy(buf2,buf);tofile=1;
	      if(dump(deliver(buf2),startchar,tobesent))
		 writeerr(buf);
	      else if(succeed=1,!flags[CONTINUE])
		 goto mailed;
	      tofile=tofolder=0;
	    }
	 }
      }
     else if(testb('#'))				   /* no comment :-) */
	getbl(buf);
     else				    /* then it must be an assignment */
      { for(*(chp=buf)='\0';;)			    /* get the variable name */
	 { switch(i=getb())
	    { case ' ':case '\t':skipspace();i=testb('=')?'=':0;
	      case '\n':case '=':case EOF:*chp='\0';goto eofvarname;
	    }
	   if(!alphanum(*chp++=i))
	      for(;;*chp++=i)			 /* it was garbage after all */
		 switch(i=getb())
		  { case ' ':case '\t':case '\n':case EOF:*chp='\0';
		       skipped(buf);goto mainloop;
		  }
	 }
eofvarname:
	if(i!='=')				   /* removal or assignment? */
	   *++chp='\0';
	else
	   *chp='=',readparse(++chp,getb,1);
argenv: sputenv(buf);chp[-1]='\0';
	if(!strcmp(buf,slinebuf))
	 { if((linebuf=renvint(0L,chp)+XTRAlinebuf)<MINlinebuf+XTRAlinebuf)
	      linebuf=MINlinebuf+XTRAlinebuf;	       /* check minimum size */
	   free(buf);free(buf2);buf=malloc(linebuf);buf2=malloc(linebuf);
	 }
	else if(!strcmp(buf,maildir))
	 { if(chdir(chp))
	      log(cldchd),logqnl(chp);
	 }
	else if(!strcmp(buf,logfile))
	 { close(STDERR);
	   if(verbose=DEBUGPREFIX==*chp)	     /* turn on diagnostics? */
	      chp++;
	   if(0>opena(chp))
	      if(0>opena(vconsole))
		 retval=EX_OSFILE;	  /* bad news, but can't tell anyone */
	      else
		 writeerr(chp);
	 }
	else if(!strcmp(buf,Log))
	   log(chp);
	else if(!strcmp(buf,sdelivered))		    /* fake delivery */
	 { lcking|=lck_LOCKFILE;	    /* just to prevent interruptions */
	   if((thepid=sfork())>0)
	    { nextexit=2;lcking&=~lck_LOCKFILE;return retvl2;
	    }					/* signals may cause trouble */
	   else
	    { if(!forkerr(thepid,procmailn))
		 fakedelivery=1;
	      thepid=getpid();lcking&=~lck_LOCKFILE;
	      if(nextexit)			 /* signals occurred so far? */
		 log(newline),terminate();
	    }
	 }
	else if(!strcmp(buf,lockfile))
	   lockit(chp,&globlock),chown(chp,uid,gid);
	else if(!strcmp(buf,eumask))
	   umask((int)strtol(chp,(char**)0,8));
	else if(!strcmp(buf,host))
	 { if(strncmp(chp,chp2=(char*)hostname(),HOSTNAMElen))
	    { yell("HOST mismatched",chp2);
	      if(rc<0||!nextrcfile())		  /* if no rcfile opened yet */
		 retval=EX_OK,terminate();	  /* exit gracefully as well */
	      rclose(rc);rc= -1;
	    }
	 }
	else
	 { i=MAXvarvals;
	   do				      /* several numeric assignments */
	      if(!strcmp(buf,strenvvar[i].name))
	       { strenvvar[i].val=renvint(strenvvar[i].val,chp);break;
	       }
	   while(i--);
	 }
      }
mainloop:;
   }
  while(rc<0||!testb(EOF));			    /* main interpreter loop */
nomore_rc:
  if(tofile!=2)
   { tofile=2;setuid(uid);chp=DEFdefaultlock;goto likearg;
   }
  if(dump(deliver(tgetenv(defaultf)),themail,filled))		  /* default */
   { writeerr(buf);	    /* if it fails, don't panic, try the last resort */
     if(dump(deliver(tgetenv(orgmail)),themail,filled))
	writeerr(buf);goto mailerr;			/* now you can panic */
   }
mailed:
  retval=EX_OK;				  /* we're home free, mail delivered */
mailerr:
  unlock(&loclock);terminate();
}

logabstract()
{ char*chp,*chp2;int i;
  if(mailread)				  /* is the mail completely read in? */
   { *thebody='\0';		       /* terminate the header, just in case */
     if(!strncmp(From,chp=themail,STRLEN(From)))       /* any "From " header */
      { if(chp=strchr(themail,'\n'))
	   *chp++='\0';
	else
	   chp=thebody;
	log(themail);log(newline);   /* preserve mailbox format (any length) */
      }
     if(!(lcking&lck_ALLOCLIB)&&		/* don't reenter malloc/free */
      (chp=egrepin(NSUBJECT,chp,(long)(thebody-chp),0)))
      { for(chp2= --chp;*--chp2!='\n'&&*chp2;);
	if(chp-++chp2>MAXSUBJECTSHOW)		    /* keep it within bounds */
	   chp2[MAXSUBJECTSHOW]='\0';
	*chp='\0';detab(chp2);log(" ");log(chp2);log(newline);
      }
   }
  log(sfolder);i=strlen(strncpy(buf,lastfolder,MAXfoldlen))+STRLEN(sfolder);
  buf[MAXfoldlen]='\0';detab(buf);log(buf);i-=i%TABWIDTH;	/* last dump */
  do log(TABCHAR);
  while((i+=TABWIDTH)<LENoffset);
  ultstr(7,lastdump,buf);log(buf);log(newline);
}

readmail(onlyhead,tobesent)const int onlyhead;const long tobesent;
{ char*chp,*chp2,*pastend,*realstart;int firstchar;
  mailread=0;
  if(onlyhead)
   { long dfilled=0;
     chp=readdyn(malloc(1),&dfilled);filled-=tobesent;
     if(tobesent<dfilled)		   /* adjust buffer size (grow only) */
	themail=realloc(themail,dfilled+filled);
     tmemmove(themail+dfilled,thebody,filled);
     tmemmove(themail,chp,dfilled);free(chp);
     themail=realloc(themail,1+(filled+=dfilled));
   }
  else
     themail=readdyn(themail,&filled);			 /* read in the mail */
  pastend=filled+(thebody=themail);
  while(thebody<pastend&&*thebody++=='\n');	     /* skip leading garbage */
  realstart=thebody;
  while(thebody=egrepin("[^\n]\n[\n\t ]",thebody,(long)(pastend-thebody),1))
     if(*--thebody!='\n')
	thebody[-1]=' ';		    /* concatenate continuated lines */
     else
	goto eofheader;			   /* empty line marks end of header */
  thebody=pastend;	      /* provide a default, in case there is no body */
eofheader:
  firstchar= *realstart;
  for(*(chp=realstart)='\0';chp=egrepin(FROM_EXPR,chp,(long)(pastend-chp),1);)
   { while(*--chp!='\n');		       /* where did this line start? */
     ++chp;tmemmove(chp+1,chp,pastend++-chp);*chp=ESCAP;	   /* bogus! */
     themail=realloc(chp2=themail,++filled+1);
#define ADJUST(x)	((x)=themail+((x)-chp2))
     ADJUST(thebody);ADJUST(pastend);ADJUST(chp);ADJUST(realstart);
   }
  *realstart=firstchar;mailread=1;
}

dirmail()				/* buf should contain directory name */
{ char*chp;struct stat stbuf;
  if((chp=strchr(buf,'\0')-1)-1>=buf&&chp[-1]==*MCDIRSEP&&*chp=='.')
     *chp='\0',strcpy(buf2,buf);			   /* it ended in /. */
  else
     chp=0,strcpy(buf2,strcat(buf,MCDIRSEP));
  if(unique(buf2,strchr(buf2,'\0'),NORMperm))
   { if(chp)
      { long i=0;		     /* first let us try to prime i with the */
#ifndef NOopendir		     /* highest MH folder number we can find */
	long j;DIR*dirp;struct dirent*dp;char*chp2;
	*chp='\0';yell("Opening directory",buf);
	if(dirp=opendir(buf))
	 { while(dp=readdir(dirp))	/* there still are directory entries */
	      if((j=strtol(dp->d_name,&chp2,10))>i&&!*chp2)
		 i=j;			    /* yep, we found a higher number */
	   closedir(dirp);			     /* aren't we neat today */
	 }
	else
	   log(couldnread),logqnl(buf);
#endif /* NOopendir */
	do ultstr(0,++i,chp);		       /* find first empty MH folder */
	while(link(buf2,buf)&&errno==EEXIST);
	unlink(buf2);goto opn;
      }
     stat(buf2,&stbuf);
     ultoan((unsigned long)stbuf.st_ino,      /* filename with i-node number */
      strchr(strcat(buf,tgetenv(msgprefix)),'\0'));
     if(!myrename(buf2,buf))	       /* rename it, we need the same i-node */
opn:	return opena(buf);
   }
  return -1;
}
