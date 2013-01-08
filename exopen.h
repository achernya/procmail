/*$Id: exopen.h,v 2.0 1991/06/10 14:39:08 berg Rel $*/
#define SERIALchars	3
#define UNIQnamelen	(1+SERIALchars+HOSTNAMElen+1)
#define SERIALmask	((1L<<6*SERIALchars)-1)
