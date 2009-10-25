
#include "util/u_memory.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_debug.h"



static void *brw_create_sampler_state( struct pipe_context *pipe,
				     const struct pipe_sampler_state *templ )
{
   struct brw_sampler_state *sampler = CALLOC_STRUCT(brw_sampler_state);


   return (void *)sampler;
}

static void brw_bind_sampler_state(struct pipe_context *pipe,
				 void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   brw->curr.sampler = (const struct brw_sampler_state *)cso;
   brw->state.dirty.mesa |= PIPE_NEW_SAMPLER;
}

static void brw_delete_sampler_state(struct pipe_context *pipe,
				  void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   FREE(cso);
}

static void brw_set_sampler_textures(struct pipe_context *pipe,
				     unsigned num_textures,
				     struct pipe_texture **tex)
{
   struct brw_context *brw = brw_context(pipe);

   brw->state.dirty.mesa |= PIPE_NEW_BOUND_TEXTURES;
}


void brw_sampler_init( struct brw_context *brw )
{
   brw->base.set_sampler_textures = brw_set_sampler_textures;
   brw->base.create_sampler_state = brw_create_sampler_state;
   brw->base.bind_sampler_state = brw_bind_sampler_state;
   brw->base.destroy_sampler_state = brw_destroy_sampler_state;
}
