/* $Id: t_vb_program.c,v 1.3 2001/12/15 21:31:28 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * -------- Regarding NV_vertex_program --------
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * o Redistribution of the source code must contain a copyright notice
 *   and this list of conditions;
 * 
 * o Redistribution in binary and source code form must contain the
 *   following Notice in the software and any documentation and/or other
 *   materials provided with the distribution; and
 * 
 * o The name of Nvidia may not be used to promote or endorse software
 *   derived from the software.
 * 
 * NOTICE: Nvidia hereby grants to each recipient a non-exclusive worldwide
 * royalty free patent license under patent claims that are licensable by
 * Nvidia and which are necessarily required and for which no commercially
 * viable non infringing alternative exists to make, use, sell, offer to sell,
 * import and otherwise transfer the vertex extension for the Mesa 3D Graphics
 * Library as distributed in source code and object code form.  No hardware or
 * hardware implementation (including a semiconductor implementation and chips)
 * are licensed hereunder. If a recipient makes a patent claim or institutes
 * patent litigation against Nvidia or Nvidia's customers for use or sale of
 * Nvidia products, then this license grant as to such recipient shall
 * immediately terminate and recipient immediately agrees to cease use and
 * distribution of the Mesa Program and derivatives thereof. 
 * 
 * THE MESA 3D GRAPHICS LIBRARY IS PROVIDED ON AN "AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED INCLUDING,
 * WITHOUT LIMITATION, ANY WARRANTIES OR CONDITIONS OF TITLE, NON-NFRINGEMENT
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * NVIDIA SHALL NOT HAVE ANY LIABILITY FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING WITHOUT LIMITATION
 * LOST PROFITS), HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OR DISTRIBUTION OF THE MESA 3D GRAPHICS
 * LIBRARY OR EVIDENCE OR THE EXERCISE OF ANY RIGHTS GRANTED HEREUNDR, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * If you do not comply with this agreement, then Nvidia may cancel the license
 * and rights granted herein.
 * ---------------------------------------------
 */

/*
 * Authors:
 *    Brian Paul
 */


#include "glheader.h"
#include "api_noop.h"
#include "colormac.h"
#include "context.h"
#include "dlist.h"
#include "hash.h"
#include "light.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "simple_list.h"
#include "mtypes.h"
#include "vpexec.h"

#include "math/m_translate.h"

#include "t_context.h"
#include "t_pipeline.h"
#include "t_imm_api.h"
#include "t_imm_exec.h"



static void
_vp_ArrayElement( GLint i )
{
   /* XXX to do */
}

static void
_vp_Color3f( GLfloat r, GLfloat g, GLfloat b )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   ASSIGN_4V(attrib, r, g, b, 1.0F);
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_Color3fv( const GLfloat *color )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   ASSIGN_4V(attrib, color[0], color[1], color[2], 1.0F);
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_Color3ub( GLubyte r, GLubyte g, GLubyte b )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   attrib[0] = UBYTE_TO_FLOAT(r);
   attrib[1] = UBYTE_TO_FLOAT(g);
   attrib[2] = UBYTE_TO_FLOAT(b);
   attrib[3] = 1.0F;
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_Color3ubv( const GLubyte *color )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   attrib[0] = UBYTE_TO_FLOAT(color[0]);
   attrib[1] = UBYTE_TO_FLOAT(color[1]);
   attrib[2] = UBYTE_TO_FLOAT(color[2]);
   attrib[3] = 1.0F;
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_Color4f( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   ASSIGN_4V(attrib, r, g, b, a);
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_Color4fv( const GLfloat *color )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   COPY_4V(attrib, color);
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_Color4ub( GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   attrib[0] = UBYTE_TO_FLOAT(r);
   attrib[1] = UBYTE_TO_FLOAT(g);
   attrib[2] = UBYTE_TO_FLOAT(b);
   attrib[3] = UBYTE_TO_FLOAT(a);
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_Color4ubv( const GLubyte *color )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR0][IM->Count];
   attrib[0] = UBYTE_TO_FLOAT(color[0]);
   attrib[1] = UBYTE_TO_FLOAT(color[1]);
   attrib[2] = UBYTE_TO_FLOAT(color[2]);
   attrib[3] = UBYTE_TO_FLOAT(color[3]);
   IM->Flag[IM->Count] |= VERT_COLOR0_BIT;
}

static void
_vp_EdgeFlag( GLboolean flag )
{
   GET_IMMEDIATE;
   IM->EdgeFlag[IM->Count] = flag;
   IM->Flag[IM->Count] |= VERT_EDGEFLAG_BIT;
}

static void
_vp_EdgeFlagv( const GLboolean *flag )
{
   GET_IMMEDIATE;
   IM->EdgeFlag[IM->Count] = *flag;
   IM->Flag[IM->Count] |= VERT_EDGEFLAG_BIT;
}

static void
_vp_EvalCoord1f( GLfloat s )
{
   (void) s;
   /* XXX no-op? */
}

static void
_vp_EvalCoord1fv( const GLfloat *v )
{
   (void) v;
   /* XXX no-op? */
}

static void
_vp_EvalCoord2f( GLfloat s, GLfloat t )
{
   (void) s;
   (void )t;
   /* XXX no-op? */
}

static void
_vp_EvalCoord2fv( const GLfloat *v )
{
   (void) v;
   /* XXX no-op? */
}

static void
_vp_EvalPoint1( GLint i )
{
   (void) i;
}

static void
_vp_EvalPoint2( GLint i, GLint j )
{
   (void) i;
   (void) j;
}

static void
_vp_FogCoordf( GLfloat f )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_FOG][IM->Count];
   ASSIGN_4V(attrib, f, 0.0F, 0.0F, 1.0F);
   IM->Flag[IM->Count] |= VERT_FOG_BIT;
}

static void
_vp_FogCoordfv( const GLfloat *f )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_FOG][IM->Count];
   ASSIGN_4V(attrib, f[0], 0.0F, 0.0F, 1.0F);
   IM->Flag[IM->Count] |= VERT_FOG_BIT;
}

static void
_vp_Indexi( GLint i )
{
   (void) i;
}

static void
_vp_Indexiv( const GLint *i )
{
   (void) i;
}

static void
_vp_Materialfv( GLenum face, GLenum pname, const GLfloat *v)
{
   /* XXX no-op? */
}

static void
_vp_MultiTexCoord1f( GLenum unit, GLfloat s )
{
   const GLint u = (GLint) unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      ASSIGN_4V(attrib, s, 0.0F, 0.0F, 1.0F);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_MultiTexCoord1fv( GLenum unit, const GLfloat *c )
{
   const GLint u = unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      ASSIGN_4V(attrib, c[0], 0.0F, 0.0F, 1.0F);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_MultiTexCoord2f( GLenum unit, GLfloat s, GLfloat t )
{
   const GLint u = unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      ASSIGN_4V(attrib, s, t, 0.0F, 1.0F);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_MultiTexCoord2fv( GLenum unit, const GLfloat *c )
{
   const GLint u = unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      ASSIGN_4V(attrib, c[0], c[1], 0.0F, 1.0F);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_MultiTexCoord3f( GLenum unit, GLfloat s, GLfloat t, GLfloat r )
{
   const GLint u = unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      ASSIGN_4V(attrib, s, t, r, 1.0F);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_MultiTexCoord3fv( GLenum unit, const GLfloat *c )
{
   const GLint u = unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      ASSIGN_4V(attrib, c[0], c[1], c[2], 1.0F);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_MultiTexCoord4f( GLenum unit, GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   const GLint u = unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      ASSIGN_4V(attrib, s, t, r, q);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_MultiTexCoord4fv( GLenum unit, const GLfloat *c )
{
   const GLint u = unit - GL_TEXTURE0_ARB;
   if (u >=0 && u < 8) {
      GET_IMMEDIATE;
      GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0 + u][IM->Count];
      COPY_4V(attrib, c);
      IM->Flag[IM->Count] |= (VERT_TEX0_BIT << u);
   }
}

static void
_vp_Normal3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_NORMAL][IM->Count];
   ASSIGN_4V(attrib, x, y, z, 1.0F);
   IM->Flag[IM->Count] |= VERT_NORMAL_BIT;
}

static void
_vp_Normal3fv( const GLfloat *n )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_NORMAL][IM->Count];
   ASSIGN_4V(attrib, n[0], n[1], n[2], 1.0F);
   IM->Flag[IM->Count] |= VERT_NORMAL_BIT;
}

static void
_vp_SecondaryColor3f( GLfloat r, GLfloat g, GLfloat b )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR1][IM->Count];
   ASSIGN_4V(attrib, r, g, b, 1.0F);
   IM->Flag[IM->Count] |= VERT_COLOR1_BIT;
}

static void
_vp_SecondaryColor3fv( const GLfloat *color )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR1][IM->Count];
   ASSIGN_4V(attrib, color[0], color[1], color[2], 1.0F);
   IM->Flag[IM->Count] |= VERT_COLOR1_BIT;
}

static void
_vp_SecondaryColor3ub( GLubyte r, GLubyte g, GLubyte b )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR1][IM->Count];
   attrib[0] = UBYTE_TO_FLOAT(r);
   attrib[1] = UBYTE_TO_FLOAT(g);
   attrib[2] = UBYTE_TO_FLOAT(b);
   attrib[3] = 1.0F;
   IM->Flag[IM->Count] |= VERT_COLOR1_BIT;
}

static void
_vp_SecondaryColor3ubv( const GLubyte *color )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_COLOR1][IM->Count];
   attrib[0] = UBYTE_TO_FLOAT(color[0]);
   attrib[1] = UBYTE_TO_FLOAT(color[1]);
   attrib[2] = UBYTE_TO_FLOAT(color[2]);
   attrib[3] = 1.0F;
   IM->Flag[IM->Count] |= VERT_COLOR1_BIT;
}

static void
_vp_TexCoord1f( GLfloat s )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   ASSIGN_4V(attrib, s, 0.0F, 0.0F, 1.0F);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_TexCoord1fv( const GLfloat *c )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   ASSIGN_4V(attrib, c[0], 0.0F, 0.0F, 1.0F);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_TexCoord2f( GLfloat s, GLfloat t )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   ASSIGN_4V(attrib, s, t, 0.0F, 1.0F);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_TexCoord2fv( const GLfloat *c )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   ASSIGN_4V(attrib, c[0], c[1], 0.0F, 1.0F);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_TexCoord3f( GLfloat s, GLfloat t, GLfloat r )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   ASSIGN_4V(attrib, s, t, r, 1.0F);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_TexCoord3fv( const GLfloat *c )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   ASSIGN_4V(attrib, c[0], c[1], c[2], 1.0F);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_TexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   ASSIGN_4V(attrib, s, t, r, 1.0F);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_TexCoord4fv( const GLfloat *c )
{
   GET_IMMEDIATE;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_TEX0][IM->Count];
   COPY_4V(attrib, c);
   IM->Flag[IM->Count] |= VERT_TEX0_BIT;
}

static void
_vp_Vertex2f( GLfloat x, GLfloat y )
{
   GET_IMMEDIATE;
   const GLuint count = IM->Count++;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_POS][count];
   ASSIGN_4V(attrib, x, y, 0.0F, 1.0F);
   IM->Flag[count] |= VERT_OBJ_BIT;
   if (count == IMM_MAXDATA - 1)
      _tnl_flush_immediate( IM );
}

static void
_vp_Vertex2fv( const GLfloat *v )
{
   GET_IMMEDIATE;
   const GLuint count = IM->Count++;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_POS][count];
   ASSIGN_4V(attrib, v[0], v[1], 0.0F, 1.0F);
   IM->Flag[count] |= VERT_OBJ_BIT;
   if (count == IMM_MAXDATA - 1)
      _tnl_flush_immediate( IM );
}

static void
_vp_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_IMMEDIATE;
   const GLuint count = IM->Count++;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_POS][count];
   ASSIGN_4V(attrib, x, y, z, 1.0F);
   IM->Flag[count] |= VERT_OBJ_BIT;
   if (count == IMM_MAXDATA - 1)
      _tnl_flush_immediate( IM );
}

static void
_vp_Vertex3fv( const GLfloat *v )
{
   GET_IMMEDIATE;
   const GLuint count = IM->Count++;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_POS][count];
   ASSIGN_4V(attrib, v[0], v[1], v[2], 1.0F);
   IM->Flag[count] |= VERT_OBJ_BIT;
   if (count == IMM_MAXDATA - 1)
      _tnl_flush_immediate( IM );
}

static void
_vp_Vertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_IMMEDIATE;
   const GLuint count = IM->Count++;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_POS][count];
   ASSIGN_4V(attrib, x, y, z, w);
   IM->Flag[count] |= VERT_OBJ_BIT;
   if (count == IMM_MAXDATA - 1)
      _tnl_flush_immediate( IM );
}

static void
_vp_Vertex4fv( const GLfloat *v )
{
   GET_IMMEDIATE;
   const GLuint count = IM->Count++;
   GLfloat *attrib = IM->Attrib[VERT_ATTRIB_POS][count];
   COPY_4V(attrib, v);
   IM->Flag[count] |= VERT_OBJ_BIT;
   if (count == IMM_MAXDATA - 1)
      _tnl_flush_immediate( IM );
}

static void
_vp_VertexAttrib4f( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   if (index < 16) {
      GET_IMMEDIATE;
      const GLuint count = IM->Count;
      GLfloat *attrib = IM->Attrib[index][count];
      ASSIGN_4V(attrib, x, y, z, w);
      IM->Flag[count] |= (1 << index);
      if (index == 0) {
         IM->Count++;
         if (count == IMM_MAXDATA - 1)
            _tnl_flush_immediate( IM );
      }
   }
}   

static void
_vp_VertexAttrib4fv( GLuint index, const GLfloat *v )
{
   if (index < 16) {
      GET_IMMEDIATE;
      const GLuint count = IM->Count;
      GLfloat *attrib = IM->Attrib[index][count];
      COPY_4V(attrib, v);
      IM->Flag[count] |= (1 << index);
      if (index == 0) {
         IM->Count++;
         if (count == IMM_MAXDATA - 1)
            _tnl_flush_immediate( IM );
      }
   }
}   


/*
 * When vertex program mode is enabled we hook in different per-vertex
 * functions.
 */
void _tnl_vprog_vtxfmt_init( GLcontext *ctx )
{
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->vtxfmt);

   printf("%s()\n", __FUNCTION__);

   /* All begin/end operations are handled by this vertex format:
    */
   vfmt->ArrayElement = _vp_ArrayElement;
   vfmt->Begin = _tnl_Begin;
   vfmt->Color3f = _vp_Color3f;
   vfmt->Color3fv = _vp_Color3fv;
   vfmt->Color3ub = _vp_Color3ub;
   vfmt->Color3ubv = _vp_Color3ubv;
   vfmt->Color4f = _vp_Color4f;
   vfmt->Color4fv = _vp_Color4fv;
   vfmt->Color4ub = _vp_Color4ub;
   vfmt->Color4ubv = _vp_Color4ubv;
   vfmt->EdgeFlag = _vp_EdgeFlag;
   vfmt->EdgeFlagv = _vp_EdgeFlagv;
   vfmt->End = _tnl_End;
   vfmt->EvalCoord1f = _vp_EvalCoord1f;
   vfmt->EvalCoord1fv = _vp_EvalCoord1fv;
   vfmt->EvalCoord2f = _vp_EvalCoord2f;
   vfmt->EvalCoord2fv = _vp_EvalCoord2fv;
   vfmt->EvalPoint1 = _vp_EvalPoint1;
   vfmt->EvalPoint2 = _vp_EvalPoint2;
   vfmt->FogCoordfEXT = _vp_FogCoordf;
   vfmt->FogCoordfvEXT = _vp_FogCoordfv;
   vfmt->Indexi = _vp_Indexi;
   vfmt->Indexiv = _vp_Indexiv;
   vfmt->Materialfv = _vp_Materialfv;
   vfmt->MultiTexCoord1fARB = _vp_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = _vp_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = _vp_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = _vp_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = _vp_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = _vp_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = _vp_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = _vp_MultiTexCoord4fv;
   vfmt->Normal3f = _vp_Normal3f;
   vfmt->Normal3fv = _vp_Normal3fv;
   vfmt->SecondaryColor3fEXT = _vp_SecondaryColor3f;
   vfmt->SecondaryColor3fvEXT = _vp_SecondaryColor3fv;
   vfmt->SecondaryColor3ubEXT = _vp_SecondaryColor3ub;
   vfmt->SecondaryColor3ubvEXT = _vp_SecondaryColor3ubv;
   vfmt->TexCoord1f = _vp_TexCoord1f;
   vfmt->TexCoord1fv = _vp_TexCoord1fv;
   vfmt->TexCoord2f = _vp_TexCoord2f;
   vfmt->TexCoord2fv = _vp_TexCoord2fv;
   vfmt->TexCoord3f = _vp_TexCoord3f;
   vfmt->TexCoord3fv = _vp_TexCoord3fv;
   vfmt->TexCoord4f = _vp_TexCoord4f;
   vfmt->TexCoord4fv = _vp_TexCoord4fv;
   vfmt->Vertex2f = _vp_Vertex2f;
   vfmt->Vertex2fv = _vp_Vertex2fv;
   vfmt->Vertex3f = _vp_Vertex3f;
   vfmt->Vertex3fv = _vp_Vertex3fv;
   vfmt->Vertex4f = _vp_Vertex4f;
   vfmt->Vertex4fv = _vp_Vertex4fv;
   vfmt->VertexAttrib4fNV = _vp_VertexAttrib4f;
   vfmt->VertexAttrib4fvNV = _vp_VertexAttrib4fv;

   /* Outside begin/end functions (from t_varray.c, t_eval.c, ...):
    */
   vfmt->Rectf = _mesa_noop_Rectf;

   /* Just use the core function:
    */
   vfmt->CallList = _mesa_CallList;

   vfmt->prefer_float_colors = GL_TRUE;
}



struct vp_stage_data {
   GLvector4f clipCoords;             /* resulting vertex positions */
   struct gl_client_array color0[2];  /* front and back */
   struct gl_client_array color1[2];  /* front and back */
   GLvector4f texCoord[MAX_TEXTURE_UNITS];
   GLvector1f fogCoord;
   GLvector1f pointSize;
};


#define VP_STAGE_DATA(stage) ((struct vp_stage_data *)(stage->privatePtr))


static GLboolean run_vp( GLcontext *ctx, struct gl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vp_stage_data *store = VP_STAGE_DATA(stage);
   struct vertex_buffer *VB = &tnl->vb;
   struct vp_machine *machine = &(ctx->VertexProgram.Machine);
   struct vp_program *program;
   GLfloat (*clip)[4];
   GLfloat (*color0)[4], (*color1)[4];
   GLfloat (*bfcolor0)[4], (*bfcolor1)[4];
   GLfloat *fog, *pointSize;
   GLfloat (*texture0)[4];
   GLfloat (*texture1)[4];
   GLfloat (*texture2)[4];
   GLfloat (*texture3)[4];
   GLint i;

   /* convenience pointers */
   store->clipCoords.size = 4;
   clip = (GLfloat (*)[4]) store->clipCoords.data;
   color0 = (GLfloat (*)[4]) store->color0[0].Ptr;
   color1 = (GLfloat (*)[4]) store->color1[0].Ptr;
   bfcolor0 = (GLfloat (*)[4]) store->color0[1].Ptr;
   bfcolor1 = (GLfloat (*)[4]) store->color1[1].Ptr;
   fog = (GLfloat *) store->fogCoord.data;
   pointSize = (GLfloat *) store->pointSize.data;
   texture0 = (GLfloat (*)[4]) store->texCoord[0].data;
   texture1 = (GLfloat (*)[4]) store->texCoord[1].data;
   texture2 = (GLfloat (*)[4]) store->texCoord[2].data;
   texture3 = (GLfloat (*)[4]) store->texCoord[3].data;


   printf("In %s()\n", __FUNCTION__);

   program = (struct vp_program *) _mesa_HashLookup(ctx->VertexProgram.HashTable, ctx->VertexProgram.Binding);
   assert(program);

   _mesa_init_tracked_matrices(ctx);
   _mesa_init_vp_registers(ctx);  /* sets temp regs to (0,0,0,1) */

   for (i = 0; i < VB->Count; i++) {
      GLuint attr;

      printf("Input  %d: %f, %f, %f, %f\n", i,
             VB->AttribPtr[0]->data[i][0],
             VB->AttribPtr[0]->data[i][1],
             VB->AttribPtr[0]->data[i][2],
             VB->AttribPtr[0]->data[i][3]);
      printf("   color: %f, %f, %f, %f\n",
             VB->AttribPtr[3]->data[i][0],
             VB->AttribPtr[3]->data[i][1],
             VB->AttribPtr[3]->data[i][2],
             VB->AttribPtr[3]->data[i][3]);
      printf("  normal: %f, %f, %f, %f\n",
             VB->AttribPtr[2]->data[i][0],
             VB->AttribPtr[2]->data[i][1],
             VB->AttribPtr[2]->data[i][2],
             VB->AttribPtr[2]->data[i][3]);


      /* load the input attribute registers */
      for (attr = 0; attr < 16; attr++) {
         if (VB->Flag[i] & (1 << attr)) {
            COPY_4V(machine->Registers[VP_INPUT_REG_START + attr],
                    VB->AttribPtr[attr]->data[i]);
         }
      }

      /* execute the program */
      _mesa_exec_program(ctx, program);

      printf("Output %d: %f, %f, %f, %f\n", i,
             machine->Registers[VP_OUT_HPOS][0],
             machine->Registers[VP_OUT_HPOS][1],
             machine->Registers[VP_OUT_HPOS][2],
             machine->Registers[VP_OUT_HPOS][3]);
      printf("   color: %f, %f, %f, %f\n",
             machine->Registers[VP_OUT_COL0][0],
             machine->Registers[VP_OUT_COL0][1],
             machine->Registers[VP_OUT_COL0][2],
             machine->Registers[VP_OUT_COL0][3]);

      /* store the attribute output registers into the VB arrays */
      COPY_4V(clip[i], machine->Registers[VP_OUT_HPOS]);
      clip[i][0] /= clip[i][3];
      clip[i][1] /= clip[i][3];
      clip[i][2] /= clip[i][3];
      COPY_4V(color0[i], machine->Registers[VP_OUT_COL0]);
      COPY_4V(color1[i], machine->Registers[VP_OUT_COL1]);
      COPY_4V(bfcolor0[i], machine->Registers[VP_OUT_BFC0]);
      COPY_4V(bfcolor1[i], machine->Registers[VP_OUT_BFC1]);
      fog[i] = machine->Registers[VP_OUT_FOGC][0];
      pointSize[i] = machine->Registers[VP_OUT_PSIZ][0];
      COPY_4V(texture0[i], machine->Registers[VP_OUT_TEX0]);
      COPY_4V(texture1[i], machine->Registers[VP_OUT_TEX0]);
      COPY_4V(texture2[i], machine->Registers[VP_OUT_TEX0]);
      COPY_4V(texture3[i], machine->Registers[VP_OUT_TEX0]);
   }

   VB->ColorPtr[0] = &store->color0[0];
   VB->ColorPtr[1] = &store->color0[1];
   VB->SecondaryColorPtr[0] = &store->color1[0];
   VB->SecondaryColorPtr[1] = &store->color1[1];
   VB->ProjectedClipPtr = &store->clipCoords;
   VB->FogCoordPtr = &store->fogCoord;
   VB->PointSizePtr = &store->pointSize;
   VB->TexCoordPtr[0] = &store->texCoord[0];
   VB->TexCoordPtr[1] = &store->texCoord[1];
   VB->TexCoordPtr[2] = &store->texCoord[2];
   VB->TexCoordPtr[3] = &store->texCoord[3];

#if 000

   GLvector4f *input = ctx->_NeedEyeCoords ? VB->EyePtr : VB->ObjPtr;
   GLuint ind;

/*     _tnl_print_vert_flags( __FUNCTION__, stage->changed_inputs ); */

   /* Make sure we can talk about elements 0..2 in the vector we are
    * lighting.
    */
   if (stage->changed_inputs & (VERT_EYE|VERT_OBJ_BIT)) {
      if (input->size <= 2) {
	 if (input->flags & VEC_NOT_WRITEABLE) {
	    ASSERT(VB->importable_data & VERT_OBJ_BIT);

	    VB->import_data( ctx, VERT_OBJ_BIT, VEC_NOT_WRITEABLE );
	    input = ctx->_NeedEyeCoords ? VB->EyePtr : VB->ObjPtr;

	    ASSERT((input->flags & VEC_NOT_WRITEABLE) == 0);
	 }

	 _mesa_vector4f_clean_elem(input, VB->Count, 2);
      }
   }

   if (VB->Flag)
      ind = LIGHT_FLAGS;
   else
      ind = 0;

   /* The individual functions know about replaying side-effects
    * vs. full re-execution. 
    */
   store->light_func_tab[ind]( ctx, VB, stage, input );
#endif

   return GL_TRUE;
}


/* Called in place of do_lighting when the light table may have changed.
 */
static GLboolean run_validate_program( GLcontext *ctx,
					struct gl_pipeline_stage *stage )
{
#if 000
   GLuint ind = 0;
   light_func *tab;

   if (ctx->Visual.rgbMode) {
      if (ctx->Light._NeedVertices) {
	 if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	    tab = _tnl_light_spec_tab;
	 else
	    tab = _tnl_light_tab;
      }
      else {
	 if (ctx->Light.EnabledList.next == ctx->Light.EnabledList.prev)
	    tab = _tnl_light_fast_single_tab;
	 else
	    tab = _tnl_light_fast_tab;
      }
   }
   else
      tab = _tnl_light_ci_tab;

   if (ctx->Light.ColorMaterialEnabled)
      ind |= LIGHT_COLORMATERIAL;

   if (ctx->Light.Model.TwoSide)
      ind |= LIGHT_TWOSIDE;

   VP_STAGE_DATA(stage)->light_func_tab = &tab[ind];

   /* This and the above should only be done on _NEW_LIGHT:
    */
   _mesa_validate_all_lighting_tables( ctx );
#endif

   /* Now run the stage...
    */
   stage->run = run_vp;
   return stage->run( ctx, stage );
}



#if 0
static void alloc_4chan( struct gl_client_array *a, GLuint sz )
{
   a->Ptr = ALIGN_MALLOC( sz * sizeof(GLchan) * 4, 32 );
   a->Size = 4;
   a->Type = CHAN_TYPE;
   a->Stride = 0;
   a->StrideB = sizeof(GLchan) * 4;
   a->Enabled = 0;
   a->Flags = 0;
}
#endif

static void alloc_4float( struct gl_client_array *a, GLuint sz )
{
   a->Ptr = ALIGN_MALLOC( sz * sizeof(GLfloat) * 4, 32 );
   a->Size = 4;
   a->Type = GL_FLOAT;
   a->Stride = 0;
   a->StrideB = sizeof(GLfloat) * 4;
   a->Enabled = 0;
   a->Flags = 0;
}


/* Called the first time stage->run is called.  In effect, don't
 * allocate data until the first time the stage is run.
 */
static GLboolean run_init_vp( GLcontext *ctx,
                              struct gl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &(tnl->vb);
   struct vp_stage_data *store;
   const GLuint size = VB->Size;
   GLuint i;

   stage->privatePtr = MALLOC(sizeof(*store));
   store = VP_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   /* The output of a vertex program is: */
   _mesa_vector4f_alloc( &store->clipCoords, 0, size, 32 );
   alloc_4float( &store->color0[0], size );
   alloc_4float( &store->color0[1], size );
   alloc_4float( &store->color1[0], size );
   alloc_4float( &store->color1[1], size );
   for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
      _mesa_vector4f_alloc( &store->texCoord[i], 0, VB->Size, 32 );
   _mesa_vector1f_alloc( &store->fogCoord, 0, size, 32 );
   _mesa_vector1f_alloc( &store->pointSize, 0, size, 32 );


   /* Now validate the stage derived data...
    */
   stage->run = run_validate_program;
   return stage->run( ctx, stage );
}



/*
 * Check if vertex program mode is enabled. 
 * If so, configure the pipeline stage's type, inputs, and outputs.
 */
static void check_vp( GLcontext *ctx, struct gl_pipeline_stage *stage )
{
   stage->active = ctx->VertexProgram.Enabled;

   if (stage->active) {
#if 000
      if (stage->privatePtr)
	 stage->run = run_validate_program;
      stage->inputs = VERT_NORMAL_BIT|VERT_MATERIAL;
      if (ctx->Light._NeedVertices)
	 stage->inputs |= VERT_EYE; /* effectively, even when lighting in obj */
      if (ctx->Light.ColorMaterialEnabled)
	 stage->inputs |= VERT_COLOR0_BIT;

      stage->outputs = VERT_COLOR0_BIT;
      if (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)
	 stage->outputs |= VERT_COLOR1_BIT;
#endif
   }
}


static void dtr( struct gl_pipeline_stage *stage )
{
   struct vp_stage_data *store = VP_STAGE_DATA(stage);

   if (store) {
      GLuint i;
      _mesa_vector4f_free( &store->clipCoords );
      ALIGN_FREE( store->color0[0].Ptr );
      ALIGN_FREE( store->color0[1].Ptr );
      ALIGN_FREE( store->color1[0].Ptr );
      ALIGN_FREE( store->color1[1].Ptr );
      for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++)
	 if (store->texCoord[i].data)
            _mesa_vector4f_free( &store->texCoord[i] );
      _mesa_vector1f_free( &store->fogCoord );
      _mesa_vector1f_free( &store->pointSize );

      FREE( store );
      stage->privatePtr = 0;
   }
}

const struct gl_pipeline_stage _tnl_vertex_program_stage =
{
   "vertex-program",
   _NEW_ALL,	/*XXX FIX */	/* recheck */
   _NEW_ALL,	/*XXX FIX */    /* recalc -- modelview dependency
				 * otherwise not captured by inputs
				 * (which may be VERT_OBJ_BIT) */
   GL_FALSE,			/* active */
   0,				/* inputs */
   VERT_CLIP | VERT_COLOR0_BIT,			/* outputs */
   0,				/* changed_inputs */
   NULL,			/* private_data */
   dtr,				/* destroy */
   check_vp,			/* check */
   run_init_vp			/* run -- initially set to ctr */
};
