/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <stdlib.h>

#include "main/glheader.h"
#include "main/context.h"
#include "main/state.h"
#include "main/api_validate.h"
#include "main/enums.h"

#include "brw_draw.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_fallback.h"

#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"

#include "tnl/tnl.h"
#include "vbo/vbo_context.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"

#define FILE_DEBUG_FLAG DEBUG_BATCH

static GLuint prim_to_hw_prim[GL_POLYGON+1] = {
   _3DPRIM_POINTLIST,
   _3DPRIM_LINELIST,
   _3DPRIM_LINELOOP,
   _3DPRIM_LINESTRIP,
   _3DPRIM_TRILIST,
   _3DPRIM_TRISTRIP,
   _3DPRIM_TRIFAN,
   _3DPRIM_QUADLIST,
   _3DPRIM_QUADSTRIP,
   _3DPRIM_POLYGON
};


static const GLenum reduced_prim[GL_POLYGON+1] = {  
   GL_POINTS,
   GL_LINES,
   GL_LINES,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES
};


/* When the primitive changes, set a state bit and re-validate.  Not
 * the nicest and would rather deal with this by having all the
 * programs be immune to the active primitive (ie. cope with all
 * possibilities).  That may not be realistic however.
 */
static GLuint brw_set_prim(struct brw_context *brw, GLenum prim)
{
   GLcontext *ctx = &brw->intel.ctx;

   if (INTEL_DEBUG & DEBUG_PRIMS)
      _mesa_printf("PRIM: %s\n", _mesa_lookup_enum_by_nr(prim));
   
   /* Slight optimization to avoid the GS program when not needed:
    */
   if (prim == GL_QUAD_STRIP &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL)
      prim = GL_TRIANGLE_STRIP;

   if (prim != brw->primitive) {
      brw->primitive = prim;
      brw->state.dirty.brw |= BRW_NEW_PRIMITIVE;

      if (reduced_prim[prim] != brw->intel.reduced_primitive) {
	 brw->intel.reduced_primitive = reduced_prim[prim];
	 brw->state.dirty.brw |= BRW_NEW_REDUCED_PRIMITIVE;
      }
   }

   return prim_to_hw_prim[prim];
}


static GLuint trim(GLenum prim, GLuint length)
{
   if (prim == GL_QUAD_STRIP)
      return length > 3 ? (length - length % 2) : 0;
   else if (prim == GL_QUADS)
      return length - length % 4;
   else 
      return length;
}


static void brw_emit_prim(struct brw_context *brw,
			  const struct _mesa_prim *prim,
			  uint32_t hw_prim)
{
   struct brw_3d_primitive prim_packet;

   if (INTEL_DEBUG & DEBUG_PRIMS)
      _mesa_printf("PRIM: %s %d %d\n", _mesa_lookup_enum_by_nr(prim->mode), 
		   prim->start, prim->count);

   prim_packet.header.opcode = CMD_3D_PRIM;
   prim_packet.header.length = sizeof(prim_packet)/4 - 2;
   prim_packet.header.pad = 0;
   prim_packet.header.topology = hw_prim;
   prim_packet.header.indexed = prim->indexed;

   prim_packet.verts_per_instance = trim(prim->mode, prim->count);
   prim_packet.start_vert_location = prim->start;
   prim_packet.instance_count = 1;
   prim_packet.start_instance_location = 0;
   prim_packet.base_vert_location = 0;

   /* Can't wrap here, since we rely on the validated state. */
   brw->no_batch_wrap = GL_TRUE;
   if (prim_packet.verts_per_instance) {
      intel_batchbuffer_data( brw->intel.batch, &prim_packet,
			      sizeof(prim_packet), LOOP_CLIPRECTS);
   }
   brw->no_batch_wrap = GL_FALSE;
}

static void brw_merge_inputs( struct brw_context *brw,
		       const struct gl_client_array *arrays[])
{
   struct brw_vertex_info old = brw->vb.info;
   GLuint i;

   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      dri_bo_unreference(brw->vb.inputs[i].bo);

   memset(&brw->vb.inputs, 0, sizeof(brw->vb.inputs));
   memset(&brw->vb.info, 0, sizeof(brw->vb.info));

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      brw->vb.inputs[i].glarray = arrays[i];

      if (arrays[i]->StrideB != 0)
	 brw->vb.info.varying |= 1 << i;

	 brw->vb.info.sizes[i/16] |= (brw->vb.inputs[i].glarray->Size - 1) <<
	    ((i%16) * 2);
   }

   /* Raise statechanges if input sizes and varying have changed: 
    */
   if (memcmp(brw->vb.info.sizes, old.sizes, sizeof(old.sizes)) != 0)
      brw->state.dirty.brw |= BRW_NEW_INPUT_DIMENSIONS;

   if (brw->vb.info.varying != old.varying)
      brw->state.dirty.brw |= BRW_NEW_INPUT_VARYING;
}

/* XXX: could split the primitive list to fallback only on the
 * non-conformant primitives.
 */
static GLboolean check_fallbacks( struct brw_context *brw,
				  const struct _mesa_prim *prim,
				  GLuint nr_prims )
{
   GLcontext *ctx = &brw->intel.ctx;
   GLuint i;

   if (!brw->intel.strict_conformance)
      return GL_FALSE;

   if (ctx->Polygon.SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (reduced_prim[prim[i].mode] == GL_TRIANGLES) 
	    return GL_TRUE;
   }

   /* BRW hardware will do AA lines, but they are non-conformant it
    * seems.  TBD whether we keep this fallback:
    */
   if (ctx->Line.SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (reduced_prim[prim[i].mode] == GL_LINES) 
	    return GL_TRUE;
   }

   /* Stipple -- these fallbacks could be resolved with a little
    * bit of work?
    */
   if (ctx->Line.StippleFlag) {
      for (i = 0; i < nr_prims; i++) {
	 /* GS doesn't get enough information to know when to reset
	  * the stipple counter?!?
	  */
	 if (prim[i].mode == GL_LINE_LOOP || prim[i].mode == GL_LINE_STRIP) 
	    return GL_TRUE;
	    
	 if (prim[i].mode == GL_POLYGON &&
	     (ctx->Polygon.FrontMode == GL_LINE ||
	      ctx->Polygon.BackMode == GL_LINE))
	    return GL_TRUE;
      }
   }


   if (ctx->Point.SmoothFlag) {
      for (i = 0; i < nr_prims; i++)
	 if (prim[i].mode == GL_POINTS) 
	    return GL_TRUE;
   }
      
   return GL_FALSE;
}

/* May fail if out of video memory for texture or vbo upload, or on
 * fallback conditions.
 */
static GLboolean brw_try_draw_prims( GLcontext *ctx,
				     const struct gl_client_array *arrays[],
				     const struct _mesa_prim *prim,
				     GLuint nr_prims,
				     const struct _mesa_index_buffer *ib,
				     GLuint min_index,
				     GLuint max_index )
{
   struct intel_context *intel = intel_context(ctx);
   struct brw_context *brw = brw_context(ctx);
   GLboolean retval = GL_FALSE;
   GLboolean warn = GL_FALSE;
   GLboolean first_time = GL_TRUE;
   GLuint i;

   if (ctx->NewState)
      _mesa_update_state( ctx );

   if (check_fallbacks(brw, prim, nr_prims))
      return GL_FALSE;

   brw_validate_textures( brw );

   /* Bind all inputs, derive varying and size information:
    */
   brw_merge_inputs( brw, arrays );

   brw->ib.ib = ib;
   brw->state.dirty.brw |= BRW_NEW_INDICES;

   brw->vb.min_index = min_index;
   brw->vb.max_index = max_index;
   brw->state.dirty.brw |= BRW_NEW_VERTICES;

   /* Have to validate state quite late.  Will rebuild tnl_program,
    * which depends on varying information.  
    * 
    * Note this is where brw->vs->prog_data.inputs_read is calculated,
    * so can't access it earlier.
    */

   LOCK_HARDWARE(intel);

   if (!intel->constant_cliprect && intel->driDrawable->numClipRects == 0) {
      UNLOCK_HARDWARE(intel);
      return GL_TRUE;
   }

   for (i = 0; i < nr_prims; i++) {
      uint32_t hw_prim;

      /* Flush the batch if it's approaching full, so that we don't wrap while
       * we've got validated state that needs to be in the same batch as the
       * primitives.  This fraction is just a guess (minimal full state plus
       * a primitive is around 512 bytes), and would be better if we had
       * an upper bound of how much we might emit in a single
       * brw_try_draw_prims().
       */
      intel_batchbuffer_require_space(intel->batch, intel->batch->size / 4,
				      LOOP_CLIPRECTS);

      hw_prim = brw_set_prim(brw, prim[i].mode);

      if (first_time || (brw->state.dirty.brw & BRW_NEW_PRIMITIVE)) {
	 first_time = GL_FALSE;

	 brw_validate_state(brw);

	 /* Various fallback checks:  */
	 if (brw->intel.Fallback)
	    goto out;

	 /* Check that we can fit our state in with our existing batchbuffer, or
	  * flush otherwise.
	  */
	 if (dri_bufmgr_check_aperture_space(brw->state.validated_bos,
					     brw->state.validated_bo_count)) {
	    static GLboolean warned;
	    intel_batchbuffer_flush(intel->batch);

	    /* Validate the state after we flushed the batch (which would have
	     * changed the set of dirty state).  If we still fail to
	     * check_aperture, warn of what's happening, but attempt to continue
	     * on since it may succeed anyway, and the user would probably rather
	     * see a failure and a warning than a fallback.
	     */
	    brw_validate_state(brw);
	    if (!warned &&
		dri_bufmgr_check_aperture_space(brw->state.validated_bos,
						brw->state.validated_bo_count)) {
	       warn = GL_TRUE;
	       warned = GL_TRUE;
	    }
	 }

	 brw_upload_state(brw);
      }

      brw_emit_prim(brw, &prim[i], hw_prim);

      retval = GL_TRUE;
   }

 out:
   UNLOCK_HARDWARE(intel);

   if (warn)
      fprintf(stderr, "i965: Single primitive emit potentially exceeded "
	      "available aperture space\n");

   if (!retval)
      DBG("%s failed\n", __FUNCTION__);

   return retval;
}

static GLboolean brw_need_rebase( GLcontext *ctx,
				  const struct gl_client_array *arrays[],
				  const struct _mesa_index_buffer *ib,
				  GLuint min_index )
{
   if (min_index == 0) 
      return GL_FALSE;

   if (ib) {
      if (!vbo_all_varyings_in_vbos(arrays))
	 return GL_TRUE;
      else
	 return GL_FALSE;
   }
   else {
      /* Hmm.  This isn't quite what I wanted.  BRW can actually
       * handle the mixed case well enough that we shouldn't need to
       * rebase.  However, it's probably not very common, nor hugely
       * expensive to do it this way:
       */
      if (!vbo_all_varyings_in_vbos(arrays))
	 return GL_TRUE;
      else
	 return GL_FALSE;
   }
}
				  

void brw_draw_prims( GLcontext *ctx,
		     const struct gl_client_array *arrays[],
		     const struct _mesa_prim *prim,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     GLuint min_index,
		     GLuint max_index )
{
   GLboolean retval;

   /* Decide if we want to rebase.  If so we end up recursing once
    * only into this function.
    */
   if (brw_need_rebase( ctx, arrays, ib, min_index )) {
      vbo_rebase_prims( ctx, arrays, 
			prim, nr_prims, 
			ib, min_index, max_index, 
			brw_draw_prims );
      
      return;
   }

   /* Make a first attempt at drawing:
    */
   retval = brw_try_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);

   /* Otherwise, we really are out of memory.  Pass the drawing
    * command to the software tnl module and which will in turn call
    * swrast to do the drawing.
    */
   if (!retval) {
       _swsetup_Wakeup(ctx);
      _tnl_draw_prims(ctx, arrays, prim, nr_prims, ib, min_index, max_index);
   }

}

void brw_draw_init( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct vbo_context *vbo = vbo_context(ctx);

   /* Register our drawing function: 
    */
   vbo->draw_prims = brw_draw_prims;
}

void brw_draw_destroy( struct brw_context *brw )
{
   int i;

   if (brw->vb.upload.bo != NULL) {
      dri_bo_unreference(brw->vb.upload.bo);
      brw->vb.upload.bo = NULL;
   }

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      dri_bo_unreference(brw->vb.inputs[i].bo);
      brw->vb.inputs[i].bo = NULL;
   }

   dri_bo_unreference(brw->ib.bo);
   brw->ib.bo = NULL;
}
