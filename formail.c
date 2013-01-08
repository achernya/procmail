/************************************************************************
 *	formail.c	a mail (re)formatter				*
 *									*
 *	Seems to be relatively bug free.				*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 *	#include "STYLE"						*
 *									*
 ************************************************************************/
#ifdef	RCS
static char rcsid[]="$Id: formail.c,v 2.6 1991/06/19 17:47:00 berg Rel $";
#endif
static char rcsdate[]="$Date: 1991/06/19 17:47:00 $";
#include "config.h"			/* I know, overkill, only need BinSh */
#include "includes.h"

#define BSIZE	4096

#define FROM		"From "
#define UNKNOWN		"foo@bar"

#define Re		(re+1)
#define Nextchar(x)	do{x=getchar();if(feof(stdin))goto foundeof;}while(0)
#define putssn(a,l)	tputssn(a,(size_t)(l))
#define putcs(a)	(errout=putc(a,mystdout))
#define PRDO		poutfd[0]
#define PWRO		poutfd[1]

static const char From[]=FROM,replyto[]="Reply-To:",Fromm[]="From:",
 returnpath[]="Return-Path",sender[]="Sender:",outofmem[]="Out of memory\n",
 subject[]="Subject:",re[]=" Re:",couldntw[]="Couldn't write to stdout",
 references[]="References:",messageid[]="Message-ID:",Date[]="Date:",
 article[]="Article ";
const char binsh[]=BinSh;
static struct {const char*const head;const int len,wrepl;}sest[]={
 {sender,STRLEN(sender),0},{replyto,STRLEN(replyto),4},
 {Fromm,STRLEN(Fromm),2},{returnpath,STRLEN(returnpath),1}};
static struct {const char*const headr;const int lenr;size_t offset;}rex[]={
 {subject,STRLEN(subject)},{references,STRLEN(references)},
 {messageid,STRLEN(messageid)}};
#define subj	rex[0]
#define refr	rex[1]
#define msid	rex[2]
static struct {const char*const hedr;const int lnr;}cdigest[]={
 {Fromm,STRLEN(Fromm)},{Date,STRLEN(Date)},{subject,STRLEN(subject)},
 {article,STRLEN(article)}};
#define mxl(a,b)	mx(STRLEN(a),STRLEN(b))
#define dig_HDR_LEN	mx(mxl(From,Fromm),mxl(Date,subject))
static errout,oldstdout;
static pid_t child= -1;
static FILE*mystdout;
static size_t nrskip,nrtotal= -1;

#ifdef	NOstrstr
char*strstr(whole,part)const char*whole,*const part;{size_t len;
 if(!(len=strlen(part)))
   return(char*)whole;
 while(*whole&&strncmp(whole,part,len))
   ++whole;
 return *whole?(char*)whole:(char*)0;}
#endif

void*tmalloc(len)const size_t len;{void*p;
 if(p=malloc(len))
   return p;
 log(outofmem);exit(EX_OSERR);}

void*trealloc(old,len)void*old;const size_t len;{
 if(old=realloc(old,len))
   return old;
 log(outofmem);exit(EX_OSERR);}

#include "shell.h"

main(lastm,argv)const char*const argv[];{time_t t;
 int i,nowm,thelen=0,split=0,force=0,bogus=1,every=0,areply=0,
   trust=0,digest=0,nowait=0;
 size_t buflen,p=0,lnl=0,thename,ll;
 char*buf,*chp;
 while(*++argv){
   if((lastm= **argv)=='+')
      goto number;
   else if(lastm!='-')
      goto usg;
   for(i=1;;){
      switch((*argv)[i++]){
	 case 't':trust=1;continue;    /* trust the sender for valid headers */
	 case 'r':areply=1;continue;			 /* generate a reply */
	 case 'f':force=1;continue;		  /* accept arbitrary format */
	 case 'e':every=1;continue;		      /* split on every From */
	 case 'd':digest=1;continue;			 /* split up digests */
	 case 'n':nowait=1;continue;	      /* don't wait for the programs */
	 case 's':split=1;bogus=0;
	    if(!(*argv++)[i])
	       goto parsedoptions;
	    goto usg;
number:	 default:
	    if((*argv)[1]-'0'>(unsigned)9){
usg:	       log("Usage: formail [+nnn] [-nnn] [-bfrtned] \
[-s command argument ...]\n");return EX_USAGE;}
	    ll=strtol((*argv)+1,(char**)0,10);
	    if(lastm=='+')
	       nrskip=ll;
	    else
	       nrtotal=ll;
	    break;
	 case 'b':bogus=0;continue;		 /* leave bogus Froms intact */
	 case '\0':;}
      break;}}
parsedoptions:
 mystdout=stdout;
 if(split){
   oldstdout=dup(STDOUT);fclose(stdout);startprog(argv);}
 while('\n'==(i=getchar()));
 buf=malloc(buflen=BSIZE);t=time((time_t*)0);
 for(;;){					 /* start parsing the header */
   if((buf[p++]=i)=='\n'){
      chp=buf+lnl;i=maxindex(rex);
      while(strnicmp(rex[i].headr,chp,rex[i].lenr)&&i--);
      if(i>=0)					  /* found anything already? */
	 rex[i].offset=lnl+rex[i].lenr;
      else if(!strncmp(From,chp,STRLEN(From))){
	 if(!lnl){				/* was the real "From " line */
	    if(!areply)
	       goto endofheader;
	    nowm=trust?1:3/*wreply*/;ll=lnl+STRLEN(From);goto foundfrom;}
	 if(bogus){
	    tmemmove(chp+1,chp,p++-lnl);*chp='>';}}		   /* disarm */
      else{
	 i=maxindex(sest);
	 do
	    if(!strnicmp(sest[i].head,chp,sest[i].len)){
	       nowm=areply?sest[i].wrepl:i;ll=lnl+sest[i].len;
foundfrom:     buf[p]='\0';
	       if(chp=strchr(buf+ll,'<'))	      /* extract the address */
		  ll=chp-buf+1;
	       chp=buf+(ll+=strspn(buf+ll," \t"));
	       if((i=strcspn(chp,">(\n \t"))&&(!thelen||nowm>lastm)){
		  thename=ll;thelen=i;
		  lastm=strstr(chp,".UUCP")?nowm-maxindex(sest)-1:nowm;}
	       break;}
	 while(i--);
	 if(lnl==p-1)
	    break;}				    /* end of header reached */
      lnl=p;}
   if(p>=buflen-2)
      buf=realloc(buf,buflen+=BSIZE);
redigest:
   i=getchar();
   if(feof(stdin))
      i='\n';}			/* make sure the header ends with 2 newlines */
 if(areply||!force){
   putss(areply?"To: ":From);
   if(thelen)					/* found any sender address? */
      putssn(buf+thename,thelen);
   else
      putss(UNKNOWN);
   if(areply){
      putcs('\n');
      if(subj.offset){					   /* any Subject: ? */
	 putss(subject);chp=buf+subj.offset;
	 if(strnicmp(chp+strspn(chp," "),Re,STRLEN(Re)))
	    putss(re);			       /* no Re: , add one ourselves */
	 nlputss(chp);}
      if(refr.offset||msid.offset){	 /* any Message-ID: or References: ? */
	 putss(references);
	 if(refr.offset){
	    chp=buf+refr.offset;
	    putssn(chp,strchr(chp,'\n')-chp+!msid.offset);}
	 if(msid.offset){
	    nlputss(buf+msid.offset);putss("In-Reply-To:");
	    nlputss(buf+msid.offset);}}
      putcs('\n');
      while(getchar(),!feof(stdin));return EX_OK;}
   putcs(' ');putss(ctime(&t));}
endofheader:
 putssn(buf,p);p=0;lnl=1;
 if(!bogus&&!split)
   for(;;putcs(i))
      Nextchar(i);
 for(;;){					       /* continue the quest */
   do{						 /* read line until not From */
      if(p==buflen-1)
	 buf=realloc(buf,++buflen);
      Nextchar(i=buf[p]);
      if(++p==STRLEN(From))
	 if(!strncmp(From,buf,STRLEN(From))){
	    if(bogus){
	       putcs('>');break;}				   /* disarm */
	    else if(every)
	       goto splitit;
	    else if(split&&lnl)
	       lnl=2;}			   /* mark line as possible postmark */
      if(lnl==1&&digest){
	 thelen=maxindex(cdigest);
	 do				      /* check for new digest header */
	    if(p==cdigest[thelen].lnr&&!strncmp(buf,cdigest[thelen].hedr,p)){
	       lnl=thelen=0;goto splitit;}	  /* pretend we started over */
	 while(thelen--);}}
   while(i!='\n'&&(lnl==2||p<dig_HDR_LEN));
   if(lnl==2){
      buf[p]='\0';		 /* perform more thorough check for postmark */
      for(i=STRLEN(From)-1;buf[++i]==' ';);
      if(ll=strcspn(buf+i,"\n \t")){
	 i+=ll;
	 if(ll=strspn(buf+i," ")&&(ll=buf[i+ll])!='\t'&&ll!='\n'){
splitit:    if(fclose(mystdout)==EOF||errout==EOF){
	       log(couldntw);log(", continuing...\n");split= -1;}
	    if(!nowait)
	       waitforit();
	    startprog(argv);
	    if(!lnl)					    /* digest split? */
	       goto redigest;
	    i='\n';}}}
   lnl=p==1;putssn(buf,p);p=0;
   if(i!='\n')
      do Nextchar(i);
      while(putcs(i),i!='\n');}
foundeof:
 putssn(buf,p);
 if(fclose(mystdout)==EOF||errout==EOF){
   log(couldntw);return EX_IOERR;}
 child= -1;waitforit();return split<0?EX_IOERR:EX_OK;}	/* wait for everyone */

strnicmp(a,b,l)register const char*a,*b;register unsigned l;{int i,j;
 if(l)						 /* case insensitive strncmp */
   do{
      while(*a&&*a==*b&&--l)
	 ++a,++b;
      if(!l)
	 break;
      if((i= *a++)>='A'&&i<='Z')
	 i+='a'-'A';
      if((j= *b++)>='A'&&j<='Z')
	 j+='a'-'A';
      if(j!=i)
	 return i>j?1:-1;}
   while(i&&j&&--l);
 return 0;}

log(a)const char*const a;{
 fputs(a,stderr);}

logqnl(a)const char*a;{
 log(" \"");log(a);log("\"\n");}

nlputss(a)const char*const a;{
 putssn(a,strchr(a,'\n')+1-a);}

putss(a)const char*a;{
 while(*a)
   putcs(*a++);}

tputssn(a,l)const char*a;size_t l;{
 while(l--)
   putcs(*a++);}

startprog(argv)const char*const*const argv;{int poutfd[2];
 if(!nrtotal)
   goto squelch;
 if(nrskip){
   --nrskip;
squelch:
   if(!(mystdout=fopen(DevNull,"a")))
      goto nofild;
   return;}
 if(nrtotal>0)
   --nrtotal;
 dup(oldstdout);pipe(poutfd);
 if(!(child=fork())){
   close(oldstdout);close(PWRO);fclose(stdin);dup(PRDO);close(PRDO);
   shexec(argv);}
 close(STDOUT);close(PRDO);
 if(STDOUT!=dup(PWRO)||!(mystdout=fdopen(STDOUT,"a"))){
nofild:
   log("File table full\n");exit(EX_OSERR);}
 close(PWRO);
 if(0>child){
   log("Can't fork\n");exit(EX_OSERR);}}

waitforit(){int i;
 while(child!=wait(&i)||(i&127)==127);}
