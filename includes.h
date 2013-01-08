#include <stdio.h>
#ifndef NO_ANSI_PROT		/* define this if your headers are not ansi */
# include <stdlib.h>
# include <time.h>
#else
# include <malloc.h>
  char*getenv();
  typedef int pid_t;
#endif
#include <fcntl.h>
#include <pwd.h>
#include <sys/wait.h>
#ifndef NOuname
#include <sys/utsname.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#ifndef SEEK_SET
#define SEEK_SET	0
#endif
#ifndef O_SYNC
#define O_SYNC		0
#endif
extern char**environ;

#define STDIN	0
#define STDOUT	1
#define STDERR	2
