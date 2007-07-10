
/**
 * \brief Quad stencil testing
 */


#include "main/glheader.h"
#include "main/imports.h"
#include "sp_context.h"
#include "sp_headers.h"
#include "sp_surface.h"
#include "sp_quad.h"
#include "pipe/p_defines.h"


static void
stencil_test_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   struct softpipe_surface *sps = softpipe_surface(softpipe->framebuffer.zbuf);
   GLuint bzzzz[QUAD_SIZE];  /**< Z values fetched from depth buffer */
   GLuint qzzzz[QUAD_SIZE];  /**< Z values from the quad */
   GLuint zmask = 0;
   GLuint j;
   GLfloat scale;

   assert(sps); /* shouldn't get here if there's no zbuffer */

   /*
    * To increase efficiency, we should probably have multiple versions
    * of this function that are specifically for Z16, Z32 and FP Z buffers.
    * Try to effectively do that with codegen...
    */
   if (sps->surface.format == PIPE_FORMAT_U_Z16)
      scale = 65535.0;
   else
      assert(0);  /* XXX fix this someday */

   /*
    * Convert quad's float depth values to int depth values.
    * If the Z buffer stores integer values, we _have_ to do the depth
    * compares with integers (not floats).  Otherwise, the float->int->float
    * conversion of Z values (which isn't an identity function) will cause
    * Z-fighting errors.
    */
   for (j = 0; j < QUAD_SIZE; j++) {
      qzzzz[j] = (GLuint) (quad->outputs.depth[j] * scale);
   }

   /* get zquad from zbuffer */
   sps->read_quad_z(sps, quad->x0, quad->y0, bzzzz);

   switch (softpipe->depth_test.func) {
   case PIPE_FUNC_NEVER:
      /* zmask = 0 */
      break;
   case PIPE_FUNC_LESS:
      /* Note this is pretty much a single sse or cell instruction.  
       * Like this:  quad->mask &= (quad->outputs.depth < zzzz);
       */
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] < bzzzz[j]) 
	    zmask |= 1 << j;
      }
      break;
   case PIPE_FUNC_EQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] == bzzzz[j]) 
	    zmask |= 1 << j;
      }
      break;
   case PIPE_FUNC_LEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] <= bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_GREATER:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] > bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_NOTEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] != bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_GEQUAL:
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (qzzzz[j] >= bzzzz[j]) 
	    zmask |= (1 << j);
      }
      break;
   case PIPE_FUNC_ALWAYS:
      zmask = MASK_ALL;
      break;
   default:
      abort();
   }

   quad->mask &= zmask;

   if (softpipe->depth_test.writemask) {
      
      /* This is also efficient with sse / spe instructions: 
       */
      for (j = 0; j < QUAD_SIZE; j++) {
	 if (quad->mask & (1 << j)) {
	    bzzzz[j] = qzzzz[j];
	 }
      }

      /* write updated zquad to zbuffer */
      sps->write_quad_z(sps, quad->x0, quad->y0, bzzzz);
   }

   if (quad->mask)
      qs->next->run(qs->next, quad);
}




struct quad_stage *sp_quad_stencil_test_stage( struct softpipe_context *softpipe )
{
   struct quad_stage *stage = CALLOC_STRUCT(quad_stage);

   stage->softpipe = softpipe;
   stage->run = stencil_test_quad;

   return stage;
}
