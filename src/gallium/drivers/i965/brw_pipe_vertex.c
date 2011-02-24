#include "brw_context.h"
#include "brw_defines.h"
#include "brw_structs.h"

#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_transfer.h"


static unsigned brw_translate_surface_format( unsigned id )
{
   switch (id) {
   case PIPE_FORMAT_R64_FLOAT:
      return BRW_SURFACEFORMAT_R64_FLOAT;
   case PIPE_FORMAT_R64G64_FLOAT:
      return BRW_SURFACEFORMAT_R64G64_FLOAT;
   case PIPE_FORMAT_R64G64B64_FLOAT:
      return BRW_SURFACEFORMAT_R64G64B64_FLOAT;
   case PIPE_FORMAT_R64G64B64A64_FLOAT:
      return BRW_SURFACEFORMAT_R64G64B64A64_FLOAT;

   case PIPE_FORMAT_R32_FLOAT:
      return BRW_SURFACEFORMAT_R32_FLOAT;
   case PIPE_FORMAT_R32G32_FLOAT:
      return BRW_SURFACEFORMAT_R32G32_FLOAT;
   case PIPE_FORMAT_R32G32B32_FLOAT:
      return BRW_SURFACEFORMAT_R32G32B32_FLOAT;
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      return BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   case PIPE_FORMAT_R32_UNORM:
      return BRW_SURFACEFORMAT_R32_UNORM;
   case PIPE_FORMAT_R32G32_UNORM:
      return BRW_SURFACEFORMAT_R32G32_UNORM;
   case PIPE_FORMAT_R32G32B32_UNORM:
      return BRW_SURFACEFORMAT_R32G32B32_UNORM;
   case PIPE_FORMAT_R32G32B32A32_UNORM:
      return BRW_SURFACEFORMAT_R32G32B32A32_UNORM;

   case PIPE_FORMAT_R32_USCALED:
      return BRW_SURFACEFORMAT_R32_USCALED;
   case PIPE_FORMAT_R32G32_USCALED:
      return BRW_SURFACEFORMAT_R32G32_USCALED;
   case PIPE_FORMAT_R32G32B32_USCALED:
      return BRW_SURFACEFORMAT_R32G32B32_USCALED;
   case PIPE_FORMAT_R32G32B32A32_USCALED:
      return BRW_SURFACEFORMAT_R32G32B32A32_USCALED;

   case PIPE_FORMAT_R32_SNORM:
      return BRW_SURFACEFORMAT_R32_SNORM;
   case PIPE_FORMAT_R32G32_SNORM:
      return BRW_SURFACEFORMAT_R32G32_SNORM;
   case PIPE_FORMAT_R32G32B32_SNORM:
      return BRW_SURFACEFORMAT_R32G32B32_SNORM;
   case PIPE_FORMAT_R32G32B32A32_SNORM:
      return BRW_SURFACEFORMAT_R32G32B32A32_SNORM;

   case PIPE_FORMAT_R32_SSCALED:
      return BRW_SURFACEFORMAT_R32_SSCALED;
   case PIPE_FORMAT_R32G32_SSCALED:
      return BRW_SURFACEFORMAT_R32G32_SSCALED;
   case PIPE_FORMAT_R32G32B32_SSCALED:
      return BRW_SURFACEFORMAT_R32G32B32_SSCALED;
   case PIPE_FORMAT_R32G32B32A32_SSCALED:
      return BRW_SURFACEFORMAT_R32G32B32A32_SSCALED;

   case PIPE_FORMAT_R16_UNORM:
      return BRW_SURFACEFORMAT_R16_UNORM;
   case PIPE_FORMAT_R16G16_UNORM:
      return BRW_SURFACEFORMAT_R16G16_UNORM;
   case PIPE_FORMAT_R16G16B16_UNORM:
      return BRW_SURFACEFORMAT_R16G16B16_UNORM;
   case PIPE_FORMAT_R16G16B16A16_UNORM:
      return BRW_SURFACEFORMAT_R16G16B16A16_UNORM;

   case PIPE_FORMAT_R16_USCALED:
      return BRW_SURFACEFORMAT_R16_USCALED;
   case PIPE_FORMAT_R16G16_USCALED:
      return BRW_SURFACEFORMAT_R16G16_USCALED;
   case PIPE_FORMAT_R16G16B16_USCALED:
      return BRW_SURFACEFORMAT_R16G16B16_USCALED;
   case PIPE_FORMAT_R16G16B16A16_USCALED:
      return BRW_SURFACEFORMAT_R16G16B16A16_USCALED;

   case PIPE_FORMAT_R16_SNORM:
      return BRW_SURFACEFORMAT_R16_SNORM;
   case PIPE_FORMAT_R16G16_SNORM:
      return BRW_SURFACEFORMAT_R16G16_SNORM;
   case PIPE_FORMAT_R16G16B16_SNORM:
      return BRW_SURFACEFORMAT_R16G16B16_SNORM;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      return BRW_SURFACEFORMAT_R16G16B16A16_SNORM;

   case PIPE_FORMAT_R16_SSCALED:
      return BRW_SURFACEFORMAT_R16_SSCALED;
   case PIPE_FORMAT_R16G16_SSCALED:
      return BRW_SURFACEFORMAT_R16G16_SSCALED;
   case PIPE_FORMAT_R16G16B16_SSCALED:
      return BRW_SURFACEFORMAT_R16G16B16_SSCALED;
   case PIPE_FORMAT_R16G16B16A16_SSCALED:
      return BRW_SURFACEFORMAT_R16G16B16A16_SSCALED;

   case PIPE_FORMAT_R8_UNORM:
      return BRW_SURFACEFORMAT_R8_UNORM;
   case PIPE_FORMAT_R8G8_UNORM:
      return BRW_SURFACEFORMAT_R8G8_UNORM;
   case PIPE_FORMAT_R8G8B8_UNORM:
      return BRW_SURFACEFORMAT_R8G8B8_UNORM;
   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return BRW_SURFACEFORMAT_R8G8B8A8_UNORM;

   case PIPE_FORMAT_R8_USCALED:
      return BRW_SURFACEFORMAT_R8_USCALED;
   case PIPE_FORMAT_R8G8_USCALED:
      return BRW_SURFACEFORMAT_R8G8_USCALED;
   case PIPE_FORMAT_R8G8B8_USCALED:
      return BRW_SURFACEFORMAT_R8G8B8_USCALED;
   case PIPE_FORMAT_R8G8B8A8_USCALED:
      return BRW_SURFACEFORMAT_R8G8B8A8_USCALED;

   case PIPE_FORMAT_R8_SNORM:
      return BRW_SURFACEFORMAT_R8_SNORM;
   case PIPE_FORMAT_R8G8_SNORM:
      return BRW_SURFACEFORMAT_R8G8_SNORM;
   case PIPE_FORMAT_R8G8B8_SNORM:
      return BRW_SURFACEFORMAT_R8G8B8_SNORM;
   case PIPE_FORMAT_R8G8B8A8_SNORM:
      return BRW_SURFACEFORMAT_R8G8B8A8_SNORM;

   case PIPE_FORMAT_R8_SSCALED:
      return BRW_SURFACEFORMAT_R8_SSCALED;
   case PIPE_FORMAT_R8G8_SSCALED:
      return BRW_SURFACEFORMAT_R8G8_SSCALED;
   case PIPE_FORMAT_R8G8B8_SSCALED:
      return BRW_SURFACEFORMAT_R8G8B8_SSCALED;
   case PIPE_FORMAT_R8G8B8A8_SSCALED:
      return BRW_SURFACEFORMAT_R8G8B8A8_SSCALED;

   default:
      assert(0);
      return 0;
   }
}

static void brw_translate_vertex_elements(struct brw_context *brw,
                                          struct brw_vertex_element_packet *brw_velems,
                                          const struct pipe_vertex_element *attribs,
                                          unsigned count)
{
   unsigned i;

   /* If the VS doesn't read any inputs (calculating vertex position from
    * a state variable for some reason, for example), emit a single pad
    * VERTEX_ELEMENT struct and bail.
    *
    * The stale VB state stays in place, but they don't do anything unless
    * a VE loads from them.
    */
   brw_velems->header.opcode = CMD_VERTEX_ELEMENT;

   if (count == 0) {
      brw_velems->header.length = 1;
      brw_velems->ve[0].ve0.src_offset = 0;
      brw_velems->ve[0].ve0.src_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;
      brw_velems->ve[0].ve0.valid = 1;
      brw_velems->ve[0].ve0.vertex_buffer_index = 0;
      brw_velems->ve[0].ve1.dst_offset = 0;
      brw_velems->ve[0].ve1.vfcomponent0 = BRW_VE1_COMPONENT_STORE_0;
      brw_velems->ve[0].ve1.vfcomponent1 = BRW_VE1_COMPONENT_STORE_0;
      brw_velems->ve[0].ve1.vfcomponent2 = BRW_VE1_COMPONENT_STORE_0;
      brw_velems->ve[0].ve1.vfcomponent3 = BRW_VE1_COMPONENT_STORE_1_FLT;
      return;
   }


   /* Now emit vertex element (VEP) state packets.
    *
    */
   brw_velems->header.length = (1 + count * 2) - 2;
   for (i = 0; i < count; i++) {
      const struct pipe_vertex_element *input = &attribs[i];
      unsigned nr_components = util_format_get_nr_components(input->src_format);

      uint32_t format = brw_translate_surface_format( input->src_format );
      uint32_t comp0 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp1 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp2 = BRW_VE1_COMPONENT_STORE_SRC;
      uint32_t comp3 = BRW_VE1_COMPONENT_STORE_SRC;

      switch (nr_components) {
      case 0: comp0 = BRW_VE1_COMPONENT_STORE_0; /* fallthrough */
      case 1: comp1 = BRW_VE1_COMPONENT_STORE_0; /* fallthrough */
      case 2: comp2 = BRW_VE1_COMPONENT_STORE_0; /* fallthrough */
      case 3: comp3 = BRW_VE1_COMPONENT_STORE_1_FLT;
         break;
      }

      brw_velems->ve[i].ve0.src_offset = input->src_offset;
      brw_velems->ve[i].ve0.src_format = format;
      brw_velems->ve[i].ve0.valid = 1;
      brw_velems->ve[i].ve0.vertex_buffer_index = input->vertex_buffer_index;
      brw_velems->ve[i].ve1.vfcomponent0 = comp0;
      brw_velems->ve[i].ve1.vfcomponent1 = comp1;
      brw_velems->ve[i].ve1.vfcomponent2 = comp2;
      brw_velems->ve[i].ve1.vfcomponent3 = comp3;

      if (brw->gen == 5)
         brw_velems->ve[i].ve1.dst_offset = 0;
      else
         brw_velems->ve[i].ve1.dst_offset = i * 4;
   }
}

static void* brw_create_vertex_elements_state( struct pipe_context *pipe,
                                               unsigned count,
                                               const struct pipe_vertex_element *attribs )
{
   /* note: for the brw_swtnl.c code (if ever we need draw fallback) we'd also need
      to store the original data */
   struct brw_context *brw = brw_context(pipe);
   struct brw_vertex_element_packet *velems;
   assert(count <= BRW_VEP_MAX);
   velems = (struct brw_vertex_element_packet *) MALLOC(sizeof(struct brw_vertex_element_packet));
   if (velems) {
      brw_translate_vertex_elements(brw, velems, attribs, count);
   }
   return velems;
}

static void brw_bind_vertex_elements_state(struct pipe_context *pipe,
                                           void *velems)
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_vertex_element_packet *brw_velems = (struct brw_vertex_element_packet *) velems;

   brw->curr.velems = brw_velems;

   brw->state.dirty.mesa |= PIPE_NEW_VERTEX_ELEMENT;
}

static void brw_delete_vertex_elements_state(struct pipe_context *pipe, void *velems)
{
   FREE( velems );
}


static void brw_set_vertex_buffers(struct pipe_context *pipe,
                                   unsigned count,
                                   const struct pipe_vertex_buffer *buffers)
{
   struct brw_context *brw = brw_context(pipe);

   /* Check for no change */
   if (count == brw->curr.num_vertex_buffers &&
       memcmp(brw->curr.vertex_buffer,
              buffers,
              count * sizeof buffers[0]) == 0)
      return;

   util_copy_vertex_buffers(brw->curr.vertex_buffer,
                            &brw->curr.num_vertex_buffers,
                            buffers, count);

   brw->state.dirty.mesa |= PIPE_NEW_VERTEX_BUFFER;
}


static void brw_set_index_buffer(struct pipe_context *pipe,
                                 const struct pipe_index_buffer *ib)
{
   struct brw_context *brw = brw_context(pipe);

   if (ib) {
      if (brw->curr.index_buffer == ib->buffer &&
          brw->curr.index_offset == ib->offset &&
          brw->curr.index_size == ib->index_size)
         return;

      pipe_resource_reference(&brw->curr.index_buffer, ib->buffer);
      brw->curr.index_offset = ib->offset;
      brw->curr.index_size = ib->index_size;
   }
   else {
      if (!brw->curr.index_buffer &&
          !brw->curr.index_offset &&
          !brw->curr.index_size)
         return;

      pipe_resource_reference(&brw->curr.index_buffer, NULL);
      brw->curr.index_offset = 0;
      brw->curr.index_size = 0;
   }

   brw->state.dirty.mesa |= PIPE_NEW_INDEX_BUFFER;
}


void 
brw_pipe_vertex_init( struct brw_context *brw )
{
   brw->base.set_vertex_buffers = brw_set_vertex_buffers;
   brw->base.set_index_buffer = brw_set_index_buffer;
   brw->base.create_vertex_elements_state = brw_create_vertex_elements_state;
   brw->base.bind_vertex_elements_state = brw_bind_vertex_elements_state;
   brw->base.delete_vertex_elements_state = brw_delete_vertex_elements_state;
   brw->base.redefine_user_buffer = u_default_redefine_user_buffer;
}


void 
brw_pipe_vertex_cleanup( struct brw_context *brw )
{
   unsigned i;

   /* Release bound pipe vertex_buffers
    */
   for (i = 0; i < brw->curr.num_vertex_buffers; i++) {
      pipe_resource_reference(&brw->curr.vertex_buffer[i].buffer, NULL);
   }

   /* Release some other stuff
    */
#if 0
   for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
      bo_reference(&brw->vb.inputs[i].bo, NULL);
      brw->vb.inputs[i].bo = NULL;
   }
#endif
}
