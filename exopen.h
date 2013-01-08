/*$Id: exopen.h,v 2.1 1991/10/22 15:31:26 berg Rel $*/
#define SERIALchars	3
#define UNIQnamelen	(1+SERIALchars+HOSTNAMElen+1)
#define SERIALmask	((1L<<6*SERIALchars)-1)

#ifdef NOstrpbrk
char*strpbrk();
#endif
