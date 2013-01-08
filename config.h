/*$Id: config.h,v 2.1 1991/07/08 10:47:56 berg Rel $*/

/*#define console	"/dev/console"	/* uncomment if you want procmail to
					   use the console (or any other
	terminal) to print any error messages that could not be dumped in the
	"logfile".  (Only recommended for debugging purposes, if you have
	trouble creating a "logfile") */

/*#define MAILBOX_SEPARATOR	"\1\1\1\1\n"	/* uncomment if your mail
						   system uses nonstandard
	mail separators (non sendmail or smail compatible mailers like MMDF),
	if yours is even different, uncomment and change the value of course */

/************************************************************************
 * Only edit below this line if you *think* you know what you are doing *
 ************************************************************************/

#define DEFlinebuf	2048		 /* default max expanded line length */
#define BLKSIZ		16384		  /* blocksize while reading/writing */
#define STDBUF		1024		     /* blocksize for emulated stdio */
#define HOSTNAMElen	8	  /* nr of significant chararacters for HOST */
#define PROCMAILRC	".procmailrc"
#define DEFsuspend	16		 /* multi-purpose 'idle loop' period */
#define DEFlocksleep	8
#define TOkey		"^TO"
#define TOsubstitute	"^(To|Cc|Apparently-To):.*"
#define DEFshellmetas	"&()[]*?|<>~;:"		    /* never put '$' in here */
#define DEFmaildir	"$HOME"
#define DEFdefault	"$MAILDIR/.mailbox"
#define DEFmsgprefix	"msg."
#define DEForgmail	"/usr/spool/mail/$USER"
#define DEFgrep		"/usr/bin/egrep"
#define DEFsendmail	"/usr/lib/sendmail"
#define DEFlockext	".lock"
#define DEFshellflags	"-c"
#define DEFlocktimeout	3600			     /* defaults to one hour */
#define DEFtimeout	(DEFlocktimeout-60)	   /* 60 seconds to clean up */
#define DEFnoresretry	2      /* default nr of retries if no resources left */

#define NRRECFLAGS	(10+1)
#define RECFLAGS	" %10[HBDAhbfcwIs] %[^\n]"   /* 'I','s' are obsolete */
#define BinSh		"/bin/sh"
#define Tmp		"/tmp"
#define DevNull		"/dev/null"
#define DIRSEP		"/"		 /* directory separator symbols, the */
				   /* last one should be the most common one */

/* the regular expression we use to look for bogus headers
	(which I took from /usr/ucb/mail) is:
					      "\n\nFrom +[^\t\n ]+ +[^\n\t]" */

#define FromSCAN	"From%*[ ]%*[^\t\n ]%*[ ]%1[^\n\t]"
#define EOFName		"%[^ \t\n#'\")};]"

#define VERSIONOPT	'v'			/* option to display version */
#define PRESERVOPT	'p'
#define DEBUGOPT	'd'

#define MINlinebuf	128    /* minimal LINEBUF length (don't change this) */
#define SFROM		"From "
#define SSUBJECT	" Subject:"
#define SSUBJECT_S	\
		"%*1[Ss]%*1[Uu]%*1[Bb]%*1[Jj]%*1[Ee]%*1[Cc]%*1[Tt]:%70[^\n]"
#define FOLDER		"  Folder: "
#define LENtSTOP	9 /* tab stop at which message length will be logged */

#define TABCHAR		"\t"
#define TABWIDTH	8
