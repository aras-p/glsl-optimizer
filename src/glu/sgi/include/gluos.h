/*
** gluos.h - operating system dependencies for GLU
**
** $Header: /home/krh/git/sync/mesa-cvs-repo/Mesa/src/glu/sgi/include/gluos.h,v 1.3 2001/07/16 15:46:42 brianp Exp $
*/
#ifdef __VMS
#ifdef __cplusplus 
#pragma message disable nocordel
#pragma message disable codeunreachable
#pragma message disable codcauunr
#endif
#endif

#ifdef _WIN32
#include <stdlib.h>         /* For _MAX_PATH definition */
#include <stdio.h>
#include <malloc.h>

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOIME
#define NOMINMAX

#define _WIN32_WINNT 0x0400
#ifndef STRICT
  #define STRICT 1
#endif

#include <windows.h>

/* Disable warnings */
#pragma warning(disable : 4101)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)

#pragma comment(linker, "/OPT:NOWIN98")

#else

/* Disable Microsoft-specific keywords */
#define GLAPIENTRY
#define WINGDIAPI

#endif
