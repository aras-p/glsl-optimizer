/* $Id: texobj.c,v 1.2 1999/09/30 11:18:22 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "context.h"
#include "enums.h"
#include "hash.h"
#include "macros.h"
#include "teximage.h"
#include "texstate.h"
#include "texobj.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



/*
 * Allocate a new texture object and add it to the linked list of texture
 * objects.  If name>0 then also insert the new texture object into the hash
 * table.
 * Input:  shared - the shared GL state structure to contain the texture object
 *         name - integer name for the texture object
 *         dimensions - either 1, 2 or 3
 * Return:  pointer to new texture object
 */
struct gl_texture_object *
gl_alloc_texture_object( struct gl_shared_state *shared, GLuint name,
                         GLuint dimensions)
{
   struct gl_texture_object *obj;

   assert(dimensions <= 3);

   obj = (struct gl_texture_object *)
                     calloc(1,sizeof(struct gl_texture_object));
   if (obj) {
      /* init the non-zero fields */
      obj->Name = name;
      obj->Dimensions = dimensions;
      obj->WrapS = GL_REPEAT;
      obj->WrapT = GL_REPEAT;
      obj->MinFilter = GL_NEAREST_MIPMAP_LINEAR;
      obj->MagFilter = GL_LINEAR;
      obj->MinLod = -1000.0;
      obj->MaxLod = 1000.0;
      obj->BaseLevel = 0;
      obj->MaxLevel = 1000;
      obj->MinMagThresh = 0.0F;
      obj->Palette[0] = 255;
      obj->Palette[1] = 255;
      obj->Palette[2] = 255;
      obj->Palette[3] = 255;
      obj->PaletteSize = 1;
      obj->PaletteIntFormat = GL_RGBA;
      obj->PaletteFormat = GL_RGBA;

      /* insert into linked list */
      if (shared) {
         obj->Next = shared->TexObjectList;
         shared->TexObjectList = obj;
      }

      if (name > 0) {
         /* insert into hash table */
         HashInsert(shared->TexObjects, name, obj);
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
void gl_free_texture_object( struct gl_shared_state *shared,
                             struct gl_texture_object *t )
{
   struct gl_texture_object *tprev, *tcurr;

   assert(t);

   /* Remove t from dirty list so we don't touch free'd memory later.
    * Test for shared since Proxy texture aren't in global linked list.
    */
   if (shared)
      gl_remove_texobj_from_dirty_list( shared, t );

   /* unlink t from the linked list */
   if (shared) {
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
   }

   if (t->Name) {
      /* remove from hash table */
      HashRemove(shared->TexObjects, t->Name);
   }

   /* free texture image */
   {
      GLuint i;
      for (i=0;i<MAX_TEXTURE_LEVELS;i++) {
         if (t->Image[i]) {
            gl_free_texture_image( t->Image[i] );
         }
      }
   }
   /* free this object */
   free( t );
}



/*
 * Examine a texture object to determine if it is complete or not.
 * The t->Complete flag will be set to GL_TRUE or GL_FALSE accordingly.
 */
void gl_test_texture_object_completeness( const GLcontext *ctx, struct gl_texture_object *t )
{
   t->Complete = GL_TRUE;  /* be optimistic */

   /* Always need level zero image */
   if (!t->Image[0] || !t->Image[0]->Data) {
      t->Complete = GL_FALSE;
      return;
   }

   /* Compute number of mipmap levels */
   if (t->Dimensions==1) {
      t->P = t->Image[0]->WidthLog2;
   }
   else if (t->Dimensions==2) {
      t->P = MAX2(t->Image[0]->WidthLog2, t->Image[0]->HeightLog2);
   }
   else if (t->Dimensions==3) {
      GLint max = MAX2(t->Image[0]->WidthLog2, t->Image[0]->HeightLog2);
      max = MAX2(max, (GLint)(t->Image[0]->DepthLog2));
      t->P = max;
   }

   /* Compute M (see the 1.2 spec) used during mipmapping */
   t->M = (GLfloat) (MIN2(t->MaxLevel, t->P) - t->BaseLevel);


   if (t->MinFilter!=GL_NEAREST && t->MinFilter!=GL_LINEAR) {
      /*
       * Mipmapping: determine if we have a complete set of mipmaps
       */
      GLint i;
      GLint minLevel = t->BaseLevel;
      GLint maxLevel = MIN2(t->P, ctx->Const.MaxTextureLevels-1);
      maxLevel = MIN2(maxLevel, t->MaxLevel);

      if (minLevel > maxLevel) {
         t->Complete = GL_FALSE;
         return;
      }

      /* Test dimension-independent attributes */
      for (i = minLevel; i <= maxLevel; i++) {
         if (t->Image[i]) {
            if (!t->Image[i]->Data) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Format != t->Image[0]->Format) {
               t->Complete = GL_FALSE;
               return;
            }
            if (t->Image[i]->Border != t->Image[0]->Border) {
               t->Complete = GL_FALSE;
               return;
            }
         }
      }

      /* Test things which depend on number of texture image dimensions */
      if (t->Dimensions==1) {
         /* Test 1-D mipmaps */
         GLuint width = t->Image[0]->Width2;
         for (i=1; i<ctx->Const.MaxTextureLevels; i++) {
            if (width>1) {
               width /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (!t->Image[i]->Data) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Width2 != width ) {
                  t->Complete = GL_FALSE;
                  return;
               }
            }
            if (width==1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else if (t->Dimensions==2) {
         /* Test 2-D mipmaps */
         GLuint width = t->Image[0]->Width2;
         GLuint height = t->Image[0]->Height2;
         for (i=1; i<ctx->Const.MaxTextureLevels; i++) {
            if (width>1) {
               width /= 2;
            }
            if (height>1) {
               height /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Width2 != width) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Height2 != height) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (width==1 && height==1) {
                  return;  /* found smallest needed mipmap, all done! */
               }
            }
         }
      }
      else if (t->Dimensions==3) {
         /* Test 3-D mipmaps */
         GLuint width = t->Image[0]->Width2;
         GLuint height = t->Image[0]->Height2;
         GLuint depth = t->Image[0]->Depth2;
	 for (i=1; i<ctx->Const.MaxTextureLevels; i++) {
            if (width>1) {
               width /= 2;
            }
            if (height>1) {
               height /= 2;
            }
            if (depth>1) {
               depth /= 2;
            }
            if (i >= minLevel && i <= maxLevel) {
               if (!t->Image[i]) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Width2 != width) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Height2 != height) {
                  t->Complete = GL_FALSE;
                  return;
               }
               if (t->Image[i]->Depth2 != depth) {
                  t->Complete = GL_FALSE;
                  return;
               }
            }
            if (width==1 && height==1 && depth==1) {
               return;  /* found smallest needed mipmap, all done! */
            }
         }
      }
      else {
         /* Dimensions = ??? */
         gl_problem(NULL, "Bug in gl_test_texture_object_completeness\n");
      }
   }
}



/*
 * Execute glGenTextures
 */
void gl_GenTextures( GLcontext *ctx, GLsizei n, GLuint *texName )
{
   GLuint first;
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGenTextures");
   if (n<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glGenTextures" );
      return;
   }

   first = HashFindFreeKeyBlock(ctx->Shared->TexObjects, n);

   /* Return the texture names */
   for (i=0;i<n;i++) {
      texName[i] = first + i;
   }

   /* Allocate new, empty texture objects */
   for (i=0;i<n;i++) {
      GLuint name = first + i;
      GLuint dims = 0;
      (void) gl_alloc_texture_object(ctx->Shared, name, dims);
   }
}



/*
 * Execute glDeleteTextures
 */
void gl_DeleteTextures( GLcontext *ctx, GLsizei n, const GLuint *texName)
{
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDeleteTextures");

   for (i=0;i<n;i++) {
      struct gl_texture_object *t;
      if (texName[i]>0) {
         t = (struct gl_texture_object *)
            HashLookup(ctx->Shared->TexObjects, texName[i]);
         if (t) {
            GLuint u;
            for (u=0; u<MAX_TEXTURE_UNITS; u++) {
               struct gl_texture_unit *unit = &ctx->Texture.Unit[u];
	       GLuint d;
	       for (d = 1 ; d <= 3 ; d++) {
		  if (unit->CurrentD[d]==t) {
		     unit->CurrentD[d] = ctx->Shared->DefaultD[d][u];
		     ctx->Shared->DefaultD[d][u]->RefCount++;
		     t->RefCount--;
		     assert( t->RefCount >= 0 );
		  }
	       }
            }

            /* tell device driver to delete texture */
            if (ctx->Driver.DeleteTexture) {
               (*ctx->Driver.DeleteTexture)( ctx, t );
            }

            if (t->RefCount==0) {
               gl_free_texture_object(ctx->Shared, t);
            }
         }
      }
   }
}



/*
 * Execute glBindTexture
 */
void gl_BindTexture( GLcontext *ctx, GLenum target, GLuint texName )
{
   GLuint unit = ctx->Texture.CurrentUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
   struct gl_texture_object *oldTexObj;
   struct gl_texture_object *newTexObj;
   GLint dim;

   if (MESA_VERBOSE & (VERBOSE_API|VERBOSE_TEXTURE))
      fprintf(stderr, "glBindTexture %s %d\n",
	      gl_lookup_enum_by_nr(target), (GLint) texName);

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glBindTexture");

   dim = target - GL_TEXTURE_1D;

   if (dim < 0 || dim > 2) {
      gl_error( ctx, GL_INVALID_ENUM, "glBindTexture" );
      return;
   }

   dim++;
   oldTexObj = texUnit->CurrentD[dim];

   if (oldTexObj->Name == texName)
      return;

   if (texName == 0) 
      newTexObj = ctx->Shared->DefaultD[unit][dim];
   else {
      struct HashTable *hash = ctx->Shared->TexObjects;
      newTexObj = (struct gl_texture_object *) HashLookup(hash, texName);

      if (!newTexObj)
	 newTexObj = gl_alloc_texture_object(ctx->Shared, texName, dim);

      if (newTexObj->Dimensions != dim) {
	 if (newTexObj->Dimensions) {
	    gl_error( ctx, GL_INVALID_OPERATION, "glBindTexture" );
	    return;
	 }
	 newTexObj->Dimensions = dim;
      }
   }

   oldTexObj->RefCount--;
   newTexObj->RefCount++;
   texUnit->CurrentD[dim] = newTexObj;

   /* If we've changed the CurrentD[123] texture object then update the
    * ctx->Texture.Current pointer to point to the new texture object.
    */
   texUnit->Current = texUnit->CurrentD[texUnit->CurrentDimension];

   /* Check if we may have to use a new triangle rasterizer */
   if ((ctx->IndirectTriangles & DD_SW_RASTERIZE) &&
       (   oldTexObj->WrapS != newTexObj->WrapS
        || oldTexObj->WrapT != newTexObj->WrapT
        || oldTexObj->WrapR != newTexObj->WrapR
        || oldTexObj->MinFilter != newTexObj->MinFilter
        || oldTexObj->MagFilter != newTexObj->MagFilter
        || (oldTexObj->Image[0] && newTexObj->Image[0] && 
	   (oldTexObj->Image[0]->Format!=newTexObj->Image[0]->Format))))
   {
      ctx->NewState |= (NEW_RASTER_OPS | NEW_TEXTURING);
   }

   if (oldTexObj->Complete != newTexObj->Complete)
      ctx->NewState |= NEW_TEXTURING;

   /* Pass BindTexture call to device driver */
   if (ctx->Driver.BindTexture) {
      (*ctx->Driver.BindTexture)( ctx, target, newTexObj );
   }
}



/*
 * Execute glPrioritizeTextures
 */
void gl_PrioritizeTextures( GLcontext *ctx,
                            GLsizei n, const GLuint *texName,
                            const GLclampf *priorities )
{
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPrioritizeTextures");
   if (n<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glPrioritizeTextures" );
      return;
   }

   for (i=0;i<n;i++) {
      struct gl_texture_object *t;
      if (texName[i]>0) {
         t = (struct gl_texture_object *)
            HashLookup(ctx->Shared->TexObjects, texName[i]);
         if (t) {
            t->Priority = CLAMP( priorities[i], 0.0F, 1.0F );

	    if (ctx->Driver.PrioritizeTexture)
	       ctx->Driver.PrioritizeTexture( ctx, t, t->Priority );
         }
      }
   }
}



/*
 * Execute glAreTexturesResident 
 */
GLboolean gl_AreTexturesResident( GLcontext *ctx, GLsizei n,
                                  const GLuint *texName,
                                  GLboolean *residences )
{
   GLboolean resident = GL_TRUE;
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, 
						  "glAreTexturesResident",
						  GL_FALSE);
   if (n<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glAreTexturesResident(n)" );
      return GL_FALSE;
   }

   for (i=0;i<n;i++) {
      struct gl_texture_object *t;
      if (texName[i]==0) {
         gl_error( ctx, GL_INVALID_VALUE, "glAreTexturesResident(textures)" );
         return GL_FALSE;
      }
      t = (struct gl_texture_object *)
         HashLookup(ctx->Shared->TexObjects, texName[i]);
      if (t) {
	 if (ctx->Driver.IsTextureResident)
	    residences[i] = ctx->Driver.IsTextureResident( ctx, t );
	 else 
	    residences[i] = GL_TRUE;
      }
      else {
         gl_error( ctx, GL_INVALID_VALUE, "glAreTexturesResident(textures)" );
         return GL_FALSE;
      }
   }
   return resident;
}



/*
 * Execute glIsTexture
 */
GLboolean gl_IsTexture( GLcontext *ctx, GLuint texture )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glIsTextures",
						  GL_FALSE);
   if (texture>0 && HashLookup(ctx->Shared->TexObjects, texture)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}

