
/**
 * This is called when we need to set up GL rendering to a new X window.
 */
static boolean
dri_create_buffer(__DRIscreenPrivate *sPriv,
                  __DRIdrawablePrivate *dPriv,
                  const __GLcontextModes *visual,
                  boolean isPixmap)
{
   enum pipe_format colorFormat, depthFormat, stencilFormat;
   struct dri_drawable *drawable;

   if (isPixmap) 
      goto fail;          /* not implemented */

   drawable = CALLOC_STRUCT(dri_drawable);
   if (drawable == NULL)
      goto fail;

   /* XXX: todo: use the pipe_screen queries to figure out which
    * render targets are supportable.
    */
   if (visual->redBits == 5)
      colorFormat = PIPE_FORMAT_R5G6B5_UNORM;
   else
      colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

   if (visual->depthBits == 16)
      depthFormat = PIPE_FORMAT_Z16_UNORM;
   else if (visual->depthBits == 24) {
      if (visual->stencilBits == 8)
         depthFormat = PIPE_FORMAT_S8Z24_UNORM;
      else
         depthFormat = PIPE_FORMAT_X8Z24_UNORM;
   }

   drawable->stfb = st_create_framebuffer(visual,
                                    colorFormat,
                                    depthFormat,
                                    dPriv->w,
                                    dPriv->h,
                                    (void*) drawable);
   if (drawable->stfb == NULL)
      goto fail;

   dPriv->driverPrivate = (void *) drawable;
   return GL_TRUE;

fail:
   FREE(drawable);
   return GL_FALSE;
}

static void
dri_destroy_buffer(__DRIdrawablePrivate *dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   assert(drawable->stfb);
   st_unreference_framebuffer(drawable->stfb);
   FREE(drawable);
}

