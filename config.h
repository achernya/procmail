/*$Id: config.h,v 2.13 1992/01/31 11:32:45 berg Rel $*/

/*#define KERNEL_LOCKS	/* uncomment if you want to use kernel locks on file
			   descriptors (not recommended if your system uses a
	buggy lockd across a net, or if your mailer uses tmp files in updating
	mailboxes and moves them into place); only advisable if your mailreader
	can't be convinced to use "dotfile"-locks */

/*#define MAILBOX_SEPARATOR	"\1\1\1\1\n"	/* uncomment if your mail
						   system uses nonstandard
	mail separators (non sendmail or smail compatible mailers like MMDF),
	if yours is even different, uncomment and change the value of course */

/*#define DONT_DISCARD_IFS	/* uncomment if you don't want procmail to
				   discard the value of the IFS environment
	variable, forget about this if you don't know what IFS is for */

/*#define SYSTEM_MBOX	"$HOME/.mbox"	/* uncomment and/or change if the
					   preset default mailbox is *not*
	either /usr/spool/mail/$USER or /usr/mail/$USER (it will supersede
	the value of SYSTEM_MAILBOX) */

/*#define DEFsendmail	"/bin/mail"	/* uncomment and/or change if the
					   preset default SENDMAIL is not
	suitable */

/*#define console	":/dev/console" /* uncomment if you want procmail to
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
#define HOSTNAMElen	8	  /* nr of significant chararacters for HOST */
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
#define DIRSEP		"/"		 /* directory separator symbols, the */
				   /* last one should be the most common one */

#define EOFName		" \t\n#`'\");"

#define VERSIONOPT	'v'			/* option to display version */
#define PRESERVOPT	'p'			     /* preserve environment */
#define TEMPFAILOPT	't'		      /* return EX_TEMPFAIL on error */
#define DELIVEROPT	'd'		  /* deliver mail to named recipient */
#define PROCMAIL_USAGE	\
 "Usage: procmail [-vpt] [-d recipient ] [ parameter=value | rcfile ] ...\n"

#define MINlinebuf	128    /* minimal LINEBUF length (don't change this) */
#define FROM_EXPR	"\n\nFrom +[^\t\n ]+ +[^\t\n ]"
#define FROM		"From "
#define NSUBJECT	"^Subject:.*$"
#define MAXSUBJECTSHOW	78
#define FOLDER		"  Folder: "
#define LENtSTOP	9 /* tab stop at which message length will be logged */

#define TABCHAR		"\t"
#define TABWIDTH	8

#define RECFLAGS	"HBDAahbfcwi"
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
#define IGNORE_WRITERR		   10

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
#define FM_EVERY	'e'			 /* split on every From line */
#define FM_DIGEST	'd'				 /* split up digests */
#define FM_QUIET	'q'		    /* ignore write errors on stdout */
#define FM_EXTRACT	'x'			   /* extract field contents */
#define FM_ADD_IFNOT	'a'		 /* add a field if not already there */
#define FM_REN_INSERT	'i'			/* rename and insert a field */
#define FM_DEL_INSERT	'I'			/* delete and insert a field */
#define FM_USAGE	"Usage: \
formail [+nnn] [-nnn] [-bfrktnedq] [-xaiI field] [-s command arg ...]\n"
