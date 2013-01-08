/************************************************************************
 *	recommend.c	analyses the installation, and makes		*
 *			recommendations about suid/sgid modes		*
 ************************************************************************/
/*$Id: recommend.c,v 2.3 1992/04/29 15:54:33 berg Rel $*/
#include "config.h"
#include "includes.h"				       /* also for fprintf() */
#include "strpbrk.h"

#ifndef SYSTEM_MBOX
#define SYSTEM_MBOX	SYSTEM_MAILBOX
#endif

#define PERMIS	(S_IRWXU|S_IRWXG&~S_IWGRP|S_IRWXO&~S_IWOTH)

char system_mbox[]=SYSTEM_MBOX;
const char dirsep[]=DIRSEP,
 *const checkf[]={"/bin/mail","/bin/lmail","/usr/lib/sendmail",
 "/usr/lib/smail",0};
			     /* lastdirsep() has been lifted out of exopen.c */
char*lastdirsep(filename)const char*filename;	 /* finds the next character */
{ const char*p;					/* following the last DIRSEP */
  while(p=strpbrk(filename,dirsep))
     filename=p+1;
  return(char*)filename;
}

main(argc,argv)const int argc;const char*const argv[];
{ struct passwd*pass;struct group*grp;struct stat stbuf;
  uid_t uid=ROOT_uid;gid_t gid=NOBODY_gid;const char*const*p;
  mode_t suid=0,sgid=0;
  if(argc!=3)
   { fprintf(stderr,"Please run this program via 'make recommend'\n");
     return EX_USAGE;
   }
  *lastdirsep(system_mbox)='\0';
  for(p=checkf;*p;++p)
     if(!stat(*p,&stbuf)&&stbuf.st_mode&(S_ISUID|S_ISGID))
      { if(stbuf.st_mode&S_ISUID&&stbuf.st_uid!=ROOT_uid)
	   suid=S_ISUID,uid=stbuf.st_uid;
	if(stbuf.st_mode&S_ISGID)
	   sgid=S_ISGID,gid=stbuf.st_gid;
	break;
      }
  if(!stat(system_mbox,&stbuf)&&!(stbuf.st_mode&S_IWOTH))
     if(stbuf.st_mode&S_IWGRP)
	sgid=S_ISGID,gid=stbuf.st_gid;
     else
	suid=S_ISUID,uid=stbuf.st_uid;
  if(gid!=stbuf.st_gid)
     sgid=0;
  printf("chown root %s\n",argv[1]);
  if(suid)
     if(pass=getpwuid(uid))
	printf("chown %s %s\n",pass->pw_name,argv[2]);
     else
	printf("chown %u %s\n",(int)uid,argv[2]);
  if(sgid)
     if(grp=getgrgid(gid))
	printf("chgrp %s %s %s\n",grp->gr_name,argv[1],argv[2]);
     else
	printf("chgrp %u %s %s\n",(int)gid,argv[1],argv[2]);
  printf("chmod %o %s\n",sgid|S_ISUID|PERMIS,argv[1]);
  if(suid||sgid)
     printf("chmod %o %s\n",suid|sgid|PERMIS,argv[2]);
  return EX_OK;
}
