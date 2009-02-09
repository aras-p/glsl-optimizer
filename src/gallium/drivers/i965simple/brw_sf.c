/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_sf.h"
#include "brw_state.h"
#include "tgsi/tgsi_parse.h"


static void compile_sf_prog( struct brw_context *brw,
			     struct brw_sf_prog_key *key )
{
   struct brw_sf_compile c;
   const unsigned *program;
   unsigned program_size;

   memset(&c, 0, sizeof(c));

   /* Begin the compilation:
    */
   brw_init_compile(&c.func);

   c.key = *key;


   c.nr_attrs = c.key.vp_output_count;
   c.nr_attr_regs = (c.nr_attrs+1)/2;

   c.nr_setup_attrs = c.key.fp_input_count + 1; /* +1 for position */
   c.nr_setup_regs = (c.nr_setup_attrs+1)/2;

   c.prog_data.urb_read_length = c.nr_attr_regs;
   c.prog_data.urb_entry_size = c.nr_setup_regs * 2;


   /* Which primitive?  Or all three?
    */
   switch (key->primitive) {
   case SF_TRIANGLES:
      c.nr_verts = 3;
      brw_emit_tri_setup( &c );
      break;
   case SF_LINES:
      c.nr_verts = 2;
      brw_emit_line_setup( &c );
      break;
   case SF_POINTS:
      c.nr_verts = 1;
      brw_emit_point_setup( &c );
      break;

   case SF_UNFILLED_TRIS:
   default:
      assert(0);
      return;
   }



   /* get the program
    */
   program = brw_get_program(&c.func, &program_size);

   /* Upload
    */
   brw->sf.prog_gs_offset = brw_upload_cache( &brw->cache[BRW_SF_PROG],
					      &c.key,
					      sizeof(c.key),
					      program,
					      program_size,
					      &c.prog_data,
					      &brw->sf.prog_data );
}


static boolean search_cache( struct brw_context *brw,
			       struct brw_sf_prog_key *key )
{
   return brw_search_cache(&brw->cache[BRW_SF_PROG],
			   key, sizeof(*key),
			   &brw->sf.prog_data,
			   &brw->sf.prog_gs_offset);
}


/* Calculate interpolants for triangle and line rasterization.
 */
static void upload_sf_prog( struct brw_context *brw )
{
   const struct brw_fragment_program *fs = brw->attribs.FragmentProgram;
   struct brw_sf_prog_key key;
   struct tgsi_parse_context parse;
   int i, done = 0;


   memset(&key, 0, sizeof(key));

   /* Populate the key, noting state dependencies:
    */
   /* CACHE_NEW_VS_PROG */
   key.vp_output_count = brw->vs.prog_data->outputs_written;

   /* BRW_NEW_FS */
   key.fp_input_count = brw->attribs.FragmentProgram->info.file_max[TGSI_FILE_INPUT] + 1;


   /* BRW_NEW_REDUCED_PRIMITIVE */
   switch (brw->reduced_primitive) {
   case PIPE_PRIM_TRIANGLES:
//      if (key.attrs & (1<<VERT_RESULT_EDGE))
//	 key.primitive = SF_UNFILLED_TRIS;
//      else
      key.primitive = SF_TRIANGLES;
      break;
   case PIPE_PRIM_LINES:
      key.primitive = SF_LINES;
      break;
   case PIPE_PRIM_POINTS:
      key.primitive = SF_POINTS;
      break;
   }



   /* Scan fp inputs to figure out what interpolation modes are
    * required for each incoming vp output.  There is an assumption
    * that the state tracker makes sure there is a 1:1 linkage between
    * these sets of attributes (XXX: position??)
    */
   tgsi_parse_init( &parse, fs->program.tokens );
   while( !done &&
	  !tgsi_parse_end_of_tokens( &parse ) ) 
   {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
	 if (parse.FullToken.FullDeclaration.Declaration.File == TGSI_FILE_INPUT) 
	 {
	    int first = parse.FullToken.FullDeclaration.DeclarationRange.First;
	    int last = parse.FullToken.FullDeclaration.DeclarationRange.Last;
	    int interp_mode = parse.FullToken.FullDeclaration.Declaration.Interpolate;
	    //int semantic = parse.FullToken.FullDeclaration.Semantic.SemanticName;
	    //int semantic_index = parse.FullToken.FullDeclaration.Semantic.SemanticIndex;

	    debug_printf("fs input %d..%d interp mode %d\n", first, last, interp_mode);
	    
	    switch (interp_mode) {
	    case TGSI_INTERPOLATE_CONSTANT:
	       for (i = first; i <= last; i++) 
		  key.const_mask |= (1 << i);
	       break;
	    case TGSI_INTERPOLATE_LINEAR:
	       for (i = first; i <= last; i++) 
		  key.linear_mask |= (1 << i);
	       break;
	    case TGSI_INTERPOLATE_PERSPECTIVE:
	       for (i = first; i <= last; i++) 
		  key.persp_mask |= (1 << i);
	       break;
	    default:
	       break;
	    }

	    /* Also need stuff for flat shading, twosided color.
	     */

	 }
	 break;
      default:
	 done = 1;
	 break;
      }
   }

   /* Hack: Adjust for position.  Optimize away when not required (ie
    * for perspective interpolation).
    */
   key.persp_mask <<= 1;
   key.linear_mask <<= 1; 
   key.linear_mask |= 1;
   key.const_mask <<= 1;

   debug_printf("key.persp_mask: %x\n", key.persp_mask);
   debug_printf("key.linear_mask: %x\n", key.linear_mask);
   debug_printf("key.const_mask: %x\n", key.const_mask);


//   key.do_point_sprite = brw->attribs.Point->PointSprite;
//   key.SpriteOrigin = brw->attribs.Point->SpriteOrigin;

//   key.do_flat_shading = (brw->attribs.Raster->flatshade);
//   key.do_twoside_color = (brw->attribs.Light->Enabled && brw->attribs.Light->Model.TwoSide);

//   if (key.do_twoside_color)
//      key.frontface_ccw = (brw->attribs.Polygon->FrontFace == GL_CCW);


   if (!search_cache(brw, &key))
      compile_sf_prog( brw, &key );
}


const struct brw_tracked_state brw_sf_prog = {
   .dirty = {
      .brw   = (BRW_NEW_RASTERIZER |
		BRW_NEW_REDUCED_PRIMITIVE |
		BRW_NEW_VS |
		BRW_NEW_FS),
      .cache = 0,
   },
   .update = upload_sf_prog
};



#if 0
/* Build a struct like the one we'd like the state tracker to pass to
 * us.
 */
static void update_sf_linkage( struct brw_context *brw )
{
   const struct brw_vertex_program *vs = brw->attribs.VertexProgram;
   const struct brw_fragment_program *fs = brw->attribs.FragmentProgram;
   struct pipe_setup_linkage state;
   struct tgsi_parse_context parse;

   int i, j;
   int nr_vp_outputs = 0;
   int done = 0;

   struct { 
      unsigned semantic:8;
      unsigned semantic_index:16;
   } fp_semantic[32], vp_semantic[32];

   memset(&state, 0, sizeof(state));

   state.fp_input_count = 0;



   


   assert(state.fp_input_count == fs->program.num_inputs);

      
   /* Then scan vp outputs
    */
   done = 0;
   tgsi_parse_init( &parse, vs->program.tokens );
   while( !done &&
	  !tgsi_parse_end_of_tokens( &parse ) ) 
   {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
	 if (parse.FullToken.FullDeclaration.Declaration.File == TGSI_FILE_INPUT) 
	 {
	    int first = parse.FullToken.FullDeclaration.DeclarationRange.First;
	    int last = parse.FullToken.FullDeclaration.DeclarationRange.Last;

	    for (i = first; i < last; i++) {
	       vp_semantic[i].semantic = 
		  parse.FullToken.FullDeclaration.Semantic.SemanticName;
	       vp_semantic[i].semantic_index = 
		  parse.FullToken.FullDeclaration.Semantic.SemanticIndex;
	    }
	    
	    assert(last > nr_vp_outputs);
	    nr_vp_outputs = last;
	 }
	 break;
      default:
	 done = 1;
	 break;
      }
   }


   /* Now match based on semantic information.
    */
   for (i = 0; i< state.fp_input_count; i++) {
      for (j = 0; j < nr_vp_outputs; j++) {
	 if (fp_semantic[i].semantic == vp_semantic[j].semantic &&
	     fp_semantic[i].semantic_index == vp_semantic[j].semantic_index) {
	    state.fp_input[i].vp_output = j;
	 }
      }
      if (fp_semantic[i].semantic == TGSI_SEMANTIC_COLOR) {
	 for (j = 0; j < nr_vp_outputs; j++) {
	    if (TGSI_SEMANTIC_BCOLOR == vp_semantic[j].semantic &&
		fp_semantic[i].semantic_index == vp_semantic[j].semantic_index) {
	       state.fp_input[i].bf_vp_output = j;
	    }
	 }
      }
   }

   if (memcmp(&brw->sf.linkage, &state, sizeof(state)) != 0) {
      brw->sf.linkage = state;
      brw->state.dirty.brw |= BRW_NEW_SF_LINKAGE;
   }
}


const struct brw_tracked_state brw_sf_linkage = {
   .dirty = {
      .brw   = (BRW_NEW_VS |
		BRW_NEW_FS),
      .cache = 0,
   },
   .update = update_sf_linkage
};


#endif
