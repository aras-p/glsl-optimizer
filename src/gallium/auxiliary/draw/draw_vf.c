/*
 * Copyright 2003 Tungsten Graphics, inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@tungstengraphics.com>
 */


#include <stddef.h>

#include "pipe/p_compiler.h"
#include "pipe/p_util.h"
#include "rtasm/rtasm_execmem.h"

#include "draw_vf.h"


#define DRAW_VF_DBG 0


static boolean match_fastpath( struct draw_vertex_fetch *vf,
				 const struct draw_vf_fastpath *fp)
{
   unsigned j;

   if (vf->attr_count != fp->attr_count) 
      return FALSE;

   for (j = 0; j < vf->attr_count; j++) 
      if (vf->attr[j].format != fp->attr[j].format ||
	  vf->attr[j].inputsize != fp->attr[j].size ||
	  vf->attr[j].vertoffset != fp->attr[j].offset) 
	 return FALSE;
      
   if (fp->match_strides) {
      if (vf->vertex_stride != fp->vertex_stride)
	 return FALSE;

      for (j = 0; j < vf->attr_count; j++) 
	 if (vf->attr[j].inputstride != fp->attr[j].stride) 
	    return FALSE;
   }
   
   return TRUE;
}

static boolean search_fastpath_emit( struct draw_vertex_fetch *vf )
{
   struct draw_vf_fastpath *fp = vf->fastpath;

   for ( ; fp ; fp = fp->next) {
      if (match_fastpath(vf, fp)) {
         vf->emit = fp->func;
	 return TRUE;
      }
   }

   return FALSE;
}

void draw_vf_register_fastpath( struct draw_vertex_fetch *vf,
			     boolean match_strides )
{
   struct draw_vf_fastpath *fastpath = CALLOC_STRUCT(draw_vf_fastpath);
   unsigned i;

   fastpath->vertex_stride = vf->vertex_stride;
   fastpath->attr_count = vf->attr_count;
   fastpath->match_strides = match_strides;
   fastpath->func = vf->emit;
   fastpath->attr = (struct draw_vf_attr_type *)
      MALLOC(vf->attr_count * sizeof(fastpath->attr[0]));

   for (i = 0; i < vf->attr_count; i++) {
      fastpath->attr[i].format = vf->attr[i].format;
      fastpath->attr[i].stride = vf->attr[i].inputstride;
      fastpath->attr[i].size = vf->attr[i].inputsize;
      fastpath->attr[i].offset = vf->attr[i].vertoffset;
   }

   fastpath->next = vf->fastpath;
   vf->fastpath = fastpath;
}




/***********************************************************************
 * Build codegen functions or return generic ones:
 */
static void choose_emit_func( struct draw_vertex_fetch *vf, 
			      unsigned count, 
			      uint8_t *dest)
{
   vf->emit = NULL;
   
   /* Does this match an existing (hardwired, codegen or known-bad)
    * fastpath?
    */
   if (search_fastpath_emit(vf)) {
      /* Use this result.  If it is null, then it is already known
       * that the current state will fail for codegen and there is no
       * point trying again.
       */
   }
   else if (vf->codegen_emit) {
      vf->codegen_emit( vf );
   }

   if (!vf->emit) {
      draw_vf_generate_hardwired_emit(vf);
   }

   /* Otherwise use the generic version:
    */
   if (!vf->emit)
      vf->emit = draw_vf_generic_emit;

   vf->emit( vf, count, dest );
}





/***********************************************************************
 * Public entrypoints, mostly dispatch to the above:
 */



static unsigned 
draw_vf_set_vertex_attributes( struct draw_vertex_fetch *vf, 
                               const struct draw_vf_attr_map *map,
                               unsigned nr, 
                               unsigned vertex_stride )
{
   unsigned offset = 0;
   unsigned i, j;

   assert(nr < PIPE_MAX_ATTRIBS);

   for (j = 0, i = 0; i < nr; i++) {
      const unsigned format = map[i].format;
      if (format == DRAW_EMIT_PAD) {
#if (DRAW_VF_DBG)
	    debug_printf("%d: pad %d, offset %d\n", i,  
			 map[i].offset, offset);  
#endif

	 offset += map[i].offset;

      }
      else {
	 vf->attr[j].attrib = map[i].attrib;
	 vf->attr[j].format = format;
	 vf->attr[j].insert = draw_vf_format_info[format].insert;
	 vf->attr[j].vertattrsize = draw_vf_format_info[format].attrsize;
	 vf->attr[j].vertoffset = offset;
	 vf->attr[j].isconst = draw_vf_format_info[format].isconst;
	 if(vf->attr[j].isconst)
	    memcpy(vf->attr[j].data, &map[i].data, vf->attr[j].vertattrsize);
	 
#if (DRAW_VF_DBG)
	    debug_printf("%d: %s, offset %d\n", i,  
			 draw_vf_format_info[format].name,
			 vf->attr[j].vertoffset);   
#endif

	 offset += draw_vf_format_info[format].attrsize;
	 j++;
      }
   }

   vf->attr_count = j;
   vf->vertex_stride = vertex_stride ? vertex_stride : offset;
   vf->emit = choose_emit_func;

   assert(vf->vertex_stride >= offset);
   return vf->vertex_stride;
}


void draw_vf_set_vertex_info( struct draw_vertex_fetch *vf, 
                              const struct vertex_info *vinfo,
                              float point_size )
{
   unsigned i, j, k;
   struct draw_vf_attr *a = vf->attr;
   struct draw_vf_attr_map attrs[PIPE_MAX_SHADER_INPUTS];
   unsigned count = 0;  /* for debug/sanity */
   unsigned nr_attrs = 0;
   
   for (i = 0; i < vinfo->num_attribs; i++) {
      j = vinfo->src_index[i];
      switch (vinfo->emit[i]) {
      case EMIT_OMIT:
         /* no-op */
         break;
      case EMIT_ALL: {
         /* just copy the whole vertex as-is to the vbuf */
	 unsigned s = vinfo->size;
         assert(i == 0);
         assert(j == 0);
         /* copy the vertex header */
         /* XXX: we actually don't copy the header, just pad it */
	 attrs[nr_attrs].attrib = 0;
	 attrs[nr_attrs].format = DRAW_EMIT_PAD;
	 attrs[nr_attrs].offset = offsetof(struct vertex_header, data);
	 s -= offsetof(struct vertex_header, data)/4;
         count += offsetof(struct vertex_header, data)/4;
	 nr_attrs++;
	 /* copy the vertex data */
         for(k = 0; k < (s & ~0x3); k += 4) {
      	    attrs[nr_attrs].attrib = k/4;
      	    attrs[nr_attrs].format = DRAW_EMIT_4F;
      	    attrs[nr_attrs].offset = 0;
      	    nr_attrs++;
            count += 4;
         }
         /* tail */
         /* XXX: actually, this shouldn't be needed */
 	 attrs[nr_attrs].attrib = k/4;
  	 attrs[nr_attrs].offset = 0;
         switch(s & 0x3) {
         case 0:
            break;
         case 1:
      	    attrs[nr_attrs].format = DRAW_EMIT_1F;
      	    nr_attrs++;
            count += 1;
            break;
         case 2:
      	    attrs[nr_attrs].format = DRAW_EMIT_2F;
      	    nr_attrs++;
            count += 2;
            break;
         case 3:
      	    attrs[nr_attrs].format = DRAW_EMIT_3F;
      	    nr_attrs++;
            count += 3;
            break;
         }
         break;
      }
      case EMIT_HEADER:
         /* XXX emit new DRAW_EMIT_HEADER attribute??? */
	 attrs[nr_attrs].attrib = 0;
	 attrs[nr_attrs].format = DRAW_EMIT_PAD;
	 attrs[nr_attrs].offset = offsetof(struct vertex_header, data);
         count += offsetof(struct vertex_header, data)/4;
	 nr_attrs++;
         break;
      case EMIT_1F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_1F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count++;
         break;
      case EMIT_1F_PSIZE:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_1F_CONST;
	 attrs[nr_attrs].offset = 0;
	 attrs[nr_attrs].data.f[0] = point_size;
	 nr_attrs++;
         count++;
         break;
      case EMIT_2F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_2F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 2;
         break;
      case EMIT_3F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_3F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 3;
         break;
      case EMIT_4F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_4F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 4;
         break;
      case EMIT_4UB:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_4UB_4F_BGRA;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 1;
         break;
      default:
         assert(0);
      }
   }
   
   assert(count == vinfo->size);  
   
   draw_vf_set_vertex_attributes(vf, 
                                 attrs, 
                                 nr_attrs, 
                                 vinfo->size * sizeof(float) );

   for (j = 0; j < vf->attr_count; j++) {
      a[j].inputsize = 4;
      a[j].do_insert = a[j].insert[4 - 1];
      if(a[j].isconst) {
	 a[j].inputptr = a[j].data;
	 a[j].inputstride = 0;
      }
   }
}


#if 0
/* Set attribute pointers, adjusted for start position:
 */
void draw_vf_set_sources( struct draw_vertex_fetch *vf,
		     GLvector4f * const sources[],
		     unsigned start )
{
   struct draw_vf_attr *a = vf->attr;
   unsigned j;
   
   for (j = 0; j < vf->attr_count; j++) {
      const GLvector4f *vptr = sources[a[j].attrib];
      
      if ((a[j].inputstride != vptr->stride) ||
	  (a[j].inputsize != vptr->size))
	 vf->emit = choose_emit_func;
      
      a[j].inputstride = vptr->stride;
      a[j].inputsize = vptr->size;
      a[j].do_insert = a[j].insert[vptr->size - 1]; 
      a[j].inputptr = ((uint8_t *)vptr->data) + start * vptr->stride;
   }
}
#endif


/**
 * Emit a vertex to dest.  
 */
void draw_vf_emit_vertex( struct draw_vertex_fetch *vf,
                          struct vertex_header *vertex,
                          void *dest )
{
   struct draw_vf_attr *a = vf->attr;
   unsigned j;
   
   for (j = 0; j < vf->attr_count; j++) {
      if (!a[j].isconst) {
	 a[j].inputptr = (uint8_t *)&vertex->data[a[j].attrib][0];
	 a[j].inputstride = 0; /* XXX: one-vertex-max ATM */
      }
   }
   
   vf->emit( vf, 1, (uint8_t*) dest );
}



struct draw_vertex_fetch *draw_vf_create( void )
{
   struct draw_vertex_fetch *vf = CALLOC_STRUCT(draw_vertex_fetch);
   unsigned i;

   for (i = 0; i < PIPE_MAX_ATTRIBS; i++)
      vf->attr[i].vf = vf;

   vf->identity[0] = 0.0;
   vf->identity[1] = 0.0;
   vf->identity[2] = 0.0;
   vf->identity[3] = 1.0;

   vf->codegen_emit = NULL;

#ifdef USE_SSE_ASM
   if (!GETENV("GALLIUM_NO_CODEGEN"))
      vf->codegen_emit = draw_vf_generate_sse_emit;
#endif

   return vf;
}


void draw_vf_destroy( struct draw_vertex_fetch *vf )
{
   struct draw_vf_fastpath *fp, *tmp;

   for (fp = vf->fastpath ; fp ; fp = tmp) {
      tmp = fp->next;
      FREE(fp->attr);

      /* KW: At the moment, fp->func is constrained to be allocated by
       * rtasm_exec_alloc(), as the hardwired fastpaths in
       * t_vertex_generic.c are handled specially.  It would be nice
       * to unify them, but this probably won't change until this
       * module gets another overhaul.
       */
      //rtasm_exec_free((void *) fp->func);
      FREE(fp);
   }
   
   vf->fastpath = NULL;
   FREE(vf);
}
