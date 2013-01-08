/************************************************************************
 *	Collection of library-worthy routines				*
 *									*
 *	Copyright (c) 1990-1991, S.R.van den Berg, The Netherlands	*
 *	The sources can be freely copied for non-commercial use.	*
 *	#include "README"						*
 *									*
 *	#include "STYLE"						*
 *									*
 ************************************************************************/
#ifdef	RCS
static char rcsid[]="$Id: goodies.c,v 2.3 1991/06/19 17:41:41 berg Rel $";
#endif
#include "config.h"
#include "procmail.h"
#include "shell.h"

#define NOTHING_YET	(-1)	/* readparse understands a very complete    */
#define SKIPPING_SPACE	0	/* subset of the standard /bin/sh syntax    */
#define NORMAL_TEXT	1	/* that includes single-, double- and back- */
#define DOUBLE_QUOTED	2	/* quotes, backslashes and $subtitutions    */
#define SINGLE_QUOTED	3

readparse(p,fgetc,sarg)register char*p;int(*const fgetc)();
 const int sarg;{static i;int got;char*startb;
 for(got=NOTHING_YET;;){		    /* buf2 is used as scratch space */
loop:
   i=fgetc();
   if(buf+linebuf-3<p){		    /* doesn't catch everything, just a hint */
      log("Exceeded LINEBUF\n");p=buf+linebuf-3;goto ready;}
newchar:
   switch(i){
      case EOF:
	 if(got>NORMAL_TEXT)
early_eof:  log(unexpeof);
ready:	 if(got!=SKIPPING_SPACE||sarg)	  /* not terminated yet or sarg==2 ? */
	    *p++='\0';
	 *p=TMNATE;return;
      case '\\':
	 if(got==SINGLE_QUOTED)
	    break;
	 switch(i=fgetc()){
	    case EOF:goto early_eof;			  /* can't quote EOF */
	    case '\n':continue;				/* concatenate lines */
	    case '#':
	       if(got>SKIPPING_SPACE)	/* escaped comment at start of word? */
		  goto noesc;			/* apparently not, literally */
	    case ' ':case '\t':case '\'':
	       if(got==DOUBLE_QUOTED)
		  goto noesc;
	    case '"':case '\\':case '$':case '`':goto nodelim;}
	 if(got>NORMAL_TEXT)
noesc:	    *p++='\\';			/* nothing to escape, just echo both */
	 break;
      case '`':
	 if(got==SINGLE_QUOTED)
	    goto nodelim;
	 for(startb=p;;){			       /* mark your position */
	    switch(i=fgetc()){			 /* copy till next backquote */
	       case '\\':
		  switch(i=fgetc()){
		     case EOF:log(unexpeof);goto forcebquote;
		     case '\n':continue;
		     case '"':
			if(got!=DOUBLE_QUOTED)
			   break;
		     case '\\':case '$':case '`':goto escaped;}
		  *p++='\\';break;
	       case '"':
		  if(got!=DOUBLE_QUOTED)       /* missing closing backquote? */
		     break;
forcebquote:   case EOF:case '`':*p='\0';
		  if(!(sh=!!strpbrk(startb,tgetenv(shellmetas)))){
		     const char*save=sgetcp;
		     sgetcp=p=tstrdup(startb);readparse(startb,sgetc,0);
		     free(p);sgetcp=save;} /* chopped up, drop source buffer */
		  startb=fromprog(p=startb,startb);	/* read from program */
		  if(got!=DOUBLE_QUOTED){
		     i=0;startb=p;goto simplsplit;}	      /* split it up */
		  if(i=='"')		  /* was there a missing closing ` ? */
		     got=NORMAL_TEXT;			 /* yes, terminate " */
		  p=startb;goto loop;
	       case '\n':i=';';}	       /* newlines separate commands */
escaped:    *p++=i;}
      case '"':
	 switch(got){
	    case DOUBLE_QUOTED:got=NORMAL_TEXT;continue;	/* closing " */
	    case SINGLE_QUOTED:goto nodelim;}
	 got=DOUBLE_QUOTED;continue;				/* opening " */
      case '\'':
	 switch(got){
	    case DOUBLE_QUOTED:goto nodelim;
	    case SINGLE_QUOTED:got=NORMAL_TEXT;continue;}	/* closing ' */
	 got=SINGLE_QUOTED;continue;				/* opening ' */
      case '#':
	 if(got>SKIPPING_SPACE)			/* comment at start of word? */
	    break;
	 while((i=fgetc())!=EOF&&i!='\n');		    /* skip till EOL */
	 goto ready;
      case '$':
	 if(got==SINGLE_QUOTED)
	    break;
	 if(EOF==(i=fgetc())){
	    *p++='$';goto ready;}
	 startb=buf2;
	 if(i=='{'){						  /* ${name} */
	    while(EOF!=(i=fgetc())&&alphanum(i))
	       *startb++=i;
	    *startb='\0';
	    if(i!='}'){
	       log("Bad substitution of");logqnl(buf2);continue;}
	    i='\0';}
	 else if(alphanum(i)){					    /* $name */
	    do *startb++=i;
	    while(EOF!=(i=fgetc())&&alphanum(i));
	    if(i==EOF)
	       i='\0';
	    *startb='\0';}
	 else if(i=='$'){					   /* $$=pid */
	    ultstr(0,(unsigned long)thepid,p);i='\0';goto eofstr;}
	 else{
	    *p++='$';goto newchar;}		       /* not a substitution */
	 startb=(char*)tgetenv(buf2);
	 if(got!=DOUBLE_QUOTED)
simplsplit: for(;;startb++){		  /* simply split it up in arguments */
	       switch(*startb){
		  case ' ':case '\t':case '\n':
		     if(got<=SKIPPING_SPACE)
			continue;
		     *p++='\0';got=SKIPPING_SPACE;continue;
		  case '\0':goto eeofstr;}
	       *p++= *startb;got=NORMAL_TEXT;}
	 else{
	    strcpy(p,startb);				   /* simply copy it */
eofstr:	    p=strchr(p,'\0');}
eeofstr: if(i)				     /* already read next character? */
	    goto newchar;
	 continue;
      case ' ':case '\t':
	 switch(got){
	    case NORMAL_TEXT:
	       if(sarg==1)
		  goto ready;		/* already fetched a single argument */
	       got=SKIPPING_SPACE;*p++=sarg?' ':'\0';	 /* space or \0 sep. */
	    case NOTHING_YET:case SKIPPING_SPACE:continue;}    /* skip space */
      case '\n':
	 if(got<=NORMAL_TEXT)
	    goto ready;}			    /* EOL means we're ready */
nodelim:
   *p++=i;					   /* ah, a normal character */
   if(got<=SKIPPING_SPACE)		 /* should we bother to change mode? */
      got=NORMAL_TEXT;}}

ultstr(minwidth,val,dest)unsigned long val;char*dest;{int i;unsigned long j;
 j=val;i=0;					   /* a beauty, isn't it :-) */
 do i++;					   /* determine needed width */
 while(j/=10);
 while(--minwidth>=i)				 /* fill up any excess width */
   *dest++=' ';
 *(dest+=i)='\0';
 do *--dest='0'+val%10;				  /* display value backwards */
 while(val/=10);}

sputenv(a)char*a;{	      /* smart putenv, the way it was supposed to be */
 static struct lienv{struct lienv*next;char name[255];}*myenv;
 static alloced;int i,remove;char*split,**preenv;struct lienv*curr,**last;
 yell("Assigning",a);remove=0;a=tstrdup(a);		/* make working copy */
 if(!(split=strchr(a,'='))){			   /* assignment or removal? */
    remove=1;i=strlen(a);*(split=i+(a=realloc(a,i+2)))='=';
    split[1]='\0';}
 i= ++split-a;
 for(curr= *(last= &myenv);curr;curr= *(last= &curr->next))
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
 i=(preenv-environ+2)*sizeof*environ;
 if(alloced)		   /* have we ever alloced the environ array before? */
   environ=realloc(environ,i);
 else{
   alloced=1;environ=tmemmove(malloc(i),environ,i-sizeof*environ);}
 if(!remove){		  /* if not remove, then add it to both environments */
   for(preenv=environ;*preenv;preenv++);
   curr=malloc(curr->name-(char*)curr+strlen(a)+1);
   strcpy(*preenv=curr->name,a);free(a);preenv[1]=0;curr->next=myenv;
   myenv=curr;}}
