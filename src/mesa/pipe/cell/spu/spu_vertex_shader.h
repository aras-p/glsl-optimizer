#ifndef SPU_VERTEX_SHADER_H
#define SPU_VERTEX_SHADER_H

#include "pipe/p_format.h"
#include "spu_exec.h"

struct spu_vs_context;

typedef void (*spu_fetch_func)(qword *out, const qword *in);
typedef void (*spu_full_fetch_func)( struct spu_vs_context *draw,
				     struct spu_exec_machine *machine,
				     const unsigned *elts,
				     unsigned count );

struct spu_vs_context {
   struct pipe_viewport_state viewport;

   struct {
      uint64_t src_ptr[PIPE_ATTRIB_MAX];
      unsigned pitch[PIPE_ATTRIB_MAX];
      unsigned size[PIPE_ATTRIB_MAX];
      enum pipe_format format[PIPE_ATTRIB_MAX];
      unsigned nr_attrs;
      boolean dirty;

      spu_fetch_func fetch[PIPE_ATTRIB_MAX];
      spu_full_fetch_func fetch_func;
   } vertex_fetch;
   
   /* Clip derived state:
    */
   float plane[12][4];
   unsigned nr_planes;

   struct spu_exec_machine machine;
   const float (*constants)[4];

   unsigned num_vs_outputs;
};

extern void spu_update_vertex_fetch(struct spu_vs_context *draw);

static INLINE void spu_vertex_fetch(struct spu_vs_context *draw,
				    struct spu_exec_machine *machine,
				    const unsigned *elts,
				    unsigned count)
{
   if (draw->vertex_fetch.dirty) {
      spu_update_vertex_fetch(draw);
      draw->vertex_fetch.dirty = 0;
   }
   
   (*draw->vertex_fetch.fetch_func)(draw, machine, elts, count);
}

struct cell_command_vs;

extern void
spu_execute_vertex_shader(struct spu_vs_context *draw,
			  const struct cell_command_vs *vs);

#endif /* SPU_VERTEX_SHADER_H */
