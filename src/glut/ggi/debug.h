/* 
******************************************************************************

   GGIMesa debugging macros

   Copyright (C) 1998-1999 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 1999-2000 Jon Taylor		[taylorj@ggi-project.org]
  
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#ifndef _GGI_GLUT_INTERNAL_DEBUG_H
#define _GGI_GLUT_INTERNAL_DEBUG_H

#define DEBUG

#include <stdio.h>
#include <stdarg.h>
#include <ggi/types.h>
#include <ggi/gg.h>


__BEGIN_DECLS

/* Exported variables */
#ifdef BUILDING_GGIGLUT
extern uint32     _ggiglutDebugState;
extern int        _ggiglutDebugSync;
#else
IMPORTVAR uint32  _ggiglutDebugState;
IMPORTVAR int     _ggiglutDebugSync;
#endif

__END_DECLS


/* Debugging types
 * bit 0 is reserved! */

#define GGIGLUTDEBUG_CORE		(1<<1)	/*   2 */
#define GGIGLUTDEBUG_MODE		(1<<2)	/*   4 */
#define GGIGLUTDEBUG_COLOR		(1<<3)	/*   8 */
#define GGIGLUTDEBUG_DRAW		(1<<4)	/*  16 */
#define GGIGLUTDEBUG_MISC		(1<<5)	/*  32 */
#define GGIGLUTDEBUG_LIBS		(1<<6)	/*  64 */
#define GGIGLUTDEBUG_EVENTS		(1<<7)	/* 128 */

#define GGIGLUTDEBUG_ALL	0xffffffff

#ifdef __GNUC__

#ifdef DEBUG
#define GGIGLUTDPRINT(form,args...)	   if (_ggiglutDebugState) { ggDPrintf(_ggiglutDebugSync, "GGIGLUT",form, ##args); }
#define GGIGLUTDPRINT_CORE(form,args...)   if (_ggiglutDebugState & GGIGLUTDEBUG_CORE) { ggDPrintf(_ggiglutDebugSync,"GGIGLUT",form, ##args); }
#define GGIGLUTDPRINT_MODE(form,args...)   if (_ggiglutDebugState & GGIGLUTDEBUG_MODE) { ggDPrintf(_ggiglutDebugSync,"GGIGLUT",form, ##args); }
#define GGIGLUTDPRINT_COLOR(form,args...)  if (_ggiglutDebugState & GGIGLUTDEBUG_COLOR) { ggDPrintf(_ggiglutDebugSync,"GGIGLUT",form, ##args); }
#define GGIGLUTDPRINT_DRAW(form,args...)   if (_ggiglutDebugState & GGIGLUTDEBUG_DRAW) { ggDPrintf(_ggiglutDebugSync,"GGIGLUT",form, ##args); }
#define GGIGLUTDPRINT_MISC(form,args...)   if (_ggiglutDebugState & GGIGLUTDEBUG_MISC) { ggDPrintf(_ggiglutDebugSync,"GGIGLUT",form, ##args); }
#define GGIGLUTDPRINT_LIBS(form,args...)   if (_ggiglutDebugState & GGIGLUTDEBUG_LIBS) { ggDPrintf(_ggiglutDebugSync,"GGIGLUT",form, ##args); }
#define GGIGLUTDPRINT_EVENTS(form,args...) if (_ggiglutDebugState & GGIGLUTDEBUG_EVENTS) { ggDPrintf(_ggiglutDebugSync,"GGIGLUT",form, ##args); }
#else /* DEBUG */
#define GGIGLUTDPRINT(form,args...)		do{}while(0)
#define GGIGLUTDPRINT_CORE(form,args...)	do{}while(0)
#define GGIGLUTDPRINT_MODE(form,args...)	do{}while(0)
#define GGIGLUTDPRINT_COLOR(form,args...)	do{}while(0)
#define GGIGLUTDPRINT_DRAW(form,args...)	do{}while(0)
#define GGIGLUTDPRINT_MISC(form,args...)	do{}while(0)
#define GGIGLUTDPRINT_LIBS(form,args...)	do{}while(0)
#define GGIGLUTDPRINT_EVENTS(form,args...)	do{}while(0)
#endif /* DEBUG */

#else /* __GNUC__ */

__BEGIN_DECLS

static inline void GGIGLUTDPRINT(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIGLUTDPRINT_CORE(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_CORE) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIGLUTDPRINT_MODE(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_MODE) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIGLUTDPRINT_COLOR(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_COLOR) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIGLUTDPRINT_DRAW(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_DRAW) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIGLUTDPRINT_MISC(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_MISC) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIGLUTDPRINT_LIBS(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_LIBS) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

static inline void GGIGLUTDPRINT_EVENTS(const char *form,...)
{
#ifdef DEBUG
	if (_ggiDebugState & GGIDEBUG_EVENTS) {
		va_list args;

		fprintf(stderr, "GGIGLUT: ");
		va_start(args, form);
		vfprintf(stderr, form, args);
		va_end(args);
		if (_ggiglutDebugSync) fflush(stderr);
	}
#endif
}

__END_DECLS

#endif /* __GNUC__ */

#ifdef DEBUG
#define GGIGLUT_ASSERT(x,str) \
{ if (!(x)) { \
	fprintf(stderr,"GGIGLUT:%s:%d: INTERNAL ERROR: %s\n",__FILE__,__LINE__,str); \
	exit(1); \
} }
#define GGIGLUT_APPASSERT(x,str) \
{ if (!(x)) { \
	fprintf(stderr,"GGIGLUT:%s:%d: APPLICATION ERROR: %s\n",__FILE__,__LINE__,str); \
	exit(1); \
} }
#else /* DEBUG */
#define GGIGLUT_ASSERT(x,str)	do{}while(0)
#define GGIGLUT_APPASSERT(x,str)	do{}while(0)
#endif /* DEBUG */

#ifdef DEBUG
# define GGIGLUTD0(x)	x
#else
# define GGIGLUTD0(x)	/* empty */
#endif

#ifdef GGIGLUTDLEV
# if GGIGLUTDLEV == 1
#  define GGIGLUTD1(x)	x
#  define GGIGLUTD2(x)	/* empty */
#  define GGIGLUTD3(x)	/* empty */
# elif GGIGLUTDLEV == 2
#  define GGIGLUTD1(x)	x
#  define GGIGLUTD2(x)	x
#  define GGIGLUTD3(x)	/* empty */
# elif GGIGLUTDLEV > 2
#  define GGIGLUTD1(x)	x
#  define GGIGLUTD2(x)	x
#  define GGIGLUTD3(x)	x
# endif
#else
# define GGIGLUTD1(x)	/* empty */
# define GGIGLUTD2(x)	/* empty */
# define GGIGLUTD3(x)	/* empty */
#endif

#endif /* _GGI_MESA_INTERNAL_DEBUG_H */
