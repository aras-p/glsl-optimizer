/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * \file prog_parameter.c
 * Program parameter lists and functions.
 * \author Brian Paul
 */


#include "glheader.h"
#include "imports.h"
#include "macros.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_statevars.h"


struct gl_program_parameter_list *
_mesa_new_parameter_list(void)
{
   return (struct gl_program_parameter_list *)
      _mesa_calloc(sizeof(struct gl_program_parameter_list));
}


/**
 * Free a parameter list and all its parameters
 */
void
_mesa_free_parameter_list(struct gl_program_parameter_list *paramList)
{
   GLuint i;
   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Name)
	 _mesa_free((void *) paramList->Parameters[i].Name);
   }
   _mesa_free(paramList->Parameters);
   if (paramList->ParameterValues)
      _mesa_align_free(paramList->ParameterValues);
   _mesa_free(paramList);
}



/**
 * Add a new parameter to a parameter list.
 * Note that parameter values are usually 4-element GLfloat vectors.
 * When size > 4 we'll allocate a sequential block of parameters to
 * store all the values (in blocks of 4).
 *
 * \param paramList  the list to add the parameter to
 * \param name  the parameter name, will be duplicated/copied!
 * \param values  initial parameter value, up to 4 GLfloats
 * \param size  number of elements in 'values' vector (1..4, or more)
 * \param type  type of parameter, such as 
 * \return  index of new parameter in the list, or -1 if error (out of mem)
 */
GLint
_mesa_add_parameter(struct gl_program_parameter_list *paramList,
                    const char *name, const GLfloat *values, GLuint size,
                    enum register_file type)
{
   const GLuint oldNum = paramList->NumParameters;
   const GLuint sz4 = (size + 3) / 4; /* no. of new param slots needed */

   assert(size > 0);
   assert(size <= 16);  /* XXX anything larger than a matrix??? */

   if (oldNum + sz4 > paramList->Size) {
      /* Need to grow the parameter list array (alloc some extra) */
      paramList->Size = paramList->Size + 4 * sz4;

      /* realloc arrays */
      paramList->Parameters = (struct gl_program_parameter *)
	 _mesa_realloc(paramList->Parameters,
		       oldNum * sizeof(struct gl_program_parameter),
		       paramList->Size * sizeof(struct gl_program_parameter));

      paramList->ParameterValues = (GLfloat (*)[4])
         _mesa_align_realloc(paramList->ParameterValues,         /* old buf */
                             oldNum * 4 * sizeof(GLfloat),      /* old size */
                             paramList->Size * 4 *sizeof(GLfloat), /* new sz */
                             16);
   }

   if (!paramList->Parameters ||
       !paramList->ParameterValues) {
      /* out of memory */
      paramList->NumParameters = 0;
      paramList->Size = 0;
      return -1;
   }
   else {
      GLuint i;

      paramList->NumParameters = oldNum + sz4;

      _mesa_memset(&paramList->Parameters[oldNum], 0, 
		   sz4 * sizeof(struct gl_program_parameter));

      for (i = 0; i < sz4; i++) {
         struct gl_program_parameter *p = paramList->Parameters + oldNum + i;
         p->Name = name ? _mesa_strdup(name) : NULL;
         p->Type = type;
         p->Size = size;
         if (values) {
            COPY_4V(paramList->ParameterValues[oldNum + i], values);
            values += 4;
         }
         size -= 4;
      }
      return (GLint) oldNum;
   }
}


/**
 * Add a new named program parameter (Ex: NV_fragment_program DEFINE statement)
 * \return index of the new entry in the parameter list
 */
GLint
_mesa_add_named_parameter(struct gl_program_parameter_list *paramList,
                          const char *name, const GLfloat values[4])
{
   return _mesa_add_parameter(paramList, name, values, 4, PROGRAM_NAMED_PARAM);
}


/**
 * Add a new named constant to the parameter list.
 * This will be used when the program contains something like this:
 *    PARAM myVals = { 0, 1, 2, 3 };
 *
 * \param paramList  the parameter list
 * \param name  the name for the constant
 * \param values  four float values
 * \return index/position of the new parameter in the parameter list
 */
GLint
_mesa_add_named_constant(struct gl_program_parameter_list *paramList,
                         const char *name, const GLfloat values[4],
                         GLuint size)
{
#if 0 /* disable this for now -- we need to save the name! */
   GLint pos;
   GLuint swizzle;
   ASSERT(size == 4); /* XXX future feature */
   /* check if we already have this constant */
   if (_mesa_lookup_parameter_constant(paramList, values, 4, &pos, &swizzle)) {
      return pos;
   }
#endif
   size = 4; /** XXX fix */
   return _mesa_add_parameter(paramList, name, values, size, PROGRAM_CONSTANT);
}


/**
 * Add a new unnamed constant to the parameter list.
 * This will be used when the program contains something like this:
 *    MOV r, { 0, 1, 2, 3 };
 *
 * \param paramList  the parameter list
 * \param values  four float values
 * \param swizzleOut  returns swizzle mask for accessing the constant
 * \return index/position of the new parameter in the parameter list.
 */
GLint
_mesa_add_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const GLfloat values[4], GLuint size,
                           GLuint *swizzleOut)
{
   GLint pos;
   GLuint swizzle;
   ASSERT(size >= 1);
   ASSERT(size <= 4);
   size = 4; /* XXX temporary */
   /* check if we already have this constant */
   if (_mesa_lookup_parameter_constant(paramList, values,
                                       size, &pos, &swizzle)) {
      return pos;
   }
   return _mesa_add_parameter(paramList, NULL, values, size, PROGRAM_CONSTANT);
}


GLint
_mesa_add_uniform(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size)
{
   GLint i = _mesa_lookup_parameter_index(paramList, -1, name);
   if (i >= 0 && paramList->Parameters[i].Type == PROGRAM_UNIFORM) {
      /* already in list */
      return i;
   }
   else {
      i = _mesa_add_parameter(paramList, name, NULL, size, PROGRAM_UNIFORM);
      return i;
   }
}


GLint
_mesa_add_varying(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size)
{
   GLint i = _mesa_lookup_parameter_index(paramList, -1, name);
   if (i >= 0 && paramList->Parameters[i].Type == PROGRAM_VARYING) {
      /* already in list */
      return i;
   }
   else {
      assert(size == 4);
      i = _mesa_add_parameter(paramList, name, NULL, size, PROGRAM_VARYING);
      return i;
   }
}




#if 0 /* not used yet */
/**
 * Returns the number of 4-component registers needed to store a piece
 * of GL state.  For matrices this may be as many as 4 registers,
 * everything else needs
 * just 1 register.
 */
static GLuint
sizeof_state_reference(const GLint *stateTokens)
{
   if (stateTokens[0] == STATE_MATRIX) {
      GLuint rows = stateTokens[4] - stateTokens[3] + 1;
      assert(rows >= 1);
      assert(rows <= 4);
      return rows;
   }
   else {
      return 1;
   }
}
#endif


/**
 * Add a new state reference to the parameter list.
 * This will be used when the program contains something like this:
 *    PARAM ambient = state.material.front.ambient;
 *
 * \param paramList  the parameter list
 * \param state  an array of 6 state tokens
 * \return index of the new parameter.
 */
GLint
_mesa_add_state_reference(struct gl_program_parameter_list *paramList,
                          const GLint *stateTokens)
{
   const GLuint size = 4; /* XXX fix */
   const char *name;
   GLint index;

   /* Check if the state reference is already in the list */
   for (index = 0; index < paramList->NumParameters; index++) {
      GLuint i, match = 0;
      for (i = 0; i < 6; i++) {
         if (paramList->Parameters[index].StateIndexes[i] == stateTokens[i]) {
            match++;
         }
         else {
            break;
         }
      }
      if (match == 6) {
         /* this state reference is already in the parameter list */
         return index;
      }
   }

   name = _mesa_program_state_string(stateTokens);
   index = _mesa_add_parameter(paramList, name, NULL, size, PROGRAM_STATE_VAR);
   if (index >= 0) {
      GLuint i;
      for (i = 0; i < 6; i++) {
         paramList->Parameters[index].StateIndexes[i]
            = (gl_state_index) stateTokens[i];
      }
      paramList->StateFlags |= _mesa_program_state_flags(stateTokens);
   }

   /* free name string here since we duplicated it in add_parameter() */
   _mesa_free((void *) name);

   return index;
}


/**
 * Lookup a parameter value by name in the given parameter list.
 * \return pointer to the float[4] values.
 */
GLfloat *
_mesa_lookup_parameter_value(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLuint i = _mesa_lookup_parameter_index(paramList, nameLen, name);
   if (i < 0)
      return NULL;
   else
      return paramList->ParameterValues[i];
}


/**
 * Given a program parameter name, find its position in the list of parameters.
 * \param paramList  the parameter list to search
 * \param nameLen  length of name (in chars).
 *                 If length is negative, assume that name is null-terminated.
 * \param name  the name to search for
 * \return index of parameter in the list.
 */
GLint
_mesa_lookup_parameter_index(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name)
{
   GLint i;

   if (!paramList)
      return -1;

   if (nameLen == -1) {
      /* name is null-terminated */
      for (i = 0; i < (GLint) paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strcmp(paramList->Parameters[i].Name, name) == 0)
            return i;
      }
   }
   else {
      /* name is not null-terminated, use nameLen */
      for (i = 0; i < (GLint) paramList->NumParameters; i++) {
         if (paramList->Parameters[i].Name &&
	     _mesa_strncmp(paramList->Parameters[i].Name, name, nameLen) == 0
             && _mesa_strlen(paramList->Parameters[i].Name) == (size_t)nameLen)
            return i;
      }
   }
   return -1;
}


/**
 * Look for a float vector in the given parameter list.  The float vector
 * may be of length 1, 2, 3 or 4.
 * \param paramList  the parameter list to search
 * \param v  the float vector to search for
 * \param size  number of element in v
 * \param posOut  returns the position of the constant, if found
 * \param swizzleOut  returns a swizzle mask describing location of the
 *                    vector elements if found
 * \return GL_TRUE if found, GL_FALSE if not found
 */
GLboolean
_mesa_lookup_parameter_constant(const struct gl_program_parameter_list *paramList,
                                const GLfloat v[], GLsizei vSize,
                                GLint *posOut, GLuint *swizzleOut)
{
   GLuint i;

   assert(vSize >= 1);
   assert(vSize <= 4);

   if (!paramList)
      return -1;

   for (i = 0; i < paramList->NumParameters; i++) {
      if (paramList->Parameters[i].Type == PROGRAM_CONSTANT) {
         const GLint maxShift = 4 - vSize;
         GLint shift, j;
         for (shift = 0; shift <= maxShift; shift++) {
            GLint matched = 0;
            GLuint swizzle[4];
            swizzle[0] = swizzle[1] = swizzle[2] = swizzle[3] = 0;
            /* XXX we could do out-of-order swizzle matches too, someday */
            for (j = 0; j < vSize; j++) {
               assert(shift + j < 4);
               if (paramList->ParameterValues[i][shift + j] == v[j]) {
                  matched++;
                  swizzle[j] = shift + j;
                  ASSERT(swizzle[j] >= SWIZZLE_X);
                  ASSERT(swizzle[j] <= SWIZZLE_W);
               }
            }
            if (matched == vSize) {
               /* found! */
               *posOut = i;
               *swizzleOut = MAKE_SWIZZLE4(swizzle[0], swizzle[1],
                                           swizzle[2], swizzle[3]);
               return GL_TRUE;
            }
         }
      }
   }

   *posOut = -1;
   return GL_FALSE;
}


struct gl_program_parameter_list *
_mesa_clone_parameter_list(const struct gl_program_parameter_list *list)
{
   struct gl_program_parameter_list *clone;
   GLuint i;

   clone = _mesa_new_parameter_list();
   if (!clone)
      return NULL;

   /** Not too efficient, but correct */
   for (i = 0; i < list->NumParameters; i++) {
      struct gl_program_parameter *p = list->Parameters + i;
      GLuint size = MIN2(p->Size, 4);
      GLint j = _mesa_add_parameter(clone, p->Name, list->ParameterValues[i],
                                    size, p->Type);
      ASSERT(j >= 0);
      /* copy state indexes */
      if (p->Type == PROGRAM_STATE_VAR) {
         GLint k;
         struct gl_program_parameter *q = clone->Parameters + j;
         for (k = 0; k < 6; k++) {
            q->StateIndexes[k] = p->StateIndexes[k];
         }
      }
      else {
         clone->Parameters[j].Size = p->Size;
      }
   }

   return clone;
}


/**
 * Find longest name of any parameter in list.
 */
GLuint
_mesa_parameter_longest_name(const struct gl_program_parameter_list *list)
{
   GLuint i, maxLen = 0;
   if (!list)
      return 0;
   for (i = 0; i < list->NumParameters; i++) {
      GLuint len = _mesa_strlen(list->Parameters[i].Name);
      if (len > maxLen)
         maxLen = len;
   }
   return maxLen;
}

