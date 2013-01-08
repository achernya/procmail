/************************************************************************
 *	Collection of library-worthy routines				*
 *									*
 *	Copyright (c) 1990-1992, S.R. van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 ************************************************************************/
#ifdef RCS
static char rcsid[]="$Id: goodies.c,v 2.16 1992/04/09 16:16:41 berg Rel $";
#endif
#include "config.h"
#include "procmail.h"
#include "shell.h"

#define NOTHING_YET	(-1)	/* readparse understands a very complete    */
#define SKIPPING_SPACE	0	/* subset of the standard /bin/sh syntax    */
#define NORMAL_TEXT	1	/* that includes single-, double- and back- */
#define DOUBLE_QUOTED	2	/* quotes, backslashes and $subtitutions    */
#define SINGLE_QUOTED	3

/* sarg==0 : normal parsing, split up arguments like in /bin/sh
 * sarg==1 : environment assignment parsing, parse up till first whitespace
 * sarg==2 : normal parsing, split up arguments by single spaces
 */
readparse(p,fgetc,sarg)register char*p;int(*const fgetc)();const int sarg;
{ static i;int got;char*startb;
  for(got=NOTHING_YET;;)		    /* buf2 is used as scratch space */
loop:
   { i=fgetc();
     if(buf+linebuf-3<p)	    /* doesn't catch everything, just a hint */
      { log("Exceeded LINEBUF\n");p=buf+linebuf-3;goto ready;
      }
newchar:
     switch(i)
      { case EOF:	/* check sarg too to prevent warnings in the recipe- */
	   if(sarg!=2&&got>NORMAL_TEXT)		 /* condition expansion code */
early_eof:    log(unexpeof);
ready:	   if(got!=SKIPPING_SPACE||sarg)  /* not terminated yet or sarg==2 ? */
	      *p++='\0';
	   *p=TMNATE;return;
	case '\\':
	   if(got==SINGLE_QUOTED)
	      break;
	   switch(i=fgetc())
	    { case EOF:goto early_eof;			  /* can't quote EOF */
	      case '\n':continue;			/* concatenate lines */
	      case '#':
		 if(got>SKIPPING_SPACE) /* escaped comment at start of word? */
		    goto noesc;			/* apparently not, literally */
	      case ' ':case '\t':case '\'':
		 if(got==DOUBLE_QUOTED)
		    goto noesc;
	      case '"':case '\\':case '$':case '`':goto nodelim;
	    }
	   if(got>NORMAL_TEXT)
noesc:	      *p++='\\';		/* nothing to escape, just echo both */
	   break;
	case '`':
	   if(got==SINGLE_QUOTED)
	      goto nodelim;
	   for(startb=p;;)			       /* mark your position */
	    { switch(i=fgetc())			 /* copy till next backquote */
	       { case '\\':
		    switch(i=fgetc())
		     { case EOF:log(unexpeof);goto forcebquote;
		       case '\n':continue;
		       case '"':
			  if(got!=DOUBLE_QUOTED)
			     break;
		       case '\\':case '$':case '`':goto escaped;
		     }
		    *p++='\\';break;
		 case '"':
		    if(got!=DOUBLE_QUOTED)	/* missing closing backquote? */
		       break;
forcebquote:	 case EOF:case '`':
		  { int osh=sh;
		    *p='\0';
		    if(!(sh=!!strpbrk(startb,tgetenv(shellmetas))))
		     { const char*save=sgetcp;
		       sgetcp=p=tstrdup(startb);readparse(startb,sgetc,0);
		       free(p);sgetcp=save;		       /* chopped up */
		     }		    /* drop source buffer, read from program */
		    startb=fromprog(p=startb,startb);sh=osh;   /* restore sh */
		    if(!sarg&&got!=DOUBLE_QUOTED)
		     { i=0;startb=p;goto simplsplit;	      /* split it up */
		     }
		    if(i=='"'||got<=SKIPPING_SPACE)   /* missing closing ` ? */
		       got=NORMAL_TEXT;			     /* or sarg!=0 ? */
		    p=startb;goto loop;
		  }
		 case '\n':i=';';	       /* newlines separate commands */
	       }
escaped:      *p++=i;
	    }
	case '"':
	   switch(got)
	    { case DOUBLE_QUOTED:got=NORMAL_TEXT;continue;	/* closing " */
	      case SINGLE_QUOTED:goto nodelim;
	    }
	   got=DOUBLE_QUOTED;continue;				/* opening " */
	case '\'':
	   switch(got)
	    { case DOUBLE_QUOTED:goto nodelim;
	      case SINGLE_QUOTED:got=NORMAL_TEXT;continue;}	/* closing ' */
	   got=SINGLE_QUOTED;continue;				/* opening ' */
	case '#':
	   if(got>SKIPPING_SPACE)		/* comment at start of word? */
	      break;
	   while((i=fgetc())!=EOF&&i!='\n');		    /* skip till EOL */
	   goto ready;
	case '$':
	   if(got==SINGLE_QUOTED)
	      break;
	   if(EOF==(i=fgetc()))
	    { *p++='$';goto ready;
	    }
	   startb=buf2;
	   if(i=='{')						  /* ${name} */
	    { while(EOF!=(i=fgetc())&&alphanum(i))
		 *startb++=i;
	      *startb='\0';
	      if(i!='}')
	       { log("Bad substitution of");logqnl(buf2);continue;
	       }
	      i='\0';
	    }
	   else if(alphanum(i))					    /* $name */
	    { do *startb++=i;
	      while(EOF!=(i=fgetc())&&alphanum(i));
	      if(i==EOF)
		 i='\0';
	      *startb='\0';
	    }
	   else if(i=='$')					  /* $$ =pid */
	    { ultstr(0,(unsigned long)thepid,p);goto ieofstr;
	    }
	   else if(i=='-')				   /* $- =lastfolder */
	    { strcpy(p,lastfolder);
ieofstr:      i='\0';goto eofstr;
	    }
	   else
	    { *p++='$';goto newchar;		       /* not a substitution */
	    }
	   startb=(char*)tgetenv(buf2);
	   if(!sarg&&got!=DOUBLE_QUOTED)
simplsplit:   for(;;startb++)		  /* simply split it up in arguments */
	       { switch(*startb)
		  { case ' ':case '\t':case '\n':
		       if(got<=SKIPPING_SPACE)
			  continue;
		       *p++='\0';got=SKIPPING_SPACE;continue;
		    case '\0':goto eeofstr;
		  }
		 *p++= *startb;got=NORMAL_TEXT;
	       }
	   else
	    { strcpy(p,startb);				   /* simply copy it */
eofstr:	      if(got<=SKIPPING_SPACE)		/* can only occur if sarg!=0 */
		 got=NORMAL_TEXT;
	      p=strchr(p,'\0');
	    }
eeofstr:   if(i)			     /* already read next character? */
	      goto newchar;
	   continue;
	case ' ':case '\t':
	   switch(got)
	    { case NORMAL_TEXT:
		 if(sarg==1)
		    goto ready;		/* already fetched a single argument */
		 got=SKIPPING_SPACE;*p++=sarg?' ':'\0';	 /* space or \0 sep. */
	      case NOTHING_YET:case SKIPPING_SPACE:continue;	/* skip space */
	    }
	case '\n':
	   if(got<=NORMAL_TEXT)
	      goto ready;			    /* EOL means we're ready */
      }
nodelim:
     *p++=i;					   /* ah, a normal character */
     if(got<=SKIPPING_SPACE)		 /* should we bother to change mode? */
	got=NORMAL_TEXT;
   }
}

ultstr(minwidth,val,dest)unsigned long val;char*dest;
{ int i;unsigned long j;
  j=val;i=0;					   /* a beauty, isn't it :-) */
  do i++;					   /* determine needed width */
  while(j/=10);
  while(--minwidth>=i)				 /* fill up any excess width */
     *dest++=' ';
  *(dest+=i)='\0';
  do *--dest='0'+val%10;			  /* display value backwards */
  while(val/=10);
}

sputenv(a)char*a;	      /* smart putenv, the way it was supposed to be */
{ static struct lienv{struct lienv*next;char name[255];}*myenv;
  static alloced;int i,remove;char*split,**preenv;struct lienv*curr,**last;
  yell("Assigning",a);remove=0;
  if(!(split=strchr(a,'=')))			   /* assignment or removal? */
     remove=1,split=strchr(a,'\0');
  i=split-a;
  for(curr= *(last= &myenv);curr;curr= *(last= &curr->next))	/* is it one */
     if(!strncmp(a,curr->name,i)&&curr->name[i]=='=')  /* I created earlier? */
      { split=curr->name;*last=curr->next;free(curr);
	for(preenv=environ;*preenv!=split;preenv++);
	goto wipenv;
      }
  for(preenv=environ;*preenv;preenv++)		    /* is it in the standard */
     if(!strncmp(a,*preenv,i)&&(*preenv)[i]=='=')	     /* environment? */
wipenv:
      { while(*preenv=preenv[1])   /* wipe this entry out of the environment */
	   preenv++;
	break;
      }
  i=(preenv-environ+2)*sizeof*environ;
  if(alloced)		   /* have we ever alloced the environ array before? */
     environ=realloc(environ,i);
  else
     alloced=1,environ=tmemmove(malloc(i),environ,i-sizeof*environ);
  if(!remove)		  /* if not remove, then add it to both environments */
   { for(preenv=environ;*preenv;preenv++);
     curr=malloc(ioffsetof(struct lienv,name[0])+(i=strlen(a)+1));
     tmemmove(*preenv=curr->name,a,i);preenv[1]=0;curr->next=myenv;
     myenv=curr;
   }
}
			    /* strtol replacement which lacks range checking */
#ifdef NOstrtol
long strtol(start,ptr,base)const char*start,**const ptr;
{ long result;const char*str=start;unsigned i;int sign,found;
  if(base>=36||base<(sign=found=result=0))
     goto fault;
  for(;;str++)					  /* skip leading whitespace */
   { switch(*str)
      { case '\t':case '\n':case '\v':case '\f':case '\r':case ' ':continue;
      }
     break;
   }
  switch(*str)						       /* any signs? */
   { case '-':sign=1;
     case '+':str++;
   }
  if(*str=='0')						 /* leading zero(s)? */
   { start++;
     if((i= *++str)=='x'||i=='X')			/* leading 0x or 0X? */
	if(!base||base==16)
	   base=16,str++;			    /* hexadecimal all right */
	else
	   goto fault;
     else if(!base)
	base=8;						 /* then it is octal */
   }
  else if(!base)
     base=10;						  /* or else decimal */
  goto jumpin;
  do
   { found=1;result=result*base+i;++str;		 /* start converting */
jumpin:
     if((i= *str-'0')<10);
     else if(i-'A'+'0'<26)
	i-='A'-10-'0';
     else if(i-'a'+'0'<26)
	i-='a'-10-'0';
     else
	break;						/* not of this world */
   }
  while(i<base);				      /* still of this world */
fault:
  if(ptr)
    *ptr=found?str:start;			       /* how far did we get */
  return sign?-result:result;
}
#endif
