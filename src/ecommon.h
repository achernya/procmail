/*$Id: ecommon.h,v 1.3 1993/01/22 13:42:24 berg Exp $*/

void
 *tmalloc Q((const size_t len)),
 *trealloc Q((void*old,const size_t len)),
 tfree P((void*a));
