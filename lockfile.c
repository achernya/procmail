/************************************************************************
 *      lockfile.c      a conditional semaphore-file creator            *
 *                                                                      *
 *      Version 1.02 1990-12-07                                         *
 *      Is relatively bug free.                                         *
 *                                                                      *
 *      Created by S.R.van den Berg, The Netherlands                    *
 *      The sources can be freely copied for any use.                   *
 *                                                                      *
 *      If you have had to make major changes in order to get it        *
 *      running on a different machine, please send me the patches.     *
 *      That way I might include them in a future version.              *
 ************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>

int exitflag;

void failure(){
 exitflag=1;}

main(argc,argv)char*argv[];{int sleepsec,retries,i,invert;char**p,*cp;
 sleepsec=8;retries=invert=0;
 if(argc<2){
   putse("Usage: lockfile [-nnn] [-rnnn] [-!] file ...\n");return 2;}
again:
 p=argv+1;signal(SIGHUP,failure);signal(SIGINT,failure);
 signal(SIGQUIT,failure);signal(SIGTERM,failure);
 do
    if(*(cp=*p)=='-')
       switch(cp[1]){
       case'!':invert=1;break;
       case'r':retries=strtol(cp+2,NULL,10);break;
       case'-':cp++;goto filename;
       default:
          if(sleepsec>=0)
             sleepsec=strtol(cp+1,NULL,10);}
    else
filename:
      if(sleepsec<0)
         unlink(cp);
      else{
         while(0>(i=open(cp,O_WRONLY|O_CREAT|O_EXCL|O_SYNC,0))){
            if(exitflag||retries==1){
               sleepsec=-1;*p=0;goto again;}
            sleep(sleepsec);
            if(retries)
               retries--;}
         close(i);}
 while(*++p);
return invert^(sleepsec<0);}

#define STDERR  2

putse(a)char*a;{char*b;
 b=a-1;
 while(*++b);
 write(STDERR,a,b-a);}
