/************************************************************************
 *	procmail.c	an autonomous mail processor			*
 *									*
 *	Seems to be perfect.						*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: procmail.c,v 2.17 1991/10/22 15:31:26 berg Rel $";
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
 dirsep[]=DIRSEP,msgprefix[]="MSGPREFIX",devnull[]=DevNull,Mail[]="Mail ",
 executing[]="Executing",oquote[]=" \"",cquote[]="\"\n",procmailn[]="procmail",
 whilstwfor[]=" whilst waiting for ",delivered[]="DELIVERED";
static const char linebufs[]="LINEBUF",tokey[]=TOkey,eumask[]="UMASK",
 tosubstitute[]=TOsubstitute,lockfile[]="LOCKFILE",defaultf[]="DEFAULT",
 maildir[]="MAILDIR",couldnread[]="Couldn't read",logfile[]="LOGFILE",
 orgmail[]="ORGMAIL",user[]="USER",tmp[]=Tmp,home[]="HOME",sfolder[]=FOLDER,
 sendmail[]="SENDMAIL",host[]="HOST",Log[]="LOG",From[]=FROM,
 exflags[]=RECFLAGS;
struct varval strenvvar[]={{"LOCKSLEEP",DEFlocksleep},
 {"LOCKTIMEOUT",DEFlocktimeout},{"SUSPEND",DEFsuspend},
 {"NORESRETRY",DEFnoresretry},{"TIMEOUT",DEFtimeout}};
long lastdump;
int retval=EX_CANTCREAT,sh,pwait,lcking,locknext,verbose,linebuf=DEFlinebuf,
 rc= -2,tofolder,ignwerr,fakedelivery;
volatile int nextexit;
volatile time_t alrmtime;
pid_t thepid;

main(argc,argv)const char*const argv[];
{ static char flags[maxindex(exflags)-1];
  char*themail,*thebody,*chp,*startchar,*chp2;long tobesent,filled;
  int i,lastcond;uid_t uid;gid_t gid;
  for(lastcond=i=argc=0;(chp=(char*)argv[++argc])&&*chp=='-';)
     for(;;)					       /* processing options */
      { switch(*++chp)
	 { case VERSIONOPT:log(VERSION);return EX_OK;
	   case PRESERVOPT:i=1;continue;
	   case TEMPFAILOPT:retval=EX_TEMPFAIL;continue;
	   case DELIVEROPT:
 /*
  *	if delivery is not to the current uid, this option can be specified;
  *	for security reasons there may be NO command line arguments following
  *	it
  *	more command line arguments are only allowed if the uid is to be
  *	the recipient (i.e. the recipient started it itself, e.g. from its
  *	.forward file)
  */
	      if(!chp[1]&&argv[++argc]&&!argv[argc+1])
	       { lastcond=1;chp2=(char*)argv[argc];break;	 /* save the */
	       }					 /* recipient's name */
	   default:log("Unrecognised options:");logqnl(chp);log(PROCMAIL_USAGE);
	      log("Processing continued\n");
	   case '\0':;
	 }
	break;
      }
  if(!i)
     *environ=0;				     /* drop the environment */
  gargv=argv+argc;umask(INIT_UMASK);thepid=getpid();fclose(stdout);
  fclose(stderr);rclose(STDOUT);rclose(STDERR);		     /* sure is sure */
  if(0>opena(devnull)||0>opena(console)&&0>opena(devnull))
     return EX_OSFILE;			  /* couldn't open stdout and stderr */
  setbuf(stdin,(char*)0);buf=malloc(linebuf);buf2=malloc(linebuf);chdir(tmp);
  ultstr(0,(unsigned long)(i=getuid()),buf);setpwent();
 {struct passwd*pass;
  if(lastcond&&(pass=getpwnam(chp2))||(pass=getpwuid(i)))
    /*
     *	set preferred uid to the intended recipient
     */
   { gid=pass->pw_gid;uid=pass->pw_uid;
     setdef(home,pass->pw_dir);chdir(pass->pw_dir);
     setdef(user,pass->pw_name?pass->pw_name:buf);setdef(shell,pass->pw_shell);
   }
  else			 /* user could not be found, set reasonable defaults */
    /*
     *	set preferred uid to nobody, just in case we are running as root
     */
   { setdef(home,tmp);setdef(user,buf);setdef(shell,binsh);
     gid=NOBODY_gid;uid=NOBODY_uid;
   }
 /*
  *	create the original/default mailbox file, chown it to the recipient
  */
  setdef(orgmail,SYSTEM_MBOX);close(opena(chp=getenv(orgmail)));
  chown(chp,uid,gid);
 }
 /*
  *	inject the one and only allowed command line argument if we are
  *	doing delivery to an explicit recipient
  */
  if(!lastcond)				       /* not explicit delivery mode */
    /*
     *	really change the uid now, since we are not in explicit delivery mode
     */
   { setgid(gid);setuid(uid);
   }
  endpwent();setdef(shellmetas,DEFshellmetas);setdef(shellflags,DEFshellflags);
  setdef(maildir,DEFmaildir);setdef(defaultf,DEFdefault);
  setdef(sendmail,DEFsendmail);setdef(lockext,DEFlockext);
  setdef(msgprefix,DEFmsgprefix);chdir(getenv(maildir));nextrcfile();
  thebody=themail=malloc(1);filled=0;
#ifdef SIGXCPU
  signal(SIGXCPU,SIG_IGN);signal(SIGXFSZ,SIG_IGN);
#endif
  signal(SIGPIPE,SIG_IGN);signal(SIGTERM,sterminate);signal(SIGINT,sterminate);
  signal(SIGHUP,sterminate);signal(SIGQUIT,sterminate);
  signal(SIGALRM,ftimeout);
changedmail:
  themail=readdyn(themail,&filled);			 /* read in the mail */
onlyhead:
  startchar=filled+(thebody=themail);
  while(thebody<startchar&&*thebody++=='\n');	     /* skip leading garbage */
  while(thebody=egrepin("[^\n]\n[\n\t ]",thebody,(long)(startchar-thebody),1))
     if(*--thebody!='\n')
	thebody[-1]=' ';		    /* concatenate continuated lines */
     else
	goto eofheader;			   /* empty line marks end of header */
  thebody=startchar;
eofheader:
  for(chp=mx(themail,thebody-1);
   chp=egrepin(FROM_EXPR,chp,(long)(startchar-chp),1);)
   { while(*--chp!='\n');		       /* where did this line start? */
     ++chp;tmemmove(chp+1,chp,startchar++-chp);*chp=ESCAP;  /* bogus header! */
     themail=realloc(chp2=themail,++filled+1);
#define ADJUST(x)	((x)=themail+((x)-chp2))
     ADJUST(thebody);ADJUST(startchar);ADJUST(chp);
   }
  do					     /* main rcfile interpreter loop */
   { while(chp=(char*)argv[argc])      /* interpret command line specs first */
      { argc++;
likearg:
	strcpy(buf,chp);
	if(chp=strchr(buf,'='))
	 { strcpy(sgetcp=buf2,++chp);readparse(chp,sgetc,2);goto argenv;
	 }
      }
     if(rc<0)						 /* open new rc file */
      {  i=rc;
	 while(*buf='\0',0>bopen(strcat(
	  strchr(dirsep,*rcfile)?buf:cat(tgetenv(home),MCDIRSEP),rcfile)))
	 { log(couldnread);logqnl(buf);
	   if(!nextrcfile())		      /* not available? try the next */
	    { if(i==-2&&!lastcond)     /* no rcfile & explicit delivery mode */
	       { chp=DEFdefaultlock;goto likearg;	     /* acquire lock */
	       }
	      goto nomore_rc;
	    }
	 }
       /*
	*	set uid back to recipient in any case, since we might just
	*	have opened his/her .procmailrc
	*/
	lastcond=0;setgid(gid);setuid(uid);
      }
     unlock(&loclock);alarm((unsigned)(alrmtime=0));	      /* reset alarm */
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
		  { tolock=tstrdup(chp);chp=strchr(chp,'\0')+1;
		  }
	    }
	   if(concatenate(chp))
	      skipped(chp);			    /* display any leftovers */
	   break;
	 }
	if(sh<0)				   /* assume the appropriate */
	   sh=!flags[ALSO_NEXT_RECIPE];		 /* default nr of conditions */
	startchar=themail;tobesent=thebody-themail;
	if(flags[BODY_GREP])		       /* what needs to be egrepped? */
	   if(flags[HEAD_GREP])
	      tobesent=filled;
	   else
	    { startchar=thebody;tobesent=filled-tobesent;
	    }
	i=flags[ALSO_NEXT_RECIPE]?lastcond:1;		  /* init test value */
	while(sh--)				    /* any conditions (left) */
	 { skipspace();getbl(buf2);
	   if(!strncmp(buf2,tokey,STRLEN(tokey)))	     /* magic TOkey? */
	      cat(tosubstitute,buf2+STRLEN(tokey));
	   else if(*buf=='!'&&!strncmp(buf2+1,tokey,STRLEN(tokey)))  /* yes! */
	      strcat(cat("!",tosubstitute),buf2+1+STRLEN(tokey));
	   else
	      strcpy(buf,buf2);
	   if(i)				 /* check out all conditions */
	    { i=!!egrepin((*buf=='!'||*buf=='\\')+buf,startchar,tobesent,
	       flags[DISTINGUISH_CASE])^*buf=='!';
	      if(verbose)
	       { log(i?"M":"No m");log("atch on");logqnl(buf);
	       }
	    }
	 }
	if(!flags[ALSO_NEXT_RECIPE])	   /* save the outcome for posterity */
	   lastcond=i;
	startchar=themail;tobesent=filled;	    /* body, header or both? */
	if(flags[PASS_HEAD])
	 { if(!flags[PASS_BODY])
	      tobesent=thebody-themail;
	 }
	else if(flags[PASS_BODY])
	   tobesent-=(startchar=thebody)-themail;
	chp=strchr(strcpy(buf,tgetenv(sendmail)),'\0');sh=0;
	pwait=flags[WAIT_EXIT];ignwerr=flags[IGNORE_WRITERR];skipspace();
	if(testb('!'))					 /* forward the mail */
	 { readparse(chp+1,getb,0);
	   if(i)
	      goto forward;}
	else if(testb('|'))				    /* pipe the mail */
	 { getbl(buf2);
	   for(chp=buf2;*(chp=strchr(chp,'\0')-1)=='\\'&&getbl(chp););
	   if(i)
	    { if(sh=!!strpbrk(buf2,tgetenv(shellmetas)))
		 strcpy(buf,buf2);	 /* copy literally, shell will parse */
	      else
	       { sgetcp=buf2;readparse(buf,sgetc,0);	/* parse it yourself */
	       }
forward:      chp=buf;*buf2='\0';
	      while(i= *chp)   /* find the implicit lockfile name ('>>name') */
		 if(chp++,i=='>'&&*chp=='>')
		  { chp=pstrspn(chp+1," \t");
		    tmemmove(buf2,chp,i=strcspn(chp,EOFName));chp[i]='\0';
		    if(sh)		 /* expand any environment variables */
		     { chp=tstrdup(buf);sgetcp=buf2;readparse(buf,sgetc,0);
		       strcpy(buf2,buf);strcpy(buf,chp);free(chp);
		     }
		    break;
		  }
	      lcllock();inittmout(buf);
	      if(flags[FILTER])
	       { if(startchar==themail&&tobesent!=filled)     /* if only 'h' */
		  { long dfilled=0;
		    if(pipthrough(buf,startchar,tobesent))
		       continue;
		    chp=readdyn(malloc(1),&dfilled);filled-=tobesent;
		    if(tobesent<dfilled)   /* adjust buffer size (grow only) */
		       themail=realloc(themail,dfilled+filled);
		    tmemmove(themail+dfilled,thebody,filled);
		    tmemmove(themail,chp,dfilled);free(chp);
		    themail=realloc(themail,1+(filled+=dfilled));goto onlyhead;
		  }
		 if(pipthrough(buf,startchar,tobesent))
		    continue;
		 filled=startchar-themail;goto changedmail;
	       }
	      if(!pipin(buf,startchar,tobesent)&&!flags[CONTINUE])
		 goto mailed;
	    }
	 }
	else		   /* dump the mail into a mailbox file or directory */
	 { readparse(buf,getb,0);
	   if(concatenate(chp=strchr(buf,'\0')+1))
	      skipped(chp);			     /* report any leftovers */
	   if(i)
	    { strcpy(buf2,buf);lcllock();strcpy(buf2,buf);
	      if(dump(deliver(buf2),startchar,tobesent))
		 writeerr(buf);
	      else if(!flags[CONTINUE])
		 goto mailed;
	      tofolder=0;
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
	 { sputenv(buf);continue;
	 }
	*chp='=';readparse(++chp,getb,1);
argenv: sputenv(buf);chp[-1]='\0';
	if(!strcmp(buf,linebufs))
	 { if((linebuf=renvint(0L,chp)+XTRAlinebuf)<MINlinebuf+XTRAlinebuf)
	      linebuf=MINlinebuf+XTRAlinebuf;	       /* check minimum size */
	   free(buf);free(buf2);buf=malloc(linebuf);buf2=malloc(linebuf);
	 }
	else if(!strcmp(buf,maildir))
	 { if(chdir(chp))
	    { log("Couldn't chdir to");logqnl(chp);
	    }
	 }
	else if(!strcmp(buf,logfile))
	 { close(STDERR);
	   if(verbose=':'==*chp)		     /* turn on diagnostics? */
	      chp++;;
	   if(0>opena(chp))
	      if(0>opena(console))
		 retval=EX_OSFILE;	  /* bad news, but can't tell anyone */
	      else
		 writeerr(chp);}
	else if(!strcmp(buf,Log))
	   log(chp);
	else if(!strcmp(buf,delivered))			    /* fake delivery */
	 { lcking=1;
	   if((thepid=sfork())>0)
	    { nextexit=2;lcking=0;return EX_OK; /* signals may cause trouble */
	    }
	   else
	    { if(!forkerr(thepid,procmailn))
		 fakedelivery=1;
	      thepid=getpid();lcking=0;
	      if(nextexit)			 /* signals occurred so far? */
	       { log(newline);terminate();
	       }
	    }
	 }
	else if(!strcmp(buf,lockfile))
	 { lockit(chp,&globlock);chown(chp,uid,gid);
	 }
	else if(!strcmp(buf,eumask))
	   umask((int)strtol(chp,(char**)0,8));
	else if(!strcmp(buf,host))
	 { if(strcmp(chp,chp2=(char*)hostname()))
	    { yell("HOST mismatched",chp2);
	      if(rc<0||!nextrcfile())		  /* if no rcfile opened yet */
	       { retval=EX_OK;terminate();	  /* exit gracefully as well */
	       }
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
  if(dump(deliver(tgetenv(defaultf)),themail,filled))		  /* default */
   { writeerr(buf);	    /* if it fails, don't panic, try the last resort */
     if(dump(deliver(tgetenv(orgmail)),themail,filled))
	writeerr(buf);goto mailerr;			/* now you can panic */
   }
mailed:
  retval=EX_OK;				  /* we're home free, mail delivered */
mailerr:
  unlock(&loclock);*thebody='\0';      /* Terminate the header, just in case */
  if(!strncmp(From,chp=themail,STRLEN(From)))  /* Check for a "From " header */
   { if(chp=strchr(themail,'\n'))
	*chp++='\0';
     else
	chp=thebody;
     log(themail);log(newline);	     /* preserve mailbox format (any length) */
   }
  if(chp=egrepin(NSUBJECT,chp,(long)(thebody-chp),0))
   { for(chp2= --chp;*--chp2!='\n';);
     if(chp-++chp2>MAXSUBJECTSHOW)		    /* keep it within bounds */
	chp2[MAXSUBJECTSHOW]='\0';
     *chp='\0';detab(chp2);log(" ");log(chp2);log(newline);
   }
  log(sfolder);i=strlen(strncpy(buf,lastfolder,MAXfoldlen))+STRLEN(sfolder);
  buf[MAXfoldlen]='\0';detab(buf);log(buf);i-=i%TABWIDTH;	/* last dump */
  do log(TABCHAR);
  while((i+=TABWIDTH)<LENoffset);
  ultstr(7,lastdump,buf);log(buf);log(newline);terminate();
}

dirmail()				/* buf should contain directory name */
{ char*chp;struct stat stbuf;
  if((chp=strchr(buf,'\0')-1)-1>=buf&&chp[-1]==*MCDIRSEP&&*chp=='.')
   { *chp='\0';strcpy(buf2,buf);			   /* it ended in /. */
   }
  else
   { chp=0;strcpy(buf2,strcat(buf,MCDIRSEP));
   }
  if(unique(buf2,strchr(buf2,'\0'),0666))
   { if(chp)
      { unsigned long i=0;
	do ultstr(0,++i,chp);		       /* find first empty MH folder */
	while(link(buf2,buf));
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
