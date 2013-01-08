/*$Id: shell.h,v 1.4 1993/06/14 13:13:26 berg Exp $*/

#ifdef malloc
#undef malloc
#endif
#define malloc(n)	tmalloc((size_t)(n))
#define realloc(p,n)	trealloc(p,(size_t)(n))
#define free(p)		tfree(p)
#define tmemmove(t,f,n) memmove(t,f,(size_t)(n))
