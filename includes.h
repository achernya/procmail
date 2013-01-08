/*$Id: includes.h,v 2.8 1991/10/18 15:33:23 berg Rel $*/

#include "autoconf.h"
	/* not all the "library identifiers" specified here need to be
	   available for all programs in this package; some have substitutes
	   as well (see autoconf); this is just an informal list */

#include <unistd.h>		/* open() read() write() close() dup() pipe()
				   fork() getuid() getpid() execve()
				   execvp() sleep() */
#include <stdio.h>		/* setbuf() fclose() stdin stdout stderr
				   fopen() fread() fwrite() fgetc() getc()
				   putc() fputs() FILE EOF */
#include <stddef.h>		/* ptrdiff_t size_t sigatomic_t */
#include <stdlib.h>		/* getenv() malloc() realloc() free()
				   strtol() */
#include <time.h>		/* time() ctime() time_t */
#include <fcntl.h>		/* O_RDONLY O_WRONLY O_APPEND O_CREAT O_EXCL */
#include <pwd.h>		/* getpwuid() getpwnam() struct passwd */
#include <sys/wait.h>		/* wait() */
#include <sys/utsname.h>	/* uname() utsname */
#include <sys/types.h>		/* pid_t mode_t uid_t gid_t */
#include <sys/file.h>
#include <sys/stat.h>		/* stat() S_ISDIR() struct stat */
#include <signal.h>		/* signal() kill() alarm() SIG_IGN SIGHUP
				   SIGINT SIGQUIT SIGALRM SIGTERM */
#include <string.h>		/* strcpy() strncpy() strcat() strlen()
				   strspn() strcspn() strchr() strcmp()
				   strncmp() strpbrk() strstr() memmove() */
#include <errno.h>		/* EINTR EEXIST EMFILE ENFILE */
#include <sysexits.h>		/* EX_OK EX_UNAVAILABLE EX_OSERR EX_OSFILE
				   EX_CANTCREAT EX_IOERR EX_TEMPFAIL */
#if O_SYNC
#else
#undef O_SYNC
#define O_SYNC		0
#endif
#ifndef O_RDONLY
#define O_RDONLY	0
#define O_WRONLY	1
#endif
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_END	2
#endif

#ifndef EOF
#define EOF	(-1)
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode)	(((mode)&S_IFMT)==S_IFDIR)
#ifndef S_IFMT
#define S_IFMT	0170000
#endif
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#endif

extern /*const*/char**environ;
extern errno;

#ifndef STDIN_FILENO
#define STDIN	0
#define STDOUT	1
#define STDERR	2
#else
#define STDIN	STDIN_FILENO
#define STDOUT	STDOUT_FILENO
#define STDERR	STDERR_FILENO
#endif

#ifndef offsetof
#define offsetof(s,m) ((char*)&(((s*)0)->m)-(char*)0)
#endif

#define maxindex(x)	(sizeof(x)/sizeof((x)[0])-1)
#define STRLEN(x)	(sizeof(x)-1)

#define mx(a,b)		((a)>(b)?(a):(b))
