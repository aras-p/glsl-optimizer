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

#include "main/glheader.h"
#include "main/context.h"
#include "main/colormac.h"

#include "vf.h"

#define DBG 0



static GLboolean match_fastpath( struct vertex_fetch *vf,
				 const struct vf_fastpath *fp)
{
   GLuint j;

   if (vf->attr_count != fp->attr_count) 
      return GL_FALSE;

   for (j = 0; j < vf->attr_count; j++) 
      if (vf->attr[j].format != fp->attr[j].format ||
	  vf->attr[j].inputsize != fp->attr[j].size ||
	  vf->attr[j].vertoffset != fp->attr[j].offset) 
	 return GL_FALSE;
      
   if (fp->match_strides) {
      if (vf->vertex_stride != fp->vertex_stride)
	 return GL_FALSE;

      for (j = 0; j < vf->attr_count; j++) 
	 if (vf->attr[j].inputstride != fp->attr[j].stride) 
	    return GL_FALSE;
   }
   
   return GL_TRUE;
}

static GLboolean search_fastpath_emit( struct vertex_fetch *vf )
{
   struct vf_fastpath *fp = vf->fastpath;

   for ( ; fp ; fp = fp->next) {
      if (match_fastpath(vf, fp)) {
         vf->emit = fp->func;
	 return GL_TRUE;
      }
   }

   return GL_FALSE;
}

void vf_register_fastpath( struct vertex_fetch *vf,
			     GLboolean match_strides )
{
   struct vf_fastpath *fastpath = CALLOC_STRUCT(vf_fastpath);
   GLuint i;

   fastpath->vertex_stride = vf->vertex_stride;
   fastpath->attr_count = vf->attr_count;
   fastpath->match_strides = match_strides;
   fastpath->func = vf->emit;
   fastpath->attr = (struct vf_attr_type *)
      _mesa_malloc(vf->attr_count * sizeof(fastpath->attr[0]));

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
static void choose_emit_func( struct vertex_fetch *vf, 
			      GLuint count, 
			      GLubyte *dest)
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
      vf_generate_hardwired_emit(vf);
   }

   /* Otherwise use the generic version:
    */
   if (!vf->emit)
      vf->emit = vf_generic_emit;

   vf->emit( vf, count, dest );
}





/***********************************************************************
 * Public entrypoints, mostly dispatch to the above:
 */



GLuint vf_set_vertex_attributes( struct vertex_fetch *vf, 
				 const struct vf_attr_map *map,
				 GLuint nr, 
				 GLuint vertex_stride )
{
   GLuint offset = 0;
   GLuint i, j;

   assert(nr < VF_ATTRIB_MAX);

   memset(vf->lookup, 0, sizeof(vf->lookup));

   for (j = 0, i = 0; i < nr; i++) {
      const GLuint format = map[i].format;
      if (format == EMIT_PAD) {
	 if (DBG)
	    _mesa_printf("%d: pad %d, offset %d\n", i,  
			 map[i].offset, offset);  

	 offset += map[i].offset;

      }
      else {
	 assert(vf->lookup[map[i].attrib] == 0);
	 vf->lookup[map[i].attrib] = &vf->attr[j];

	 vf->attr[j].attrib = map[i].attrib;
	 vf->attr[j].format = format;
	 vf->attr[j].insert = vf_format_info[format].insert;
	 vf->attr[j].extract = vf_format_info[format].extract;
	 vf->attr[j].vertattrsize = vf_format_info[format].attrsize;
	 vf->attr[j].vertoffset = offset;
	 
	 if (DBG)
	    _mesa_printf("%d: %s, offset %d\n", i,  
			 vf_format_info[format].name,
			 vf->attr[j].vertoffset);   

	 offset += vf_format_info[format].attrsize;
	 j++;
      }
   }

   vf->attr_count = j;
   vf->vertex_stride = vertex_stride ? vertex_stride : offset;
   vf->emit = choose_emit_func;

   assert(vf->vertex_stride >= offset);
   return vf->vertex_stride;
}



void vf_set_vp_matrix( struct vertex_fetch *vf,
		       const GLfloat *viewport )
{
   assert(vf->allow_viewport_emits);

   /* scale */
   vf->vp[0] = viewport[MAT_SX];
   vf->vp[1] = viewport[MAT_SY];
   vf->vp[2] = viewport[MAT_SZ];
   vf->vp[3] = 1.0;

   /* translate */
   vf->vp[4] = viewport[MAT_TX];
   vf->vp[5] = viewport[MAT_TY];
   vf->vp[6] = viewport[MAT_TZ];
   vf->vp[7] = 0.0;
}

void vf_set_vp_scale_translate( struct vertex_fetch *vf,
				const GLfloat *scale,
				const GLfloat *translate )
{
   assert(vf->allow_viewport_emits);

   vf->vp[0] = scale[0];
   vf->vp[1] = scale[1];
   vf->vp[2] = scale[2];
   vf->vp[3] = scale[3];

   vf->vp[4] = translate[0];
   vf->vp[5] = translate[1];
   vf->vp[6] = translate[2];
   vf->vp[7] = translate[3];
}


/* Set attribute pointers, adjusted for start position:
 */
void vf_set_sources( struct vertex_fetch *vf,
		     GLvector4f * const sources[],
		     GLuint start )
{
   struct vf_attr *a = vf->attr;
   GLuint j;
   
   for (j = 0; j < vf->attr_count; j++) {
      const GLvector4f *vptr = sources[a[j].attrib];
      
      if ((a[j].inputstride != vptr->stride) ||
	  (a[j].inputsize != vptr->size))
	 vf->emit = choose_emit_func;
      
      a[j].inputstride = vptr->stride;
      a[j].inputsize = vptr->size;
      a[j].do_insert = a[j].insert[vptr->size - 1]; 
      a[j].inputptr = ((GLubyte *)vptr->data) + start * vptr->stride;
   }
}



/* Emit count VB vertices to dest.  
 */
void vf_emit_vertices( struct vertex_fetch *vf,
		       GLuint count,
		       void *dest )
{
   vf->emit( vf, count, (GLubyte*) dest );	
}


/* Extract a named attribute from a hardware vertex.  Will have to
 * reverse any viewport transformation, swizzling or other conversions
 * which may have been applied.
 *
 * This is mainly required for on-the-fly vertex translations to
 * swrast format.
 */
void vf_get_attr( struct vertex_fetch *vf,
		  const void *vertex,
		  GLenum attr, 
		  const GLfloat *dflt,
		  GLfloat *dest )
{
   const struct vf_attr *a = vf->attr;
   const GLuint attr_count = vf->attr_count;
   GLuint j;

   for (j = 0; j < attr_count; j++) {
      if (a[j].attrib == attr) {
	 a[j].extract( &a[j], dest, (GLubyte *)vertex + a[j].vertoffset );
	 return;
      }
   }

   /* Else return the value from ctx->Current.
    */
   _mesa_memcpy( dest, dflt, 4*sizeof(GLfloat));
}




struct vertex_fetch *vf_create( GLboolean allow_viewport_emits )
{
   struct vertex_fetch *vf = CALLOC_STRUCT(vertex_fetch);
   GLuint i;

   for (i = 0; i < VF_ATTRIB_MAX; i++)
      vf->attr[i].vf = vf;

   vf->allow_viewport_emits = allow_viewport_emits;

   switch(CHAN_TYPE) {
   case GL_UNSIGNED_BYTE:
      vf->chan_scale[0] = 255.0;
      vf->chan_scale[1] = 255.0;
      vf->chan_scale[2] = 255.0;
      vf->chan_scale[3] = 255.0;
      break;
   case GL_UNSIGNED_SHORT:
      vf->chan_scale[0] = 65535.0;
      vf->chan_scale[1] = 65535.0;
      vf->chan_scale[2] = 65535.0;
      vf->chan_scale[3] = 65535.0;
      break;
   default:
      vf->chan_scale[0] = 1.0;
      vf->chan_scale[1] = 1.0;
      vf->chan_scale[2] = 1.0;
      vf->chan_scale[3] = 1.0;
      break;
   }

   vf->identity[0] = 0.0;
   vf->identity[1] = 0.0;
   vf->identity[2] = 0.0;
   vf->identity[3] = 1.0;

   vf->codegen_emit = NULL;

#ifdef USE_SSE_ASM
   if (!_mesa_getenv("MESA_NO_CODEGEN"))
      vf->codegen_emit = vf_generate_sse_emit;
#endif

   return vf;
}


void vf_destroy( struct vertex_fetch *vf )
{
   struct vf_fastpath *fp, *tmp;

   for (fp = vf->fastpath ; fp ; fp = tmp) {
      tmp = fp->next;
      FREE(fp->attr);

      /* KW: At the moment, fp->func is constrained to be allocated by
       * _mesa_exec_alloc(), as the hardwired fastpaths in
       * t_vertex_generic.c are handled specially.  It would be nice
       * to unify them, but this probably won't change until this
       * module gets another overhaul.
       */
      _mesa_exec_free((void *) fp->func);
      FREE(fp);
   }
   
   vf->fastpath = NULL;
   FREE(vf);
}
