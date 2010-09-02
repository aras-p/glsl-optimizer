
#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "tgsi/tgsi_text.h"
#include "util/u_memory.h"
#include "state_tracker/graw.h"


/* Helper functions.  These are the same for all graw implementations.
 */
void *graw_parse_geometry_shader(struct pipe_context *pipe,
                                 const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_gs_state(pipe, &state);
}

void *graw_parse_vertex_shader(struct pipe_context *pipe,
                               const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_vs_state(pipe, &state);
}

void *graw_parse_fragment_shader(struct pipe_context *pipe,
                                 const char *text)
{
   struct tgsi_token tokens[1024];
   struct pipe_shader_state state;

   if (!tgsi_text_translate(text, tokens, Elements(tokens)))
      return NULL;

   state.tokens = tokens;
   return pipe->create_fs_state(pipe, &state);
}

