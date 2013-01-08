/*$Id: shell.h,v 2.0 1991/06/10 14:39:08 berg Rel $*/

#define malloc(n)	tmalloc((size_t)(n))
#define realloc(p,n)	trealloc(p,(size_t)(n))
#define tmemmove(t,f,n) memmove(t,f,(size_t)(n))
