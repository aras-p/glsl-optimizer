/* $Id: imports.h,v 1.9 2002/12/06 03:10:59 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * This file provides wrappers for all the standard C library functions
 * like malloc, free, printf, getenv, etc.
 */


#ifndef IMPORTS_H
#define IMPORTS_H


#define MALLOC(BYTES)      _mesa_malloc(BYTES)
#define CALLOC(BYTES)      _mesa_calloc(BYTES)
#define MALLOC_STRUCT(T)   (struct T *) _mesa_malloc(sizeof(struct T))
#define CALLOC_STRUCT(T)   (struct T *) _mesa_calloc(sizeof(struct T))
#define FREE(PTR)          _mesa_free(PTR)

#define ALIGN_MALLOC(BYTES, N)     _mesa_align_malloc(BYTES, N)
#define ALIGN_CALLOC(BYTES, N)     _mesa_align_calloc(BYTES, N)
#define ALIGN_MALLOC_STRUCT(T, N)  (struct T *) _mesa_align_malloc(sizeof(struct T), N)
#define ALIGN_CALLOC_STRUCT(T, N)  (struct T *) _mesa_align_calloc(sizeof(struct T), N)
#define ALIGN_FREE(PTR)            _mesa_align_free(PTR)

#define MEMCPY( DST, SRC, BYTES)   _mesa_memcpy(DST, SRC, BYTES)
#define MEMSET( DST, VAL, N )      _mesa_memset(DST, VAL, N)


/* MACs and BeOS don't support static larger than 32kb, so... */
#if defined(macintosh) && !defined(__MRC__)
/*extern char *AGLAlloc(int size);*/
/*extern void AGLFree(char* ptr);*/
#  define DEFARRAY(TYPE,NAME,SIZE)  			TYPE *NAME = (TYPE*)_mesa_alloc(sizeof(TYPE)*(SIZE))
#  define DEFMARRAY(TYPE,NAME,SIZE1,SIZE2)		TYPE (*NAME)[SIZE2] = (TYPE(*)[SIZE2])_mesa_alloc(sizeof(TYPE)*(SIZE1)*(SIZE2))
#  define DEFMNARRAY(TYPE,NAME,SIZE1,SIZE2,SIZE3)	TYPE (*NAME)[SIZE2][SIZE3] = (TYPE(*)[SIZE2][SIZE3])_mesa_alloc(sizeof(TYPE)*(SIZE1)*(SIZE2)*(SIZE3))

#  define CHECKARRAY(NAME,CMD)				do {if (!(NAME)) {CMD;}} while (0)
#  define UNDEFARRAY(NAME)          			do {if ((NAME)) {_mesa_free((char*)NAME);}  }while (0)
#elif defined(__BEOS__)
#  define DEFARRAY(TYPE,NAME,SIZE)  			TYPE *NAME = (TYPE*)_mesa_malloc(sizeof(TYPE)*(SIZE))
#  define DEFMARRAY(TYPE,NAME,SIZE1,SIZE2)  		TYPE (*NAME)[SIZE2] = (TYPE(*)[SIZE2])_mesa_malloc(sizeof(TYPE)*(SIZE1)*(SIZE2))
#  define DEFMNARRAY(TYPE,NAME,SIZE1,SIZE2,SIZE3)	TYPE (*NAME)[SIZE2][SIZE3] = (TYPE(*)[SIZE2][SIZE3])_mesa_malloc(sizeof(TYPE)*(SIZE1)*(SIZE2)*(SIZE3))
#  define CHECKARRAY(NAME,CMD)				do {if (!(NAME)) {CMD;}} while (0)
#  define UNDEFARRAY(NAME)          			do {if ((NAME)) {_mesa_free((char*)NAME);}  }while (0)
#else
#  define DEFARRAY(TYPE,NAME,SIZE)  			TYPE NAME[SIZE]
#  define DEFMARRAY(TYPE,NAME,SIZE1,SIZE2)		TYPE NAME[SIZE1][SIZE2]
#  define DEFMNARRAY(TYPE,NAME,SIZE1,SIZE2,SIZE3)	TYPE NAME[SIZE1][SIZE2][SIZE3]
#  define CHECKARRAY(NAME,CMD)				do {} while(0)
#  define UNDEFARRAY(NAME)
#endif


#ifdef MESA_EXTERNAL_BUFFERALLOC
/*
 * If you want Mesa's depth/stencil/accum/etc buffers to be allocated
 * with a specialized allocator you can define MESA_EXTERNAL_BUFFERALLOC
 * and implement _ext_mesa_alloc/free_pixelbuffer() in your app.
 * Contributed by Gerk Huisma (gerk@five-d.demon.nl).
 */
extern void *_ext_mesa_alloc_pixelbuffer( unsigned int size );
extern void _ext_mesa_free_pixelbuffer( void *pb );

#define MESA_PBUFFER_ALLOC(BYTES)  (void *) _ext_mesa_alloc_pixelbuffer(BYTES)
#define MESA_PBUFFER_FREE(PTR)     _ext_mesa_free_pixelbuffer(PTR)
#else
/* Default buffer allocation uses the aligned allocation routines: */
#define MESA_PBUFFER_ALLOC(BYTES)  (void *) _mesa_align_malloc(BYTES, 512)
#define MESA_PBUFFER_FREE(PTR)     _mesa_align_free(PTR)
#endif


extern void *
_mesa_malloc( size_t bytes );

extern void *
_mesa_calloc( size_t bytes );

extern void
_mesa_free( void *ptr );

extern void *
_mesa_align_malloc( size_t bytes, unsigned long alignment );

extern void *
_mesa_align_calloc( size_t bytes, unsigned long alignment );

extern void
_mesa_align_free( void *ptr );

extern void *
_mesa_memcpy( void *dest, const void *src, size_t n );

extern void
_mesa_memset( void *dst, int val, size_t n );

extern void
_mesa_memset16( unsigned short *dst, unsigned short val, size_t n );

extern void
_mesa_bzero( void *dst, size_t n );


extern double
_mesa_sin(double a);

extern double
_mesa_cos(double a);

extern double
_mesa_sqrt(double x);

extern double
_mesa_pow(double x, double y);


extern char *
_mesa_getenv( const char *var );

extern char *
_mesa_strstr( const char *haystack, const char *needle );

extern char *
_mesa_strncat( char *dest, const char *src, size_t n );

extern char *
_mesa_strcpy( char *dest, const char *src );

extern char *
_mesa_strncpy( char *dest, const char *src, size_t n );

extern size_t
_mesa_strlen( const char *s );

extern int
_mesa_strcmp( const char *s1, const char *s2 );

extern int
_mesa_strncmp( const char *s1, const char *s2, size_t n );

extern int
_mesa_atoi( const char *s );

extern int
_mesa_sprintf( char *str, const char *fmt, ... );

extern void
_mesa_printf( const char *fmtString, ... );


extern void
_mesa_warning( __GLcontext *gc, const char *fmtString, ... );

extern void
_mesa_problem( const __GLcontext *ctx, const char *fmtString, ... );

extern void
_mesa_error( __GLcontext *ctx, GLenum error, const char *fmtString, ... );

extern void
_mesa_debug( const __GLcontext *ctx, const char *fmtString, ... );


extern void
_mesa_init_default_imports( __GLimports *imports, void *driverCtx );


#endif /* IMPORTS_H */

