/*$Id: sublib.h,v 1.10.2.1 2001/07/15 09:27:35 guenther Exp $*/

#ifdef NOmemmove
void
 *smemmove Q((void*To,const void*From,size_t count));
#endif

#ifdef NOstrpbrk
char
 *strpbrk P((const char*const st,const char*del));
#endif

#ifdef SLOWstrstr
char
 *sstrstr P((const char*const phaystack,const char*const pneedle));
#endif

#ifdef NEEDbbzero
void
 bbzero Q((void *s,size_t n));
#endif

#ifdef NOstrlcat
size_t
 sstrlcat Q((char *dst,const char*src,size_t size)),
 sstrlcpy Q((char *dst,const char*src,size_t size));
#endif

#ifdef NOstrtol
long
 strtol P((const char*start,const char**const ptr,int base));
#endif
