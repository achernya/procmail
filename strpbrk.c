/*$Id: strpbrk.c,v 1.3 1992/04/23 16:46:41 berg Rel $*/
#include "includes.h"
#include "strpbrk.h"

#ifdef NOstrpbrk
char*strpbrk(st,del)const char*const st,*del;
{ const char*f=0,*t;
  for(f=0;*del;)
     if((t=strchr(st,*del++))&&(!f||t<f))
	f=t;
  return(char*)f;
}
#else
char dummy_strpbrk;			/* to keep some linkers from choking */
#endif
