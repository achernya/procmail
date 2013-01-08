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
static char rcsid[]="$Id: formail.c,v 2.26 1992/06/03 13:17:41 berg Rel $";
#endif
static char rcsdate[]="$Date: 1992/06/03 13:17:41 $";
#include "config.h"					  /* slight overkill */
#include "includes.h"
#include "strpbrk.h"
#include <ctype.h>

char*pstrspn();

#define BSIZE		128

#define NAMEPREFIX	"formail: "
#define HEAD_DELIMITER	':'

#define Re		(re+1)
#define putssn(a,l)	tputssn(a,(size_t)(l))
#define putcs(a)	(errout=putc(a,mystdout))
#define PRDO		poutfd[0]
#define PWRO		poutfd[1]
#define FLD_HEADSIZ	((size_t)offsetof(struct field,fld_text[0]))

static const char couldntw[]="Couldn't write to stdout",unknown[]=UNKNOWN,
 outofmem[]="Out of memory\n",re[]=" Re:",OldP[]=OLD_PREFIX,fmusage[]=FM_USAGE,
 From_[]=		FROM,				/* VNIX 'From ' line */
 path[]=		"Path:",				   /* USENET */
 article[]=		"Article ",				   /* USENET */
 returnpath[]=		"Return-Path:",				  /* RFC 822 */
 received[]=		"Received:",				/* ditto ... */
 replyto[]=		"Reply-To:",
 Fromm[]=		"From:",
 sender[]=		"Sender:",
 res_replyto[]=		"Resent-Reply-To:",
 res_from[]=		"Resent-From:",
 res_sender[]=		"Resent-Sender:",
 date[]=		"Date:",
 res_date[]=		"Resent-Date:",
 to[]=			"To:",
 res_to[]=		"Resent-To:",
 cc[]=			"Cc:",
 res_cc[]=		"Resent-Cc:",
 bcc[]=			"Bcc:",
 res_bcc[]=		"Resent-Bcc:",
 messageid[]=		"Message-ID:",
 res_messageid[]=	"Resent-Message-ID:",
 inreplyto[]=		"In-Reply-To:",
 references[]=		"References:",
 keywords[]=		"Keywords:",
 subject[]=		"Subject:",
 scomments[]=		"Comments:",
 encrypted[]=		"Encrypted:",
 errorsto[]=		"Errors-To:",		       /* sendmail extension */
 retreceiptto[]=	"Return-Receipt-To:",			/* ditto ... */
 precedence[]=		"Precedence:",
 priority[]=		"Priority:",			    /* ELM extension */
 summary[]=		"Summary:",			 /* USENET extension */
 organisation[]=	"Organisation:",			/* ditto ... */
 aorganization[]=	"Organization:",
 newsgroups[]=		"Newsgroups:",
 lines[]=		"Lines:",
 originator[]=		"Originator:",
 nntppostinghost[]=	"Nntp-Posting-Host:",
 status[]=		"Status:",			 /* mailer extension */
 x_[]=			"X-";				/* general extension */
#define ssl(str)	str,STRLEN(str)
#define bsl(str)	{ssl(str)}
const char binsh[]=BinSh;
/*
 *	sender determination fields in order of importance reliability
 *	reply-address determination fields (wrepl specifies the weight)
 */
static const struct {const char*head;int len,wrepl;}sest[]=
{ {ssl(errorsto),5},{ssl(retreceiptto),6},{ssl(sender),0},{ssl(replyto),4},
  {ssl(Fromm),2},{ssl(returnpath),1}
};
/*
 *	digest splitting fields (if you need to add one, send me a mail, I
 *	might be interested in including it in the next release)
 */
static const struct {const char*hedr;int lnr;}cdigest[]=
{ bsl(path),bsl(article),bsl(returnpath),bsl(received),bsl(replyto),bsl(Fromm),
  bsl(sender),bsl(res_replyto),bsl(res_from),bsl(res_sender),bsl(date),
  bsl(res_date),bsl(to),bsl(res_to),bsl(cc),bsl(res_cc),bsl(bcc),bsl(res_bcc),
  bsl(messageid),bsl(res_messageid),bsl(inreplyto),bsl(references),
  bsl(keywords),bsl(subject),bsl(scomments),bsl(encrypted),bsl(errorsto),
  bsl(precedence),bsl(retreceiptto),bsl(summary),bsl(organisation),
  bsl(aorganization),bsl(newsgroups),bsl(status),bsl(lines),bsl(originator),
  bsl(nntppostinghost),bsl(priority)
};

static struct saved{const char*const headr;const int lenr;int rexl;char*rexp;}
 rex[]={bsl(subject),bsl(references),bsl(messageid),bsl(date)};
#define subj	(rex+0)
#define refr	(rex+1)
#define msid	(rex+2)
#define hdate	(rex+3)

#ifdef sMAILBOX_SEPARATOR
#define emboxsep	smboxsep
#define MAILBOX_SEPARATOR
static const char smboxsep[]=sMAILBOX_SEPARATOR;
#endif /* sMAILBOX_SEPARATOR */
#ifdef eMAILBOX_SEPARATOR
#ifdef emboxsep
#undef emboxsep
#else
#define MAILBOX_SEPARATOR
#endif
static const char emboxsep[]=eMAILBOX_SEPARATOR;
#endif /* eMAILBOX_SEPARATOR */

static errout,oldstdout,quiet,buflast;
static pid_t child= -1;
static FILE*mystdout;
static size_t nrskip,nrtotal= -1,buflen,buffilled;
static char*buf;
static struct field{size_t id_len;size_t tot_len;struct field*fld_next;
 char fld_text[255];}*rdheader,*iheader,*Iheader,*aheader,*Aheader,*xheader,
    *nheader;

struct field*findf(p,hdr)const struct field*const p,*hdr;
{ size_t i;char*chp;		/* find a field in the linked list of fileds */
  for(i=p->id_len,chp=(char*)p->fld_text;hdr;hdr=hdr->fld_next)
     if(i==hdr->id_len&&!strnicmp(chp,hdr->fld_text,i))	 /* case insensitive */
	return(struct field*)hdr;
  return(struct field*)0;
}

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

tfree(a)void*a;
{ free(a);
}

#include "shell.h"
						 /* skips an RFC 822 address */
char*skipwords(start,end)const char*start,*const end;
{ int delim='>',firstch;
  if((firstch= *start)=='<')
     goto machref;				 /* machine-usable reference */
  do
   { switch(*start)
      { default:					      /* normal word */
	   if(firstch!='(')	     /* if it did *not* start with a comment */
	    { const char*p;
notend:	      if(p=strpbrk(start,"([\"<,; \t\n"))	/* find next special */
		 switch(*p)			     /* is it a big address? */
		  { case '(':case '[':case '"':start=p;continue;
		    default:return(char*)p;		/* address delimiter */
		  }
	      start=strchr(start,'\0')+1;goto notend; /* it can't be the end */
	    }
	case '(':delim=')';break;				  /* comment */
	case '[':delim=']';break;			   /* domain-literal */
	case '"':delim='"';
      }
machref:
    {int i;
     do
	if((i= *start++)==delim)		 /* corresponding delimiter? */
	   break;
	else if(i=='\\'&&*start)		    /* skip quoted character */
	   ++start;
     while(start<end);						/* anything? */
    }
   }
  while(start<end);
  return(char*)end;
}

main(lastm,argv)const char*const argv[];
{ int i,j,split=0,force=0,bogus=1,every=0,areply=0,trust=0,digest=0,
    nowait=0,keepb=0,minfields=0;
  size_t lnl;char*chp,*namep;struct field*fldp,*fp2,**afldp,*fdate;
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
	   case FM_EVERY:every=1;continue;
	   case FM_DIGEST:digest=1;continue;
	   case FM_NOWAIT:nowait=1;continue;
	   case FM_KEEPB:keepb=1;continue;
	   case FM_QUIET:quiet=1;continue;
	   case FM_SPLIT:split=1;
	      if(!*chp&&*++argv)
		 goto parsedoptions;
	      goto usg;
	   case HELPOPT1:case HELPOPT2:log(fmusage);log(FM_HELP);goto xusg;
	   case FM_MINFIELDS:
	     if(!*chp&&!(chp=(char*)*++argv))	/* concatenated or seperate? */
		goto usg;
number:	   default:
	      if(*chp-'0'>(unsigned)9)
usg:	       { log(fmusage);
xusg:		 return EX_USAGE;
	       }
	      i=strtol(chp,(char**)0,10);
	      switch(lastm)			/* where does the number go? */
	       { case FM_SKIP:nrskip=i;break;
		 case FM_TOTAL:nrtotal=i;break;
		 default:minfields=i;
	       }
	      break;
	   case FM_BOGUS:bogus=0;continue;
	   case FM_ADD_IFNOT:case FM_ADD_ALWAYS:case FM_REN_INSERT:
	   case FM_DEL_INSERT:case FM_EXTRACT:
	      if(!*chp&&!(chp=(char*)*++argv))	/* concatenated or seperate? */
		 goto usg;
	      if(!breakfield(chp,i=strlen(chp)))
	       { nlog("Invalid field-name: \"");log(chp);log("\"\n");goto usg;
	       }
	      chp[i++]='\n';			       /* terminate the line */
	      addfield(lastm==FM_REN_INSERT?&iheader:lastm==FM_DEL_INSERT?
	       &Iheader:lastm==FM_ADD_IFNOT?&aheader:lastm==FM_ADD_ALWAYS?
	       &Aheader:&xheader,chp,i);
	   case '\0':;
	 }
	break;
      }
   }
parsedoptions:
  mystdout=stdout;signal(SIGPIPE,SIG_IGN);
  if(split)
   { oldstdout=dup(STDOUT);fclose(stdout);startprog(argv);
     if(!minfields)			       /* no user specified minimum? */
	minfields=DEFminfields;				 /* take our default */
   }
  else if(every||digest||minfields)	      /* these combinations are only */
     goto usg;				  /* valid in combination with split */
  if(!areply&&keepb)
     goto usg;
  namep=malloc(1);buf=malloc(buflen=BSIZE);	       /* prime some buffers */
  i=maxindex(rex);
  do rex[i].rexp=malloc(1);
  while(i--);
  fdate=0;addfield(&fdate,date,STRLEN(date)); /* fdate is only for searching */
  while((buflast=getchar())=='\n');		     /* skip leading garbage */
#ifdef sMAILBOX_SEPARATOR
  if(!readhead())				      /* check for a leading */
   { if(!strncmp(smboxsep,buf,STRLEN(smboxsep)))	/* mailbox separator */
      { buffilled=0;goto startover;				  /* skip it */
      }
   }
  else
#endif
startover:
     while(readhead());				 /* read in the whole header */
 {size_t namel=0;size_t lenparkedbuf;void*parkedbuf;
  i=maxindex(rex);
  do rex[i].rexl=0;
  while(i--);					 /* all state has been reset */
  for(fldp=rdheader;fldp;fldp=fldp->fld_next)	    /* go through the linked */
   { int nowm;					    /* list of header-fields */
     chp=fldp->fld_text;
     if((j=fldp->id_len)==STRLEN(From_)&&fldp==rdheader&&eqFrom_(chp))
      { nowm=trust?1:3/*wreply*/;goto foundfrom;	    /* leading From_ */
      }
     concatenate(fldp);i=maxindex(sest);	  /* look for other "sender" */
     while((sest[i].len!=j||strnicmp(sest[i].head,chp,j))&&i--);   /* fields */
     if(i>=0)						  /* found anything? */
      { const char*saddr,*end;
	nowm=areply?keepb&&sest[i].head==replyto?    /* determine the weight */
	 maxindex(sest)+1:sest[i].wrepl:i;		     /* of this find */
foundfrom:
	saddr=end=chp+fldp->tot_len-1;chp+=j;
	for(;;chp=skipwords(chp,end))			/* skip RFC 822 wise */
	 { switch(*(chp=pstrspn(chp,",; \t")))
	    { default:
		 if(saddr==end)		   /* if we haven't got anything yet */
		    saddr=chp;			/* this might be the address */
	      case '(':continue;		  /* a comment, don't bother */
	      case '<':saddr=chp;		  /* hurray, machine useable */
	      case '\n':;
	    }
	   break;
	 }	       /* check if the address has any length and extract it */
	if((i=skipwords(saddr,end)-saddr)&&(!namel||nowm>lastm))
	 { tmemmove(namep=realloc(namep,i+1),saddr,namel=i);
	   lastm=mystrstr(namep,".UUCP",end)?nowm-maxindex(sest)-1:nowm;
	 }
      }					   /* save headers for later perusal */
     i=maxindex(rex);chp=fldp->fld_text;j=fldp->id_len;	  /* e.g. for areply */
     while((rex[i].lenr!=j||strnicmp(rex[i].headr,chp,j))&&i--);
     chp+=j;
     if(i>=0&&(j=fldp->tot_len-j)>1)			  /* found anything? */
	tmemmove(rex[i].rexp=realloc(rex[i].rexp,rex[i].rexl=j),chp,j);
   }
  tmemmove(parkedbuf=malloc(buffilled),buf,lenparkedbuf=buffilled);
  buffilled=0;	      /* move the contents of buf out of the way temporarily */
  if(areply)		      /* autoreply requested, we clean up the header */
   { for(fldp= *(afldp= &rdheader);fldp;)
	if(!(fp2=findf(fldp,iheader))||fp2->id_len<fp2->tot_len-1)
	   *afldp=fldp->fld_next,free(fldp),fldp= *afldp;      /* remove all */
	else					/* except the ones mentioned */
	   fldp= *(afldp= &fldp->fld_next);		       /* as -i ...: */
     loadbuf(to,STRLEN(to));loadchar(' ');	   /* generate the To: field */
     if(namel)		       /* did we find a valid return address at all? */
	loadbuf(namep,namel);			      /* then insert it here */
     else
	loadbuf(unknown,STRLEN(unknown));	    /* or insert our default */
     loadchar('\n');addbuf();			       /* add it to rdheader */
     if(subj->rexl)				      /* any Subject: found? */
      { loadbuf(subject,STRLEN(subject));	  /* sure, check for leading */
	if(strnicmp(pstrspn(chp=subj->rexp," \t"),Re,STRLEN(Re)))     /* Re: */
	   loadbuf(re,STRLEN(re));	       /* no Re: , add one ourselves */
	loadsaved(subj);addbuf();
      }
     if(refr->rexl||msid->rexl)		   /* any References: or Message-ID: */
      { loadbuf(references,STRLEN(references));	  /* yes, insert References: */
	if(refr->rexl)
	 { if(msid->rexl)	    /* if we're going to append a Message-ID */
	      --refr->rexl;		    /* suppress the trailing newline */
	   loadsaved(refr);
	 }
	if(msid->rexl)
	   loadsaved(msid);		       /* here's our missing newline */
	addbuf();
      }
     if(msid->rexl)			 /* do we add an In-Reply-To: field? */
	loadbuf(inreplyto,STRLEN(inreplyto)),loadsaved(msid),addbuf();
   }				       /* are we allowed to add From_ lines? */
  else if(!force&&(!rdheader||!eqFrom_(rdheader->fld_text))) /* was missing? */
   { struct field*old;time_t t;		     /* insert a From_ line up front */
     t=time((time_t*)0);old=rdheader;rdheader=0;loadbuf(From_,STRLEN(From_));
     if(namel)				  /* we found a valid return address */
	loadbuf(namep,namel);
     else
	loadbuf(unknown,STRLEN(unknown));
     if(!hdate->rexl||findf(fdate,iheader)||findf(fdate,Iheader))   /* Date: */
	loadchar(' '),chp=ctime(&t),loadbuf(chp,strlen(chp));	/* no Date:, */
     else					 /* we generate it ourselves */
	loadsaved(hdate);	      /* yes, found Date:, then copy from it */
     addbuf();rdheader->fld_next=old;
   }
  for(fldp=aheader;fldp;fldp=fldp->fld_next)
     if(!findf(fldp,rdheader))		       /* only add what didn't exist */
	addfield(&nheader,fldp->fld_text,fldp->tot_len);
  for(fldp= *(afldp= &rdheader);fldp;)
   { i=fldp->tot_len;
     if(findf(fldp,xheader))				   /* extract fields */
	putssn(fldp->fld_text+fldp->id_len,i-fldp->id_len);
     if(fp2=findf(fldp,Iheader))			    /* delete fields */
      { *afldp=fldp->fld_next;free(fldp);fldp= *afldp;continue;
      }
     else if((fp2=findf(fldp,iheader))&&!(areply&&fp2->id_len==fp2->tot_len-1))
      { *afldp=fldp=realloc(fldp,FLD_HEADSIZ+(fldp->tot_len=i+STRLEN(OldP)));
	tmemmove(fldp->fld_text+STRLEN(OldP),fldp->fld_text,i);
	tmemmove(fldp->fld_text,OldP,STRLEN(OldP));	    /* rename fields */
      }
     fldp= *(afldp= &fldp->fld_next);
   }					/* restore the saved contents of buf */
  tmemmove(buf,parkedbuf,buffilled=lenparkedbuf);free(parkedbuf);
 }
  if(xheader)				     /* we're just extracting fields */
     clearfield(&rdheader),clearfield(&nheader);	    /* throw it away */
  else			     /* otherwise, display the new & improved header */
   { flushfield(&rdheader);flushfield(&nheader);dispfield(Aheader);
     dispfield(iheader);dispfield(Iheader);putcs('\n');	  /* make sure it is */
   }						/* followed by an empty line */
  if(areply&&!keepb||xheader)	      /* decision time, do we keep the rest? */
   { if(split)
	closemine();
     opensink();					 /* discard the body */
   }
  lnl=1;					  /* last line was a newline */
  if(buffilled==1)		   /* the header really ended with a newline */
     buffilled=0;	      /* throw it away, since we already inserted it */
  do					 /* continue the quest, line by line */
   { if(!buffilled)				      /* is it really empty? */
	readhead();				      /* read the next field */
     if(!rdheader&&eqFrom_(buf))		 /* special case, From_ line */
	addbuf();	       /* add it manually, in case readhead() didn't */
     if(rdheader)		    /* anything looking like a header found? */
	if(eqFrom_(chp=rdheader->fld_text))	      /* check if it's From_ */
	 { register size_t k;
	   if(split&&(lnl||every)&&    /* more thorough check for a postmark */
	    (k=strcspn(chp=pstrspn(chp+STRLEN(From_)," \t")," \t\n"))&&
	    *pstrspn(chp+k," \t")!='\n')
	      goto accuhdr;		     /* ok, postmark found, split it */
	   if(bogus)						   /* disarm */
	      putcs(ESCAP);
	 }
	else if(split&&digest&&(lnl||every)&&digheadr())	  /* digest? */
accuhdr: { for(i=minfields;--i&&readhead()&&digheadr(););   /* found enough? */
	   if(!i)					   /* then split it! */
splitit:    { if(!lnl)	    /* did the previous mail end with an empty line? */
		 putcs('\n');			      /* but now it does :-) */
	      if((fclose(mystdout)==EOF||errout==EOF)&&!quiet)
		 nlog(couldntw),log(", continuing...\n"),split= -1;
	      if(!nowait)
		 waitforit();		 /* wait till the child has finished */
	      startprog(argv);goto startover;	    /* and there we go again */
	    }
	 }
#ifdef MAILBOX_SEPARATOR
     if(!strncmp(emboxsep,buf,STRLEN(emboxsep)))	     /* end of mail? */
      { if(split)		       /* gobble up the next start separator */
	 { buffilled=0;
#ifdef sMAILBOX_SEPARATOR
	   getline();buffilled=0;		 /* but only if it's defined */
#endif
	   if(buflast!=EOF)					   /* if any */
	      goto splitit;
	   break;
	 }
	else if(bogus)
	   goto putsp;				   /* escape it with a space */
      }
     else if(!strncmp(smboxsep,buf,STRLEN(smboxsep)&&bogus))
putsp:	putcs(' ');
#endif /* MAILBOX_SEPARATOR */
     lnl=buffilled==1;		      /* check if we just read an empty line */
     if(areply&&bogus)					  /* escape the body */
	if(fldp=rdheader)	      /* we already read some "valid" fields */
	 { register char*p;
	   rdheader=0;
	   do			       /* careful, then can contain newlines */
	    { fp2=fldp->fld_next;chp=fldp->fld_text;
	      do putcs(ESCAP),putssn(chp,(p=strchr(chp,'\n')+1)-chp);
	      while((chp=p)<fldp->fld_text+fldp->tot_len);
	      free(fldp);					/* delete it */
	    }
	   while(fldp=fp2);		       /* escape all fields we found */
	 }
	else
	 { if(buffilled>1)	  /* we don't escape empty lines, looks neat */
	      putcs(ESCAP);
	   goto flbuf;
	 }
     else if(rdheader)
	flushfield(&rdheader); /* beware, after this buf can still be filled */
     else
flbuf:	putssn(buf,buffilled),buffilled=0;
   }			       /* make sure the mail ends with an empty line */
  while(buffilled||!lnl||buflast!=EOF);
  closemine();child= -1;waitforit();			/* wait for everyone */
  return split<0?EX_IOERR:EX_OK;
}

concatenate(fldp)struct field*const fldp;   /* concatenate a continued field */
{ register char*p;register size_t l;
  l=fldp->tot_len;p=fldp->fld_text;
  while(l--)
     if(*p++=='\n'&&l)	     /* by substituting all newlines except the last */
	p[-1]=' ';
}

eqFrom_(a)const char*const a;
{ return!strncmp(a,From_,STRLEN(From_));
}
     /* checks if the last field in rdheader looks like a know digest header */
digheadr()
{ char*chp;int i,j;struct field*fp;
  for(fp=rdheader;fp->fld_next;fp=fp->fld_next);	 /* skip to the last */
  i=maxindex(cdigest);chp=fp->fld_text;j=fp->id_len;
  while((cdigest[i].lnr!=j||strnicmp(cdigest[i].hedr,chp,j))&&i--);
  return i>=0||j>STRLEN(x_)&&!strnicmp(x_,chp,STRLEN(x_));
}

breakfield(line,len)const char*const line;size_t len;	   /* look where the */
{ const char*p=line;			   /* fieldname ends (RFC 822 specs) */
  if(eqFrom_(line))				      /* special case, From_ */
     return STRLEN(From_);
  while(len--&&!iscntrl(*p))		    /* no control characters allowed */
   { switch(*p++)
      { default:continue;
	case HEAD_DELIMITER:len=p-line;return len==1?0:len;	  /* eureka! */
	case ' ':;					/* no spaces allowed */
      }
     break;
   }
  return 0;		    /* sorry, does not seem to be a legitimate field */
}

addfield(pointer,text,totlen)register struct field**pointer;	  /* add new */
 const char*const text;const size_t totlen;	   /* field to a linked list */
{ register struct field*p;
  while(*pointer)			      /* skip to the end of the list */
     pointer= &(*pointer)->fld_next;
  (*pointer=p=malloc(FLD_HEADSIZ+totlen))->fld_next=0;	 /* create the field */
  p->id_len=breakfield(text,totlen);
  tmemmove(p->fld_text,text,p->tot_len=totlen);		/* copy the contents */
}

clearfield(pointer)register struct field**pointer;	 /* delete the whole */
{ register struct field*p,*q;			    /* linked list of fields */
  for(p= *pointer,*pointer=0;p;p=q)
     q=p->fld_next,free(p);
}

flushfield(pointer)register struct field**pointer;	 /* delete and print */
{ register struct field*p,*q;				   /* them as you go */
  for(p= *pointer,*pointer=0;p;p=q)
     q=p->fld_next,putssn(p->fld_text,p->tot_len),free(p);
}

dispfield(p)const register struct field*p;   /* print list non-destructively */
{ for(;p;p=p->fld_next)
     if(p->id_len<p->tot_len-1)			 /* any contents to display? */
	putssn(p->fld_text,p->tot_len);
}

readhead()	    /* try and append one valid field to rdheader from stdin */
{ getline();
  if(!eqFrom_(buf))				    /* it's not a From_ line */
   { if(!breakfield(buf,buffilled))	   /* not the start of a valid field */
	return 0;
     for(;;getline())		      /* get the rest of the continued field */
      { switch(buflast)			     /* will this line be continued? */
	 { case ' ':case '\t':continue;			  /* yep, it sure is */
	 }
	break;
      }
   }
  else if(rdheader)
     return 0;				       /* the From_ line was a fake! */
  addbuf();return 1;		  /* phew, got the field, add it to rdheader */
}

addbuf()
{ addfield(&rdheader,buf,buffilled);buffilled=0;
}

getline()				   /* read a newline-terminated line */
{ if(buflast!=EOF)			     /* do we still have a leftover? */
     loadchar(buflast);				  /* load it into the buffer */
  if(buflast!='\n')
   { int ch;
     while((ch=getchar())!=EOF&&ch!='\n')
	loadchar(ch);				/* load the rest of the line */
     loadchar('\n');		    /* make sure (!), it ends with a newline */
   }		/* (some code in formail.c depends on a terminating newline) */
  return buflast=getchar();			/* look ahead, one character */
}

loadsaved(sp)const struct saved*const sp;	   /* load a some saved text */
{ switch(*sp->rexp)
   { default:loadchar(' ');	       /* make sure it has leading whitspace */
     case ' ':case '\t':;
   }
  loadbuf(sp->rexp,sp->rexl);
}

loadbuf(text,len)const char*const text;const size_t len;    /* append to buf */
{ if(buffilled+len>buflen)			  /* buf can't hold the text */
     buf=realloc(buf,buflen+=BSIZE);
  tmemmove(buf+buffilled,text,len);buffilled+=len;
}

loadchar(c)const int c;			      /* append one character to buf */
{ if(buffilled==buflen)
     buf=realloc(buf,buflen+=BSIZE);
  buf[buffilled++]=c;
}

log(a)const char*const a;				     /* error output */
{ fputs(a,stderr);
}

tputssn(a,l)const char*a;size_t l;
{ while(l--)
     putcs(*a++);
}

startprog(argv)const char*const*const argv;
{ int poutfd[2];
  if(!nrtotal)					/* no more mails to display? */
     goto squelch;
  if(nrskip)				  /* should we still skip this mail? */
   { --nrskip;							 /* count it */
squelch:
     opensink();return;
   }
  if(nrtotal>0)
     --nrtotal;							 /* count it */
  dup(oldstdout);pipe(poutfd);
  if(!(child=fork()))	/* DON'T fclose(stdin) here, provokes a bug on HP/UX */
   { close(STDIN);close(oldstdout);close(PWRO);dup(PRDO);close(PRDO);
     shexec(argv);
   }
  close(STDOUT);close(PRDO);
  if(STDOUT!=dup(PWRO)||!(mystdout=fdopen(STDOUT,"a")))
     nofild();
  close(PWRO);
  if(-1==child)
     nlog("Can't fork\n"),exit(EX_OSERR);
}

nofild()
{ nlog("File table full\n");exit(EX_OSERR);
}

waitforit()
{ int i;pid_t j;
  while(child!=(j=wait(&i))||WIFSTOPPED(i))
    if(-1==j)
       return;
}

nlog(a)const char*const a;
{ log(NAMEPREFIX);log(a);
}

logqnl(a)const char*const a;
{ log(" \"");log(a);log("\"\n");
}

closemine()
{ if((fclose(mystdout)==EOF||errout==EOF)&&!quiet)
     nlog(couldntw),log("\n"),exit(EX_IOERR);
}

opensink()
{ if(!(mystdout=fopen(DevNull,"a")))
     nofild();
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

mystrstr(whole,part,end)const char*whole,*const part,*end;
{ size_t i;
  for(end-=(i=strlen(part))+1;--end>=whole;)
     if(!strncmp(end,part,i))
	return 1;
  return 0;
}
