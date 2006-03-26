
#include "context.h"
#include "fbobject.h"
#include "texrender.h"
#include "renderbuffer.h"


/*
 * Render-to-texture code for GL_EXT_framebuffer_object
 */


/**
 * Derived from gl_renderbuffer class
 */
struct texture_renderbuffer
{
   struct gl_renderbuffer Base;   /* Base class object */
   struct gl_texture_image *TexImage;
   StoreTexelFunc Store;
   GLint Zoffset;
};



static void
texture_get_row(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, void *values)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLchan *rgbaOut = (GLchan *) values;
   GLuint i;
   for (i = 0; i < count; i++) {
      trb->TexImage->FetchTexelc(trb->TexImage, x + i, y, z, rgbaOut + 4 * i);
   }
}

static void
texture_get_values(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], void *values)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   GLchan *rgbaOut = (GLchan *) values;
   GLuint i;
   for (i = 0; i < count; i++) {
      trb->TexImage->FetchTexelc(trb->TexImage, x[i], y[i], z,
                                 rgbaOut + 4 * i);
   }
}

static void
texture_put_row(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                GLint x, GLint y, const void *values, const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   const GLchan *rgba = (const GLchan *) values;
   GLuint i;
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         trb->Store(trb->TexImage, x + i, y, z, rgba);
      }
      rgba += 4;
   }
}

static void
texture_put_mono_row(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                     GLint x, GLint y, const void *value, const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   const GLchan *rgba = (const GLchan *) value;
   GLuint i;
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         trb->Store(trb->TexImage, x + i, y, z, rgba);
      }
   }
}

static void
texture_put_values(GLcontext *ctx, struct gl_renderbuffer *rb, GLuint count,
                   const GLint x[], const GLint y[], const void *values,
                   const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   const GLchan *rgba = (const GLchan *) values;
   GLuint i;
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         trb->Store(trb->TexImage, x[i], y[i], z, rgba);
      }
      rgba += 4;
   }
}

static void
texture_put_mono_values(GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint count, const GLint x[], const GLint y[],
                        const void *value, const GLubyte *mask)
{
   const struct texture_renderbuffer *trb
      = (const struct texture_renderbuffer *) rb;
   const GLint z = trb->Zoffset;
   const GLchan *rgba = (const GLchan *) value;
   GLuint i;
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
         trb->Store(trb->TexImage, x[i], y[i], z, rgba);
      }
   }
}


static void
delete_texture_wrapper(struct gl_renderbuffer *rb)
{
   ASSERT(rb->RefCount == 0);
   _mesa_free(rb);
}


/**
 * If a render buffer attachment specifies a texture image, we'll use
 * this function to make a gl_renderbuffer wrapper around the texture image.
 * This allows other parts of Mesa to access the texture image as if it
 * was a renderbuffer.
 */
static void
wrap_texture(GLcontext *ctx, struct gl_renderbuffer_attachment *att)
{
   struct texture_renderbuffer *trb;
   const GLuint name = 0;

   ASSERT(att->Type == GL_TEXTURE);
   ASSERT(att->Renderbuffer == NULL);
   /*
   ASSERT(att->Complete);
   */

   trb = CALLOC_STRUCT(texture_renderbuffer);
   if (!trb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "wrap_texture");
      return;
   }

   _mesa_init_renderbuffer(&trb->Base, name);

   trb->TexImage = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
   assert(trb->TexImage);

   trb->Store = trb->TexImage->TexFormat->StoreTexel;
   assert(trb->Store);

   trb->Zoffset = att->Zoffset;

   trb->Base.Width = trb->TexImage->Width;
   trb->Base.Height = trb->TexImage->Height;
   trb->Base.InternalFormat = trb->TexImage->InternalFormat; /* XXX fix? */
   trb->Base._BaseFormat = trb->TexImage->TexFormat->BaseFormat;
#if 0
   /* fix/avoid this assertion someday */
   assert(trb->Base._BaseFormat == GL_RGB ||
          trb->Base._BaseFormat == GL_RGBA ||
          trb->Base._BaseFormat == GL_DEPTH_COMPONENT);
#endif
   trb->Base.DataType = GL_UNSIGNED_BYTE;  /* XXX fix! */
   trb->Base.Data = trb->TexImage->Data;

   trb->Base.GetRow = texture_get_row;
   trb->Base.GetValues = texture_get_values;
   trb->Base.PutRow = texture_put_row;
   trb->Base.PutMonoRow = texture_put_mono_row;
   trb->Base.PutValues = texture_put_values;
   trb->Base.PutMonoValues = texture_put_mono_values;

   trb->Base.Delete = delete_texture_wrapper;
   trb->Base.AllocStorage = NULL; /* illegal! */

   /* XXX fix these */
   trb->Base.RedBits = trb->TexImage->TexFormat->RedBits;
   trb->Base.GreenBits = trb->TexImage->TexFormat->GreenBits;
   trb->Base.BlueBits = trb->TexImage->TexFormat->BlueBits;
   trb->Base.AlphaBits = trb->TexImage->TexFormat->AlphaBits;
   trb->Base.DepthBits = trb->TexImage->TexFormat->DepthBits;

   att->Renderbuffer = &(trb->Base);
   trb->Base.RefCount++;
}



/**
 * Called when rendering to a texture image begins, or when changing
 * the dest mipmap level, cube face, etc.
 * This is a fallback routine for software render-to-texture.
 *
 * Called via the glRenderbufferTexture1D/2D/3D() functions
 * and elsewhere (such as glTexImage2D).
 *
 * The image we're rendering into is
 * att->Texture->Image[att->CubeMapFace][att->TextureLevel];
 * It'll never be NULL.
 *
 * \param fb  the framebuffer object the texture is being bound to
 * \param att  the fb attachment point of the texture
 *
 * \sa _mesa_framebuffer_renderbuffer
 */
void
_mesa_render_texture(GLcontext *ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{
   struct gl_texture_image *newImage
      = att->Texture->Image[att->CubeMapFace][att->TextureLevel];
   struct texture_renderbuffer *trb
      = (struct texture_renderbuffer *) att->Renderbuffer;
   struct gl_texture_image *oldImage = trb ? trb->TexImage : NULL;

   (void) fb;

   ASSERT(newImage);

   if (oldImage != newImage) {
      if (trb) {
         /* get rid of old wrapper */
         /* XXX also if Zoffset changes? */
         trb->Base.Delete(&trb->Base);
      }
      wrap_texture(ctx, att);
   }
}


void
_mesa_finish_render_texture(GLcontext *ctx,
                            struct gl_renderbuffer_attachment *att)
{
   /* do nothing */
   /* The renderbuffer texture wrapper will get deleted by the
    * normal mechanism for deleting renderbuffers.
    */
}
