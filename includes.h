/*$Id: includes.h,v 2.13 1992/01/14 17:32:53 berg Rel $*/

#include "autoconf.h"
	/* not all the "library identifiers" specified here need to be
	   available for all programs in this package; some have substitutes
	   as well (see autoconf); this is just an informal list */

#include <sys/types.h>		/* pid_t mode_t uid_t gid_t */
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
#include <sys/stat.h>		/* stat() S_ISDIR() struct stat */
#include <signal.h>		/* signal() kill() alarm() SIG_IGN SIGHUP
				   SIGINT SIGQUIT SIGALRM SIGTERM */
#include <string.h>		/* strcpy() strncpy() strcat() strlen()
				   strspn() strcspn() strchr() strcmp()
				   strncmp() strpbrk() strstr() memmove() */
#include <errno.h>		/* EINTR EEXIST EMFILE ENFILE */
#include <sysexits.h>		/* EX_OK EX_UNAVAILABLE EX_OSERR EX_OSFILE
				   EX_CANTCREAT EX_IOERR EX_TEMPFAIL */
#ifdef KERNEL_LOCKS
#include <sys/file.h>
#endif

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
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#endif

#ifndef S_ISLNK
#ifndef S_IFLNK
#define lstat(path,stbuf)	stat(path,stbuf)
#define S_ISLNK(mode)	0
#else
#define S_ISLNK(mode)	(((mode)&S_IFMT)==S_IFLNK)
#endif
#endif

#ifndef S_IFMT
#define S_IFMT	0170000
#endif

#ifndef S_IRWXU
#define S_IRWXU 00700
#define S_IRWXG 00070
#define S_IRWXO 00007
#endif
#ifndef S_IWUSR
#ifdef S_IREAD
#define S_IRUSR	 S_IREAD
#define S_IWUSR	 S_IWRITE
#define S_IXUSR	 S_IEXEC
#else
#define S_IRUSR	 0400
#define S_IWUSR	 0200
#define S_IXUSR	 0100
#endif
#define S_IRGRP	 0040
#define S_IWGRP	 0020
#define S_IXGRP	 0010
#define S_IROTH	 0004
#define S_IWOTH	 0002
#define S_IXOTH	 0001
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

#ifdef NOrename
#define rename(old,new) (-(link(old,new)||unlink(old)))
#endif

#ifdef NOmemmove
#define memmove(to,from,count) smemmove(to,from,count)
#endif

#ifdef oBRAIN_DAMAGE
#undef offsetof
#endif
#ifndef offsetof
#define offsetof(s,m) ((char*)&(((s*)0)->m)-(char*)0)
#endif

#define maxindex(x)	(sizeof(x)/sizeof((x)[0])-1)
#define STRLEN(x)	(sizeof(x)-1)
#define ioffsetof(s,m)	((int)offsetof(s,m))

#define mx(a,b)		((a)>(b)?(a):(b))
