/* A sed script generator (for transmogrifying the man pages automagically) */

/*$Id: manconf.c,v 2.17 1992/06/03 14:40:24 berg Rel $*/

#include "../config.h"
#include "../procmail.h"

#define pn(name,val)	pnr(name,(long)(val))

const char devnull[]=DevNull;
const char*const keepenv[]=KEEPENV,*const prestenv[]=PRESTENV,
 *const trusted_ids[]=TRUSTED_IDS;

main()
{
#ifndef MAILBOX_SEPARATOR
  ps("DOT_FORWARD",".forward");
  ps("FW_content","\"|IFS=' ';exec /usr/local/bin/procmail #YOUR_LOGIN_NAME\"");
#else
  ps("DOT_FORWARD",".maildelivery");
  ps("FW_content",
   "* - | ? \"IFS=' ';exec /usr/local/bin/procmail #YOUR_LOGIN_NAME\"");
#endif
  plist("PRESTENV","\1.PP\1Other preset environment variables are "
   ,prestenv,".\1",""," and ");
  plist("KEEPENV",", except for the values of ",keepenv,"",""," and ");
  plist("TRUSTED_IDS",", and procmail is invoked with one of the following\
 user or group ids: ",trusted_ids,",",""," or ");
#ifdef LD_ENV_FIX
  ps("LD_ENV_FIX","\1.PP\1For security reasons, procmail will wipe out all\
 environment variables starting with LD_ upon startup.");
#else
  ps("LD_ENV_FIX","");
#endif
#ifdef NO_USER_TO_LOWERCASE_HACK
  ps("UPPERCASE_USERNAMES","\1.PP\1If the standard\1.BR getpwnam() (3)\1\
is case sensitive, and some users have login names with uppercase letters in\
 them, procmail will be unable to deliver mail to them, unless started with\
 their uid.");
#else
  ps("UPPERCASE_USERNAMES","");
#endif
  ps("SYSTEM_MBOX",SYSTEM_MBOX);
#ifdef console
  ps("console",console);
#else
  ps("console",vconsole);
#endif
  pname("INIT_UMASK");printf("0%lo/g\n",INIT_UMASK);
  pn("DEFlinebuf",DEFlinebuf);
  ps("BOGUSprefix",BOGUSprefix);
  ps("PROCMAILRC",PROCMAILRC);
  pn("HOSTNAMElen",HOSTNAMElen);
  pn("DEFsuspend",DEFsuspend);
  pn("DEFlocksleep",DEFlocksleep);
  ps("TOkey",TOkey);
  ps("TOsubstitute",TOsubstitute);
  ps("DEFshellmetas",DEFshellmetas);
  ps("DEFmaildir",DEFmaildir);
  ps("DEFdefault",DEFdefault);
  ps("DEFdefaultlock",strchr(DEFdefaultlock,'=')+1);
  ps("DEFmsgprefix",DEFmsgprefix);
  ps("DEFsendmail",DEFsendmail);
  ps("DEFlockext",DEFlockext);
  ps("DEFshellflags",DEFshellflags);
  pn("DEFlocktimeout",DEFlocktimeout);
  pn("DEFtimeout",DEFtimeout);
  ps("Tmp",Tmp);
  pc("DEBUGPREFIX",DEBUGPREFIX);
  pc("HELPOPT1",HELPOPT1);
  pc("HELPOPT2",HELPOPT2);
  pc("VERSIONOPT",VERSIONOPT);
  pc("PRESERVOPT",PRESERVOPT);
  pc("TEMPFAILOPT",TEMPFAILOPT);
  pc("FROMWHOPT",FROMWHOPT);
  pc("ALTFROMWHOPT",ALTFROMWHOPT);
  pc("DELIVEROPT",DELIVEROPT);
  pn("MINlinebuf",MINlinebuf);
  ps("FROM",FROM);
  pc("HEAD_GREP",RECFLAGS[HEAD_GREP]);
  pc("BODY_GREP",RECFLAGS[BODY_GREP]);
  pc("DISTINGUISH_CASE",RECFLAGS[DISTINGUISH_CASE]);
  pc("ALSO_NEXT_RECIPE",RECFLAGS[ALSO_NEXT_RECIPE]);
  pc("ALSO_N_IF_SUCC",RECFLAGS[ALSO_N_IF_SUCC]);
  pc("PASS_HEAD",RECFLAGS[PASS_HEAD]);
  pc("PASS_BODY",RECFLAGS[PASS_BODY]);
  pc("FILTER",RECFLAGS[FILTER]);
  pc("CONTINUE",RECFLAGS[CONTINUE]);
  pc("WAIT_EXIT",RECFLAGS[WAIT_EXIT]);
  pc("WAIT_EXIT_QUIET",RECFLAGS[WAIT_EXIT_QUIET]);
  pc("IGNORE_WRITERR",RECFLAGS[IGNORE_WRITERR]);
  ps("FROM_EXPR",FROM_EXPR);
  pc("UNIQ_PREFIX",UNIQ_PREFIX);
  pc("ESCAP",ESCAP);
  ps("UNKNOWN",UNKNOWN);
  ps("OLD_PREFIX",OLD_PREFIX);
  pc("FM_SKIP",FM_SKIP);
  pc("FM_TOTAL",FM_TOTAL);
  pc("FM_BOGUS",FM_BOGUS);
  pc("FM_FORCE",FM_FORCE);
  pc("FM_REPLY",FM_REPLY);
  pc("FM_KEEPB",FM_KEEPB);
  pc("FM_TRUST",FM_TRUST);
  pc("FM_SPLIT",FM_SPLIT);
  pc("FM_NOWAIT",FM_NOWAIT);
  pc("FM_EVERY",FM_EVERY);
  pc("FM_MINFIELDS",FM_MINFIELDS);
  pn("DEFminfields",DEFminfields);
  pc("FM_DIGEST",FM_DIGEST);
  pc("FM_QUIET",FM_QUIET);
  pc("FM_EXTRACT",FM_EXTRACT);
  pc("FM_ADD_IFNOT",FM_ADD_IFNOT);
  pc("FM_ADD_ALWAYS",FM_ADD_ALWAYS);
  pc("FM_REN_INSERT",FM_REN_INSERT);
  pc("FM_DEL_INSERT",FM_DEL_INSERT);
  pn("EX_OK",EX_OK);
  return EX_OK;
}

pname(name)const char*const name;
{ putchar('s');putchar('/');putchar('\\');putchar('+');putsesc(name);
  putchar('\\');putchar('+');putchar('/');
}

pnr(name,value)const char*const name;const long value;
{ pname(name);printf("%ld/g\n",value);
}

plist(name,preamble,list,postamble,ifno,andor)const char*const name,
 *const preamble,*const postamble,*const ifno,*const andor;
 const char*const*list;
{ pname(name);
  if(!*list)
     putsesc(ifno);
  else
   { putsesc(preamble);goto jin;
     do
      { putsesc(list[1]?", ":andor);
jin:	putsesc(*list);
      }
     while(*++list);
     putsesc(postamble);
   }
  puts("/g");
}

ps(name,value)const char*const name,*const value;
{ pname(name);putsesc(value);puts("/g");
}

pc(name,value)const char*const name;const int value;
{ pname(name);putcesc(value);puts("/g");
}

putcesc(i)
{ switch(i)
   { case '\1':i='\n';goto singesc;
     case '\t':i='t';goto fin;
     case '\n':i='n';
fin:	putchar('\\');
     case '\\':putchar('\\');putchar('\\');
singesc:
     case '&':case '/':putchar('\\');
   }
  putchar(i);
}

putsesc(a)const char*a;
{ while(*a)
     putcesc(*a++);
}
