/* $Id: texobj.c,v 1.41 2001/02/26 18:25:25 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "colortab.h"
#include "context.h"
#include "enums.h"
#include "hash.h"
#include "macros.h"
#include "mem.h"
#include "teximage.h"
#include "texstate.h"
#include "texobj.h"
#include "mtypes.h"
#endif



/*
 * Allocate a new texture object and add it to the linked list of texture
 * objects.  If name>0 then also insert the new texture object into the hash
 * table.
 * Input:  shared - the shared GL state structure to contain the texture object
 *         name - integer name for the texture object
 *         dimensions - either 1, 2, 3 or 6 (cube map)
 *                      zero is ok for the sake of GenTextures()
 * Return:  pointer to new texture object
 */
struct gl_texture_object *
_mesa_alloc_texture_object( struct gl_shared_state *shared, 
			    GLuint name, GLuint dimensions )
{
   struct gl_texture_object *obj;

   ASSERT(dimensions <= 3 || dimensions == 6);

   obj = CALLOC_STRUCT(gl_texture_object);

   if (obj) {
      /* init the non-zero fields */
      _glthread_INIT_MUTEX(obj->Mutex);
      obj->RefCount = 1;
      obj->Name = name;
      obj->Dimensions = dimensions;
      obj->Priority = 1.0F;
      obj->WrapS = GL_REPEAT;
      obj->WrapT = GL_REPEAT;
      obj->WrapR = GL_REPEAT;
      obj->MinFilter = GL_NEAREST_MIPMAP_LINEAR;
      obj->MagFilter = GL_LINEAR;
      obj->MinLod = -1000.0;
      obj->MaxLod = 1000.0;
      obj->BaseLevel = 0;
      obj->MaxLevel = 1000;
      obj->CompareFlag = GL_FALSE;
      obj->CompareOperator = GL_TEXTURE_LEQUAL_R_SGIX;
      obj->ShadowAmbient = 0;
      _mesa_init_colortable(&obj->Palette);

      /* insert into linked list */
      if (shared) {
         _glthread_LOCK_MUTEX(shared->Mutex);
         obj->Next = shared->TexObjectList;
         shared->TexObjectList = obj;
         _glthread_UNLOCK_MUTEX(shared->Mutex);
      }

      if (name > 0) {
         /* insert into hash table */
         _mesa_HashInsert(shared->TexObjects, name, obj);
      }
   }
   return obj;
}


/*
 * Deallocate a texture object struct and remove it from the given
 * shared GL state.
 * Input:  shared - the shared GL state to which the object belongs
 *         t - the texture object to delete
 */
void _mesa_free_texture_object( struct gl_shared_state *shared,
                                struct gl_texture_object *t )
{
   struct gl_texture_object *tprev, *tcurr;

   assert(t);

   /* unlink t from the linked list */
   if (shared) {
      _glthread_LOCK_MUTEX(shared->Mutex);
      tprev = NULL;
      tcurr = shared->TexObjectList;
      while (tcurr) {
         if (tcurr==t) {
            if (tprev) {
               tprev->Next = t->Next;
            }
            else {
               shared->TexObjectList = t->Next;
            }
            break;
         }
         tprev = tcurr;
         tcurr = tcurr->Next;
      }
      _glthread_UNLOCK_MUTEX(shared->Mutex);
   }

   if (t->Name) {
      /* remove from hash table */
      _mesa_HashRemove(shared->TexObjects, t->Name);
   }

   _mesa_free_colortable_data(&t->Palette);

   /* free the texture images */
   {
      GLuint i;
      for (i=0;i<MAX_TEXTURE_LEVELS;i++) {
         if (t->Image[i]) {
            _mesa_free_texture_image( t->Image[i] );
         }
      }
   }

   /* free this object */
   FREE( t );
}


/*
 * Report why a texture object is incomplete.  (for debug only)
 */
#if 0
static void
incomplete(const struct gl_texture_object *t, const char *why)
{
   printf("Texture Obj %d incomplete because: %s\n", t->Name, why);
}
#else
#define incomplete(a, b)
#endif


/*
 * Examine a texture object to determine if it is complete.
 * The t->Complete flag will be set to GL_TRUE or GL_FALSE accordingly.
 */
void
_mesa_test_texobj_completeness( const GLcontext *ctx,
                                struct gl_texture_object *t )
{
   const GLint baseLevel = t->BaseLevel;
   GLint maxLog2 = 0;

   t->Complete = GL_TRUE;  /* be optimistic */

   /* Always need the base level image */
   if (!t->Image[baseLevel]) {
      incomplete(t, "Image[baseLevel] == NULL");
      t->Complete = GL_FALSE;
      return;
   }

   /* Compute _MaxLevel */
   if (t->Dimensions == 1) {
      maxLog2 = t->Image[baseLevel]->WidthLog2;
   }
   else if (t->Dimensions == 2 || t->Dimensions == 6) {
      maxLog2 = MAX2(t->Image[baseLevel]->WidthLog2,
                     t->Image[baseLevel]->HeightLog2);
   }
   else if (t->Dimensions == 3) {
      GLint max = MAX2(t->Image[baseLevel]->WidthLog2,
                       t->Image[baseLevel]->HeightLog2);
      maxLog2 = MAX2(max, (GLint)(t->Image[baseLevel]->DepthLog2));
   }

   t->_MaxLevel = baseLevel + maxLog2;
   t->_MaxLevel = MIN2(t->_MaxLevel, t->MaxLevel);
   t->_MaxLevel = MIN2(t->_MaxLevel, ctx->Const.MaxTextureLevels - 1);

   /* Compute _MaxLambda = q - b (see the 1.2 spec) used during mipmapping */
   t->_MaxLambda = (GLfloat) (t->_MaxLevel - t->BaseLevel);

   if (t->Dimensions == 6) {
      /* make sure that all six cube map level 0 images are the same size */
      const GLint w = t->Image[baseLevel]->Width2;
      const GLint h = t->Image[baseLevel]->Height2;
      if (!t->NegX[baseLevel] ||
          t->NegX[baseLevel]->Width2 != w ||
          t->NegX[baseLevel]->Height2 != h ||
          !t->PosY[baseLevel] ||
          t->PosY[baseLevel]->Width2 != w ||
          t->PosY[baseLevel]->Height2 != h ||
          !t->NegY[baseLevel] ||
          t->NegY[baseLevel]->Width2 != w ||
          t->NegY[baseLevel]->Height2 != h ||
          !t->PosZ[baseLevel] ||
          t->PosZ[baseLevel]->Width2 != w ||
          t->PosZ[baseLevel]->Height2 != h ||
          !t->NegZ[baseLevel] ||
          t->NegZ[baseLevel]->Width2 != w ||
          t->NegZ[baseLevel]->Height2 != h) {
         t->Complete = GL_FALSE;
         incomplete(t, "Non-quare cubemap image");
         return;
      }
   }

   if (t->MinFilter != GL_NEAREST && t->MinFilter != GL_LINEAR) {
      /*
       * Mipmapping: determine if we have a complete set of mipmaps
       */
      GLint i;
      GLint minLevel = baseLevel;
      GLint maxLevel = t->_MaxLevel;

      if (minLevel > maxLevel) {
         t->Complete = GL_FALSE;
         incomplete(t, "minLevel > maxLevel");
         return;
      }

      /* Test dimension-independent attributes */
      for (i = minLevel; i <= maxLevel; i++) {
         if (t->Image[i]) {
            if (t->Image[i]->Format != t->Image[baseLevel]->Format) {
               t->Complete = GL_FALSE;
               incomplete(t, "Format[i] != Format[baseLevel]");
               return;
            }
            if (t->Image[i]->Border != t->Image[baseLevel]->Border) {
               t->Complete = GL_FALSE;
               incomplete(t, "Border[i] != Border[baseLevel]");
               return;
            }
         }
      }

      /* Test things which depend on number of texture image dimensions */
      if (t->Dimensions == 1) {
         /* Test 1-D mipmaps */
         GLuint width = t->Image[baseLevel]->Width2;
         for (i = baseLevel + 1; i < ctx->Const.MaxTextureLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "1D Image[i] == NULL");
                  return;
               }
               if (t->Image[i]->Width2 != width ) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "1D Image[i] bad width");
                  return;
               }
            }
            if (width == 1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if (t->Dimensions == 2) {
         /* Test 2-D mipmaps */
         GLuint width = t->Image[baseLevel]->Width2;
         GLuint height = t->Image[baseLevel]->Height2;
         for (i = baseLevel + 1; i < ctx->Const.MaxTextureLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "2D Image[i] == NULL");
                  return;
               }
               if (t->Image[i]->Width2 != width) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "2D Image[i] bad width");
                  return;
               }
               if (t->Image[i]->Height2 != height) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "2D Image[i] bad height");
                  return;
               }
               if (width==1 && height==1) {
                  return;  /* found smallest needed mipmap, all done! */
               }
            }
         }
      }
      else if (t->Dimensions == 3) {
         /* Test 3-D mipmaps */
         GLuint width = t->Image[baseLevel]->Width2;
         GLuint height = t->Image[baseLevel]->Height2;
         GLuint depth = t->Image[baseLevel]->Depth2;
	 for (i = baseLevel + 1; i < ctx->Const.MaxTextureLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (depth > 1) {
               depth /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  incomplete(t, "3D Image[i] == NULL");
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Width2 != width) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "3D Image[i] bad width");
                  return;
               }
               if (t->Image[i]->Height2 != height) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "3D Image[i] bad height");
                  return;
               }
               if (t->Image[i]->Depth2 != depth) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "3D Image[i] bad depth");
                  return;
               }
            }
            if (width == 1 && height == 1 && depth == 1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if (t->Dimensions == 6) {
         /* make sure 6 cube faces are consistant */
         GLuint width = t->Image[baseLevel]->Width2;
         GLuint height = t->Image[baseLevel]->Height2;
	 for (i = baseLevel + 1; i < ctx->Const.MaxTextureLevels; i++) {
            if (width > 1) {
               width /= 2;
            }
            if (height > 1) {
               height /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               /* check that we have images defined */
               if (!t->Image[i] || !t->NegX[i] ||
                   !t->PosY[i]  || !t->NegY[i] ||
                   !t->PosZ[i]  || !t->NegZ[i]) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "CubeMap Image[i] == NULL");
                  return;
               }
               /* check that all six images have same size */
               if (t->NegX[i]->Width2!=width || t->NegX[i]->Height2!=height ||
                   t->PosY[i]->Width2!=width || t->PosY[i]->Height2!=height ||
                   t->NegY[i]->Width2!=width || t->NegY[i]->Height2!=height ||
                   t->PosZ[i]->Width2!=width || t->PosZ[i]->Height2!=height ||
                   t->NegZ[i]->Width2!=width || t->NegZ[i]->Height2!=height) {
                  t->Complete = GL_FALSE;
                  incomplete(t, "CubeMap Image[i] bad size");
                  return;
               }
            }
            if (width == 1 && height == 1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else {
         /* Dimensions = ??? */
         gl_problem(ctx, "Bug in gl_test_texture_object_completeness\n");
      }
   }
}


_glthread_DECLARE_STATIC_MUTEX(GenTexturesLock);


/*
 * Execute glGenTextures
 */
void
_mesa_GenTextures( GLsizei n, GLuint *texName )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint first;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (n < 0) {
      gl_error( ctx, GL_INVALID_VALUE, "glGenTextures" );
      return;
   }

   if (!texName)
      return;

   /*
    * This must be atomic (generation and allocation of texture IDs)
    */
   _glthread_LOCK_MUTEX(GenTexturesLock);

   first = _mesa_HashFindFreeKeyBlock(ctx->Shared->TexObjects, n);

   /* Return the texture names */
   for (i=0;i<n;i++) {
      texName[i] = first + i;
   }

   /* Allocate new, empty texture objects */
   for (i=0;i<n;i++) {
      GLuint name = first + i;
      GLuint dims = 0;
      (void) _mesa_alloc_texture_object( ctx->Shared, name, dims);
   }

   _glthread_UNLOCK_MUTEX(GenTexturesLock);
}



/*
 * Execute glDeleteTextures
 */
void
_mesa_DeleteTextures( GLsizei n, const GLuint *texName)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex */

   if (!texName)
      return;

   for (i=0;i<n;i++) {
      if (texName[i] > 0) {
         struct gl_texture_object *delObj = (struct gl_texture_object *)
            _mesa_HashLookup(ctx->Shared->TexObjects, texName[i]);
         if (delObj) {
            /* First check if this texture is currently bound.
             * If so, unbind it and decrement the reference count.
             */
            GLuint u;
            for (u = 0; u < MAX_TEXTURE_UNITS; u++) {
               struct gl_texture_unit *unit = &ctx->Texture.Unit[u];
               if (delObj == unit->Current1D) {
                  unit->Current1D = ctx->Shared->Default1D;
                  ctx->Shared->Default1D->RefCount++;
               }
               else if (delObj == unit->Current2D) {
                  unit->Current2D = ctx->Shared->Default2D;
                  ctx->Shared->Default2D->RefCount++;
               }
               else if (delObj == unit->Current3D) {
                  unit->Current3D = ctx->Shared->Default3D;
                  ctx->Shared->Default3D->RefCount++;
               }
               else if (delObj == unit->CurrentCubeMap) {
                  unit->CurrentCubeMap = ctx->Shared->DefaultCubeMap;
                  ctx->Shared->DefaultCubeMap->RefCount++;
               }
            }
            ctx->NewState |= _NEW_TEXTURE;

            /* Decrement reference count and delete if zero */
            delObj->RefCount--;
            ASSERT( delObj->RefCount >= 0 );
            if (delObj->RefCount == 0) {
               if (ctx->Driver.DeleteTexture)
                  (*ctx->Driver.DeleteTexture)( ctx, delObj );
               _mesa_free_texture_object(ctx->Shared, delObj);
            }
         }
      }
   }
}



/*
 * Execute glBindTexture
 */
void
_mesa_BindTexture( GLenum target, GLuint texName )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *oldTexObj;
   struct gl_texture_object *newTexObj = 0;
   GLuint targetDim;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glBindTexture %s %d\n",
	      gl_lookup_enum_by_nr(target), (GLint) texName);

   switch (target) {
      case GL_TEXTURE_1D:
         targetDim = 1;
         oldTexObj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         targetDim = 2;
         oldTexObj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D:
         targetDim = 3;
         oldTexObj = texUnit->Current3D;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         if (ctx->Extensions.ARB_texture_cube_map) {
            targetDim = 6;
            oldTexObj = texUnit->CurrentCubeMap;
            break;
         }
         /* fallthrough */
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glBindTexture(target)" );
         return;
   }

   if (oldTexObj->Name == texName)
      return;   /* rebinding the same texture- no change */

   /*
    * Get pointer to new texture object (newTexObj)
    */
   if (texName == 0) {
      /* newTexObj = a default texture object */
      switch (target) {
         case GL_TEXTURE_1D:
            newTexObj = ctx->Shared->Default1D;
            break;
         case GL_TEXTURE_2D:
            newTexObj = ctx->Shared->Default2D;
            break;
         case GL_TEXTURE_3D:
            newTexObj = ctx->Shared->Default3D;
            break;
         case GL_TEXTURE_CUBE_MAP_ARB:
            newTexObj = ctx->Shared->DefaultCubeMap;
            break;
         default:
            ; /* Bad targets are caught above */
      }
   }
   else {
      /* non-default texture object */
      const struct _mesa_HashTable *hash = ctx->Shared->TexObjects;
      newTexObj = (struct gl_texture_object *) _mesa_HashLookup(hash, texName);
      if (newTexObj) {
         /* error checking */
         if (newTexObj->Dimensions > 0 && newTexObj->Dimensions != targetDim) {
            /* the named texture object's dimensions don't match the target */
            gl_error( ctx, GL_INVALID_OPERATION, "glBindTexture" );
            return;
         }
      }
      else {
         /* if this is a new texture id, allocate a texture object now */
	 newTexObj = _mesa_alloc_texture_object( ctx->Shared, texName,
						 targetDim);
         if (!newTexObj) {
            gl_error(ctx, GL_OUT_OF_MEMORY, "glBindTexture");
            return;
         }
      }
      newTexObj->Dimensions = targetDim;
   }

   newTexObj->RefCount++;

   
   /* do the actual binding, but first flush outstanding vertices:
    */
   FLUSH_VERTICES(ctx, _NEW_TEXTURE);

   switch (target) {
      case GL_TEXTURE_1D:
         texUnit->Current1D = newTexObj;
         break;
      case GL_TEXTURE_2D:
         texUnit->Current2D = newTexObj;
         break;
      case GL_TEXTURE_3D:
         texUnit->Current3D = newTexObj;
         break;
      case GL_TEXTURE_CUBE_MAP_ARB:
         texUnit->CurrentCubeMap = newTexObj;
         break;
      default:
         gl_problem(ctx, "bad target in BindTexture");
   }

   /* Pass BindTexture call to device driver */
   if (ctx->Driver.BindTexture)
      (*ctx->Driver.BindTexture)( ctx, target, newTexObj );

   if (oldTexObj->Name > 0) {
      /* never delete default (id=0) texture objects */
      oldTexObj->RefCount--;
      if (oldTexObj->RefCount <= 0) {
         if (ctx->Driver.DeleteTexture) {
	    (*ctx->Driver.DeleteTexture)( ctx, oldTexObj );
	 }
         _mesa_free_texture_object(ctx->Shared, oldTexObj);
      }
   }
}



/*
 * Execute glPrioritizeTextures
 */
void
_mesa_PrioritizeTextures( GLsizei n, const GLuint *texName,
                          const GLclampf *priorities )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (n < 0) {
      gl_error( ctx, GL_INVALID_VALUE, "glPrioritizeTextures" );
      return;
   }

   if (!priorities)
      return;

   for (i = 0; i < n; i++) {
      if (texName[i] > 0) {
         struct gl_texture_object *t = (struct gl_texture_object *)
            _mesa_HashLookup(ctx->Shared->TexObjects, texName[i]);
         if (t) {
            t->Priority = CLAMP( priorities[i], 0.0F, 1.0F );
	    if (ctx->Driver.PrioritizeTexture)
	       ctx->Driver.PrioritizeTexture( ctx, t, t->Priority );
         }
      }
   }

   ctx->NewState |= _NEW_TEXTURE;
}



/*
 * Execute glAreTexturesResident
 */
GLboolean
_mesa_AreTexturesResident(GLsizei n, const GLuint *texName,
                          GLboolean *residences)
{
   GET_CURRENT_CONTEXT(ctx);
   GLboolean allResident = GL_TRUE;
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (n < 0) {
      gl_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident(n)");
      return GL_FALSE;
   }

   if (!texName || !residences)
      return GL_FALSE;

   for (i = 0; i < n; i++) {
      struct gl_texture_object *t;
      if (texName[i] == 0) {
         gl_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident(textures)");
         return GL_FALSE;
      }
      t = (struct gl_texture_object *)
         _mesa_HashLookup(ctx->Shared->TexObjects, texName[i]);
      if (t) {
	 if (ctx->Driver.IsTextureResident) {
	    residences[i] = ctx->Driver.IsTextureResident(ctx, t);
            if (!residences[i])
               allResident = GL_FALSE;
         }
	 else {
	    residences[i] = GL_TRUE;
         }
      }
      else {
         gl_error(ctx, GL_INVALID_VALUE, "glAreTexturesResident(textures)");
         return GL_FALSE;
      }
   }
   return allResident;
}



/*
 * Execute glIsTexture
 */
GLboolean
_mesa_IsTexture( GLuint texture )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);
   return texture > 0 && _mesa_HashLookup(ctx->Shared->TexObjects, texture);
}

