/* A sed script generator (for transmogrifying the man pages automagically) */

/*$Id: manconf.c,v 2.10 1992/01/31 12:07:40 berg Rel $*/

#include "../config.h"
#include "../procmail.h"

#define pn(name,val)	pnr(name,(long)(val))

const char devnull[]=DevNull;

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
#ifndef DONT_DISCARD_IFS
  ps("IFS_DISCARDING","For security reasons, procmail will unset the IFS\
 environment variable upon startup, even if the environment is supposed to\
 be preserved.");
#else
  ps("IFS_DISCARDING","Be sure to set the environment variable IFS to a\
 known value when using /bin/sh from within procmail.");
#endif
  ps("SYSTEM_MBOX",SYSTEM_MBOX);
  ps("console",console);
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
  pc("VERSIONOPT",VERSIONOPT);
  pc("PRESERVOPT",PRESERVOPT);
  pc("TEMPFAILOPT",TEMPFAILOPT);
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
  pc("FM_DIGEST",FM_DIGEST);
  pc("FM_QUIET",FM_QUIET);
  pc("FM_EXTRACT",FM_EXTRACT);
  pc("FM_ADD_IFNOT",FM_ADD_IFNOT);
  pc("FM_REN_INSERT",FM_REN_INSERT);
  pc("FM_DEL_INSERT",FM_DEL_INSERT);
  return EX_OK;
}

pname(name)const char*const name;
{ putchar('s');putchar('/');putchar('\\');putchar('+');putsesc(name);
  putchar('\\');putchar('+');putchar('/');
}

pnr(name,value)const char*const name;const long value;
{ pname(name);printf("%ld/g\n",value);
}

ps(name,value)const char*const name,*const value;
{ pname(name);putsesc(value);puts("/g");
}

pc(name,value)const char*const name;const int value;
{ pname(name);putcesc(value);puts("/g");
}

putcesc(i)
{ switch(i)
   { case '\t':i='t';goto fin;
     case '\n':i='n';
fin:	putchar('\\');
     case '\\':putchar('\\');putchar('\\');
     case '&':case '/':putchar('\\');
   }
  putchar(i);
}

putsesc(a)const char*a;
{ while(*a)
     putcesc(*a++);
}
