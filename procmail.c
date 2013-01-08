/************************************************************************
 *      procmail.c      an autonomous mail processor                    *
 *                                                                      *
 *      Version 1.10 1991-02-04                                         *
 *      Seems to be relatively bug free.                                *
 *                                                                      *
 *      Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands      *
 *      The sources can be freely copied for non-commercial use.        *
 *                                                                      *
 *      Don't complain about the formatting, I know it's                *
 *      unconventional, but it's my standard format (I have my          *
 *      reasons).  If you don't like it, feed it through your           *
 *      favourite C beautifier.  The program has been tested on         *
 *      SUN's, but should work on almost every *NIX system that         *
 *      has a fork() and execvp() command.  The only things that        *
 *      might need some changes are the include files.                  *
 *      There might be one problem if your system doesn't know          *
 *      fsync(fd).  Define NOfsync in that case.                        *
 *                                                                      *
 *      If you have had to make major changes in order to get it        *
 *      running on a different machine, please send me the patches.     *
 *      That way I might include them in a future version.              *
 *                                                                      *
 *      Please note that this program essentially is supposed to be     *
 *      static, that means no extra features (IMHO no more are          *
 *      needed) are supposed to be added.                               *
 ************************************************************************/
#ifdef  NOsync                  /* If you don't want syncs at all define */
#define fsync(fd) 0             /* NOsync.  Only recommended if procmail */
#define sync() 0                /* isn't used in a networked environment */
#else
# ifdef NOfsync                 /* If you don't have fsync, define NOfsync */
# define fsync(fd) 0            /* sync will be used instead.  Is a bit    */
# endif                         /* slower, but works nevertheless          */
#endif

#include <stdio.h>
#ifndef NO_ANSI_PROT            /* define this if your headers are not ansi */
# include <stdlib.h>
#else
  char*getenv();
  typedef int pid_t;
#endif
#include <fcntl.h>
#include <pwd.h>
#include <sys/wait.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#ifndef SYSV
#include <sysexits.h>
#else
#include <bsd/sysexits.h>
#endif
#include "sysexits.h"
#ifndef SEEK_SET
#define SEEK_SET        0
#endif
extern char**environ;
#define STDIN   0
#define STDOUT  1
#define STDERR  2
typedef unsigned t_buf; /* for malloc, realloc and memmove
                          This type determines the maximum message length */

#define BFSIZ           2048            /* max expanded line length */
#define BLKSIZ          (1<<14)         /* blocksize while reading/writing */
#define PROCMAILRC      ".procmailrc"
#define SUSPENSION      64
#define DEFlocksleep    8
#define DEFlocksleeps   "8"
#define TOkey           "^TO"
#define TOkeylen        3
#define TOsubstitute    "^(To|Cc|Apparently-To):.*"
#define DEFshellmeta    "\"'`&#{}()[]*?|<>~;!\\"   /* never put '$' in here */
#define DEFmaildir      "$HOME"
#define DEFdefault      "$MAILDIR/.mailbox"
#define DEForgmail      "/var/spool/mail/$USER"
#define DEFgrep         "/usr/bin/egrep"
#define DEFsendmail     "/usr/lib/sendmail"
#define DEFlockext      ".lock"
#define DEFshellflags   "-c"

char buf[BFSIZ],buf2[BFSIZ],maildir[]="MAILDIR",defaultf[]="DEFAULT",
 logfile[]="LOGFILE",lockfile[]="LOCKFILE",grep[]="GREP",host[]="HOST",
 locksleep[]="LOCKSLEEP",orgmail[]="ORGMAIL",eumask[]="UMASK",
 shellmeta[]="SHELLMETA",shellflags[]="SHELLFLAGS",shell[]="SHELL",
 sendmail[]="SENDMAIL",lockext[]="LOCKEXT",devnull[]="/dev/null",
 newline[]="\n",binsh[]="/bin/sh",home[]="HOME",tmp[]="/tmp",user[]="USER",
 **gargv,*rcfile=PROCMAILRC,*globlock,*loclock,*tolock;
#define PREAD   (poutfd[0])
#define PWRITE  (poutfd[1])
#define tscrc(a,b)      (0<fscanf(rc,a,b))
#define scrc(a,b)       fscanf(rc,a,b)
int retval=EX_CANTCREAT,flaggerd=1,verrgrandchild,sh,pwait,secur,locking,
 nextexit,locknext;
pid_t mother;
FILE*rc;

#ifndef __STDC__
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

pid_t sfork(){pid_t i;                 /* if secur is set, it doesn't return */
 while((i=fork())&&secur&&i==-1)       /* until the fork was successfull     */
   sleep(SUSPENSION);
 return i;}

void*tmalloc(len)t_buf len;{void*p;
 if(p=malloc(len))
   return p;
 nomemerr();}

void*trealloc(old,len)void*old;t_buf len;{
 if(old=realloc(old,len))
   return old;
 nomemerr();}

void terminate(){
 if(locking){
   nextexit=1;return;}
 sync();         /* sorry, but this is mail we're dealing with, safety first */
 unlock(&loclock);               /* local lock files are removed in any case */
 if(mother&&mother!=getpid())
   if(retval!=EX_OK) /* tell mam we're in trouble, let her clean up the mess */
      kill(mother,SIGQUIT);              /* don't try this at home, kids :-) */
   else{
      kill(mother,SIGHUP);goto allok;} /* You can go home mam, everything ok */
 else
allok:            /* global lockfile should be removed by the last surviving */
   unlock(&globlock);   /* procmail */
 exit(retval);}

void flagger(){         /* hey, we received a SIGHUP */
 flaggerd=1;}

void errgrandchild(){   /* my grandchildren scream in despair */
 verrgrandchild=1;}

long dump(s,source,len)int s;char*source;long len;{int i;long ol;
 if(s>=0){
    ol=len;
    while(i=rwrite(s,source,BLKSIZ<len?BLKSIZ:(int)len)){
       if(i<0){
          i=0;goto writefin;}
       len-=i;source+=i;}
    if(!len&&(ol<2||!(source[-1]=='\n'&&source[-2]=='\n')))
       rwrite(s,newline,1);         /* message always ends with a newline */
writefin:
    if(fsync(s)&&errno!=EINVAL)
       len++;           /* if there is a physical write error */
    rclose(s);return len-i;}
 return len?len:-1;}    /* return an error even if nothing was to be sent */

long pipin(line,source,len)char*line,*source;long len;{pid_t pid;
 int poutfd[2];
 pipe(poutfd);
 if(!(pid=sfork(line))){
   rclose(PWRITE);redirect(PREAD);callnewprog(line);}
 rclose(PREAD);forkerr(pid,line);
 if(len=dump(PWRITE,source,len))
   writeerr(line);
 if(pwait&&waitfor(pid)){
   progerr(line);len=1;}
 return len;}

char*readdyn(bf,filled)char*bf;long*filled;{int i;
 goto jumpin;
 do{
   *filled+=i;
jumpin:
   bf=trealloc(bf,(t_buf)*filled+BLKSIZ);}         /* dynamic adjust */
 while(0<(i=read(STDIN,bf+*filled,BLKSIZ)));
 if(!*filled)
   return trealloc(bf,1);
 return trealloc(bf,(t_buf)*filled);}   /* minimize the buffer space */

char*cat(a,b)char*a,*b;{
 return strcat(strcpy(buf,a),b);}

char*findel(start,end)register char*start,*end;{/* find the first empty line */
 while(start<end)
   if(*start++=='\n'&&start<end&&*start=='\n')
      return start+1;
 return end;}

main(argc,argv)char*argv[];{static char flags[10];int i;
 char*themail,*thebody,*chp,*startchar,*chp2;long tobesent,filled,rcoffset;
 mother=getpid();setbuf(stdin,(void*)0);umask(077);
 sprintf(buf,"%u",i=getuid());setpwent();
 {struct passwd*pass;
 if(pass=getpwuid(i)){                  /* find user defaults in /etc/passwd */
   setdef(home,pass->pw_dir);chdir(pass->pw_dir);
   setdef(user,pass->pw_name?pass->pw_name:buf);setdef(shell,pass->pw_shell);}
 else{                   /* user could not be found, set reasonable defaults */
   setdef(home,tmp);chdir(tmp);setdef(user,buf);setdef(shell,binsh);}}
 endpwent();setdef(shellmeta,DEFshellmeta);setdef(shellflags,DEFshellflags);
 setdef(maildir,DEFmaildir);setdef(defaultf,DEFdefault);
 setdef(orgmail,DEForgmail);setdef(grep,DEFgrep);setdef(sendmail,DEFsendmail);
 setdef(lockext,DEFlockext);setdef(locksleep,DEFlocksleeps);
 chdir(getenv(maildir));fdreopena(devnull,STDERR);fdreopena(devnull,STDOUT);
 gargv=argv+1;nextrcfile();
 thebody=themail=tmalloc((t_buf)(argc=1));filled=rcoffset=0;
changedmail:
 themail=readdyn(themail,&filled);      /* read in the mail */
onlyhead:
 startchar=filled+(thebody=themail);
 while(thebody<startchar&&*thebody++=='\n');        /* skip leading garbage */
 thebody=findel(thebody,startchar);     /* find the end of the header       */
 chp=thebody;
 do{                                    /* search for bogus headers         */
   if(startchar-chp<8)                  /* we're looking for:               */
      break;                            /* "\n\nFrom +[^\t\n ]+ +[^\n\t]"   */
   if(0>=sscanf(chp,"From%1[ ]",buf))   /* thats the regular expression     */
      continue;                         /* that defines the start of a mail */
   chp2=chp;chp+=5;                     /* message.                         */
#define SKIPWHILE(x)    while(x){ if(++chp>=startchar) break;}
   SKIPWHILE(*chp==' ')
   SKIPWHILE((i=*chp)&&i!=' '&&i!='\t'&&i!='\n')
   SKIPWHILE(*chp==' ')
   if((i=*chp)&&i!='\n'&&i!='\t'){      /* insert '>' before bogus header */
      i=startchar[-1];memmove(chp2+1,chp2,(t_buf)(startchar-chp2)-1);
      *chp2='>';themail=trealloc(chp2=themail,++filled);
#define ADJUST(x)       ((x)=themail+((x)-chp2))
      ADJUST(thebody);ADJUST(startchar);ADJUST(chp);*startchar++=i;}}
 while(startchar>(chp=findel(chp,startchar)));
 waitflagger();         /* if we're a child, wait for the parental guidance */
changerc:
 rc=fopen(strcat(cat(getenv(home),"/"),rcfile),"r");
 fseek(rc,rcoffset,SEEK_SET);signal(SIGINT,terminate);
 signal(SIGQUIT,terminate);signal(SIGTERM,terminate);signal(SIGHUP,SIG_IGN);
goon:
 while(unlock(&loclock),!feof(rc)||argv[argc]){
   while(chp=argv[argc]){       /* interpret command line specs first */
      argc++;strcpy(buf2,chp);
      if(chp=strchr(buf2,'=')){
         chp++;goto argenv;}}
   if(tscrc(" %1[:]",flags)){   /* check for a recipe */
      skipblanks();i=sh=1;
      if(tscrc("%[0-9]",buf2)){
         sscanf(buf2,"%d",&sh);skipblanks();}
      *flags='\0';scrc("%9[HBIhbfcws]",flags);skipblanks();
      if(tolock)                /* clear temporary buffer for lockfile name */
         free(tolock);
      tolock=0;
      if((locknext=(tscrc("%1[:]",buf)))&&
         (skipblanks(),tscrc("%[^ \t\n#]",buf)))
         tolock=strdup(buf);
      startchar=themail;tobesent=thebody-themail;
      if(strchr(flags,'B'))     /* what needs to be piped into grep? */
         if(strchr(flags,'H'))
            tobesent=filled;
         else{
            startchar=thebody;tobesent=filled-tobesent;}
      while(sh--){                              /* any conditions (left) */
         skiptoeol();scrc("%[^\n]",buf2);
         if(!strncmp(buf2,TOkey,TOkeylen))
            cat(TOsubstitute,buf2+TOkeylen);
         else
            strcpy(buf,buf2);
         if(i)                                  /* check out all conditions */
            i=!grepin(buf,startchar,tobesent,!strchr(flags,'I'));}
      startchar=themail;tobesent=filled;        /* body, header or both? */
      if(strchr(flags,'h')){
         if(!strchr(flags,'b'))
            tobesent=thebody-themail;}
      else if(strchr(flags,'b'))
         tobesent-=(startchar=thebody)-themail;
      chp=buf+strlen(cat(getenv(sendmail)," "));sh=0;
      pwait=!!strchr(flags,'w');secur=!!strchr(flags,'s');
      if(tscrc(" ! %[^\n]",chp)){               /* forward the mail */
         if(i)
            goto forward;}
      else if(tscrc("| %[^\n]",buf2)){          /* pipe the mail */
         if(i){
            if(sh=!!strpbrk(buf2,getenv(shellmeta)))
               strcpy(buf,buf2);
            else
               parse();
            chp=buf;*buf2='\0';
            while(i=*chp)     /* find the implicit lockfile name ('>>name') */
              if(chp++,i=='>'&&*chp=='>'){
                 while((i=*++chp)==' '||i=='\t');
                 sscanf(chp,"%[^ \t\n#'\");|<>]",buf2);break;}
            lcllock();
            if(strchr(flags,'f')){
               if(startchar==themail&&tobesent!=filled){     /* if only 'h' */
                  char*dest;long dfilled=0;
                  if(pipthrough(buf,startchar,tobesent))
                     goto goon;
                  dest=readdyn(tmalloc(1),&dfilled);filled-=tobesent;
                  if(tobesent<dfilled)  /* adjust buffer size (only bigger) */
                     themail=trealloc(themail,(t_buf)(dfilled+filled));
                  memmove(themail+dfilled,themail+tobesent,(t_buf)filled);
                  memmove(themail,dest,(t_buf)dfilled);free(dest);
                  themail=trealloc(themail,(t_buf)(filled+=dfilled));
                  goto onlyhead;}         /* and determine the header again */
               rcoffset=ftell(rc);   /* needed because we have to fclose it */
               if(pipthrough(buf,startchar,tobesent))
                  goto goon;
               filled=startchar-themail;goto changedmail;}
forward:    if(!pipin(buf,startchar,tobesent)&&!strchr(flags,'c'))
               goto mailed;}}
      else{                     /* append the mail to a file */
         scrc("%s",buf2);skipcomment();
         if(i){
            parse();strcpy(buf2,buf);lcllock();
            if(!dump(opena(buf),startchar,tobesent)&&!strchr(flags,'c'))
               goto mailed;}}}
   else if(tscrc("%[A-Z_a-z0-9] = ",buf)){ /* then it must be an assignment */
      *(chp=buf+strlen(buf))='=';*++chp='\0';scrc("%[^ \t\n#]",chp);
      skipcomment();strcpy(buf2,buf);
argenv:
      parse();putenv(strdup(buf));chp[-1]='\0';
      if(!strcmp(buf,maildir))
         chdir(chp);
      else if(!strcmp(buf,logfile))
         fdreopena(chp,STDERR);
      else if(!strcmp(buf,lockfile))
         lockit(chp,&globlock);
      else if(!strcmp(buf,eumask)){
         sscanf(chp,"%o",&i);umask(i);}
      else if(!strcmp(buf,host)){
         *buf2='\0';gethostname(buf2,BFSIZ);
         if(strcmp(chp,buf2)){
            if(nextrcfile()){
               fclose(rc);rcoffset=0;goto changerc;}
            retval=EX_OK;terminate();}}}
   else if(!skipcomment()){                    /* either comment or garbage */
      scrc("%[^\n] ",buf);log("Skipped: \"");log(buf);logqnl();}}
 if(dump(opena(chp=getenv(defaultf)),themail,filled)){  /* default maildest */
   writeerr(chp);          /* if it fails, don't panic, try the last resort */
   if(dump(opena(chp=getenv(orgmail)),themail,filled))
      writeerr(chp);goto mailerr;}                     /* now you can panic */
mailed:
 retval=EX_OK;                   /* we're home free, mail delivered */
mailerr:
 unlock(&loclock);               /* any local lock file still around? */
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
      log(buf2);log(newline);}}  /* log it's arrival */
 while(themail<thebody);
 terminate();}

setdef(name,contents)char*name,*contents;{
 strcat(strcat(strcpy(buf2,name),"="),contents);parse();putenv(strdup(buf));}

skipblanks(){
 fscanf(rc,"%*[ \t]");}

skiptoeol(){
 fscanf(rc,"%*[^\n]");return fgetc(rc);}

skipcomment(){char i[2];        /* no comment :-) */
 if(tscrc("%1[\n]",i))
   return 1;
 skipblanks();return scrc("%1[#]",i)?skiptoeol():feof(rc);}

pipthrough(line,source,len)char*line,*source;long len;{pid_t pidf,pid;
 int pinfd[2],poutfd[2];
#define PWR2 (pinfd[1])
#define PRD2 (pinfd[0])
 pipe(poutfd);pipe(pinfd);
 if(!(pidf=sfork())){           /* the filter is started here */
   rclose(PWRITE);redirect(PREAD);rclose(STDOUT);dup(PWR2);rclose(PWR2);
   rclose(PRD2);callnewprog(line);}
 rclose(PREAD);rclose(PWR2);
 if(forkerr(pidf,line)){
   rclose(PRD2);return 1;}
 flaggerd=0;signal(SIGHUP,flagger);
 if(!(pid=sfork())){        /* this process will read back the filtered mail */
   rclose(PWRITE);signal(SIGHUP,flagger);kill(getppid(),SIGHUP);redirect(PRD2);
   if(!pwait&&mother){         /* if the wait chain ends here, notify mother */
      kill(mother,SIGHUP);mother=0;}  /* everything ok mam, go ahead and die */
   return 0;}           /* we will go ahead however (with or without mother) */
 rclose(PRD2);
 if(forkerr(pid,"procmail")){    /* An ingenious signal communication system */
   kill(pidf,SIGTERM);return 1;} /* will ensure that NO lock file and        */
 if(dump(PWRITE,source,len)){    /* NO mail get's lost.                      */
   kill(pidf,SIGTERM);kill(pid,SIGTERM);writeerr(line);return 1;}
 waitflagger();verrgrandchild=flaggerd=0;signal(SIGQUIT,errgrandchild);
 if(pwait){
   if(waitfor(pidf)){       /* if the pipe fails, we will continue ourselves */
      progerr(line);kill(pid,SIGTERM);return 1;}
   for(kill(pid,SIGHUP),loclock=0;;sleep(SUSPENSION)){
      if(flaggerd)
         break;
      if(verrgrandchild){    /* Check if one of my grandchildren has paniced */
         signal(SIGQUIT,terminate);return 1;}}}
 else
   kill(pid,SIGHUP);      /* if we don't wait, signal that we're bailing out */
 exit(EX_OK);}                           /* go ahead child, you have control */

waitflagger(){
 while(!flaggerd)
   sleep(SUSPENSION);}

grepin(expr,source,len,casesens)char*expr,*source;long len;{pid_t pid;
 int poutfd[2];static char*newargv[5]={0,"-e"};
 newargv[3]=casesens?(char*)0:"-i";*newargv=getenv(grep);newargv[2]=expr;
 pipe(poutfd);
 if(!(pid=sfork())){
   rclose(PWRITE);redirect(PREAD);shexec(newargv);}
 rclose(PREAD);dump(PWRITE,source,len);
 if(!forkerr(pid,*newargv))
    return waitfor(pid);
 return 127;}

waitfor(pid)pid_t pid;{int i;
 while(pid!=wait(&i)||(i&127)==127);
 return i>>8&255;}

redirect(pip){
 getstdinpipe(pip);fclose(rc);}

getstdinpipe(pip){
 rclose(STDIN);dup(pip);rclose(pip);}

callnewprog(newname)char*newname;{int argc;char*endp,**newargv;
 register char*p;
 p=newname;
 if(sh){char*newargv[4];        /* should we start a shell? */
   newargv[3]=0;newargv[2]=p;newargv[1]=getenv(shellflags);
   *newargv=getenv(shell);shexec(newargv);}
 argc=2;
 while(*p){  /* If no shell, we'll have to chop up the arguments ourselves */
   if(*p==' '||*p=='\t'){
      argc++;*p='\0';
      while(*++p==' '||*p=='\t')
         *p='\0';
      continue;}
   p++;}
 endp=p;*(newargv=tmalloc(argc*sizeof*newargv))=p=newname;argc=1;
 for(;;){
   while(*p)
      p++;
   while(!*p){
      if(p==endp){
         newargv[argc]=0;shexec(newargv);}
      p++;}
   newargv[argc++]=p;}}

parse(){int i;char*org,*dest,*bd;   /* Implicitly copies from buf2 to buf */
 dest=buf;org=buf2;
 while(i=*org++)
   if(i!='$')
      *dest++=i;
   else{                        /* substitute the thing... */
      bd=buf2;
      while((i=*org)>='A'&&i<='Z'||i>='a'&&i<='z'||i>='0'&&i<='9'||i=='_'){
         *bd++=i;org++;}
      *bd='\0';
      if(bd=getenv(buf2)){
         strcpy(dest,bd);
         while(*dest)
            dest++;}}
 *dest='\0';}

writeerr(line)char*line;{
 log("Error while writing to \"");log(line);logqnl();}

forkerr(pid,a)pid_t pid;char*a;{
 if(pid==-1){
   log("Failed forking \"");log(a);logqnl();return 1;}
 return 0;}

progerr(line)char*line;{
 log("Program failure of \"");log(line);logqnl();}

log(a)char*a;{char*b;
 b=a-1;
 while(*++b);
 rwrite(STDERR,a,b-a);}

opena(a)char*a;{
return open(a,O_WRONLY|O_APPEND|O_CREAT,0666);}

fdreopena(a,fd)char*a;{  /* Currently only works for 0,1,2 */
 rclose(fd);return opena(a);}

shexec(argv)char**argv;{int i;char**newargv,**p;
 execvp(*argv,argv);    /* if this one fails, we retry it as a shell script */
 for(p=argv,i=1;i++,*p++;);
 newargv=tmalloc(i*sizeof*p);
 for(*(p=newargv)=binsh;*++p=*++argv;);
 execve(*newargv,newargv,environ);      /* no shell script? -> trouble */
 log("Failed to execute \"");log(*argv);logqnl();exit(EX_UNAVAILABLE);}

unlock(lockp)char**lockp;{
 locking=1;
 if(*lockp){
   if(unlink(*lockp)){
      log("Couldn't unlink \"");log(*lockp);logqnl();}
   free(*lockp);*lockp=0;}
 locking=0;}

nomemerr(){
 log("Out of memory\nbuffer 0: \"");log(buf);log("\"\nbuffer 1: \"");
 log(buf2);logqnl();retval=EX_OSERR;terminate();}

logqnl(){
 log("\"\n");}

nextrcfile(){char*p;   /* find the next rcfile specified on the command line */
 while(p=*gargv){
   gargv++;
   if(!strchr(p,'=')){
      rcfile=p;return 1;}}
 return 0;}

rclose(fd){int i;               /* a sysV secure close (signal immune) */
 while((i=close(fd))&&errno==EINTR);
 return i;}

rwrite(fd,a,len)void*a;{int i;  /* a sysV secure write (signal immune) */
 while(0>(i=write(fd,a,len))&&errno==EINTR);
 return i;}

lockit(name,lockp)char*name,**lockp;{int i;
 unlock(lockp);                 /* unlock any previous lockfile FIRST      */
 for(locking=1;;){              /* to prevent deadlocks (I hate deadlocks) */
   if(0<=(i=open(name,O_WRONLY|O_CREAT|O_EXCL|O_SYNC,0))){
      rclose(i);*lockp=strdup(name);
term: locking=0;
      if(nextexit)
         terminate();
      return;}
   if(errno==ENAMETOOLONG){     /* if too long, make it shorter and retry */
      if(0<(i=strlen(name)-1)){
         name[i]='\0';continue;}
      log("Lockfailure\n");return;}
   i=DEFlocksleep;sscanf(getenv(locksleep),"%i",&i);sleep(i);
   if(nextexit)
      goto term;}}

lcllock(){                      /* lock a local file (if need be) */
 if(locknext)
   if(tolock)
      lockit(tolock,&loclock);
   else
      lockit(strcat(buf2,getenv(lockext)),&loclock);}

/* That's all folks */
