/************************************************************************
 *	Custom regular expression library, *fully* egrep compatible	*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: regexp.c,v 2.4 1991/10/18 15:34:23 berg Rel $";
#endif
#include "config.h"
#include "procmail.h"
#include "shell.h"

#define R_BEG_GROUP	'('
#define R_END_GROUP	')'
#define R_OR		'|'
#define R_0_OR_MORE	'*'
#define R_0_OR_1	'?'
#define R_1_OR_MORE	'+'
#define R_DOT		'.'
#define R_SOL		'^'
#define R_EOL		'$'
#define R_BEG_CLASS	'['
#define R_NOT_CLASS	'^'
#define R_RANGE		'-'
#define R_END_CLASS	']'
#define R_ESCAPE	'\\'

#define OPB	256
enum opcode {OPC_EPS=OPB,OPC_CLASS,OPC_DOT,OPC_FIN};

#define bit_type		unsigned
#define bit_bits		(sizeof(bit_type)*8)
#define bit_index(which)	((unsigned)(which)/bit_bits)
#define bit_mask(which)		((unsigned)1<<(unsigned)(which)%bit_bits)
#define bit_toggle(name,which)	(name[bit_index(which)]^=bit_mask(which))
#define bit_test(name,which)	(!!(name[bit_index(which)]&bit_mask(which)))
#define bit_set(name,which,value)	\
 (value?(name[bit_index(which)]|=bit_mask(which)):\
 (name[bit_index(which)]&=~bit_mask(which)))
#define bit_field(name,size)	bit_type name[((size)+bit_bits-1)/bit_bits]

#define SZ(x)		(sizeof(struct x))
#define Ceps		(struct eps*)

/* the spawn and stack members are reused in the normal opcodes as pc fields */
static struct eps{int opc;struct eps*next,*spawn,*stack;}*r,aleps;
static uchar*p;
static ignore_case;

struct chclass {int opc_;struct eps*next_,*spawn_,*stack_;bit_field(c,256);};

static puteps(spot,to,aswell)struct eps*const spot;    /* epsilon transition */
 const struct eps*const to,*const aswell;
{ spot->opc=OPC_EPS;spot->next=to!=spot?Ceps to:Ceps aswell;
  spot->spawn=aswell!=spot?Ceps aswell:Ceps to;spot->stack=0;
}

static putneps(spot,to)struct eps*const spot;const struct eps*const to;
{ puteps(spot,to,spot+1);
}

#define rAc	(((struct chclass*)r)->c)

static bseti(i,j)unsigned i;const int j;
{ bit_set(rAc,i,j);			   /* mark 'i' as being in the class */
  if(ignore_case)				  /* mark the other case too */
   { if(i-'A'<26)						/* uppercase */
	i+='a'-'A';
     else if(i-'a'<26)						/* lowercase */
	i-='a'-'A';
     else return;						  /* no case */
     bit_set(rAc,i,j);
   }
}

static por();

static psimp(e)struct eps const*const e;
{ switch(*p)
   { case R_BEG_GROUP:				  /* not so simple after all */
	++p;por(e);return;
     case R_BEG_CLASS:					   /* a simple class */
      { uchar i,j=R_NOT_CLASS==*++p;		       /* char to save space */
	if(e)
	 { r->opc=OPC_CLASS;r->next=Ceps e;r->spawn=r->stack=0;
	   i=maxindex(rAc);
	   do rAc[i]=j?~0:0;			     /* preset the bit field */
	   while(i--);
	 }
	if(j)					  /* skip the 'not' modifier */
	 { ++p;bit_toggle(rAc,'\n');
	 }
	if(*p==R_END_CLASS)	 /* right at the start, can not mean the end */
	 { ++p;
	   if(e)
	    { i=R_END_CLASS;bit_toggle(rAc,R_END_CLASS);
	    }
	 }
	else if(*p==R_RANGE)				/* take it literally */
	 { ++p;
	   if(e)
	    { i=R_RANGE;bit_toggle(rAc,R_RANGE);
	    }
	 }
	for(;;++p)
	 { switch(*p)
	    { case R_END_CLASS:++p;
	      case '\0':r=Ceps((char*)r+SZ(chclass));return;
			    /* add code here to take care of special escapes */
	      case R_RANGE:
		 switch(*++p)
		  { default:
		       if(e)
			  while(++i<*p)		    /* mark all in the range */
			     bseti(i,!j);
		       break;
		    case '\0':case R_END_CLASS:--p;		/* literally */
		  }
	    }
	   if(e)
	      bseti(i= *p,!j);		      /* a normal character, mark it */
	 }
      }
     case '\0':return;
     case R_DOT:			 /* matches everything but a newline */
	if(e)
	 { r->opc=OPC_DOT;goto fine;
	 }
	goto fine2;
     case R_SOL:case R_EOL:		      /* match a newline (in effect) */
	if(e)
	 { r->opc='\n';goto fine;
	 }
	goto fine2;
     case R_ESCAPE:					  /* quote something */
	if(!*++p)					 /* nothing to quote */
	   --p;
			    /* add code here to take care of special escapes */
   }
  if(e)						      /* a regular character */
   { r->opc=ignore_case&&(unsigned)*p-'A'<26?*p+'a'-'A':*p;
fine:
     r->next=Ceps e;r->spawn=r->stack=0;
   }
fine2:
  ++p;++r;
}

static skipent()
{ switch(*p++)
   { case '\0':p--;break;
     case R_ESCAPE:				      /* something is quoted */
			    /* add code here to take care of special escapes */
	if(*p)
	   ++p;
	break;
     case R_BEG_GROUP:				 /* something big lies ahead */
	for(;;)
	 { switch(*p)
	    { case R_END_GROUP:++p;	      /* back in the civilised world */
	      case '\0':return;
	    }
	   skipent();			     /* skip it one entity at a time */
	 }
     case R_BEG_CLASS:					   /* skip class :-) */
	if(*p==R_NOT_CLASS)
	   ++p;
	if(*p==R_END_CLASS)
	   ++p;
	for(;;)
	   switch(*p++)
	    { case '\0':--p;
	      case R_END_CLASS:return;
			    /* add code here to take care of special escapes */
	    }
   }
}
					/* EOS is needed to save stack space */
#define EOS(x)	(p[1]==R_OR||p[1]==R_END_GROUP||!p[1]?Ceps e:(x))

static pnorm(e)struct eps const*const e;
{ void*pold;struct eps*rold;
  for(;;)
   { pold=p;skipent();rold=r;	   /* skip it first, so we can look if it is */
     switch(*p)			 /* followed by any of the postfix operators */
      { case R_0_OR_MORE:p=pold;++r;
	   if(e)			  /* first an epsilon, then the rest */
	    { psimp(rold);putneps(rold,EOS(r));
	    }
	   else
	      psimp((struct eps*)0);
	   goto incagoon;
	case R_1_OR_MORE:p=pold;psimp((struct eps*)0);	   /* first the rest */
	   if(e)				      /* and then an epsilon */
	    { puteps(r,rold,EOS(r+1));p=pold;pold=r;r=rold;psimp(pold);
	    }
	   ++r;goto incagoon;
	case R_0_OR_1:p=pold;++r;psimp((struct eps*)0);
	   if(e)			  /* first an epsilon, then the rest */
	    { putneps(rold,r=EOS(r));p=pold;pold=r;r=rold+1;psimp(pold);
	    }
incagoon:  switch(*++p)			/* at the end of this group already? */
	    { case R_OR:case R_END_GROUP:case '\0':return;
	    }
	   continue;				 /* regular end of the group */
	case R_OR:case R_END_GROUP:case '\0':p=pold;psimp(e);return;
      }
     p=pold;psimp((struct eps*)0);
     if(e)			/* no fancy postfix operators, plain vanilla */
      { p=pold;pold=r;r=rold;psimp(pold);
      }
   }
}

static por(e)struct eps const*const e;
{ uchar*pold;struct eps*rold;
  for(;;)
     for(pold=p;;)
      { switch(*p)
	 { default:skipent();continue;		 /* still in this 'or' group */
	   case '\0':case R_END_GROUP:	       /* found the end of the group */
	      if(p==pold)				 /* empty 'or' group */
	       { if(e)
		    puteps(r,e,e);		/* misused epsilon as branch */
		 ++r;
	       }
	      else
	       { p=pold;pnorm(e);			/* normal last group */
	       }
	      if(*p)
		 ++p;
	      return;
	   case R_OR:rold=r++;
	      if(p==pold)				 /* empty 'or' group */
	       { if(e)
		    putneps(rold,e);			  /* special epsilon */
	       }
	      else
	       { p=pold;pnorm(e);	      /* normal 'or' group, first an */
		 if(e)				   /* epsilon, then the rest */
		    putneps(rold,r);
	       }
	      ++p;
	 }
	break;
      }
}

static findandrep(old,new)register struct eps*const*const old;
 struct eps*const new;
{ register struct eps*i;
  for(i=r;;++i)			     /* change all pointers from *old to new */
   { if(&i->next!=old&&i->next==*old)
	i->next=new;
     if(&i->spawn!=old&&i->spawn==*old)
	i->spawn=new;
     switch(i->opc)
      { case OPC_FIN:return;				  /* last one, ready */
	case OPC_CLASS:i=Ceps((char*)(i-1)+SZ(chclass));
      }
   }
}
    /* break up any closed epsilon circles, otherwise they can't be executed */
static fillout(stack)struct eps**const stack;
{ if((*stack)->opc!=OPC_EPS||(*stack)->stack)
     return 0;
  (*stack)->stack=(struct eps*)p;		    /* mark this one as used */
#define RECURS(nxt,spwn)	\
  do\
     if((*stack)->nxt->stack==(struct eps*)p)\
      { findandrep(*stack,(*stack)->nxt);*stack=(*stack)->spwn;return 1;\
      }\
  while(fillout(&(*stack)->nxt));
  RECURS(next,spawn);RECURS(spawn,next);return 0;		  /* recurse */
}

void*regcomp(a,ign_case)char const*const a;
{ struct eps*st;size_t i;      /* first a trial run, determine memory needed */
  p=(uchar*)a;ignore_case=ign_case;r= &aleps+2;por((struct eps*)0);p=(uchar*)a;
  st=malloc(i=(char*)r-(char*)&aleps);putneps(st,r=st+1);  /* really compile */
  por(Ceps((char*)st+i-SZ(eps)));r->opc=OPC_FIN;r->stack=0;	  /* add end */
  for(r=st;;++st)			 /* simplify the compiled code (i.e. */
     switch(st->opc)		      /* take out cyclic epsilon references) */
      { case OPC_FIN:return r;					 /* finished */
	case OPC_EPS:p=(uchar*)st;fillout(&st);break;	       /* check tree */
	case OPC_CLASS:st=Ceps((char*)(st-1)+SZ(chclass));	     /* skip */
      }
}

char*regexec(code,text,len,ign_case)struct eps*code;const uchar*text;
 long len;const int ign_case;
{ register struct eps*reg,*t,*stack,*other,*this;int i,th1,ot1;
  if(code[1].opc==OPC_EPS)
     ++code;
  (this=code)->stack=0;th1=offsetof(struct eps,spawn);
  ot1=offsetof(struct eps,stack);
#define XOR1		(offsetof(struct eps,spawn)^offsetof(struct eps,stack))
#define PC(this,t)	(*(struct eps**)((char*)(this)+(t)))
  i='\n';goto setups;	      /* make sure any beginning-of-line-hooks catch */
  do
   { i= *text++;			 /* get the next real-text character */
lastrun:				     /* switch this & other pc-stack */
     th1^=XOR1;ot1^=XOR1;this=other;
setups:
     reg=(other=stack=code)->next;goto nostack;
     do					 /* pop next entry off this pc-stack */
      { reg=(t=this)->next;this=PC(t,th1);PC(t,th1)=0;goto nostack;
	do				/* pop next entry off the work-stack */
	 { stack=(t=stack)->stack;t->stack=0;reg=t->spawn;
nostack:   switch(reg->opc-OPB)	    /* push spawned branch on the work-stack */
	    { default:
		 if(ign_case&&(unsigned)i-'A'<26)
		    i+='a'-'A';		     /* transmogrify it to lowercase */
		 if(i==reg->opc)		  /* regular character match */
		    goto yep;
		 break;
	      case OPC_EPS-OPB:reg->stack=stack;reg=(stack=reg)->next;
		 goto nostack;
	      case OPC_FIN-OPB:		   /* hurray!  complete regexp match */
		 return (char*)text-1;		/* return one past the match */
	      case OPC_CLASS-OPB:
		 if(bit_test(((struct chclass*)reg)->c,i))
		    goto yep;			       /* character in class */
		 break;
	      case OPC_DOT-OPB:				     /* dot-wildcard */
		 if(i!='\n')
yep:		    if(!PC(reg,ot1))		     /* state not yet pushed */
		     { PC(reg,ot1)=other;other=reg;    /* push location onto */
		     }					   /* other pc-stack */
	    }
	 }
	while(stack);			      /* the work-stack is not empty */
      }
     while(this!=code);			       /* this pc-stack is not empty */
   }
  while(--len>=0);				     /* still text to search */
  if(i>=0)
   { ++text;i= -1;goto lastrun;	    /* out of text, check if we just matched */
   }
  return 0;							 /* no match */
}
