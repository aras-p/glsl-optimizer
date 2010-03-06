/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

/**
 * \file glapi_getproc.c
 *
 * Code for implementing glXGetProcAddress(), etc.
 * This was originally in glapi.c but refactored out.
 */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include "glapi/mesa.h"
#else
#include "main/glheader.h"
#include "main/compiler.h"
#endif

#include "glapi/glapi.h"
#include "glapi/glapi_priv.h"
#include "glapi/glapitable.h"
#include "glapi/glapioffsets.h"


/**********************************************************************
 * Static function management.
 */


#if !defined(DISPATCH_FUNCTION_SIZE) && !defined(XFree86Server)
# define NEED_FUNCTION_POINTER
#endif
#include "glapi/glprocs.h"


/**
 * Search the table of static entrypoint functions for the named function
 * and return the corresponding glprocs_table_t entry.
 */
static const glprocs_table_t *
get_static_proc( const char * n )
{
   GLuint i;
   for (i = 0; static_functions[i].Name_offset >= 0; i++) {
      const char *testName = gl_string_table + static_functions[i].Name_offset;
#ifdef MANGLE
      /* skip the prefix on the name */
      if (strcmp(testName, n + 1) == 0)
#else
      if (strcmp(testName, n) == 0)
#endif
      {
	 return &static_functions[i];
      }
   }
   return NULL;
}


/**
 * Return dispatch table offset of the named static (built-in) function.
 * Return -1 if function not found.
 */
static GLint
get_static_proc_offset(const char *funcName)
{
   const glprocs_table_t * const f = get_static_proc( funcName );
   if (f == NULL) {
      return -1;
   }

   return f->Offset;
}


#if !defined(XFree86Server)

/**
 * Return dispatch function address for the named static (built-in) function.
 * Return NULL if function not found.
 */
static _glapi_proc
get_static_proc_address(const char *funcName)
{
   const glprocs_table_t * const f = get_static_proc( funcName );
   if (f == NULL) {
      return NULL;
   }

#if defined(DISPATCH_FUNCTION_SIZE) && defined(GLX_INDIRECT_RENDERING)
   return (f->Address == NULL)
      ? get_entrypoint_address(f->Offset)
      : f->Address;
#elif defined(DISPATCH_FUNCTION_SIZE)
   return get_entrypoint_address(f->Offset);
#else
   return f->Address;
#endif
}

#else

static _glapi_proc
get_static_proc_address(const char *funcName)
{
   (void) funcName;
   return NULL;
}

#endif /* !defined(XFree86Server) */


/**
 * Return the name of the function at the given offset in the dispatch
 * table.  For debugging only.
 */
static const char *
get_static_proc_name( GLuint offset )
{
   GLuint i;
   for (i = 0; static_functions[i].Name_offset >= 0; i++) {
      if (static_functions[i].Offset == offset) {
	 return gl_string_table + static_functions[i].Name_offset;
      }
   }
   return NULL;
}



/**********************************************************************
 * Extension function management.
 */


/**
 * Track information about a function added to the GL API.
 */
struct _glapi_function {
   /**
    * Name of the function.
    */
   const char * name;


   /**
    * Text string that describes the types of the parameters passed to the
    * named function.   Parameter types are converted to characters using the
    * following rules:
    *   - 'i' for \c GLint, \c GLuint, and \c GLenum
    *   - 'p' for any pointer type
    *   - 'f' for \c GLfloat and \c GLclampf
    *   - 'd' for \c GLdouble and \c GLclampd
    */
   const char * parameter_signature;


   /**
    * Offset in the dispatch table where the pointer to the real function is
    * located.  If the driver has not requested that the named function be
    * added to the dispatch table, this will have the value ~0.
    */
   unsigned dispatch_offset;


   /**
    * Pointer to the dispatch stub for the named function.
    * 
    * \todo
    * The semantic of this field should be changed slightly.  Currently, it
    * is always expected to be non-\c NULL.  However, it would be better to
    * only allocate the entry-point stub when the application requests the
    * function via \c glXGetProcAddress.  This would save memory for all the
    * functions that the driver exports but that the application never wants
    * to call.
    */
   _glapi_proc dispatch_stub;
};


/*
 * Number of extension functions which we can dynamically add at runtime.
 */
#define MAX_EXTENSION_FUNCS 300


static struct _glapi_function ExtEntryTable[MAX_EXTENSION_FUNCS];
static GLuint NumExtEntryPoints = 0;


static struct _glapi_function *
get_extension_proc(const char *funcName)
{
   GLuint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].name, funcName) == 0) {
         return & ExtEntryTable[i];
      }
   }
   return NULL;
}


static GLint
get_extension_proc_offset(const char *funcName)
{
   const struct _glapi_function * const f = get_extension_proc( funcName );
   if (f == NULL) {
      return -1;
   }

   return f->dispatch_offset;
}


static _glapi_proc
get_extension_proc_address(const char *funcName)
{
   const struct _glapi_function * const f = get_extension_proc( funcName );
   if (f == NULL) {
      return NULL;
   }

   return f->dispatch_stub;
}


static const char *
get_extension_proc_name(GLuint offset)
{
   GLuint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (ExtEntryTable[i].dispatch_offset == offset) {
         return ExtEntryTable[i].name;
      }
   }
   return NULL;
}


/**
 * strdup() is actually not a standard ANSI C or POSIX routine.
 * Irix will not define it if ANSI mode is in effect.
 */
static char *
str_dup(const char *str)
{
   char *copy;
   copy = (char*) malloc(strlen(str) + 1);
   if (!copy)
      return NULL;
   strcpy(copy, str);
   return copy;
}


/**
 * Generate new entrypoint
 *
 * Use a temporary dispatch offset of ~0 (i.e. -1).  Later, when the driver
 * calls \c _glapi_add_dispatch we'll put in the proper offset.  If that
 * never happens, and the user calls this function, he'll segfault.  That's
 * what you get when you try calling a GL function that doesn't really exist.
 * 
 * \param funcName  Name of the function to create an entry-point for.
 * 
 * \sa _glapi_add_entrypoint
 */

static struct _glapi_function *
add_function_name( const char * funcName )
{
   struct _glapi_function * entry = NULL;
   
   if (NumExtEntryPoints < MAX_EXTENSION_FUNCS) {
      _glapi_proc entrypoint = generate_entrypoint(~0);
      if (entrypoint != NULL) {
	 entry = & ExtEntryTable[NumExtEntryPoints];

	 ExtEntryTable[NumExtEntryPoints].name = str_dup(funcName);
	 ExtEntryTable[NumExtEntryPoints].parameter_signature = NULL;
	 ExtEntryTable[NumExtEntryPoints].dispatch_offset = ~0;
	 ExtEntryTable[NumExtEntryPoints].dispatch_stub = entrypoint;
	 NumExtEntryPoints++;
      }
   }

   return entry;
}


/**
 * Fill-in the dispatch stub for the named function.
 * 
 * This function is intended to be called by a hardware driver.  When called,
 * a dispatch stub may be created created for the function.  A pointer to this
 * dispatch function will be returned by glXGetProcAddress.
 *
 * \param function_names       Array of pointers to function names that should
 *                             share a common dispatch offset.
 * \param parameter_signature  String representing the types of the parameters
 *                             passed to the named function.  Parameter types
 *                             are converted to characters using the following
 *                             rules:
 *                               - 'i' for \c GLint, \c GLuint, and \c GLenum
 *                               - 'p' for any pointer type
 *                               - 'f' for \c GLfloat and \c GLclampf
 *                               - 'd' for \c GLdouble and \c GLclampd
 *
 * \returns
 * The offset in the dispatch table of the named function.  A pointer to the
 * driver's implementation of the named function should be stored at
 * \c dispatch_table[\c offset].  Return -1 if error/problem.
 *
 * \sa glXGetProcAddress
 *
 * \warning
 * This function can only handle up to 8 names at a time.  As far as I know,
 * the maximum number of names ever associated with an existing GL function is
 * 4 (\c glPointParameterfSGIS, \c glPointParameterfEXT,
 * \c glPointParameterfARB, and \c glPointParameterf), so this should not be
 * too painful of a limitation.
 *
 * \todo
 * Determine whether or not \c parameter_signature should be allowed to be
 * \c NULL.  It doesn't seem like much of a hardship for drivers to have to
 * pass in an empty string.
 *
 * \todo
 * Determine if code should be added to reject function names that start with
 * 'glX'.
 * 
 * \bug
 * Add code to compare \c parameter_signature with the parameter signature of
 * a static function.  In order to do that, we need to find a way to \b get
 * the parameter signature of a static function.
 */

PUBLIC int
_glapi_add_dispatch( const char * const * function_names,
		     const char * parameter_signature )
{
   static int next_dynamic_offset = _gloffset_FIRST_DYNAMIC;
   const char * const real_sig = (parameter_signature != NULL)
     ? parameter_signature : "";
   struct _glapi_function * entry[8];
   GLboolean is_static[8];
   unsigned i;
   unsigned j;
   int offset = ~0;
   int new_offset;


   (void) memset( is_static, 0, sizeof( is_static ) );
   (void) memset( entry, 0, sizeof( entry ) );

   for ( i = 0 ; function_names[i] != NULL ; i++ ) {
      /* Do some trivial validation on the name of the function.
       */

      if (!function_names[i] || function_names[i][0] != 'g' || function_names[i][1] != 'l')
         return -1;
   
      /* Determine if the named function already exists.  If the function does
       * exist, it must have the same parameter signature as the function
       * being added.
       */

      new_offset = get_static_proc_offset(function_names[i]);
      if (new_offset >= 0) {
	 /* FIXME: Make sure the parameter signatures match!  How do we get
	  * FIXME: the parameter signature for static functions?
	  */

	 if ( (offset != ~0) && (new_offset != offset) ) {
	    return -1;
	 }

	 is_static[i] = GL_TRUE;
	 offset = new_offset;
      }
   
   
      for ( j = 0 ; j < NumExtEntryPoints ; j++ ) {
	 if (strcmp(ExtEntryTable[j].name, function_names[i]) == 0) {
	    /* The offset may be ~0 if the function name was added by
	     * glXGetProcAddress but never filled in by the driver.
	     */

	    if (ExtEntryTable[j].dispatch_offset != ~0) {
	       if (strcmp(real_sig, ExtEntryTable[j].parameter_signature) 
		   != 0) {
		  return -1;
	       }

	       if ( (offset != ~0) && (ExtEntryTable[j].dispatch_offset != offset) ) {
		  return -1;
	       }

	       offset = ExtEntryTable[j].dispatch_offset;
	    }
	    
	    entry[i] = & ExtEntryTable[j];
	    break;
	 }
      }
   }

   if (offset == ~0) {
      offset = next_dynamic_offset;
      next_dynamic_offset++;
   }

   for ( i = 0 ; function_names[i] != NULL ; i++ ) {
      if (! is_static[i] ) {
	 if (entry[i] == NULL) {
	    entry[i] = add_function_name( function_names[i] );
	    if (entry[i] == NULL) {
	       /* FIXME: Possible memory leak here.
		*/
	       return -1;
	    }
	 }

	 entry[i]->parameter_signature = str_dup(real_sig);
	 fill_in_entrypoint_offset(entry[i]->dispatch_stub, offset);
	 entry[i]->dispatch_offset = offset;
      }
   }
   
   return offset;
}


/**
 * Return offset of entrypoint for named function within dispatch table.
 */
PUBLIC GLint
_glapi_get_proc_offset(const char *funcName)
{
   GLint offset;

   /* search extension functions first */
   offset = get_extension_proc_offset(funcName);
   if (offset >= 0)
      return offset;

   /* search static functions */
   return get_static_proc_offset(funcName);
}



/**
 * Return pointer to the named function.  If the function name isn't found
 * in the name of static functions, try generating a new API entrypoint on
 * the fly with assembly language.
 */
PUBLIC _glapi_proc
_glapi_get_proc_address(const char *funcName)
{
   _glapi_proc func;
   struct _glapi_function * entry;

#ifdef MANGLE
   /* skip the prefix on the name */
   if (funcName[1] != 'g' || funcName[2] != 'l')
      return NULL;
#else
   if (funcName[0] != 'g' || funcName[1] != 'l')
      return NULL;
#endif

   /* search extension functions first */
   func = get_extension_proc_address(funcName);
   if (func)
      return func;

   /* search static functions */
   func = get_static_proc_address(funcName);
   if (func)
      return func;

   entry = add_function_name(funcName);
   return (entry == NULL) ? NULL : entry->dispatch_stub;
}



/**
 * Return the name of the function at the given dispatch offset.
 * This is only intended for debugging.
 */
const char *
_glapi_get_proc_name(GLuint offset)
{
   const char * n;

   /* search built-in functions */
   n = get_static_proc_name(offset);
   if ( n != NULL ) {
      return n;
   }

   /* search added extension functions */
   return get_extension_proc_name(offset);
}



/**********************************************************************
 * GL API table functions.
 */


/*
 * The dispatch table size (number of entries) is the size of the
 * _glapi_table struct plus the number of dynamic entries we can add.
 * The extra slots can be filled in by DRI drivers that register new extension
 * functions.
 */
#define DISPATCH_TABLE_SIZE (sizeof(struct _glapi_table) / sizeof(void *) + MAX_EXTENSION_FUNCS)


/**
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
PUBLIC GLuint
_glapi_get_dispatch_table_size(void)
{
   return DISPATCH_TABLE_SIZE;
}


/**
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intended for debugging purposes.
 */
void
_glapi_check_table_not_null(const struct _glapi_table *table)
{
#ifdef EXTRA_DEBUG /* set to DEBUG for extra DEBUG */
   const GLuint entries = _glapi_get_dispatch_table_size();
   const void **tab = (const void **) table;
   GLuint i;
   for (i = 1; i < entries; i++) {
      assert(tab[i]);
   }
#else
   (void) table;
#endif
}


/**
 * Do some spot checks to be sure that the dispatch table
 * slots are assigned correctly. For debugging only.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
#ifdef EXTRA_DEBUG /* set to DEBUG for extra DEBUG */
   {
      GLuint BeginOffset = _glapi_get_proc_offset("glBegin");
      char *BeginFunc = (char*) &table->Begin;
      GLuint offset = (BeginFunc - (char *) table) / sizeof(void *);
      assert(BeginOffset == _gloffset_Begin);
      assert(BeginOffset == offset);
   }
   {
      GLuint viewportOffset = _glapi_get_proc_offset("glViewport");
      char *viewportFunc = (char*) &table->Viewport;
      GLuint offset = (viewportFunc - (char *) table) / sizeof(void *);
      assert(viewportOffset == _gloffset_Viewport);
      assert(viewportOffset == offset);
   }
   {
      GLuint VertexPointerOffset = _glapi_get_proc_offset("glVertexPointer");
      char *VertexPointerFunc = (char*) &table->VertexPointer;
      GLuint offset = (VertexPointerFunc - (char *) table) / sizeof(void *);
      assert(VertexPointerOffset == _gloffset_VertexPointer);
      assert(VertexPointerOffset == offset);
   }
   {
      GLuint ResetMinMaxOffset = _glapi_get_proc_offset("glResetMinmax");
      char *ResetMinMaxFunc = (char*) &table->ResetMinmax;
      GLuint offset = (ResetMinMaxFunc - (char *) table) / sizeof(void *);
      assert(ResetMinMaxOffset == _gloffset_ResetMinmax);
      assert(ResetMinMaxOffset == offset);
   }
   {
      GLuint blendColorOffset = _glapi_get_proc_offset("glBlendColor");
      char *blendColorFunc = (char*) &table->BlendColor;
      GLuint offset = (blendColorFunc - (char *) table) / sizeof(void *);
      assert(blendColorOffset == _gloffset_BlendColor);
      assert(blendColorOffset == offset);
   }
   {
      GLuint secondaryColor3fOffset = _glapi_get_proc_offset("glSecondaryColor3fEXT");
      char *secondaryColor3fFunc = (char*) &table->SecondaryColor3fEXT;
      GLuint offset = (secondaryColor3fFunc - (char *) table) / sizeof(void *);
      assert(secondaryColor3fOffset == _gloffset_SecondaryColor3fEXT);
      assert(secondaryColor3fOffset == offset);
   }
   {
      GLuint pointParameterivOffset = _glapi_get_proc_offset("glPointParameterivNV");
      char *pointParameterivFunc = (char*) &table->PointParameterivNV;
      GLuint offset = (pointParameterivFunc - (char *) table) / sizeof(void *);
      assert(pointParameterivOffset == _gloffset_PointParameterivNV);
      assert(pointParameterivOffset == offset);
   }
   {
      GLuint setFenceOffset = _glapi_get_proc_offset("glSetFenceNV");
      char *setFenceFunc = (char*) &table->SetFenceNV;
      GLuint offset = (setFenceFunc - (char *) table) / sizeof(void *);
      assert(setFenceOffset == _gloffset_SetFenceNV);
      assert(setFenceOffset == offset);
   }
#else
   (void) table;
#endif
}
