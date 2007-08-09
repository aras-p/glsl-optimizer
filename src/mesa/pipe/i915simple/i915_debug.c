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

#include "imports.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_debug.h"



static GLboolean debug( struct debug_stream *stream, const char *name, GLuint len )
{
   GLuint i;
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   
   if (len == 0) {
      _mesa_printf("Error - zero length packet (0x%08x)\n", stream->ptr[0]);
      assert(0);
      return GL_FALSE;
   }

   if (stream->print_addresses)
      _mesa_printf("%08x:  ", stream->offset);


   _mesa_printf("%s (%d dwords):\n", name, len);
   for (i = 0; i < len; i++)
      _mesa_printf("\t\t0x%08x\n",  ptr[i]);   
   _mesa_printf("\n");

   stream->offset += len * sizeof(GLuint);
   
   return GL_TRUE;
}


static const char *get_prim_name( GLuint val )
{
   switch (val & PRIM3D_MASK) {
   case PRIM3D_TRILIST: return "TRILIST"; break;
   case PRIM3D_TRISTRIP: return "TRISTRIP"; break;
   case PRIM3D_TRISTRIP_RVRSE: return "TRISTRIP_RVRSE"; break;
   case PRIM3D_TRIFAN: return "TRIFAN"; break;
   case PRIM3D_POLY: return "POLY"; break;
   case PRIM3D_LINELIST: return "LINELIST"; break;
   case PRIM3D_LINESTRIP: return "LINESTRIP"; break;
   case PRIM3D_RECTLIST: return "RECTLIST"; break;
   case PRIM3D_POINTLIST: return "POINTLIST"; break;
   case PRIM3D_DIB: return "DIB"; break;
   case PRIM3D_CLEAR_RECT: return "CLEAR_RECT"; break;
   case PRIM3D_ZONE_INIT: return "ZONE_INIT"; break;
   default: return "????"; break;
   }
}

static GLboolean debug_prim( struct debug_stream *stream, const char *name, 
			     GLboolean dump_floats,
			     GLuint len )
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   const char *prim = get_prim_name( ptr[0] );
   GLuint i;
   


   _mesa_printf("%s %s (%d dwords):\n", name, prim, len);
   _mesa_printf("\t\t0x%08x\n",  ptr[0]);   
   for (i = 1; i < len; i++) {
      if (dump_floats)
	 _mesa_printf("\t\t0x%08x // %f\n",  ptr[i], *(GLfloat *)&ptr[i]);   
      else
	 _mesa_printf("\t\t0x%08x\n",  ptr[i]);   
   }

      
   _mesa_printf("\n");

   stream->offset += len * sizeof(GLuint);
   
   return GL_TRUE;
}
   



static GLboolean debug_program( struct debug_stream *stream, const char *name, GLuint len )
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);

   if (len == 0) {
      _mesa_printf("Error - zero length packet (0x%08x)\n", stream->ptr[0]);
      assert(0);
      return GL_FALSE;
   }

   if (stream->print_addresses)
      _mesa_printf("%08x:  ", stream->offset);

   _mesa_printf("%s (%d dwords):\n", name, len);
   i915_disassemble_program( ptr, len );

   stream->offset += len * sizeof(GLuint);
   return GL_TRUE;
}


static GLboolean debug_chain( struct debug_stream *stream, const char *name, GLuint len )
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint old_offset = stream->offset + len * sizeof(GLuint);
   GLuint i;

   _mesa_printf("%s (%d dwords):\n", name, len);
   for (i = 0; i < len; i++)
      _mesa_printf("\t\t0x%08x\n",  ptr[i]);

   stream->offset = ptr[1] & ~0x3;
   
   if (stream->offset < old_offset)
      _mesa_printf("\n... skipping backwards from 0x%x --> 0x%x ...\n\n", 
		   old_offset, stream->offset );
   else
      _mesa_printf("\n... skipping from 0x%x --> 0x%x ...\n\n", 
		   old_offset, stream->offset );


   return GL_TRUE;
}


static GLboolean debug_variable_length_prim( struct debug_stream *stream )
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   const char *prim = get_prim_name( ptr[0] );
   GLuint i, len;

   GLushort *idx = (GLushort *)(ptr+1);
   for (i = 0; idx[i] != 0xffff; i++)
      ;

   len = 1+(i+2)/2;

   _mesa_printf("3DPRIM, %s variable length %d indicies (%d dwords):\n", prim, i, len);
   for (i = 0; i < len; i++)
      _mesa_printf("\t\t0x%08x\n",  ptr[i]);
   _mesa_printf("\n");

   stream->offset += len * sizeof(GLuint);
   return GL_TRUE;
}


static GLboolean debug_load_immediate( struct debug_stream *stream,
				       const char *name,
				       GLuint len )
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint bits = (ptr[0] >> 4) & 0xff;
   GLuint i, j = 0;
   
   _mesa_printf("%s (%d dwords, flags: %x):\n", name, len, bits);
   _mesa_printf("\t\t0x%08x\n",  ptr[j++]);

   for (i = 0; i < 8; i++) {
      if (bits & (1<<i)) {
	 _mesa_printf("\t  LIS%d: 0x%08x\n", i, ptr[j++]);
      }
   }

   _mesa_printf("\n");

   assert(j == len);

   stream->offset += len * sizeof(GLuint);
   
   return GL_TRUE;
}
 


static GLboolean debug_load_indirect( struct debug_stream *stream,
				      const char *name,
				      GLuint len )
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint bits = (ptr[0] >> 8) & 0x3f;
   GLuint i, j = 0;
   
   _mesa_printf("%s (%d dwords):\n", name, len);
   _mesa_printf("\t\t0x%08x\n",  ptr[j++]);

   for (i = 0; i < 6; i++) {
      if (bits & (1<<i)) {
	 switch (1<<(8+i)) {
	 case LI0_STATE_STATIC_INDIRECT:
	    _mesa_printf("        STATIC: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    _mesa_printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_DYNAMIC_INDIRECT:
	    _mesa_printf("       DYNAMIC: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    break;
	 case LI0_STATE_SAMPLER:
	    _mesa_printf("       SAMPLER: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    _mesa_printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_MAP:
	    _mesa_printf("           MAP: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    _mesa_printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_PROGRAM:
	    _mesa_printf("       PROGRAM: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    _mesa_printf("                0x%08x\n", ptr[j++]);
	    break;
	 case LI0_STATE_CONSTANTS:
	    _mesa_printf("     CONSTANTS: 0x%08x | %x\n", ptr[j]&~3, ptr[j]&3); j++;
	    _mesa_printf("                0x%08x\n", ptr[j++]);
	    break;
	 default:
	    assert(0);
	    break;
	 }
      }
   }

   if (bits == 0) {
      _mesa_printf("\t  DUMMY: 0x%08x\n", ptr[j++]);
   }

   _mesa_printf("\n");


   assert(j == len);

   stream->offset += len * sizeof(GLuint);
   
   return GL_TRUE;
}
 		   

static GLboolean i915_debug_packet( struct debug_stream *stream )
{
   GLuint *ptr = (GLuint *)(stream->ptr + stream->offset);
   GLuint cmd = *ptr;
   
   switch (((cmd >> 29) & 0x7)) {
   case 0x0:
      switch ((cmd >> 23) & 0x3f) {
      case 0x0:
	 return debug(stream, "MI_NOOP", 1);
      case 0x3:
	 return debug(stream, "MI_WAIT_FOR_EVENT", 1);
      case 0x4:
	 return debug(stream, "MI_FLUSH", 1);
      case 0xA:
	 debug(stream, "MI_BATCH_BUFFER_END", 1);
	 return GL_FALSE;
      case 0x22:
	 return debug(stream, "MI_LOAD_REGISTER_IMM", 3);
      case 0x31:
	 return debug_chain(stream, "MI_BATCH_BUFFER_START", 2);
      default:
	 break;
      }
      break;
   case 0x1:
      break;
   case 0x2:
      switch ((cmd >> 22) & 0xff) {	 
      case 0x50:
	 return debug(stream, "XY_COLOR_BLT", (cmd & 0xff) + 2);
      case 0x53:
	 return debug(stream, "XY_SRC_COPY_BLT", (cmd & 0xff) + 2);
      default:
	 return debug(stream, "blit command", (cmd & 0xff) + 2);
      }
      break;
   case 0x3:
      switch ((cmd >> 24) & 0x1f) {	 
      case 0x6:
	 return debug(stream, "3DSTATE_ANTI_ALIASING", 1);
      case 0x7:
	 return debug(stream, "3DSTATE_RASTERIZATION_RULES", 1);
      case 0x8:
	 return debug(stream, "3DSTATE_BACKFACE_STENCIL_OPS", 2);
      case 0x9:
	 return debug(stream, "3DSTATE_BACKFACE_STENCIL_MASKS", 1);
      case 0xb:
	 return debug(stream, "3DSTATE_INDEPENDENT_ALPHA_BLEND", 1);
      case 0xc:
	 return debug(stream, "3DSTATE_MODES5", 1);	 
      case 0xd:
	 return debug(stream, "3DSTATE_MODES4", 1);
      case 0x15:
	 return debug(stream, "3DSTATE_FOG_COLOR", 1);
      case 0x16:
	 return debug(stream, "3DSTATE_COORD_SET_BINDINGS", 1);
      case 0x1c:
	 /* 3DState16NP */
	 switch((cmd >> 19) & 0x1f) {
	 case 0x10:
	    return debug(stream, "3DSTATE_SCISSOR_ENABLE", 1);
	 case 0x11:
	    return debug(stream, "3DSTATE_DEPTH_SUBRECTANGLE_DISABLE", 1);
	 default:
	    break;
	 }
	 break;
      case 0x1d:
	 /* 3DStateMW */
	 switch ((cmd >> 16) & 0xff) {
	 case 0x0:
	    return debug(stream, "3DSTATE_MAP_STATE", (cmd & 0x1f) + 2);
	 case 0x1:
	    return debug(stream, "3DSTATE_SAMPLER_STATE", (cmd & 0x1f) + 2);
	 case 0x4:
	    return debug_load_immediate(stream, "3DSTATE_LOAD_STATE_IMMEDIATE", (cmd & 0xf) + 2);
	 case 0x5:
	    return debug_program(stream, "3DSTATE_PIXEL_SHADER_PROGRAM", (cmd & 0x1ff) + 2);
	 case 0x6:
	    return debug(stream, "3DSTATE_PIXEL_SHADER_CONSTANTS", (cmd & 0xff) + 2);
	 case 0x7:
	    return debug_load_indirect(stream, "3DSTATE_LOAD_INDIRECT", (cmd & 0xff) + 2);
	 case 0x80:
	    return debug(stream, "3DSTATE_DRAWING_RECTANGLE", (cmd & 0xffff) + 2);
	 case 0x81:
	    return debug(stream, "3DSTATE_SCISSOR_RECTANGLE", (cmd & 0xffff) + 2);
	 case 0x83:
	    return debug(stream, "3DSTATE_SPAN_STIPPLE", (cmd & 0xffff) + 2);
	 case 0x85:
	    return debug(stream, "3DSTATE_DEST_BUFFER_VARS", (cmd & 0xffff) + 2);
	 case 0x88:
	    return debug(stream, "3DSTATE_CONSTANT_BLEND_COLOR", (cmd & 0xffff) + 2);
	 case 0x89:
	    return debug(stream, "3DSTATE_FOG_MODE", (cmd & 0xffff) + 2);
	 case 0x8e:
	    return debug(stream, "3DSTATE_BUFFER_INFO", (cmd & 0xffff) + 2);
	 case 0x97:
	    return debug(stream, "3DSTATE_DEPTH_OFFSET_SCALE", (cmd & 0xffff) + 2);
	 case 0x98:
	    return debug(stream, "3DSTATE_DEFAULT_Z", (cmd & 0xffff) + 2);
	 case 0x99:
	    return debug(stream, "3DSTATE_DEFAULT_DIFFUSE", (cmd & 0xffff) + 2);
	 case 0x9a:
	    return debug(stream, "3DSTATE_DEFAULT_SPECULAR", (cmd & 0xffff) + 2);
	 case 0x9c:
	    return debug(stream, "3DSTATE_CLEAR_PARAMETERS", (cmd & 0xffff) + 2);
	 default:
	    assert(0);
	    return 0;
	 }
	 break;
      case 0x1e:
	 if (cmd & (1 << 23))
	    return debug(stream, "???", (cmd & 0xffff) + 1);
	 else
	    return debug(stream, "", 1);
	 break;
      case 0x1f:
	 if ((cmd & (1 << 23)) == 0)	
	    return debug_prim(stream, "3DPRIM (inline)", 1, (cmd & 0x1ffff) + 2);
	 else if (cmd & (1 << 17)) 
	 {
	    if ((cmd & 0xffff) == 0)
	       return debug_variable_length_prim(stream);
	    else
	       return debug_prim(stream, "3DPRIM (indexed)", 0, (((cmd & 0xffff) + 1) / 2) + 1);
	 }
	 else
	    return debug_prim(stream, "3DPRIM  (indirect sequential)", 0, 2); 
	 break;
      default:
	 return debug(stream, "", 0);
      }
   default:
      assert(0);
      return 0;
   }

   assert(0);
   return 0;
}



void
i915_dump_batchbuffer( struct i915_context *i915,
		       GLuint *start,
		       GLuint *end )
{
   struct debug_stream stream;
   GLuint bytes = (end - start) * 4;
   GLboolean done = GL_FALSE;

   fprintf(stderr, "\n\nBATCH: (%d)\n", bytes / 4);

   stream.offset = 0;
   stream.ptr = (char *)start;
   stream.print_addresses = 0;

   while (!done &&
	  stream.offset < bytes &&
	  stream.offset >= 0)
   {
      if (!i915_debug_packet( &stream ))
	 break;

      assert(stream.offset <= bytes &&
	     stream.offset >= 0);
   }

   fprintf(stderr, "END-BATCH\n\n\n");
}


