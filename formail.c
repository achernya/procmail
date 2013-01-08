/************************************************************************
 *	formail.c	a mail (re)formatter				*
 *									*
 *	Seems to be relatively bug free.				*
 *									*
 *	Copyright (c) 1990-1992, S.R. van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: formail.c,v 2.16 1992/01/09 17:23:14 berg Rel $";
#endif
static char rcsdate[]="$Date: 1992/01/09 17:23:14 $";
#include "config.h"					  /* slight overkill */
#include "includes.h"

char*pstrspn();

#define BSIZE		4096

#define NAMEPREFIX	"formail: "
#define HEAD_DELIMITER	':'

#define Re		(re+1)
#define Nextchar(x)	do{if((x=getchar())==EOF)goto foundeof;}while(0)
#define putssn(a,l)	tputssn(a,(size_t)(l))
#define putcs(a)	(errout=putc(a,mystdout))
#define PRDO		poutfd[0]
#define PWRO		poutfd[1]

static const char From[]=FROM,replyto[]="Reply-To:",Fromm[]="From:",
 returnpath[]="Return-Path",sender[]="Sender:",outofmem[]="Out of memory\n",
 subject[]="Subject:",re[]=" Re:",couldntw[]="Couldn't write to stdout",
 references[]="References:",messageid[]="Message-ID:",Date[]="Date:",
 article[]="Article ",Path[]="Path:",Received[]="Received:",To[]="To: ",
 OldP[]=OLD_PREFIX,inreplyto[]="In-Reply-To:",errorsto[]="Errors-To",
 retreceiptto[]="Return-Receipt-To";
const char binsh[]=BinSh;
/*
 *	sender determination fields in order of importance reliability
 *	reply-address determination fields (wrepl specifies the weight)
 */
static const struct {const char*head;int len,wrepl;}sest[]=
{ {errorsto,STRLEN(errorsto),5},{retreceiptto,STRLEN(retreceiptto),6},
  {sender,STRLEN(sender),0},{replyto,STRLEN(replyto),4},
  {Fromm,STRLEN(Fromm),2},{returnpath,STRLEN(returnpath),1}
};
/*
 *	digest splitting fields
 */
static const struct {const char*hedr;int lnr;}cdigest[]=
{ {Fromm,STRLEN(Fromm)},{Date,STRLEN(Date)},{subject,STRLEN(subject)},
  {article,STRLEN(article)},{Path,STRLEN(Path)},{Received,STRLEN(Received)}
};

static struct {const char*const headr;const int lenr;char*rexp;}rex[]=
{ {subject,STRLEN(subject)},{references,STRLEN(references)},
  {messageid,STRLEN(messageid)}
};
#define subj	rex[0]
#define refr	rex[1]
#define msid	rex[2]
#define mxl(a,b)	mx(STRLEN(a),STRLEN(b))
#ifndef MAILBOX_SEPARATOR
#define dig_HDR_LEN	mx(mxl(From,Fromm),mxl(Date,subject))
#define mboxseparator	From
#define flushseparator(i,p)
#else
static const char mboxseparator[]=MAILBOX_SEPARATOR;
#define flushseparator(i,p)	\
 do{i=p;p=0;do{int x;Nextchar(x);}while(--i);}while(0)
#define dig_HDR_LEN	\
 mx(mx(mxl(From,Fromm),mxl(Date,subject)),STRLEN(mboxseparator))
#endif
static struct hedit{char*hline;size_t hlen;
 enum{h_ren,h_del,h_add,h_was,h_extract}htype;struct hedit*next;}*hlist;
static errout,oldstdout,quiet;
static pid_t child= -1;
static FILE*mystdout;
static size_t nrskip,nrtotal= -1;

#ifdef	NOstrstr
char*strstr(whole,part)const char*whole,*const part;
{ register const char*w,*p;
  do
   { w=whole;p=part;
     do
	if(!*p)
	   return(char*)whole;
     while(*w++==*p++);
   }
  while(*whole++);
  return(char*)0;
}
#endif

void*tmalloc(len)const size_t len;
{ void*p;
  if(p=malloc(len))
     return p;
  nlog(outofmem);exit(EX_OSERR);
}

void*trealloc(old,len)void*old;const size_t len;
{ if(old=realloc(old,len))
     return old;
  nlog(outofmem);exit(EX_OSERR);
}

#include "shell.h"

struct hedit*overrideh(target,keep)const char*const target;const int keep;
{ struct hedit*hlp;
  for(hlp=hlist;hlp;hlp=hlp->next)
     if(hlp->htype!=h_was&&!strnicmp(hlp->hline,target,hlp->hlen))
      { if(keep>0||hlp->htype!=h_add)			  /* found field ok? */
	   return (struct hedit*)hlp;
	hlp->htype=h_was;			/* temporarily disable field */
	if(keep)					/* smaller than zero */
	   break;
      }
  return(struct hedit*)0;				   /* no field found */
}

main(lastm,argv)const char*const argv[];
{ int i,ch,nowm,split=0,force=0,bogus=1,every=0,areply=0,trust=0,digest=0,
   nowait=0,keepb=0,extract=0;
  size_t buflen,p=0,lnl=0,ll;time_t t;char*buf,*chp,*namep;struct hedit*hlp;
  while(chp=(char*)*++argv)
   { if((lastm= *chp++)==FM_SKIP)
	goto number;
     else if(lastm!=FM_TOTAL)
	goto usg;
     for(;;)
      { switch(lastm= *chp++)
	 { case FM_TRUST:trust=1;continue;
	   case FM_REPLY:areply=1;continue;
	   case FM_FORCE:force=1;continue;
	   case FM_EVERY:every=1;bogus=0;continue;
	   case FM_DIGEST:digest=1;continue;
	   case FM_NOWAIT:nowait=1;continue;
	   case FM_KEEPB:keepb=1;continue;
	   case FM_QUIET:quiet=1;continue;
	   case FM_SPLIT:split=1;
	      if(!*chp&&*++argv)
		 goto parsedoptions;
	      goto usg;
number:	   default:
	      if(*chp-'0'>(unsigned)9)
	       {
usg:		 log(FM_USAGE);return EX_USAGE;
	       }
	      ll=strtol(chp,(char**)0,10);
	      if(lastm==FM_SKIP)
		 nrskip=ll;
	      else
		 nrtotal=ll;
	      break;
	   case FM_BOGUS:bogus=0;continue;
	   case FM_EXTRACT:extract=1;
	   case FM_ADD_IFNOT:case FM_REN_INSERT:case FM_DEL_INSERT:hlp=hlist;
	      (hlist=malloc(sizeof*hlist))->next=hlp;
	      if(!*chp&&!(chp=(char*)*++argv))	/* concatenated or seperate? */
		 goto usg;
	      hlist->hline=chp;				 /* add field string */
	      if(!(buf=strchr(chp,HEAD_DELIMITER)))
	       { nlog("Invalid field-name:");logqnl(chp);goto usg;
	       }
	      hlist->hlen=buf-chp+1;
	      hlist->htype=lastm==FM_REN_INSERT?h_ren:
	       lastm==FM_DEL_INSERT?h_del:lastm==FM_ADD_IFNOT?h_add:h_extract;
	   case '\0':;
	 }
	break;
      }
   }
parsedoptions:
#ifdef MAILBOX_SEPARATOR
  if(split)
   { every=1;
     if(!areply)
	bogus=0;
   }
#endif
  mystdout=stdout;signal(SIGPIPE,SIG_IGN);
  if(split)
   { oldstdout=dup(STDOUT);fclose(stdout);startprog(argv);
   }
  else if(every)
     goto usg;
  namep=malloc(1);buf=malloc(buflen=BSIZE);t=time((time_t*)0);
  i=maxindex(rex);
  do rex[i].rexp=malloc(1);
  while(i--);
  while('\n'==(ch=getchar()));
startover:
  *namep='\0';i=maxindex(rex);
  do *rex[i].rexp='\0';
  while(i--);
  for(;;)					 /* start parsing the header */
   { if((buf[p++]=ch)=='\n')
      { if(lnl==p-1)				    /* end of header reached */
	   break;
	switch(ch=getchar())		      /* concatenate continued lines */
	 { case ' ':case '\t':p--;continue;
	   case EOF:ch='\n';
	 }
	chp=buf+lnl;
#ifdef MAILBOX_SEPARATOR
	if(!strncmp(mboxseparator,chp,STRLEN(mboxseparator)))
	 { if(!lnl)
	    { if(split)
	       { p=0;goto redigest;
	       }
	      force=1;	     /* separator up front, don't add a 'From ' line */
	    }
	   else if(bogus)
	      *chp=' ';
	 }
#endif
	i=maxindex(rex);
	while(strnicmp(rex[i].headr,chp,ll=rex[i].lenr)&&i--);
	if(i>=0)				  /* found anything already? */
	 { ll=p-lnl-ll;
	   ((char*)tmemmove(rex[i].rexp=realloc(rex[i].rexp,ll),
	    buf+lnl+rex[i].lenr,ll))[ll-1]='\0';
	 }
	else if(!strncmp(From,chp,STRLEN(From)))
	 { if(!lnl)				/* was the real "From " line */
	    { nowm=trust?1:3/*wreply*/;ll=lnl+STRLEN(From);goto foundfrom;
	    }
#ifndef MAILBOX_SEPARATOR
	   if(bogus)
	    { tmemmove(chp+1,chp,p++-lnl);*chp=ESCAP;		   /* disarm */
	    }
#endif
	 }
	else
	 { i=maxindex(sest);
	   do
	      if(!strnicmp(sest[i].head,chp,sest[i].len))
	       { nowm=areply?keepb&&sest[i].head==replyto?
		  maxindex(sest)+1:sest[i].wrepl:i;
		 ll=lnl+sest[i].len;
foundfrom:	 buf[p]='\0';
		 if(chp=strchr(buf+ll,'<'))	      /* extract the address */
		    ll=chp-buf+1;
		 if((i=strcspn(chp=pstrspn(buf+ll," \t"),">(\n \t"))&&
		  (!*namep||nowm>lastm))
		  { ((char*)tmemmove(namep=realloc(namep,i+1),chp,i))[i]='\0';
		    lastm=strstr(chp,".UUCP")?nowm-maxindex(sest)-1:nowm;
		  }
		 break;
		}
	   while(i--);
	 }
	if(hlp=overrideh(buf+lnl,areply))	 /* replace or delete field? */
	   switch(hlp->htype)
	    { case h_extract:putssn(buf+lnl+hlp->hlen,p-lnl-hlp->hlen);
	      case h_del:p=lnl;continue;		   /* just delete it */
	      case h_ren:
		 if(p+2>=buflen)	    /* trouble if BSIZE<STRLEN(OldP) */
		    buf=realloc(buf,buflen+=BSIZE);
		 tmemmove(buf+lnl+STRLEN(OldP),buf+lnl,p-lnl);
		 tmemmove(buf+lnl,OldP,STRLEN(OldP));p+=STRLEN(OldP);
	    }
	lnl=p;continue;
      }
     if(p>=buflen-2)
	buf=realloc(buf,buflen+=BSIZE);
redigest:
     if((ch=getchar())==EOF)
	ch='\n';		/* make sure the header ends with 2 newlines */
   }
  if(!extract)
   { if(areply||!force&&strncmp(buf,From,STRLEN(From)))
      { if(!areply||!overrideh(To,!*namep))
	 { putss(areply?(areply=2),To:From);
	   if(*namep)				/* found any sender address? */
	      putss(namep);
	   else
	      putss(UNKNOWN);
	 }
	if(areply)
	 { if(areply==2)
	      putnl();
	   if(*subj.rexp&&!overrideh(subject,-1))	   /* any Subject: ? */
	    { putss(subject);chp=subj.rexp;
	      if(strnicmp(pstrspn(chp," "),Re,STRLEN(Re)))
		 putss(re);		       /* no Re: , add one ourselves */
	      putss(chp);putnl();
	    }
	   if(*refr.rexp||*msid.rexp)	 /* any Message-ID: or References: ? */
	    { if(!overrideh(references,-1))
	       { putss(references);
		 if(*refr.rexp)
		  { putss(refr.rexp);
		    if(*msid.rexp)
		       putnl();
		  }
		 if(*msid.rexp)
		  { putss(msid.rexp);putnl();
		  }
	       }
	      if(*msid.rexp&&!overrideh(inreplyto,-1))
	       { putss(inreplyto);putss(msid.rexp);putnl();
	       }
	    }
	 }
	else
	 { putcs(' ');putss(ctime(&t));
	 }
      }
     if(!areply)
	putssn(buf,p-1);
     for(hlp=hlist;hlp;hlp=hlp->next)
	if(hlp->htype==h_was)
	   hlp->htype=h_add;		      /* enable disabled field again */
	else if(hlp->hline[hlp->hlen])
	 { putss(hlp->hline);putnl();		    /* inject our new fields */
	 }
     putnl();
   }
  if(areply&&!keepb||extract)
   { if(split)
	closemine();
     opensink();					 /* discard the body */
   }
  p=0;lnl=1;					 /* clear buffer, important! */
  if(!bogus&&!split)
     for(;;putcs(i))
	Nextchar(i);
  for(;;)					       /* continue the quest */
   { do						 /* read line until not From */
      { if(p==buflen-1)
	   buf=realloc(buf,++buflen);
	Nextchar(i=buf[p]);
	if(++p==STRLEN(mboxseparator))
	   if(!strncmp(mboxseparator,buf,STRLEN(mboxseparator)))
	    { if(every)
	       { flushseparator(i,p);goto splitit;	 /* optionally flush */
	       }
	      else if(split&&lnl)
		 lnl=2;			   /* mark line as possible postmark */
	      else if(bogus)					   /* disarm */
	       {
#ifndef MAILBOX_SEPARATOR
		 putcs(ESCAP);break;
#else
		 Nextchar(i);*buf=' ';putssn(buf,p);*buf=i;p=1;continue;
#endif
	       }
	    }
	if(lnl==1&&digest)
	 { ll=maxindex(cdigest);
	   do				      /* check for new digest header */
	      if(p==cdigest[ll].lnr&&!strncmp(buf,cdigest[ll].hedr,p))
		 goto splitit;
	   while(ll--);
	 }
      }
     while(i!='\n'&&(lnl==2||p<dig_HDR_LEN));
     if(lnl==2)
      { buf[p]='\0';		 /* perform more thorough check for postmark */
	if((ll=strcspn(chp=pstrspn(buf+STRLEN(From)," ")," \t\n"))&&
	 *(chp+=ll)==' '&&(ll= *(chp=pstrspn(chp," ")))!='\t'&&ll!='\n')
	 {
splitit:   if((fclose(mystdout)==EOF||errout==EOF)&&!quiet)
	    { nlog(couldntw);log(", continuing...\n");split= -1;
	    }
	   if(!nowait)
	      waitforit();
	   startprog(argv);ch=i;--p;lnl=0;goto startover;
	 }
      }
     if(areply&&bogus&&*buf!='\n')
	putcs(ESCAP);					  /* escape the body */
     lnl=p==1;putssn(buf,p);p=0;
     if(i!='\n')
	do Nextchar(i);
	while(putcs(i),i!='\n');
   }
foundeof:
  putssn(buf,p);closemine();child= -1;waitforit();	/* wait for everyone */
  return split<0?EX_IOERR:EX_OK;
}

log(a)const char*const a;
{ fputs(a,stderr);
}

logqnl(a)const char*a;
{ log(" \"");log(a);log("\"\n");
}

putss(a)const char*a;
{ while(*a)
     putcs(*a++);
}

tputssn(a,l)const char*a;size_t l;
{ while(l--)
     putcs(*a++);
}

startprog(argv)const char*const*const argv;
{ int poutfd[2];
  if(!nrtotal)
     goto squelch;
  if(nrskip)
   { --nrskip;
squelch:
     opensink();return;
   }
  if(nrtotal>0)
     --nrtotal;
  dup(oldstdout);pipe(poutfd);
  if(!(child=fork()))
   { close(oldstdout);close(PWRO);fclose(stdin);dup(PRDO);close(PRDO);
     shexec(argv);
   }
  close(STDOUT);close(PRDO);
  if(STDOUT!=dup(PWRO)||!(mystdout=fdopen(STDOUT,"a")))
     nofild();
  close(PWRO);
  if(-1==child)
   { nlog("Can't fork\n");exit(EX_OSERR);
   }
}

nofild()
{ nlog("File table full\n");exit(EX_OSERR);
}

waitforit()
{ int i;pid_t j;
  while(child!=(j=wait(&i))||(i&127)==127)
    if(-1==j)
       return;
}

nlog(a)const char*const a;
{ log(NAMEPREFIX);log(a);
}

closemine()
{ if((fclose(mystdout)==EOF||errout==EOF)&&!quiet)
   { nlog(couldntw);log("\n");;exit(EX_IOERR);
   }
}

opensink()
{ if(!(mystdout=fopen(DevNull,"a")))
     nofild();
}

putnl()
{ putcs('\n');
}

strnicmp(a,b,l)register const char*a,*b;register unsigned l;
{ int i,j;
  if(l)						 /* case insensitive strncmp */
     do
      { while(*a&&*a==*b&&--l)
	   ++a,++b;
	if(!l)
	   break;
	if((i= *a++)>='A'&&i<='Z')
	   i+='a'-'A';
	if((j= *b++)>='A'&&j<='Z')
	   j+='a'-'A';
	if(j!=i)
	   return i>j?1:-1;
      }
     while(i&&j&&--l);
  return 0;
}
