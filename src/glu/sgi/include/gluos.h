/*
** gluos.h - operating system dependencies for GLU
**
** $Header: /home/krh/git/sync/mesa-cvs-repo/Mesa/src/glu/sgi/include/gluos.h,v 1.1 2001/03/17 00:25:40 brianp Exp $
*/

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOIME
#include <windows.h>

/* Disable warnings */
#pragma warning(disable : 4101)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)

#else

/* Disable Microsoft-specific keywords */
#define GLAPIENTRY
#define WINGDIAPI

#endif
