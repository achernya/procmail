/*#define SYSV		/* you guessed it, uncomment if sysV machine	*/

#define NOmemmove	/* if your library has it, comment this line	*/

/*#define NO_ANSI_PROT	/* uncomment if you don't have ANSI headers	*/

/*#define void char	/* uncomment if your compiler is brain damaged	*/
 
#ifdef SYSV
# define NOfsync	/* in my experience no sysV machine has it	*/
#else
# define NOuname	/* some BSD machines seem to have it anyway	*/
#endif

/*#define NOfsync	/* if you don't want to use it or don't have it */

#ifndef t_buf		/* should not be a macro anyway			*/
typedef unsigned t_buf; /* comment out if already defined		*/
			/* when in doubt, please check if this is the	*/
			/* correct type for your library as the size	*/
			/* argument to malloc, realloc and memmove	*/
			/* this is important, since it determines the	*/
			/* maximum message length that can be processed */
			/* by procmail					*/
#endif

/* If need be, you can change some of the defines below, for most
   people these defaults should do however				*/

#define BFSIZ		2048			 /* max expanded line length */
#define BLKSIZ		(1<<14)		  /* blocksize while reading/writing */
#define PROCMAILRC	".procmailrc"
#define SUSPENSION	16		 /* multi-purpose 'idle loop' period */
#define DEFlocksleep	8
#define DEFlocksleeps	"8"
#define TOkey		"^TO"
#define TOkeylen	3			/* should be length of TOkey */
#define TOsubstitute	"^(To|Cc|Apparently-To):.*"
#define DEFshellmetas	"\"'`&#{}()[]*?|<>~;!\\"    /* never put '$' in here */
#define DEFmaildir	"$HOME"
#define DEFdefault	"$MAILDIR/.mailbox"
#define DEForgmail	"/var/spool/mail/$USER"
#define DEFgrep		"/usr/bin/egrep"
#define DEFsendmail	"/usr/lib/sendmail"
#define DEFlockext	".lock"
#define DEFshellflags	"-c"
#define DEFlocktimeout	"3600"			     /* defaults to one hour */
#define iDEFnomemretry	2	    /* standard nr of retries if no mem left */
				    /* valid as soon as procmail runs */
#define DEFnomemretry	"2"
