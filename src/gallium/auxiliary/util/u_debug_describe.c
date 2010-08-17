#include <pipe/p_state.h>
#include <util/u_format.h>
#include <util/u_debug_describe.h>
#include <util/u_string.h>

void
debug_describe_reference(char* buf, const struct pipe_reference*ptr)
{
   strcpy(buf, "pipe_object");
}

void
debug_describe_resource(char* buf, const struct pipe_resource *ptr)
{
   if(ptr->target == PIPE_BUFFER)
      util_sprintf(buf, "pipe_buffer<%u>", util_format_get_stride(ptr->format, ptr->width0));
   else if(ptr->target == PIPE_TEXTURE_1D)
      util_sprintf(buf, "pipe_texture1d<%u,%s,%u>", ptr->width0, util_format_short_name(ptr->format), ptr->last_level);
   else if(ptr->target == PIPE_TEXTURE_2D)
      util_sprintf(buf, "pipe_texture2d<%u,%u,%s,%u>", ptr->width0, ptr->height0, util_format_short_name(ptr->format), ptr->last_level);
   else if(ptr->target == PIPE_TEXTURE_CUBE)
      util_sprintf(buf, "pipe_texture_cube<%u,%u,%s,%u>", ptr->width0, ptr->height0, util_format_short_name(ptr->format), ptr->last_level);
   else if(ptr->target == PIPE_TEXTURE_3D)
      util_sprintf(buf, "pipe_texture3d<%u,%u,%u,%s,%u>", ptr->width0, ptr->height0, ptr->depth0, util_format_short_name(ptr->format), ptr->last_level);
   else
      util_sprintf(buf, "pipe_martian_resource<%u>", ptr->target);
}

void
debug_describe_surface(char* buf, const struct pipe_surface *ptr)
{
   char res[128];
   debug_describe_resource(res, ptr->texture);
   util_sprintf(buf, "pipe_surface<%s,%u,%u,%u>", res, ptr->face, ptr->level, ptr->zslice);
}

void
debug_describe_sampler_view(char* buf, const struct pipe_sampler_view *ptr)
{
   char res[128];
   debug_describe_resource(res, ptr->texture);
   util_sprintf(buf, "pipe_sampler_view<%s,%s>", res, util_format_short_name(ptr->format));
}
