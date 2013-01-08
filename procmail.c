/************************************************************************
 *	procmail.c	an autonomous mail processor			*
 *									*
 *	Seems to be perfect.						*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 *	#include "STYLE"						*
 *									*
 ************************************************************************/
#ifdef	RCS
static char rcsid[]="$Id: procmail.c,v 2.10 1991/07/17 14:58:38 berg Rel $";
#endif
#include "config.h"
#define MAIN
#include "procmail.h"
#include "shell.h"

#define VERSION "procmail v2.11 1991/07/17 written by Stephen R.van den Berg\n\
\t\t\t\tberg@messua.informatik.rwth-aachen.de\n\
\t\t\t\tberg@physik.tu-muenchen.de\n"

char*buf,*buf2,*globlock,*loclock,*tolock,*lastfolder;
const char grep[]="GREP",shellflags[]="SHELLFLAGS",shell[]="SHELL",
 shellmetas[]="SHELLMETAS",lockext[]="LOCKEXT",newline[]="\n",binsh[]=BinSh,
 unexpeof[]="Unexpected EOL\n",*const*gargv,*sgetcp,*rcfile=PROCMAILRC,
 dirsep[]=DIRSEP,msgprefix[]="MSGPREFIX",devnull[]=DevNull,
 executing[]="Executing",oquote[]=" \"",cquote[]="\"\n",
 whilstwfor[]=" whilst waiting for ";
static const char linebufs[]="LINEBUF",tokey[]=TOkey,eumask[]="UMASK",
 tosubstitute[]=TOsubstitute,lockfile[]="LOCKFILE",defaultf[]="DEFAULT",
 maildir[]="MAILDIR",couldnread[]="Couldn't read",logfile[]="LOGFILE",
 orgmail[]="ORGMAIL",user[]="USER",tmp[]=Tmp,home[]="HOME",sfolder[]=FOLDER,
 sendmail[]="SENDMAIL",host[]="HOST",Log[]="LOG",From[]=SFROM;
struct varval strenvvar[]={{"LOCKSLEEP",DEFlocksleep},
 {"LOCKTIMEOUT",DEFlocktimeout},{"SUSPEND",DEFsuspend},
 {"NORESRETRY",DEFnoresretry},{"TIMEOUT",DEFtimeout}};
long lastdump;
int retval=EX_CANTCREAT,sh,pwait,lcking,locknext,verbose,linebuf=DEFlinebuf,
 rc= -1,tofolder;
volatile int flaggerd=2,nextexit;
volatile time_t alrmtime;
pid_t thepid;

main(argc,argv)const char*const argv[];{static char flags[NRRECFLAGS];
 char*themail,*thebody,*chp,*startchar,*chp2;long tobesent,filled;
 int i,lastcond=0;
 if((chp=(char*)argv[argc=1])&&*chp=='-'&&*++chp&&!chp[1])
   switch(*chp){		     /* these options are mutually exclusive */
      case VERSIONOPT:log(VERSION);return EX_OK;
      case DEBUGOPT:verbose=1;
      default:*environ=0;
      case PRESERVOPT:argc++;}
 else
   *environ=0;					     /* drop the environment */
 gargv=argv+argc;umask(077);thepid=getpid();fclose(stdout);fclose(stderr);
 rclose(STDOUT);rclose(STDERR);		    /* don't trust the stdio library */
 if(0>opena(devnull)||0>opena(console))
   return EX_OSFILE;
 setbuf(stdin,(char*)0);buf=malloc(linebuf);buf2=malloc(linebuf);chdir(tmp);
 ultstr(0,(unsigned long)(i=getuid()),buf);
 setpwent();
 {struct passwd*pass;
 if(pass=getpwuid(i)){			/* find user defaults in /etc/passwd */
   setdef(home,pass->pw_dir);chdir(pass->pw_dir);
   setdef(user,pass->pw_name?pass->pw_name:buf);setdef(shell,pass->pw_shell);}
 else{			 /* user could not be found, set reasonable defaults */
   setdef(home,tmp);setdef(user,buf);setdef(shell,binsh);}}
 endpwent();setdef(shellmetas,DEFshellmetas);setdef(shellflags,DEFshellflags);
 setdef(maildir,DEFmaildir);setdef(defaultf,DEFdefault);
 setdef(orgmail,DEForgmail);setdef(grep,DEFgrep);setdef(sendmail,DEFsendmail);
 setdef(lockext,DEFlockext);setdef(msgprefix,DEFmsgprefix);
 chdir(getenv(maildir));nextrcfile();thebody=themail=malloc(1);filled=0;
#ifdef SIGXCPU
 signal(SIGXCPU,SIG_IGN);signal(SIGXFSZ,SIG_IGN);
#endif
 signal(SIGPIPE,SIG_IGN);signal(SIGTERM,sterminate);signal(SIGINT,sterminate);
 signal(SIGHUP,sterminate);signal(SIGQUIT,flagger);signal(SIGALRM,ftimeout);
changedmail:
 themail=readdyn(themail,&filled);			 /* read in the mail */
onlyhead:
 startchar=filled+(thebody=themail);
 while(thebody<startchar&&*thebody++=='\n');	     /* skip leading garbage */
 while(thebody<startchar&&startchar>(thebody=findnl(thebody,startchar)))
   switch(*thebody++){
      case '\n':goto eofheader;		   /* empty line marks end of header */
      case '\t':case ' ':thebody[-2]=' ';}  /* concatenate continuated lines */
eofheader:
 if((chp=thebody)<startchar){
   goto firstel;				 /* search for bogus headers */
   do{
      if(*chp++!='\n'||chp==startchar)		/* is the line really empty? */
	 continue;
firstel:
      *startchar='\0';		   /* put a terminator at the end for sscanf */
      if(0>=sscanf(chp,FromSCAN,buf)){
	 chp--;continue;}		  /* no match, back up, and on we go */
      tmemmove(chp+1,chp,startchar++-chp);*chp='>';	/* insert '>' before */
      themail=realloc(chp2=themail,++filled+1);		     /* bogus header */
#define ADJUST(x)	((x)=themail+((x)-chp2))
      ADJUST(thebody);ADJUST(startchar);ADJUST(chp);}/* find next empty line */
 while(startchar>(chp=findnl(chp,startchar)));}
do{					     /* main rcfile interpreter loop */
   while(chp=(char*)argv[argc]){       /* interpret command line specs first */
      argc++;strcpy(buf,chp);
      if(chp=strchr(buf,'=')){
	 strcpy(sgetcp=buf2,++chp);readparse(chp,sgetc,2);goto argenv;}}
   if(rc<0)						 /* open new rc file */
      while(*buf='\0',0>bopen(strcat(
	 strchr(dirsep,*rcfile)?buf:cat(tgetenv(home),MCDIRSEP),rcfile))){
	 log(couldnread);logqnl(buf);
	 if(!nextrcfile())		      /* not available? try the next */
	    goto nomore_rc;}
   unlock(&loclock);alarm((unsigned)(alrmtime=0));	      /* reset alarm */
   do skipspace();					  /* skip whitespace */
   while(testb('\n'));
   if(testb(':')){				       /* check for a recipe */
      readparse(buf,getb,2);sh= -1;*buf2='\0';
      if(0>=sscanf(buf,"%d%[^\n]",&sh,buf2))		 /* nr of conditions */
	 strcpy(buf2,buf);
      *buf= *flags='\0';
      if(0>=sscanf(buf2,RECFLAGS,flags,buf)){		   /* read the flags */
	 strcpy(buf,buf2);}
      if(sh<0)		  /* assume the appropriate default nr of conditions */
	 sh=!strchr(flags,'A');
      if(tolock)		 /* clear temporary buffer for lockfile name */
	 free(tolock);
      tolock=0;*buf2='\0';
      if((locknext=':'==*buf)&&0<sscanf(buf,": %[^ \t\n] %[^\n]",buf,buf2))
	 tolock=tstrdup(buf);		    /* yep, local lockfile specified */
      if(*buf2)
	 skipped(buf2);				    /* display any leftovers */
      startchar=themail;tobesent=thebody-themail;
      if(strchr(flags,'B'))		/* what needs to be piped into grep? */
	 if(strchr(flags,'H'))
	    tobesent=filled;
	 else{
	    startchar=thebody;tobesent=filled-tobesent;}
      i=strchr(flags,'A')?lastcond:1;			  /* init test value */
      while(sh--){				    /* any conditions (left) */
	 getbl(buf2);
	 if(!strncmp(buf2,tokey,STRLEN(tokey)))		     /* magic TOkey? */
	    cat(tosubstitute,buf2+STRLEN(tokey));
	 else if(*buf=='!'&&!strncmp(buf2+1,tokey,STRLEN(tokey))) /* !TOkey? */
	    strcat(cat("!",tosubstitute),buf2+1+STRLEN(tokey));
	 else
	    strcpy(buf,buf2);
	 if(i){					 /* check out all conditions */
	    i=!grepin((*buf=='!'||*buf=='\\')+buf,startchar,tobesent,
	       !!strchr(flags,'D'))^*buf=='!';
	    if(verbose){
	       log(i?"M":"No m");log("atch on");logqnl(buf);}}}
      if(!strchr(flags,'A'))		   /* save the outcome for posterity */
	 lastcond=i;
      startchar=themail;tobesent=filled;	    /* body, header or both? */
      if(strchr(flags,'h')){
	 if(!strchr(flags,'b'))
	    tobesent=thebody-themail;}
      else if(strchr(flags,'b'))
	 tobesent-=(startchar=thebody)-themail;
      chp=strchr(strcpy(buf,tgetenv(sendmail)),'\0');sh=0;
      pwait=!!strchr(flags,'w');skipspace();
      if(testb('!')){					 /* forward the mail */
	 readparse(chp+1,getb,0);
	 if(i)
	    goto forward;}
      else if(testb('|')){				    /* pipe the mail */
	 getbl(buf2);
	 for(chp=buf2;*(chp=strchr(chp,'\0')-1)=='\\'&&getbl(chp););
	 if(i){
	    if(sh=!!strpbrk(buf2,tgetenv(shellmetas)))
	       strcpy(buf,buf2);	 /* copy literally, shell will parse */
	    else{
	       sgetcp=buf2;readparse(buf,sgetc,0);}	/* parse it yourself */
forward:    chp=buf;*buf2='\0';
	    while(i= *chp)     /* find the implicit lockfile name ('>>name') */
	      if(chp++,i=='>'&&*chp=='>'){
		 while((i= *++chp)==' '||i=='\t');
		 sscanf(chp,EOFName,buf2);break;}
	    lcllock();inittmout(buf);
	    if(strchr(flags,'f')){
	       if(startchar==themail&&tobesent!=filled){      /* if only 'h' */
		  long dfilled=0;
		  if(pipthrough(buf,startchar,tobesent))
		     continue;
		  chp=readdyn(malloc(1),&dfilled);filled-=tobesent;
		  if(tobesent<dfilled)	   /* adjust buffer size (grow only) */
		     themail=realloc(themail,dfilled+filled);
		  tmemmove(themail+dfilled,thebody,filled);
		  tmemmove(themail,chp,dfilled);free(chp);
		  themail=realloc(themail,1+(filled+=dfilled));goto onlyhead;}
	       if(pipthrough(buf,startchar,tobesent))
		  continue;
	       filled=startchar-themail;goto changedmail;}
	    if(!pipin(buf,startchar,tobesent)&&!strchr(flags,'c'))
	       goto mailed;}}
      else{		   /* dump the mail into a mailbox file or directory */
	 readparse(buf,getb,0);chp2=chp=strchr(buf,'\0')+1;
	 while(*chp!=TMNATE){		  /* concatenate all other arguments */
	    while(*chp++);
	    chp[-1]=' ';}
	 *chp=chp[-1]='\0';
	 if(*chp2)
	    skipped(chp2);			     /* report any leftovers */
	 if(i){
	    strcpy(buf2,buf);lcllock();strcpy(buf2,buf);
	    if(dump(deliver(buf2),startchar,tobesent))
	       writeerr(buf);
	    else if(!strchr(flags,'c'))
	       goto mailed;
	    tofolder=0;}}}
   else if(testb('#'))					   /* no comment :-) */
      getbl(buf);
   else{				    /* then it must be an assignment */
      for(*(chp=buf)='\0';;){			    /* get the variable name */
	 switch(i=getb()){
	    case ' ':case '\t':skipspace();i=testb('=')?'=':0;
	    case '\n':case '=':case EOF:
	       *chp='\0';goto eofvarname;}
	 if(!alphanum(*chp++=i))
	    for(;;*chp++=i)			 /* it was garbage after all */
	       switch(i=getb()){
		  case ' ':case '\t':case '\n':case EOF:*chp='\0';
		     skipped(buf);goto mainloop;}}
eofvarname:
      if(i!='='){				   /* removal or assignment? */
	 sputenv(buf);continue;}
      *chp='=';readparse(++chp,getb,1);
argenv:
      sputenv(buf);chp[-1]='\0';
      if(!strcmp(buf,linebufs)){
	 if((linebuf=renvint(0L,chp)+XTRAlinebuf)<MINlinebuf+XTRAlinebuf)
	    linebuf=MINlinebuf+XTRAlinebuf;	       /* check minimum size */
	 free(buf);free(buf2);buf=malloc(linebuf);buf2=malloc(linebuf);}
      else if(!strcmp(buf,maildir)){
	 if(chdir(chp)){
	    log("Couldn't chdir to");logqnl(chp);}}
      else if(!strcmp(buf,logfile)){
	 close(STDERR);
	 if(0>opena(chp))
	    if(0>opena(console))
	       retval=EX_OSFILE;	  /* bad news, but can't tell anyone */
	    else
	       writeerr(chp);}
      else if(!strcmp(buf,Log))
	 log(chp);
      else if(!strcmp(buf,lockfile))
	 lockit(chp,&globlock);
      else if(!strcmp(buf,eumask)){
	 sscanf(chp,"%o",&i);umask(i);}
      else if(!strcmp(buf,host)){
	 if(strcmp(chp,chp2=(char*)hostname())){
	    yell("HOST mismatched",chp2);
	    if(rc<0||!nextrcfile()){		  /* if no rcfile opened yet */
	       retval=EX_OK;terminate();}	  /* exit gracefully as well */
	    rclose(rc);rc= -1;}}
      else{
	 i=MAXvarvals;
	 do				      /* several numeric assignments */
	    if(!strcmp(buf,strenvvar[i].name)){
	       strenvvar[i].val=renvint(strenvvar[i].val,chp);break;}
	 while(i--);}}
mainloop:
    ;}
 while(rc<0||!testb(EOF));			    /* main interpreter loop */
nomore_rc:
 if(dump(deliver(tgetenv(defaultf)),themail,filled)){		  /* default */
   writeerr(buf);	    /* if it fails, don't panic, try the last resort */
   if(dump(deliver(tgetenv(orgmail)),themail,filled))
      writeerr(buf);goto mailerr;}			/* now you can panic */
mailed:
 retval=EX_OK;				  /* we're home free, mail delivered */
mailerr:
 unlock(&loclock);*thebody='\0';       /* Terminate the header, just in case */
 if(!strncmp(From,chp=themail,STRLEN(From))){  /* Check for a "From " header */
   if(chp=strchr(themail,'\n'))
      *chp++='\0';
   else
      chp=thebody;
   log(themail);log(newline);} /* preserve mailbox format (arbitrary length) */
 while(chp<thebody){
   if(0<sscanf(chp,SSUBJECT_S,buf2)){	    /* case insensitive "Subject:" ? */
      log(SSUBJECT);log(buf2);log(newline);}		  /* log the Subject */
   chp=findnl(chp,thebody);}
 log(sfolder);i=strlen(strncpy(buf,lastfolder,MAXfoldlen))+STRLEN(sfolder);
 buf[MAXfoldlen]='\0';
 while(chp=strchr(buf,'\t'))
   *chp=' ';						/* take out all tabs */
 log(buf);i-=i%TABWIDTH;		     /* tell where we last dumped it */
 do log(TABCHAR);
 while((i+=TABWIDTH)<LENoffset);
 ultstr(7,lastdump,buf);log(buf);log(newline);terminate();}

dirmail(){char*chp;struct stat stbuf;	/* buf should contain directory name */
 if((chp=strchr(buf,'\0')-1)-1>=buf&&chp[-1]==*MCDIRSEP&&*chp=='.'){
   *chp='\0';strcpy(buf2,buf);}				   /* it ended in /. */
 else{
   chp=0;strcpy(buf2,strcat(buf,MCDIRSEP));}
 if(unique(buf2,strchr(buf2,'\0'),0666)){
   if(chp){unsigned long i=0;
      do ultstr(0,++i,chp);		       /* find first empty MH folder */
      while(link(buf2,buf));
      unlink(buf2);goto opn;}
   stat(buf2,&stbuf);
   ultoan((unsigned long)stbuf.st_ino,	      /* filename with i-node number */
      strchr(strcat(buf,tgetenv(msgprefix)),'\0'));
   if(!myrename(buf2,buf))	       /* rename it, we need the same i-node */
opn:  return opena(buf);}
 return -1;}
