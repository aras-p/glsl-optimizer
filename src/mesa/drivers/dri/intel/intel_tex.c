#include "swrast/swrast.h"
#include "texobj.h"
#include "teximage.h"
#include "mipmap.h"
#include "intel_context.h"
#include "intel_mipmap_tree.h"
#include "intel_tex.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static GLboolean
intelIsTextureResident(GLcontext * ctx, struct gl_texture_object *texObj)
{
#if 0
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   return
      intelObj->mt &&
      intelObj->mt->region &&
      intel_is_region_resident(intel, intelObj->mt->region);
#endif
   return 1;
}



static struct gl_texture_image *
intelNewTextureImage(GLcontext * ctx)
{
   DBG("%s\n", __FUNCTION__);
   (void) ctx;
   return (struct gl_texture_image *) CALLOC_STRUCT(intel_texture_image);
}


static struct gl_texture_object *
intelNewTextureObject(GLcontext * ctx, GLuint name, GLenum target)
{
   struct intel_texture_object *obj = CALLOC_STRUCT(intel_texture_object);

   DBG("%s\n", __FUNCTION__);
   _mesa_initialize_texture_object(&obj->base, name, target);

   return &obj->base;
}

static void 
intelDeleteTextureObject(GLcontext *ctx,
			 struct gl_texture_object *texObj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   if (intelObj->mt)
      intel_miptree_release(intel, &intelObj->mt);

   _mesa_delete_texture_object(ctx, texObj);
}


static void
intelFreeTextureImageData(GLcontext * ctx, struct gl_texture_image *texImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   DBG("%s\n", __FUNCTION__);

   if (intelImage->mt) {
      intel_miptree_release(intel, &intelImage->mt);
   }

   if (texImage->Data) {
      _mesa_free_texmemory(texImage->Data);
      texImage->Data = NULL;
   }
}


/* The system memcpy (at least on ubuntu 5.10) has problems copying
 * to agp (writecombined) memory from a source which isn't 64-byte
 * aligned - there is a 4x performance falloff.
 *
 * The x86 __memcpy is immune to this but is slightly slower
 * (10%-ish) than the system memcpy.
 *
 * The sse_memcpy seems to have a slight cliff at 64/32 bytes, but
 * isn't much faster than x86_memcpy for agp copies.
 * 
 * TODO: switch dynamically.
 */
static void *
do_memcpy(void *dest, const void *src, size_t n)
{
   if ((((unsigned) src) & 63) || (((unsigned) dest) & 63)) {
      return __memcpy(dest, src, n);
   }
   else
      return memcpy(dest, src, n);
}


#if DO_DEBUG && !defined(__ia64__)

#ifndef __x86_64__
static unsigned
fastrdtsc(void)
{
   unsigned eax;
   __asm__ volatile ("\t"
                     "pushl  %%ebx\n\t"
                     "cpuid\n\t" ".byte 0x0f, 0x31\n\t"
                     "popl %%ebx\n":"=a" (eax)
                     :"0"(0)
                     :"ecx", "edx", "cc");

   return eax;
}
#else
static unsigned
fastrdtsc(void)
{
   unsigned eax;
   __asm__ volatile ("\t" "cpuid\n\t" ".byte 0x0f, 0x31\n\t":"=a" (eax)
                     :"0"(0)
                     :"ecx", "edx", "ebx", "cc");

   return eax;
}
#endif

static unsigned
time_diff(unsigned t, unsigned t2)
{
   return ((t < t2) ? t2 - t : 0xFFFFFFFFU - (t - t2 - 1));
}


static void *
timed_memcpy(void *dest, const void *src, size_t n)
{
   void *ret;
   unsigned t1, t2;
   double rate;

   if ((((unsigned) src) & 63) || (((unsigned) dest) & 63))
      _mesa_printf("Warning - non-aligned texture copy!\n");

   t1 = fastrdtsc();
   ret = do_memcpy(dest, src, n);
   t2 = fastrdtsc();

   rate = time_diff(t1, t2);
   rate /= (double) n;
   _mesa_printf("timed_memcpy: %u %u --> %f clocks/byte\n", t1, t2, rate);
   return ret;
}
#endif /* DO_DEBUG */

/**
 * Generate new mipmap data from BASE+1 to BASE+p (the minimally-sized mipmap
 * level).
 *
 * The texture object's miptree must be mapped.
 *
 * It would be really nice if this was just called by Mesa whenever mipmaps
 * needed to be regenerated, rather than us having to remember to do so in
 * each texture image modification path.
 *
 * This function should also include an accelerated path.
 */
void
intel_generate_mipmap(GLcontext *ctx, GLenum target,
                      struct gl_texture_object *texObj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);
   GLuint nr_faces = (intelObj->base.Target == GL_TEXTURE_CUBE_MAP) ? 6 : 1;
   int face, i;

   _mesa_generate_mipmap(ctx, target, texObj);

   /* Update the level information in our private data in the new images, since
    * it didn't get set as part of a normal TexImage path.
    */
   for (face = 0; face < nr_faces; face++) {
      for (i = texObj->BaseLevel + 1; i < texObj->MaxLevel; i++) {
         struct intel_texture_image *intelImage;

	 intelImage = intel_texture_image(texObj->Image[face][i]);
	 if (intelImage == NULL)
	    break;

	 intelImage->level = i;
	 intelImage->face = face;
	 /* Unreference the miptree to signal that the new Data is a bare
	  * pointer from mesa.
	  */
	 intel_miptree_release(intel, &intelImage->mt);
      }
   }
}

static void intelGenerateMipmap(GLcontext *ctx, GLenum target, struct gl_texture_object *texObj)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_object *intelObj = intel_texture_object(texObj);

   intel_tex_map_level_images(intel, intelObj, texObj->BaseLevel);
   intel_generate_mipmap(ctx, target, texObj);
   intel_tex_unmap_level_images(intel, intelObj, texObj->BaseLevel);
}

void
intelInitTextureFuncs(struct dd_function_table *functions)
{
   functions->ChooseTextureFormat = intelChooseTextureFormat;
   functions->TexImage1D = intelTexImage1D;
   functions->TexImage2D = intelTexImage2D;
   functions->TexImage3D = intelTexImage3D;
   functions->TexSubImage1D = intelTexSubImage1D;
   functions->TexSubImage2D = intelTexSubImage2D;
   functions->TexSubImage3D = intelTexSubImage3D;
#ifdef I915
   functions->CopyTexImage1D = intelCopyTexImage1D;
   functions->CopyTexImage2D = intelCopyTexImage2D;
   functions->CopyTexSubImage1D = intelCopyTexSubImage1D;
   functions->CopyTexSubImage2D = intelCopyTexSubImage2D;
#else
   functions->CopyTexImage1D = _swrast_copy_teximage1d;
   functions->CopyTexImage2D = _swrast_copy_teximage2d;
   functions->CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   functions->CopyTexSubImage2D = _swrast_copy_texsubimage2d;
#endif
   functions->GetTexImage = intelGetTexImage;
   functions->GenerateMipmap = intelGenerateMipmap;

   /* compressed texture functions */
   functions->CompressedTexImage2D = intelCompressedTexImage2D;
   functions->GetCompressedTexImage = intelGetCompressedTexImage;

   functions->NewTextureObject = intelNewTextureObject;
   functions->NewTextureImage = intelNewTextureImage;
   functions->DeleteTexture = intelDeleteTextureObject;
   functions->FreeTexImageData = intelFreeTextureImageData;
   functions->UpdateTexturePalette = 0;
   functions->IsTextureResident = intelIsTextureResident;

#if DO_DEBUG && !defined(__ia64__)
   if (INTEL_DEBUG & DEBUG_BUFMGR)
      functions->TextureMemCpy = timed_memcpy;
   else
#endif
      functions->TextureMemCpy = do_memcpy;
}
