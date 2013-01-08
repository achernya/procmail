/*$Id: config.h,v 2.22 1992/06/30 17:43:44 berg Rel $*/

/*#define KERNEL_LOCKS	/* uncomment if you want to use kernel locks on file
			   descriptors (not recommended if your system uses a
	buggy lockd across a net, or if your mailer uses tmp files in updating
	mailboxes and moves them into place); only advisable if your mailreader
	can't be convinced to use "dotfile"-locks */

/*#define sMAILBOX_SEPARATOR	"\1\1\1\1\n"	/* sTART- and eNDing separ.  */
/*#define eMAILBOX_SEPARATOR	"\1\1\1\1\n"	/* uncomment (one or both)
						   if your mail system uses
	nonstandard mail separators (non sendmail or smail compatible mailers
	like MMDF), if yours is even different, uncomment and change the
	value of course */

/* KEEPENV and PRESTENV should be defined as a comma-separated null-terminated
   list of strings */

/* every environment variable appearing in KEEPENV will not be thrown away
 * upon startup of procmail, e.g. you could define KEEPENV as follows:
 * #define KEEPENV	{"TZ","LANG",0}
 */
#define KEEPENV		{"TZ",0}

/* every environment variable appearing in PRESTENV will be set or wiped
 * out of the environment (variables without an '=' sign will be thrown
 * out), e.g. you could define PRESTENV as follows:
 * #define PRESTENV	{"IFS","PATH=$HOME/bin:/bin:/usr/bin",0}
 * any side effects (like setting the umask after an assignment to UMASK) will
 * *not* take place
 */
#define PRESTENV	{"IFS",0}

/*****************************************************************
 * Only edit below this line if you have edited this file before *
 *****************************************************************/

/* every user & group appearing in TRUSTED_IDS is allowed to use the -f option
   if the list is empty (just a terminating 0), everyone can use it
   TRUSTED_IDS should be defined as a comma-separated null-terminated
   list of strings */

#define TRUSTED_IDS	{"root","daemon","uucp","mail","x400",0}

/*#define NO_USER_TO_LOWERCASE_HACK	/* uncomment if your getpwnam() is
					   case insensitive or if procmail
	will always be supplied with the correct case in the explicit
	delivery mode argument */

/*#define NO_NFS_ATIME_HACK	/* uncomment if you're definitely not using
				   NFS mounted filesystems and can't afford
	procmail to sleep for 1 sec. before writing a mailbox */

/*#define SYSTEM_MBOX	"$HOME/.mbox"	/* uncomment and/or change if the
					   preset default mailbox is *not*
	either /usr/spool/mail/$USER or /usr/mail/$USER (it will supersede
	the value of SYSTEM_MAILBOX) */

/*#define DEFsendmail	"/bin/mail"	/* uncomment and/or change if the
					   preset default SENDMAIL is not
	suitable */

/*#define console	"/dev/console"	/* uncomment if you want procmail to
					   use the console (or any other
	terminal) to print any error messages that could not be dumped in the
	"logfile".  (Only recommended for debugging purposes, if you have
	trouble creating a "logfile") */

/************************************************************************
 * Only edit below this line if you *think* you know what you are doing *
 ************************************************************************/

#define NOBODY_uid	0xfffe	      /* default uid when no valid recipient */
#define NOBODY_gid	0xfffe	      /* default gid when no valid recipient */
#define ROOT_uid	0

#define INIT_UMASK	(S_IRWXG|S_IRWXO)			   /* == 077 */
#define NORMperm	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
	     /* == 0666, normal mode bits used to create files, before umask */
#define LOCKperm	0	  /* mode bits used while creating lockfiles */
#define MAX_LOCK_SIZE	0	  /* lockfiles are expected not to be longer */
#ifndef SMALLHEAP
#define DEFlinebuf	2048		 /* default max expanded line length */
#define BLKSIZ		16384		  /* blocksize while reading/writing */
#define STDBUF		1024		     /* blocksize for emulated stdio */
#else		   /* and some lower defaults for the unfortunate amongst us */
#define DEFlinebuf	512
#define BLKSIZ		1024
#define STDBUF		128
#endif /* SMALLHEAP */
#define HOSTNAMElen	9	  /* nr of significant chararacters for HOST */
#define BOGUSprefix	"BOGUS."	     /* prepended to bogus mailboxes */
#define PROCMAILRC	".procmailrc"
#define DEFsuspend	16		 /* multi-purpose 'idle loop' period */
#define DEFlocksleep	8
#define TOkey		"^TO"
#define TOsubstitute	"^(To|Cc|Apparently-To):.*"
#define DEFshellmetas	"&|<>~;?*[]="		    /* never put '$' in here */
#define DEFmaildir	"$HOME"
#define DEFdefault	"$ORGMAIL"
#define DEFdefaultlock	"LOCKFILE=$DEFAULT$LOCKEXT"
#define DEFmsgprefix	"msg."
#define DEFlockext	".lock"
#define DEFshellflags	"-c"
#define DEFlocktimeout	3600			     /* defaults to one hour */
#define DEFtimeout	(DEFlocktimeout-60)	   /* 60 seconds to clean up */
#define DEFnoresretry	4      /* default nr of retries if no resources left */

#define BinSh		"/bin/sh"
#define Tmp		"/tmp"
#define DEBUGPREFIX	':'			 /* debug prefix for LOGFILE */
#define DevNull		"/dev/null"
#define chCURDIR	'.'			    /* the current directory */
#define DIRSEP		"/"		 /* directory separator symbols, the */
				   /* last one should be the most common one */

#define EOFName		" \t\n#`'\");"

#define HELPOPT1	'h'		 /* options to get command line help */
#define HELPOPT2	'?'

#define VERSIONOPT	'v'			/* option to display version */
#define PRESERVOPT	'p'			     /* preserve environment */
#define TEMPFAILOPT	't'		      /* return EX_TEMPFAIL on error */
#define FROMWHOPT	'f'			   /* set name on From_ line */
#define ALTFROMWHOPT	'r'		/* alternate and obsolete form of -f */
#define DELIVEROPT	'd'		  /* deliver mail to named recipient */
#define PM_USAGE	\
 "Usage: procmail [-vpt] [-f fromwhom] [parameter=value | rcfile] ...\
\n   Or: procmail [-vpt] [-f fromwhom] -d recipient ...\n"
#define PM_HELP		\
 "\t-v\t\tdisplay the version number and exit\
\n\t-p\t\tpreserve (most of) the environment upon startup\
\n\t-t\t\tfail softly if mail is undeliverable\
\n\t-f fromwhom\t(re)generate the leading 'From ' line\
\n\t-d recipient\texplicit delivery mode\n"
#define PM_QREFERENCE	\
 "\nRecipe flags quick reference:\
\n\tH\tegrep the header (default)\
\n\tB\tegrep the body\
\n\tD\tdistinguish case\
\n\tA\talso execute this recipe if the common condition matched\
\n\ta\tsame as 'A', but only if the previous recipe was succesful\
\n\th\tfeed the header to the pipe (default)\
\n\tb\tfeed the body to the pipe (default)\
\n\tf\tfilter\
\n\tc\tcontinue with the next recipe in any case\
\n\tw\twait for a filter or program\
\n\tW\tsame as 'w', but suppresses 'Program failure' messages\
\n\ti\tignore write errors\n"

#define MINlinebuf	128    /* minimal LINEBUF length (don't change this) */
#define FROM_EXPR	"\nFrom "
#define FROM		"From "
#define NSUBJECT	"^Subject:.*$"
#define MAXSUBJECTSHOW	78
#define FOLDER		"  Folder: "
#define LENtSTOP	9 /* tab stop at which message length will be logged */

#define TABCHAR		"\t"
#define TABWIDTH	8

#define RECFLAGS	"HBDAahbfcwWi"
#define HEAD_GREP	 0
#define BODY_GREP	  1
#define DISTINGUISH_CASE   2
#define ALSO_NEXT_RECIPE    3
#define ALSO_N_IF_SUCC	     4
#define PASS_HEAD	      5
#define PASS_BODY	       6
#define FILTER			7
#define CONTINUE		 8
#define WAIT_EXIT		  9
#define WAIT_EXIT_QUIET		   10
#define IGNORE_WRITERR		    11

#define UNIQ_PREFIX	'_'	  /* prepended to temporary unique filenames */
#define ESCAP		'>'

		/* some formail-specific configuration options: */

#define UNKNOWN		"foo@bar"	  /* formail default originator name */
#define OLD_PREFIX	"Old-"			 /* formail field-Old-prefix */

#define FM_SKIP		'+'		      /* skip the first nnn messages */
#define FM_TOTAL	'-'	    /* only spit out a total of nnn messages */
#define FM_BOGUS	'b'			 /* leave bogus Froms intact */
#define FM_FORCE	'f'   /* force formail to accept an arbitrary format */
#define FM_REPLY	'r'		    /* generate an auto-reply header */
#define FM_KEEPB	'k'		   /* keep the header, when replying */
#define FM_TRUST	't'	/* trust the sender to supply a valid header */
#define FM_SPLIT	's'				      /* split it up */
#define FM_NOWAIT	'n'		      /* don't wait for the programs */
#define FM_EVERY	'e'	/* don't require empty lines leading headers */
#define FM_MINFIELDS	'm'    /* the number of fields that have to be found */
#define DEFminfields	2	    /* before a header is recognised as such */
#define FM_DIGEST	'd'				 /* split up digests */
#define FM_QUIET	'q'		    /* ignore write errors on stdout */
#define FM_EXTRACT	'x'			   /* extract field contents */
#define FM_ADD_IFNOT	'a'		 /* add a field if not already there */
#define FM_ADD_ALWAYS	'A'		       /* add this field in any case */
#define FM_REN_INSERT	'i'			/* rename and insert a field */
#define FM_DEL_INSERT	'I'			/* delete and insert a field */
#define FM_USAGE	"Usage: \
formail [+nn] [-nn] [-bfrktnedq] [-m nn] [-xaAiI field] [-s prg arg ...]\n"
#define FM_HELP		\
 "\t-b\tdon't escape bogus mailbox headers\
\n\t-f\tforce formail to pass along any non-mailbox format\
\n\t-r\tgenerate an auto-reply header, preserve fields with -i\
\n\t-k\ton auto-reply keep the body, prevent escaping with -b\
\n\t-t\ttrust the sender for his return address\
\n\t-q\tbe quiet about write errors on stdout\
\n\t-s prg arg\tsplit the mail, startup prg for every message\
\n\t-n\tdon't wait for every prg while splitting\
\n\t-e\tdon't require empty lines to preceed a header\
\n\t-d\taccept digest format\
\n\t-m nn\tmin fields threshold (default 2) for start of message\
\n\t+nn\tskip the first nn messages while splitting\
\n\t-nn\toutput at most nn messages while splitting\
\n\t-x field\textract\
\n\t-a field\tadd if not present\
\n\t-A field\tadd in any case\
\n\t-i field\trename and insert\
\n\t-I field\tdelete and insert\n"
