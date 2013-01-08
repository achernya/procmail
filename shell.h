#define malloc(n)	tmalloc((t_buf)(n))
#define realloc(p,n)	trealloc(p,(t_buf)(n))
#define tmemmove(t,f,n) memmove(t,f,(t_buf)(n))
