/************************************************************************
 *	procmail.c	an autonomous mail processor			*
 *									*
 *	Version 1.30 1991-03-01						*
 *	Seems to be relatively bug free.				*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	See the accompanying README file for more info.			*
 *									*
 *	Don't complain about the formatting, I know it's		*
 *	unconventional, but it's my standard format (I have my		*
 *	reasons).  If you don't like it, feed it through your		*
 *	favourite C beautifier.	 The program has been tested on		*
 *	SUN's, but should work on almost every *NIX system that		*
 *	has a fork() and execvp() command.  The only things that	*
 *	might need some changes are the include files.			*
 *	There might be one problem if your system doesn't know		*
 *	fsync(fd).  Define NOfsync in that case.			*
 *									*
 *	If you have had to make major changes in order to get it	*
 *	running on a different machine, please send me the patches.	*
 *	That way I might include them in a future version.		*
 *									*
 *	Please note that this program essentially is supposed to be	*
 *	static, that means no extra features (IMHO no more are		*
 *	needed) are supposed to be added (though any usefull		*
 *	suggestions will be appreciated and evaluated if time permits). *
 ************************************************************************/
#include "config.h"
#define MAIN
#include "procmail.h"
#include "sysexits.h"
#include "shell.h"

#define VERSION "Procmail v1.30 1991-03-01 written by Stephen R.van den Berg\n"
#define VERSIONOPT	"-v"			/* option to display version */

char buf[BFSIZ],buf2[BFSIZ],maildir[]="MAILDIR",defaultf[]="DEFAULT",
 logfile[]="LOGFILE",lockfile[]="LOCKFILE",grep[]="GREP",host[]="HOST",
 locksleep[]="LOCKSLEEP",orgmail[]="ORGMAIL",eumask[]="UMASK",
 shellmetas[]="SHELLMETAS",shellflags[]="SHELLFLAGS",shell[]="SHELL",
 sendmail[]="SENDMAIL",lockext[]="LOCKEXT",locktimeout[]="LOCKTIMEOUT",
 devnull[]="/dev/null",newline[]="\n",binsh[]="/bin/sh",home[]="HOME",
 tmp[]="/tmp",user[]="USER",nomemretry[]="NOMEMRETRY",*rcfile=PROCMAILRC,
 **gargv,*globlock,*loclock,*tolock;
int retval=EX_CANTCREAT,flaggerd=1,verrgrandchild,sh,pwait,secur,lcking,
 nextexit,locknext;
pid_t mother;
FILE*rc;

main(argc,argv)char*argv[];{static char flags[10];int i;
 char*themail,*thebody,*chp,*startchar,*chp2;long tobesent,filled,rcoffset;
 if((chp=argv[1])&&!strcmp(chp,VERSIONOPT)){
   log(VERSION);return EX_OK;}
 mother=getpid();setbuf(stdin,(void*)0);umask(077);
 sprintf(buf,"%u",i=getuid());setpwent();
 {struct passwd*pass;
 if(pass=getpwuid(i)){			/* find user defaults in /etc/passwd */
   setdef(home,pass->pw_dir);chdir(pass->pw_dir);
   setdef(user,pass->pw_name?pass->pw_name:buf);setdef(shell,pass->pw_shell);}
 else{			 /* user could not be found, set reasonable defaults */
   setdef(home,tmp);chdir(tmp);setdef(user,buf);setdef(shell,binsh);}}
 endpwent();setdef(shellmetas,DEFshellmetas);setdef(shellflags,DEFshellflags);
 setdef(maildir,DEFmaildir);setdef(defaultf,DEFdefault);
 setdef(orgmail,DEForgmail);setdef(grep,DEFgrep);setdef(sendmail,DEFsendmail);
 setdef(lockext,DEFlockext);setdef(locksleep,DEFlocksleeps);
 setdef(locktimeout,DEFlocktimeout);setdef(nomemretry,DEFnomemretry);
 chdir(getenv(maildir));fdreopena(devnull,STDERR);fdreopena(devnull,STDOUT);
 gargv=argv+1;nextrcfile();
 thebody=themail=malloc(argc=1);filled=rcoffset=0;
changedmail:
 themail=readdyn(themail,&filled);			 /* read in the mail */
onlyhead:
 startchar=filled+(thebody=themail);
 while(thebody<startchar&&*thebody++=='\n');	     /* skip leading garbage */
 thebody=findel(thebody,startchar);	 /* find the end of the header	     */
 chp=thebody;
 do{					 /* search for bogus headers	     */
   if(startchar-chp<8)			 /* we're looking for:		     */
      break;				 /* "\n\nFrom +[^\t\n ]+ +[^\n\t]"   */
   if(0>=sscanf(chp,"From%1[ ]",buf))	 /* thats the regular expression     */
      continue;				 /* that defines the start of a mail */
   chp2=chp;chp+=5;			 /* message.			     */
#define SKIPWHILE(x)	while(x){ if(++chp>=startchar) break;}
   SKIPWHILE(*chp==' ')
   SKIPWHILE((i=*chp)&&i!=' '&&i!='\t'&&i!='\n')
   SKIPWHILE(*chp==' ')
   if((i=*chp)&&i!='\n'&&i!='\t'){	   /* insert '>' before bogus header */
      i=startchar[-1];tmemmove(chp2+1,chp2,(startchar-chp2)-1);
      *chp2='>';themail=realloc(chp2=themail,++filled);
#define ADJUST(x)	((x)=themail+((x)-chp2))
      ADJUST(thebody);ADJUST(startchar);ADJUST(chp);*startchar++=i;}}
 while(startchar>(chp=findel(chp,startchar)));
 waitflagger();		 /* if we're a child, wait for the parental guidance */
changerc:
 rc=fopen(strcat(cat(getenv(home),"/"),rcfile),"r");
 fseek(rc,rcoffset,SEEK_SET);signal(SIGINT,sterminate);
 signal(SIGQUIT,sterminate);signal(SIGTERM,sterminate);signal(SIGHUP,SIG_IGN);
goon:
 while(unlock(&loclock),!feof(rc)||argv[argc]){
   while(chp=argv[argc]){	       /* interpret command line specs first */
      argc++;strcpy(buf2,chp);
      if(chp=strchr(buf2,'=')){
	 chp++;goto argenv;}}
   if(tscrc(" %1[:]",flags)){			       /* check for a recipe */
      skipblanks();i=sh=1;
      if(tscrc("%[0-9]",buf2)){
	 sscanf(buf2,"%d",&sh);skipblanks();}
      *flags='\0';scrc("%9[HBIhbfcws]",flags);skipblanks();
      if(tolock)		 /* clear temporary buffer for lockfile name */
	 free(tolock);
      tolock=0;
      if((locknext=(tscrc("%1[:]",buf)))&&
	 (skipblanks(),tscrc("%[^ \t\n#]",buf2))){
	 parse();tolock=tstrdup(buf);}
      startchar=themail;tobesent=thebody-themail;
      if(strchr(flags,'B'))		/* what needs to be piped into grep? */
	 if(strchr(flags,'H'))
	    tobesent=filled;
	 else{
	    startchar=thebody;tobesent=filled-tobesent;}
      while(sh--){				    /* any conditions (left) */
	 skiptoeol();scrc("%[^\n]",buf2);
	 if(!strncmp(buf2,TOkey,TOkeylen))
	    cat(TOsubstitute,buf2+TOkeylen);
	 else
	    strcpy(buf,buf2);
	 if(i)					 /* check out all conditions */
	    i=!grepin(buf,startchar,tobesent,!strchr(flags,'I'));}
      startchar=themail;tobesent=filled;	    /* body, header or both? */
      if(strchr(flags,'h')){
	 if(!strchr(flags,'b'))
	    tobesent=thebody-themail;}
      else if(strchr(flags,'b'))
	 tobesent-=(startchar=thebody)-themail;
      chp=buf+strlen(cat(getenv(sendmail)," "));sh=0;
      pwait=!!strchr(flags,'w');secur=!!strchr(flags,'s');
      if(tscrc(" ! %[^\n]",chp)){			 /* forward the mail */
	 if(i)
	    goto forward;}
      else if(tscrc("| %[^\n]",buf2)){			    /* pipe the mail */
	 if(i){
	    if(sh=!!strpbrk(buf2,getenv(shellmetas)))
	       strcpy(buf,buf2);
	    else
	       parse();
	    chp=buf;*buf2='\0';
	    while(i=*chp)      /* find the implicit lockfile name ('>>name') */
	      if(chp++,i=='>'&&*chp=='>'){
		 while((i=*++chp)==' '||i=='\t');
		 sscanf(chp,"%[^ \t\n#'\");|<>]",buf2);break;}
	    lcllock();
	    if(strchr(flags,'f')){
	       if(startchar==themail&&tobesent!=filled){      /* if only 'h' */
		  char*dest;long dfilled=0;
		  if(pipthrough(buf,startchar,tobesent))
		     goto goon;
		  dest=readdyn(malloc(1),&dfilled);filled-=tobesent;
		  if(tobesent<dfilled)	 /* adjust buffer size (only bigger) */
		     themail=realloc(themail,dfilled+filled);
		  tmemmove(themail+dfilled,themail+tobesent,filled);
		  tmemmove(themail,dest,dfilled);free(dest);
		  themail=realloc(themail,filled+=dfilled);
		  goto onlyhead;}	   /* and determine the header again */
	       rcoffset=ftell(rc);    /* needed because we have to fclose it */
	       if(pipthrough(buf,startchar,tobesent))
		  goto goon;
	       filled=startchar-themail;goto changedmail;}
forward:    if(!pipin(buf,startchar,tobesent)&&!strchr(flags,'c'))
	       goto mailed;}}
      else{					/* append the mail to a file */
	 scrc("%s",buf2);skipcomment();
	 if(i){
	    parse();strcpy(buf2,buf);lcllock();
	    if(!dump(opena(buf),startchar,tobesent)&&!strchr(flags,'c'))
	       goto mailed;}}}
   else if(tscrc("%[A-Z_a-z0-9] = ",buf)){  /* then it must be an assignment */
      *(chp=buf+strlen(buf))='=';*++chp='\0';scrc("%[^\t\n#]",chp);
      for(chp2=chp+strlen(chp);*--chp2==' ';);	     /* skip trailing blanks */
      chp2[1]='\0';skipcomment();strcpy(buf2,buf);
argenv:
      parse();sputenv(buf);chp[-1]='\0';
      if(!strcmp(buf,maildir))
	 chdir(chp);
      else if(!strcmp(buf,logfile))
	 fdreopena(chp,STDERR);
      else if(!strcmp(buf,lockfile))
	 lockit(chp,&globlock);
      else if(!strcmp(buf,eumask)){
	 sscanf(chp,"%o",&i);umask(i);}
      else if(!strcmp(buf,host)){
#ifndef NOuname
	 struct utsname name;
	 uname(&name);
	 if(strcmp(chp,name.nodename)){
#else
	 *buf2='\0';gethostname(buf2,BFSIZ);
	 if(strcmp(chp,buf2)){
#endif
	    if(nextrcfile()){
	       fclose(rc);rcoffset=0;goto changerc;}
	    retval=EX_OK;terminate();}}}
   else if(!skipcomment()){			/* either comment or garbage */
      scrc("%[^\n] ",buf);log("Skipped:");logqnl(buf);}}
 if(dump(opena(chp=getenv(defaultf)),themail,filled)){	 /* default maildest */
   writeerr(chp);	    /* if it fails, don't panic, try the last resort */
   if(dump(opena(chp=getenv(orgmail)),themail,filled))
      writeerr(chp);goto mailerr;}			/* now you can panic */
mailed:
 retval=EX_OK;				  /* we're home free, mail delivered */
mailerr:
 unlock(&loclock);			/* any local lock file still around? */
 do{
   chp=buf-1;
   while(themail<thebody&&chp<buf+BFSIZ-1&&(*++chp=*themail++)!='\n');
   if(chp<buf)
      chp++;
   *chp='\0';
   if(0<sscanf(buf,"From%*1[^:]%[^\n]",buf2)){
      log("From ");goto foundsorf;}
   else if(0<sscanf(buf,"Subject:%[^\n]",buf2)){
      log(" Subject:");
foundsorf:
      log(buf2);log(newline);}}				 /* log it's arrival */
 while(themail<thebody);
 terminate();}
