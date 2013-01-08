/************************************************************************ 
 *	Collection of routines that return an int (sort of anyway :-)	*
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
#include "shell.h"

setdef(name,contents)char*name,*contents;{
 strcat(strcat(strcpy(buf2,name),"="),contents);parse();sputenv(buf);}

skipblanks(){
 fscanf(rc,"%*[ \t]");}

skiptoeol(){
 fscanf(rc,"%*[^\n]");return fgetc(rc);}

skipcomment(){char i[2];				   /* no comment :-) */
 if(tscrc("%1[\n]",i))
   return 1;
 skipblanks();return scrc("%1[#]",i)?skiptoeol():feof(rc);}

pipthrough(line,source,len)char*line,*source;long len;{pid_t pidf,pid;
 int pinfd[2],poutfd[2];
#define PWR2 (pinfd[1])
#define PRD2 (pinfd[0])
 pipe(poutfd);pipe(pinfd);
 if(!(pidf=sfork())){			       /* the filter is started here */
   rclose(PWRITE);redirect(PREAD);rclose(STDOUT);dup(PWR2);rclose(PWR2);
   rclose(PRD2);callnewprog(line);}
 rclose(PREAD);rclose(PWR2);
 if(forkerr(pidf,line)){
   rclose(PRD2);return 1;}
 flaggerd=0;signal(SIGHUP,flagger);
 if(!(pid=sfork())){	    /* this process will read back the filtered mail */
   rclose(PWRITE);signal(SIGHUP,flagger);kill(getppid(),SIGHUP);redirect(PRD2);
   if(!pwait&&mother){	       /* if the wait chain ends here, notify mother */
      kill(mother,SIGHUP);mother=0;}  /* everything ok mam, go ahead and die */
   return 0;}		/* we will go ahead however (with or without mother) */
 rclose(PRD2);
 if(forkerr(pid,"procmail")){	 /* An ingenious signal communication system */
   kill(pidf,SIGTERM);return 1;} /* will ensure that NO lock file and	     */
 if(dump(PWRITE,source,len)){	 /* NO mail gets lost.			     */
   kill(pidf,SIGTERM);kill(pid,SIGTERM);writeerr(line);return 1;}
 waitflagger();verrgrandchild=flaggerd=0;signal(SIGQUIT,errgrandchild);
 if(pwait){
   if(waitfor(pidf)!=EX_OK){	/* if pipe fails, we will continue ourselves */
      progerr(line);kill(pid,SIGTERM);return 1;}
   for(kill(pid,SIGHUP),loclock=0;;sleep(SUSPENSION)){
      if(flaggerd)
	 break;
      if(verrgrandchild){    /* Check if one of my grandchildren has paniced */
	 signal(SIGQUIT,sterminate);return 1;}}}
 else
   kill(pid,SIGHUP);	  /* if we don't wait, signal that we're bailing out */
 exit(EX_OK);}				 /* go ahead child, you have control */

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
    return waitfor(pid)!=EX_OK;
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
 if(sh){char*newargv[4];			 /* should we start a shell? */
   newargv[3]=0;newargv[2]=p;newargv[1]=getenv(shellflags);
   *newargv=getenv(shell);shexec(newargv);}
 argc=2;
 while(*p){    /* If no shell, we'll have to chop up the arguments ourselves */
   if(*p==' '||*p=='\t'){
      argc++;*p='\0';
      while(*++p==' '||*p=='\t')
	 *p='\0';
      continue;}
   p++;}
 endp=p;*(newargv=malloc(argc*sizeof*newargv))=p=newname;argc=1;
 for(;;){
   while(*p)
      p++;
   while(!*p){
      if(p==endp){
	 newargv[argc]=0;shexec(newargv);}
      p++;}
   newargv[argc++]=p;}}

parse(){int i;char*org,*dest,*bd;      /* Implicitly copies from buf2 to buf */
 dest=buf;org=buf2;
 while(i=*org++)
   if(i!='$')
      *dest++=i;
   else{					  /* substitute the thing... */
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
 log("Error while writing to");logqnl(line);}

forkerr(pid,a)pid_t pid;char*a;{
 if(pid==-1){
   log("Failed forking");logqnl(a);return 1;}
 return 0;}

progerr(line)char*line;{
 log("Program failure of");logqnl(line);}

log(a)char*a;{char*b;
 b=a-1;
 while(*++b);
 rwrite(STDERR,a,b-a);}

opena(a)char*a;{
return open(a,O_WRONLY|O_APPEND|O_CREAT,0666);}

fdreopena(a,fd)char*a;{			   /* Currently only works for 0,1,2 */
 rclose(fd);return opena(a);}

shexec(argv)char**argv;{int i;char**newargv,**p;
 execvp(*argv,argv);	 /* if this one fails, we retry it as a shell script */
 for(p=argv,i=1;i++,*p++;);
 newargv=malloc(i*sizeof*p);
 for(*(p=newargv)=binsh;*++p=*++argv;);
 execve(*newargv,newargv,environ);	      /* no shell script? -> trouble */
 log("Failed to execute");logqnl(*argv);exit(EX_UNAVAILABLE);}

unlock(lockp)char**lockp;{
 lcking=1;
 if(*lockp){
   if(unlink(*lockp)){
      log("Couldn't unlink");logqnl(*lockp);}
   free(*lockp);*lockp=0;}
 if(nextexit==1){	    /* make sure we are not inside terminate already */
   log(newline);terminate();}
 lcking=0;}

nomemerr(){
 log("Out of memory\nbuffer 0: \"");log(buf);log("\"\nbuffer 1:");
 logqnl(buf2);retval=EX_OSERR;terminate();}

logqnl(a)char*a;{
 log(" \"");log(a);log("\"\n");}

nextrcfile(){char*p;   /* find the next rcfile specified on the command line */
 while(p=*gargv){
   gargv++;
   if(!strchr(p,'=')){
      rcfile=p;return 1;}}
 return 0;}

rclose(fd){int i;		      /* a sysV secure close (signal immune) */
 while((i=close(fd))&&errno==EINTR);
 return i;}

rwrite(fd,a,len)void*a;{int i;	      /* a sysV secure write (signal immune) */
 while(0>(i=write(fd,a,len))&&errno==EINTR);
 return i;}

lockit(name,lockp)char*name,**lockp;{int i;struct stat stbuf;
 unlock(lockp);			  /* unlock any previous lockfile FIRST	     */
 for(lcking=1;;){		  /* to prevent deadlocks (I hate deadlocks) */
   if(0<=(i=open(name,O_WRONLY|O_CREAT|O_EXCL|O_SYNC,0))){
      rclose(i);*lockp=tstrdup(name);
term: if(nextexit){
	 log(" whilst waiting for lockfile");logqnl(name);terminate();}
      lcking=0;return;}
   if(errno==EEXIST){		   /* check if it's time for a lock override */
      if((i=renvint(0,locktimeout))&&!stat(name,&stbuf)&&
	 i<time((time_t*)0)-stbuf.st_mtime){
	 log("Forcing lock on");logqnl(name);
	 if(unlink(name)){
	    log("Forced unlock denied on");logqnl(name);}}}
   else{		       /* maybe filename too long, shorten and retry */
      if(0<(i=strlen(name)-1)&&name[i-1]!='/'){
	 name[i]='\0';continue;}
      log("Lockfailure on");logqnl(name);return;}
   sleep(DEFlocksleep,locksleep);
   if(nextexit)
      goto term;}}

lcllock(){				   /* lock a local file (if need be) */
 if(locknext)
   if(tolock)
      lockit(tolock,&loclock);
   else
      lockit(strcat(buf2,getenv(lockext)),&loclock);}

sputenv(a)char*a;{	      /* smart putenv, the way it was supposed to be */
 static struct lienv{struct lienv*next;char name[255];}*myenv;
 static alloced;int i,remove;char*split,**preenv;struct lienv*curr,**last;
 remove=0;a=strdup(a);					/* make working copy */
 if(!(split=strchr(a,'='))){			   /* assignment or removal? */
    remove=1;i=strlen(a);*(split=i+(a=realloc(a,i+2)))='=';
    split[1]='\0';}
 i=++split-a;
 for(curr=*(last=&myenv);curr;curr=*(last=&curr->next))
   if(!strncmp(a,curr->name,i)){	     /* is it one I created earlier? */
      split=curr->name;*last=curr->next;free(curr);
      for(preenv=environ;*preenv!=split;preenv++);
      goto wipenv;}
 for(preenv=environ;*preenv;preenv++)
   if(!strncmp(a,*preenv,i)){	       /* is it in the standard environment? */
wipenv:
      while(*preenv=preenv[1])	   /* wipe this entry out of the environment */
	 preenv++;
      break;}
 if(alloced)		   /* have we ever alloced the environ array before? */
   environ=realloc(environ,(preenv-environ+2)*sizeof*environ);
 else{
   alloced=1;i=(preenv-environ+2)*sizeof*environ;
   environ=tmemmove(malloc(i),environ,i-sizeof*environ);}
 if(!remove){		  /* if not remove, then add it to both environments */
   for(preenv=environ;*preenv;preenv++);
   curr=malloc(curr->name-(char*)curr+strlen(a)+1);
   strcpy(*preenv=curr->name,a);free(a);preenv[1]=0;curr->next=myenv;
   myenv=curr;}}

renvint(init,env)char*env;{int i;
 i=init;
 if(env=getenv(env))			      /* check if the env var exists */
   sscanf(env,"%i",&i);			       /* parse it like a C constant */
 return i;}

terminate(){
 nextexit=2;			/* prevent multiple invocations of terminate */
 unlock(&loclock);		 /* local lock files are removed in any case */
 if(mother&&mother!=getpid())
   if(retval!=EX_OK) /* tell mam we're in trouble, let her clean up the mess */
      kill(mother,SIGQUIT);		 /* don't try this at home, kids :-) */
   else{
      kill(mother,SIGHUP);goto allok;} /* You can go home mam, everything ok */
 else  
allok:					/* global lockfile should be removed */
   unlock(&globlock);			/* by the last surviving procmail    */
 sync();	 /* sorry, but this is mail we're dealing with, safety first */
 exit(retval);}
