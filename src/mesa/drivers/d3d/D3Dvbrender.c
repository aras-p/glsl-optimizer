/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                                 */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#include <stdio.h>
#include "clip.h"
#include "context.h"
#include "light.h"
#include "lines.h"
#include "macros.h"
#include "matrix.h"
#include "pb.h"
#include "points.h"
#include "mtypes.h"
#include "vb.h"
#include "vbrender.h"
#include "xform.h"
#include "D3DMesa.h"

static void SetRenderStates( GLcontext *ctx );
static void DebugRenderStates( GLcontext *ctx, BOOL bForce );

static void RenderPointsVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderTriangleVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderTriangleFanVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderTriangleStripVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderQuadVB( GLcontext *ctx, GLuint start, GLuint end );
static void RenderQuad( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint v4, GLuint pv );
void		RenderOneTriangle( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint pv );
void 	RenderOneLine( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv );

/*  I went with a D3D vertex buffer that is 6 times that of the Mesa one */
/* instead of having the D3D one flush when its full.  This way Mesa will*/
/* handle all the flushing.  I need x6 as points can use 4 vertex each.  */
D3DTLVERTEX 	D3DTLVertices[ (VB_MAX*6) ];
GLuint   		VList[VB_SIZE];
/*===========================================================================*/
/* Compute Z offsets for a polygon with plane defined by (A,B,C,D)           */
/* D is not needed. TODO: Currently we are calculating this but not using it.*/
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void OffsetPolygon( GLcontext *ctx, GLfloat a, GLfloat b, GLfloat c )
{
  GLfloat	ac,
          bc,
          m,
          offset;

  DPF(( DBG_FUNC, "OffsetPolygon();" ));

  if ( (c < 0.001F) && (c > - 0.001F) )
  {
    /* Prevents underflow problems. */
    ctx->PointZoffset   = 0.0F;
    ctx->LineZoffset    = 0.0F;
    ctx->PolygonZoffset = 0.0F;
  }
  else
  {
    ac = a / c;
    bc = b / c;
    if ( ac < 0.0F )
	 ac = -ac;
    if ( bc<0.0F )
	 bc = -bc;
    m = MAX2( ac, bc );     /* m = sqrt( ac*ac + bc*bc ); */

    offset = (m * ctx->Polygon.OffsetFactor + ctx->Polygon.OffsetUnits);
    ctx->PointZoffset   = ctx->Polygon.OffsetPoint ? offset : 0.0F;
    ctx->LineZoffset    = ctx->Polygon.OffsetLine  ? offset : 0.0F;
    ctx->PolygonZoffset = ctx->Polygon.OffsetFill  ? offset : 0.0F;
  }

  DPF(( DBG_PRIM_INFO, "OffsetPolygon: %f", offset ));
}
/*===========================================================================*/
/*  Compute signed area of the n-sided polgyon specified by vertices         */
/* vb->Win[] and vertex list vlist[].                                        */
/*  A clockwise polygon will return a negative area.  A counter-clockwise    */
/* polygon will return a positive area.  I have changed this function to     */
/* actually calculate twice the area as its faster and still gives the sign. */
/*===========================================================================*/
/* RETURN: signed area of the polgon.                                        */
/*===========================================================================*/
static GLfloat PolygonArea( const struct vertex_buffer *vb, GLuint n, const GLuint vlist[] )
{
  GLfloat	area;
  GLuint  i;

  DPF(( DBG_FUNC, "PolygonArea();" ));

#define  j0    vlist[i]
#define  j1    vlist[(i+1)%n]
#define  x0    vb->Win[j0][0]
#define  y0    vb->Win[j0][1]
#define  x1    vb->Win[j1][0]
#define  y1    vb->Win[j1][1]

  /* area = sum of trapezoids */
  for( i = 0, area = 0.0; i < n; i++ )
    area += ((x0 - x1) * (y0 + y1));  /* Note: no divide by two here! */

#undef x0
#undef y0
#undef x1
#undef y1
#undef j1
#undef j0

  // TODO: I don't see the point or * 0.5 as we just want the sign...
  return area;
}
/*===========================================================================*/
/*  Render a polygon that needs clipping on at least one vertex. The function*/
/* will first clip the polygon to any user clipping planes then clip to the  */
/* viewing volume.  The final polygon will be draw as single triangles that  */
/* first need minor proccessing (culling, offset, etc) before we draw the    */
/* polygon as a fan.  NOTE: the fan is draw as single triangles as its not   */
/* formed sequentaly in the VB but is in the vlist[].                        */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderClippedPolygon( GLcontext *ctx, GLuint n, GLuint vlist[] )
{
  struct vertex_buffer	*VB = ctx->VB;
  GLfloat 			(*win)[3] = VB->Win,
                         *proj = ctx->ProjectionMatrix,
                         ex, ey,
                         fx, fy, c,
	                    wInv;
  GLuint				index,
	                    pv,
                         facing;

  DPF(( DBG_FUNC, "RenderClippedPolygon();" ));

  DPF(( DBG_PRIM_INFO, "RenderClippedtPolygon( %d )", n ));

  /* Which vertex dictates the color when flat shading. */
  pv = (ctx->Primitive==GL_POLYGON) ? vlist[0] : vlist[n-1];

  /*  Clipping may introduce new vertices.  New vertices will be stored in */
  /* the vertex buffer arrays starting with location VB->Free.  After we've*/
  /* rendered the polygon, these extra vertices can be overwritten.        */
  VB->Free = VB_MAX;

  /* Clip against user clipping planes in eye coord space. */
  if ( ctx->Transform.AnyClip )
  {
    n = gl_userclip_polygon( ctx, n, vlist );
    if ( n < 3 )
	 return;

    /* Transform vertices from eye to clip coordinates:  clip = Proj * eye */
    for( index = 0; index < n; index++ )
    {
	 TRANSFORM_POINT( VB->Clip[vlist[index]], proj, VB->Eye[vlist[index]] );
    }
  }

  /* Clip against view volume in clip coord space */
  n = gl_viewclip_polygon( ctx, n, vlist );
  if ( n < 3 )
    return;

  /* Transform new vertices from clip to ndc to window coords.    */
  /* ndc = clip / W    window = viewport_mapping(ndc)             */
  /* Note that window Z values are scaled to the range of integer */
  /* depth buffer values.                                         */

  /* Only need to compute window coords for new vertices */
  for( index = VB_MAX; index < VB->Free; index++ )
  {
    if ( VB->Clip[index][3] != 0.0F )
    {
	 wInv = 1.0F / VB->Clip[index][3];

	 win[index][0] = VB->Clip[index][0] * wInv * ctx->Viewport.Sx + ctx->Viewport.Tx;
	 win[index][1] = VB->Clip[index][1] * wInv * ctx->Viewport.Sy + ctx->Viewport.Ty;
	 win[index][2] = VB->Clip[index][2] * wInv * ctx->Viewport.Sz + ctx->Viewport.Tz;
    }
    else
    {
	 /* Can't divide by zero, so... */
	 win[index][0] = win[index][1] = win[index][2] = 0.0F;
    }
  }

  /* Draw filled polygon as a triangle fan */
  for( index = 2; index < n; index++ )
  {
    /* Compute orientation of triangle */
    ex = win[vlist[index-1]][0] - win[vlist[0]][0];
    ey = win[vlist[index-1]][1] - win[vlist[0]][1];
    fx = win[vlist[index]][0]   - win[vlist[0]][0];
    fy = win[vlist[index]][1]   - win[vlist[0]][1];
    c = (ex * fy) - (ey * fx);

    /* polygon is perpindicular to view plane, don't draw it */
    if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	 continue;

    /* Backface culling. */
    facing = (c < 0.0F) ^ ctx->Polygon.FrontBit;
    if ( (facing + 1) & ctx->Polygon.CullBits )
	 continue;

    if ( ctx->IndirectTriangles & DD_TRI_LIGHT_TWOSIDE )
    {
	 if ( facing == 1 )
	 {
	   /* use back color */
	   VB->Color   = VB->Bcolor;
	   VB->Specular= VB->Bspec;
	 }
	 else
	 {
	   /* use front color */
	   VB->Color   = VB->Fcolor;
	   VB->Specular= VB->Fspec;
	 }
    }

    if ( ctx->IndirectTriangles & DD_TRI_OFFSET )
    {
	 /* finish computing plane equation of polygon, compute offset */
	 GLfloat fz = win[vlist[index]][2]   - win[vlist[0]][2];
	 GLfloat ez = win[vlist[index-1]][2] - win[vlist[0]][2];
	 GLfloat a = (ey * fz) - (ez * fy);
	 GLfloat b = (ez * fx) - (ex * fz);
	 OffsetPolygon( ctx, a, b, c );
    }
    RenderOneTriangle( ctx, vlist[0], vlist[index-1], vlist[index], pv );
  }
}
/*===========================================================================*/
/*  This function gets called when either the vertex buffer is full or glEnd */
/* has been called.  If the we aren't in rendering mode (FEEDBACK) then I    */
/* pass the vertex buffer back to Mesa to deal with by returning FALSE.      */
/*  If I can render the primitive types in the buffer directly then I will   */
/* return TRUE after I render the vertex buffer and reset the vertex buffer. */
/*                                                                           */
/* TODO: I don't handle the special case of when the vertex buffer is full   */
/*      and we have a primitive that bounds this buffer and the next one to  */
/*      come.  I'm not sure right now if Mesa handles this for me...         */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
GLboolean RenderVertexBuffer( GLcontext *ctx, GLboolean allDone )
{
  struct vertex_buffer	*VB = ctx->VB;
  GLuint 		    		index,
	                    vlist[VB_SIZE];

  DPF(( DBG_FUNC, "RenderVertexBuffer();" ));

  /* We only need to hook actual tri's that need rendering. */
  if ( ctx->RenderMode != GL_RENDER )
  {
	   //	   (ctx->Visual->AccumBits > 0) )
	   //	   (ctx->Visual->StencilBits > 0) )
    DPF(( DBG_PRIM_INFO, "Passing VB back to Mesa" ));
    return FALSE;
  }

  /*  I'm going to set the states here so that all functions will  */
  /* be assured to have the right states.  If Mesa's vertex bufefr */
  /* function calls one of my primitive functions (TRI,POINT,LINE) */
  /* it will need the right states.  So instead of doing it in the */
  /* primitive function I will always do it here at risk of some   */
  /* slow down to some cases...                                    */
  SetRenderStates( ctx );

  switch( ctx->Primitive )
  {
    case GL_POINTS:
	 DPF(( DBG_PRIM_INFO, "GL_POINTS( %d )", VB->Count ));
	 RenderPointsVB( ctx, 0, VB->Count );
	 break;

    case GL_LINES:
    case GL_LINE_STRIP:
    case GL_LINE_LOOP:
	 /*  Not supported functions yet so pass back that we failed to */
	 /* render the vertex buffer and Mesa will have to do it.       */
	 DPF(( DBG_PRIM_INFO, "GL_LINE_?( %d )", VB->Count ));
	 return FALSE;

    case GL_TRIANGLES:
	 if ( VB->Count < 3 )
	 {
	   DPF(( DBG_PRIM_WARN, "GL_TRIANGLES( %d )", VB->Count ));
	   return FALSE;
	 }

	 DPF(( DBG_PRIM_INFO, "GL_TRIANGLES( %d )", VB->Count ));
      RenderTriangleVB( ctx, 0, VB->Count );
	 break;

    case GL_TRIANGLE_STRIP:
	 if ( VB->Count < 3 )
	 {
	   DPF(( DBG_PRIM_WARN, "GL_TRIANGLE_STRIP( %d )", VB->Count ));
	   return FALSE;
	 }

	 DPF(( DBG_PRIM_INFO, "GL_TRIANGLE_STRIP( %d )", VB->Count ));
	 RenderTriangleStripVB( ctx, 0, VB->Count );
	 break;

    case GL_TRIANGLE_FAN:
	 if ( VB->Count < 3 )
	 {
	   DPF(( DBG_PRIM_WARN, "GL_TRIANGLE_FAN( %d )", VB->Count ));
	   return FALSE;
	 }

	 DPF(( DBG_PRIM_INFO, "GL_TRIANGLE_FAN( %d )", VB->Count ));
	 RenderTriangleFanVB( ctx, 0, VB->Count );
	 break;

    case GL_QUADS:
	 if ( VB->Count < 4 )
	 {
	   DPF(( DBG_PRIM_WARN, "GL_QUADS( %d )", VB->Count ));
	   return FALSE;
	 }

	  DPF(( DBG_PRIM_INFO, "GL_QUADS( %d )", VB->Count ));
	  RenderQuadVB( ctx, 0, VB->Count );
	  break;

    case GL_QUAD_STRIP:
	 if ( VB->Count < 4 )
	 {
	   DPF(( DBG_PRIM_WARN, "GL_QUAD_STRIP( %d )", VB->Count ));
	   return FALSE;
	 }

	 DPF(( DBG_PRIM_INFO, "GL_QUAD_STRIP( %d )", VB->Count ));

	 if ( VB->ClipOrMask )
	 {
	   for( index = 3; index < VB->Count; index += 2 )
	   {
		if ( VB->ClipMask[index-3] & VB->ClipMask[index-2] & VB->ClipMask[index-1] & VB->ClipMask[index] & CLIP_ALL_BITS )
		{
		   /* All points clipped by common plane */
		  DPF(( DBG_PRIM_WARN, "GL_QUAD_STRIP( %d )", VB->Count ));
		  continue;
		}
		else if ( VB->ClipMask[index-3] | VB->ClipMask[index-2] | VB->ClipMask[index-1] | VB->ClipMask[index] )
		{
		  vlist[0] = index - 3;
		  vlist[1] = index - 2;
		  vlist[2] = index;
		  vlist[3] = index - 1;
		  RenderClippedPolygon( ctx, 4, vlist );
		}
		else
		{
		  RenderQuad( ctx, (index-3), (index-2), index, (index-1), index );
		}	
	   }
	 }
	 else
	 {
	   /* No clipping needed */
	   for( index = 3; index < VB->Count; index += 2 )
		RenderQuad( ctx, (index-3), (index-2), index, (index-1), index );
	 }	
	 break;
	
    case GL_POLYGON:
	 if ( VB->Count < 3 )
	 {
	   DPF(( DBG_PRIM_WARN, "GL_POLYGON( %d )", VB->Count ));
	   return FALSE;
	 }

	 DPF(( DBG_PRIM_INFO, "GL_POLYGON( %d )", VB->Count ));

	 /* All points clipped by common plane, draw nothing */
	 if ( !(VB->ClipAndMask & CLIP_ALL_BITS) )
	   RenderTriangleFanVB( ctx, 0, VB->Count );
	 break;

    default:
	 /* should never get here */
	 _mesa_problem( ctx, "invalid mode in gl_render_vb" );
   }

  DPF(( DBG_PRIM_INFO, "ResetVB" ));

  /* We return TRUE to indicate we rendered the VB. */
  gl_reset_vb( ctx, allDone );
  return TRUE;
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as triangles.  The   */
/* buffer has to be able to be rendered directly.  This means that we are    */
/* filled, no offsets, no culling and one sided rendering.  Also we must be  */
/* in render mode of course.                                                 */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderTriangleVB( GLcontext *ctx, GLuint start, GLuint end )
{
  D3DMESACONTEXT      	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int                    index,
                         cVertex,
                         height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
   DWORD                	dwPVColor;
   GLfloat              	ex, ey,
                         fx, fy, c;
   GLuint               	facing;

   DPF(( DBG_FUNC, "RenderTriangleVB" ));

   if ( !VB->ClipOrMask )
   {
	DPF(( DBG_PRIM_INFO, "DirectTriangles( %d )", (end-start) ));
	for( index = start, cVertex = 0; index < end; )
	{
	  dwPVColor = (VB->Color[(index+2)][3]<<24) | (VB->Color[(index+2)][0]<<16) | (VB->Color[(index+2)][1]<<8) | VB->Color[(index+2)][2];

	  /*=====================================*/
	  /* Populate the the triangle vertices. */
	  /*=====================================*/
	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[index][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[index][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[index][2] );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[index][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[index][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[index][3]) );
	  D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
	                                  dwPVColor :
                                       (VB->Color[index][3]<<24) | (VB->Color[index][0]<<16) | (VB->Color[index][1]<<8) | VB->Color[index][2];
	  index++;

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[index][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[index][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[index][2] );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[index][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[index][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[index][3]) );
	  D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[index][3]<<24) | (VB->Color[index][0]<<16) | (VB->Color[index][1]<<8) | VB->Color[index][2];
	  index++;

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[index][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[index][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[index][2] );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[index][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[index][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[index][3]) );
	  D3DTLVertices[cVertex++].color= dwPVColor;
	  index++;
	}
   }
   else
   {
#define v1     index
#define v2     (index+1)
#define v3     (index+2)

	for( index = start, cVertex = 0; index < end; index += 3 )
	{
	  if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3] & CLIP_ALL_BITS )
	  {
	    continue;
	  }
	  else if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] )
	  {
	    VList[0] = v1;
	    VList[1] = v2;
	    VList[2] = v3;
	    RenderClippedPolygon( ctx, 3, VList );
	    continue;
	  }

	  /* Compute orientation of triangle */
	  ex = VB->Win[v2][0] - VB->Win[v1][0];
	  ey = VB->Win[v2][1] - VB->Win[v1][1];
	  fx = VB->Win[v3][0] - VB->Win[v1][0];
	  fy = VB->Win[v3][1] - VB->Win[v1][1];
	  c = (ex * fy) - (ey * fx);

	  /* polygon is perpindicular to view plane, don't draw it */
	  if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	    continue;

	  /* Backface culling. */
	  facing = (c < 0.0F) ^ ctx->Polygon.FrontBit;
	  if ( (facing + 1) & ctx->Polygon.CullBits )
	    continue;

	  if ( ctx->IndirectTriangles & DD_TRI_LIGHT_TWOSIDE )
	  {
	    if ( facing == 1 )
	    {
		 /* use back color */
		 VB->Color   = VB->Bcolor;
		 VB->Specular= VB->Bspec;
	    }
	    else
	    {
		 /* use front color */
		 VB->Color   = VB->Fcolor;
		 VB->Specular= VB->Fspec;
	    }
	  }

	  if ( ctx->IndirectTriangles & DD_TRI_OFFSET )
	  {
	    /* Finish computing plane equation of polygon, compute offset */
	    GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];
	    GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
	    GLfloat a = (ey * fz) - (ez * fy);
	    GLfloat b = (ez * fx) - (ex * fz);
	    OffsetPolygon( ctx, a, b, c );
	  }

	  /*=====================================*/
	  /* Populate the the triangle vertices. */
	  /*=====================================*/
	  /* Solve the prevoking vertex color as we need it for the 3rd triangle and flat shading. */
	  dwPVColor = (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v1][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v1][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v1][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v1][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v1][3]) );
	  D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v2][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v2][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v2][2] + ctx->PolygonZoffset) );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v2][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v2][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v2][3]) );
	  D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v3][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v3][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v3][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v3][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v3][3]) );
	  D3DTLVertices[cVertex++].color= dwPVColor;
	}
#undef v1
#undef v2
#undef v3
   }

   /* Render the converted vertex buffer. */
   if ( cVertex > 2 )
	DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLELIST, &D3DTLVertices[0], cVertex );
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as a triangle fan.   */
/* The buffer has to be able to be rendered directly.  This means that we are*/
/* filled, no offsets, no culling and one sided rendering.  Also we must be  */
/* in render mode of course.                                                 */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderTriangleFanVB( GLcontext *ctx, GLuint start, GLuint end )
{
  D3DMESACONTEXT     	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int                  	index,
                         cVertex,
                         height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
   GLfloat              	ex, ey,
                         fx, fy, c;
   GLuint               	facing;
   DWORD                	dwPVColor;

   DPF(( DBG_FUNC, "RenderTriangleFanVB();" ));

   /* Special case that we can blast the fan without culling, offset, etc... */
   if ( !VB->ClipOrMask && (ctx->Light.ShadeModel != GL_FLAT) )
   {
	DPF(( DBG_PRIM_INFO, "DirectTriangles( %d )", (end-start) ));

	/* Seed the the fan. */
	D3DTLVertices[0].sx   = D3DVAL( VB->Win[start][0] );
	D3DTLVertices[0].sy   = D3DVAL( (height - VB->Win[start][1]) );
	D3DTLVertices[0].sz   = D3DVAL( VB->Win[start][2] );
	D3DTLVertices[0].tu   = D3DVAL( VB->TexCoord[start][0] );
	D3DTLVertices[0].tv   = D3DVAL( VB->TexCoord[start][1] );
	D3DTLVertices[0].rhw  = D3DVAL( (1.0 / VB->Clip[start][3]) );
	D3DTLVertices[0].color= (VB->Color[start][3]<<24) | (VB->Color[start][0]<<16) | (VB->Color[start][1]<<8) | VB->Color[start][2];

	/* Seed the the fan. */
	D3DTLVertices[1].sx   = D3DVAL( VB->Win[(start+1)][0] );
	D3DTLVertices[1].sy   = D3DVAL( (height - VB->Win[(start+1)][1]) );
	D3DTLVertices[1].sz   = D3DVAL( VB->Win[(start+1)][2] );
	D3DTLVertices[1].tu   = D3DVAL( VB->TexCoord[(start+1)][0] );
	D3DTLVertices[1].tv   = D3DVAL( VB->TexCoord[(start+1)][1] );
	D3DTLVertices[1].rhw  = D3DVAL( (1.0 / VB->Clip[(start+1)][3]) );
	D3DTLVertices[1].color= (VB->Color[(start+1)][3]<<24) | (VB->Color[(start+1)][0]<<16) | (VB->Color[(start+1)][1]<<8) | VB->Color[(start+1)][2];

	for( index = (start+2), cVertex = 2; index < end; index++, cVertex++ )
	{
	  /*=================================*/
	  /* Add the next vertex to the fan. */
	  /*=================================*/
	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[index][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[index][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[index][2] );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[index][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[index][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[index][3]) );
	  D3DTLVertices[cVertex].color  = (VB->Color[index][3]<<24) | (VB->Color[index][0]<<16) | (VB->Color[index][1]<<8) | VB->Color[index][2];
      }

	/* Render the converted vertex buffer. */
	if ( cVertex )
	  DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLEFAN, &D3DTLVertices[0], cVertex );
   }
   else
   {
#define v1     start
#define v2     (index-1)
#define v3     index

	for( index = (start+2), cVertex = 0; index < end; index++ )
	{
	  if ( VB->ClipOrMask )
	  {
	    /* All points clipped by common plane */
	    if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3] & CLIP_ALL_BITS )
	    {
		 continue;
	    }
	    else if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] )
	    {
		 VList[0] = v1;
		 VList[1] = v2;
		 VList[2] = v3;
		 RenderClippedPolygon( ctx, 3, VList );
		 continue;
	    }
	  }

	  /* Compute orientation of triangle */
	  ex = VB->Win[v2][0] - VB->Win[v1][0];
	  ey = VB->Win[v2][1] - VB->Win[v1][1];
	  fx = VB->Win[v3][0] - VB->Win[v1][0];
	  fy = VB->Win[v3][1] - VB->Win[v1][1];
	  c = (ex * fy) - (ey * fx);

	  /* polygon is perpindicular to view plane, don't draw it */
	  if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	    continue;

	  /* Backface culling. */
	  facing = (c < 0.0F) ^ ctx->Polygon.FrontBit;
	  if ( (facing + 1) & ctx->Polygon.CullBits )
	    continue;

	  if ( ctx->IndirectTriangles & DD_TRI_OFFSET )
	  {
	    /* Finish computing plane equation of polygon, compute offset */
	    GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];
	    GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
	    GLfloat a = (ey * fz) - (ez * fy);
	    GLfloat b = (ez * fx) - (ex * fz);
	    OffsetPolygon( ctx, a, b, c );
	  }

	  /*=====================================*/
	  /* Populate the the triangle vertices. */
	  /*=====================================*/
	  dwPVColor = (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v1][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v1][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v1][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v1][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v1][3]) );
	  D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                                         (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v2][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v2][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v2][2] + ctx->PolygonZoffset)  );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v2][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v2][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v2][3]) );
	  D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                                         (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];

	  D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v3][0] );
	  D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v3][1]) );
	  D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
	  D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v3][0] );
	  D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v3][1] );
	  D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v3][3]) );
	  D3DTLVertices[cVertex++].color= dwPVColor;
	}

	/* Render the converted vertex buffer. */
	if ( cVertex )
	  DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLELIST, &D3DTLVertices[0], cVertex );
#undef v1
#undef v2
#undef v3
   }
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as a triangle strip. */
/* The buffer has to be able to be rendered directly.  This means that we are*/
/* filled, no offsets, no culling and one sided rendering.  Also we must be  */
/* in render mode of course.                                                 */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderTriangleStripVB( GLcontext *ctx, GLuint start, GLuint end )
{
  D3DMESACONTEXT     	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int                    index,
                         cVertex = 0,
	                    v1, v2, v3,
                         height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
  GLfloat              	ex, ey,
                         fx, fy, c;
  GLuint               	facing;
  DWORD                	dwPVColor;

  DPF(( DBG_FUNC, "RenderTriangleStripVB();" ));

  /* Special case that we can blast the fan without culling, offset, etc... */
  if ( !VB->ClipOrMask && (ctx->Light.ShadeModel != GL_FLAT) )
  {
    DPF(( DBG_PRIM_PROFILE, "DirectTriangles" ));

    /* Seed the the strip. */
    D3DTLVertices[0].sx   = D3DVAL( VB->Win[start][0] );
    D3DTLVertices[0].sy   = D3DVAL( (height - VB->Win[start][1]) );
    D3DTLVertices[0].sz   = D3DVAL( VB->Win[start][2] );
    D3DTLVertices[0].tu   = D3DVAL( VB->TexCoord[start][0] );
    D3DTLVertices[0].tv   = D3DVAL( VB->TexCoord[start][1] );
    D3DTLVertices[0].rhw  = D3DVAL( (1.0 / VB->Clip[start][3]) );
    D3DTLVertices[0].color= (VB->Color[start][3]<<24) | (VB->Color[start][0]<<16) | (VB->Color[start][1]<<8) | VB->Color[start][2];

    /* Seed the the strip. */
    D3DTLVertices[1].sx   = D3DVAL( VB->Win[(start+1)][0] );
    D3DTLVertices[1].sy   = D3DVAL( (height - VB->Win[(start+1)][1]) );
    D3DTLVertices[1].sz   = D3DVAL( VB->Win[(start+1)][2] );
    D3DTLVertices[1].tu   = D3DVAL( VB->TexCoord[(start+1)][0] );
    D3DTLVertices[1].tv   = D3DVAL( VB->TexCoord[(start+1)][1] );
    D3DTLVertices[1].rhw  = D3DVAL( (1.0 / VB->Clip[(start+1)][3]) );
    D3DTLVertices[1].color= (VB->Color[(start+1)][3]<<24) | (VB->Color[(start+1)][0]<<16) | (VB->Color[(start+1)][1]<<8) | VB->Color[(start+1)][2];

    for( index = (start+2), cVertex = 2; index < end; index++, cVertex++ )
    {
	 /*===================================*/
	 /* Add the next vertex to the strip. */
	 /*===================================*/
	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[index][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[index][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[index][2] );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[index][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[index][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[index][3]) );
	 D3DTLVertices[cVertex].color  = (VB->Color[index][3]<<24) | (VB->Color[index][0]<<16) | (VB->Color[index][1]<<8) | VB->Color[index][2];
    }

    /* Render the converted vertex buffer. */
    if ( cVertex )
	 DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLESTRIP, &D3DTLVertices[0], cVertex );
  }
  else
  {
    for( index = (start+2); index < end; index++ )
    {
	 /* We need to switch order so that winding won't be a problem. */
	 if ( index & 1 )
	 {
	   v1 = index - 1;
	   v2 = index - 2;
	   v3 = index - 0;
	 }
	 else
	 {
	   v1 = index - 2;
	   v2 = index - 1;
	   v3 = index - 0;
	 }

	 /* All vertices clipped by common plane */
	 if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3] & CLIP_ALL_BITS )
	   continue;

	 /* Check if any vertices need clipping. */
	 if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] )
	 {
	   VList[0] = v1;
	   VList[1] = v2;
	   VList[2] = v3;
	   RenderClippedPolygon( ctx, 3, VList );
	 }
	 else
	 {
	   /* Compute orientation of triangle */
	   ex = VB->Win[v2][0] - VB->Win[v1][0];
	   ey = VB->Win[v2][1] - VB->Win[v1][1];
	   fx = VB->Win[v3][0] - VB->Win[v1][0];
	   fy = VB->Win[v3][1] - VB->Win[v1][1];
	   c = (ex * fy) - (ey * fx);

	   /* Polygon is perpindicular to view plane, don't draw it */
	   if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
		continue;

	   /* Backface culling. */
	   facing = (c < 0.0F) ^ ctx->Polygon.FrontBit;
	   if ( (facing + 1) & ctx->Polygon.CullBits )
		continue;

	   /* Need right color if we have two sided lighting. */
	   if ( ctx->IndirectTriangles & DD_TRI_LIGHT_TWOSIDE )
	   {
		if ( facing == 1 )
		{
		  /* use back color */
		  VB->Color   = VB->Bcolor;
		  VB->Specular= VB->Bspec;
		}
		else
		{
		  /* use front color */
		  VB->Color   = VB->Fcolor;
		  VB->Specular= VB->Fspec;
		}
	   }

	   if ( ctx->IndirectTriangles & DD_TRI_OFFSET )
	   {
		/* Finish computing plane equation of polygon, compute offset */
		GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];
		GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
		GLfloat a = (ey * fz) - (ez * fy);
		GLfloat b = (ez * fx) - (ex * fz);
		OffsetPolygon( ctx, a, b, c );
	   }
	   /*=====================================*/
	   /* Populate the the triangle vertices. */
	   /*=====================================*/

	   /* Solve the prevoking vertex color as we need it for the 3rd triangle and flat shading. */
	   dwPVColor = (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

	   D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v1][0] );
	   D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v1][1]) );
	   D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
	   D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v1][0] );
	   D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v1][1] );
	   D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v1][3]) );
	   D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                        dwPVColor :
                                        (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

	   D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v2][0] );
	   D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v2][1]) );
	   D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v2][2] + ctx->PolygonZoffset) );
	   D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v2][0] );
	   D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v2][1] );
	   D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v2][3]) );
	   D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                          dwPVColor :
                                          (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];

	   D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v3][0] );
	   D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v3][1]) );
	   D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
	   D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v3][0] );
	   D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v3][1] );
	   D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v3][3]) );
	   D3DTLVertices[cVertex++].color= dwPVColor;
	 }
    }

    /* Render the converted vertex buffer. */
    if ( cVertex )	
	 DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLELIST, &D3DTLVertices[0], cVertex );
  }	
}
/*===========================================================================*/
/*  This function will render the current vertex buffer as Quads.  The buffer*/
/* has to be able to be rendered directly.  This means that we are filled, no*/
/* offsets, no culling and one sided rendering.  Also we must be in render   */
/* mode of cource.                                                           */
/*  First I will fill the global D3D vertice buffer.  Next I will set all the*/
/* states for D3D based on the current OGL state.  Finally I pass the D3D VB */
/* to the wrapper that call DrawPrimitives.                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderQuadVB( GLcontext *ctx, GLuint start, GLuint end )
{
  D3DMESACONTEXT       	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer	*VB = ctx->VB;
  int                  	index,
                         cVertex,
                         height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
  DWORD                	dwPVColor;
  GLfloat             	ex, ey,
                         fx, fy, c;
  GLuint				facing;  /* 0=front, 1=back */

  DPF(( DBG_FUNC, "RenderQuadVB();" ));

#define  v1 (index)
#define  v2 (index+1)
#define  v3 (index+2)
#define  v4 (index+3)

  if ( !VB->ClipOrMask )
  {
    DPF(( DBG_PRIM_PROFILE, "DirectTriangles" ));

    for( cVertex = 0, index = start; index < end; index += 4 )
    {
	 if ( ctx->Light.ShadeModel == GL_FLAT )
	   dwPVColor = (VB->Color[v4][3]<<24) | (VB->Color[v4][0]<<16) | (VB->Color[v4][1]<<8) | VB->Color[v4][2];

	 /*=====================================*/
	 /* Populate the the triangle vertices. */
	 /*=====================================*/
	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v1][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v1][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[v1][2] );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v1][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v1][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v1][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                      dwPVColor :
	                                 (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v2][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v2][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[v2][2] );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v2][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v2][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v2][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                      dwPVColor :
                                      (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v3][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v3][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[v3][2] );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v3][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v3][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v3][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
	                                 dwPVColor :
                                      (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v1][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v1][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[v1][2] );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v1][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v1][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v1][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
	                                 dwPVColor :
                                      (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v3][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v3][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[v3][2] );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v3][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v3][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v3][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                      dwPVColor :
                                      (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v4][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v4][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( VB->Win[v4][2] );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v4][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v4][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v4][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                      dwPVColor :
                                      (VB->Color[v4][3]<<24) | (VB->Color[v4][0]<<16) | (VB->Color[v4][1]<<8) | VB->Color[v4][2];
    }
  }
  else
  {
    for( cVertex = 0, index = start; index < end; index += 4 )
    {
	 if ( VB->ClipMask[v1] & VB->ClipMask[v2] & VB->ClipMask[v3]  & VB->ClipMask[v4] & CLIP_ALL_BITS )
	 {
	   continue;
	 }	
	 else if ( VB->ClipMask[v1] | VB->ClipMask[v2] | VB->ClipMask[v3] | VB->ClipMask[v4] )
	 {
	   VList[0] = v1;
	   VList[1] = v2;
	   VList[2] = v3;
	   VList[3] = v4;
	   RenderClippedPolygon( ctx, 4, VList );
	   continue;
	 }

	 /* Compute orientation of triangle */
	 ex = VB->Win[v2][0] - VB->Win[v1][0];
	 ey = VB->Win[v2][1] - VB->Win[v1][1];
	 fx = VB->Win[v3][0] - VB->Win[v1][0];
	 fy = VB->Win[v3][1] - VB->Win[v1][1];
	 c = (ex * fy) - (ey * fx);

	 /* polygon is perpindicular to view plane, don't draw it */
	 if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
	   continue;

	 /* Backface culling. */
	 facing = (c < 0.0F) ^ ctx->Polygon.FrontBit;
	 if ( (facing + 1) & ctx->Polygon.CullBits )
	   continue;

	 if ( ctx->IndirectTriangles & DD_TRI_LIGHT_TWOSIDE )
	 {
	   if ( facing == 1 )
	   {
		/* use back color */
		VB->Color   = VB->Bcolor;
		VB->Specular= VB->Bspec;
	   }
	   else
	   {
		/* use front color */
		VB->Color   = VB->Fcolor;
		VB->Specular= VB->Fspec;
	   }	
	 }	

	 if ( ctx->IndirectTriangles & DD_TRI_OFFSET )
	 {
	   /* Finish computing plane equation of polygon, compute offset */
	   GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];
	   GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
	   GLfloat a = (ey * fz) - (ez * fy);
	   GLfloat b = (ez * fx) - (ex * fz);
	   OffsetPolygon( ctx, a, b, c );
	 }

	 if ( ctx->Light.ShadeModel == GL_FLAT )
	   dwPVColor = (VB->Color[v4][3]<<24) | (VB->Color[v4][0]<<16) | (VB->Color[v4][1]<<8) | VB->Color[v4][2];

	 /*=====================================*/
	 /* Populate the the triangle vertices. */
	 /*=====================================*/
	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v1][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v1][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v1][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v1][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v1][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v2][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v2][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v2][2] + ctx->PolygonZoffset) );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v2][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v2][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v2][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v3][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v3][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v3][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v3][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v3][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v1][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v1][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v1][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v1][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v1][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
	                                  dwPVColor :
                                       (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v3][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v3][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v3][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v3][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v3][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

	 D3DTLVertices[cVertex].sx     = D3DVAL( VB->Win[v4][0] );
	 D3DTLVertices[cVertex].sy     = D3DVAL( (height - VB->Win[v4][1]) );
	 D3DTLVertices[cVertex].sz     = D3DVAL( (VB->Win[v4][2] + ctx->PolygonZoffset) );
	 D3DTLVertices[cVertex].tu     = D3DVAL( VB->TexCoord[v4][0] );
	 D3DTLVertices[cVertex].tv     = D3DVAL( VB->TexCoord[v4][1] );
	 D3DTLVertices[cVertex].rhw    = D3DVAL( (1.0 / VB->Clip[v4][3]) );
	 D3DTLVertices[cVertex++].color= (ctx->Light.ShadeModel == GL_FLAT) ?
                                       dwPVColor :
                                       (VB->Color[v4][3]<<24) | (VB->Color[v4][0]<<16) | (VB->Color[v4][1]<<8) | VB->Color[v4][2];
    }
  }

#undef   v4
#undef   v3
#undef   v2
#undef   v1

  /* Render the converted vertex buffer. */
  if ( cVertex )
    DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLELIST, &D3DTLVertices[0], cVertex );
}
/*===========================================================================*/
/*                                                                           */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static void RenderQuad( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint v4, GLuint pv )
{
  D3DMESACONTEXT     	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int                  	height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
  DWORD                	dwPVColor;
  GLfloat              	ex, ey,
                         fx, fy, c;
  GLuint               	facing;  /* 0=front, 1=back */
  static D3DTLVERTEX   	TLVertices[6];

  DPF(( DBG_FUNC, "RenderQuad" ));
  DPF(( DBG_PRIM_INFO, "RenderQuad( 1 )" ));

  /* Compute orientation of triangle */
  ex = VB->Win[v2][0] - VB->Win[v1][0];
  ey = VB->Win[v2][1] - VB->Win[v1][1];
  fx = VB->Win[v3][0] - VB->Win[v1][0];
  fy = VB->Win[v3][1] - VB->Win[v1][1];
  c = (ex * fy) - (ey * fx);

  /* polygon is perpindicular to view plane, don't draw it */
  if ( (c == 0.0F) && !ctx->Polygon.Unfilled )
    return;

  /* Backface culling. */
  facing = (c < 0.0F) ^ ctx->Polygon.FrontBit;
  if ( (facing + 1) & ctx->Polygon.CullBits )
    return;

  if ( ctx->IndirectTriangles & DD_TRI_LIGHT_TWOSIDE )
  {
    if ( facing == 1 )
    {
	 /* use back color */
	 VB->Color   = VB->Bcolor;
	 VB->Specular= VB->Bspec;
    }
    else
    {
	 /* use front color */
	 VB->Color   = VB->Fcolor;
	 VB->Specular= VB->Fspec;
    }
  }

  if ( ctx->IndirectTriangles & DD_TRI_OFFSET )
  {
    /* Finish computing plane equation of polygon, compute offset */
    GLfloat fz = VB->Win[v3][2] - VB->Win[v1][2];
    GLfloat ez = VB->Win[v2][2] - VB->Win[v1][2];
    GLfloat a = (ey * fz) - (ez * fy);
    GLfloat b = (ez * fx) - (ex * fz);
    OffsetPolygon( ctx, a, b, c );
  }

  if ( ctx->Light.ShadeModel == GL_FLAT )
    dwPVColor = (VB->Color[pv][3]<<24) | (VB->Color[pv][0]<<16) | (VB->Color[pv][1]<<8) | VB->Color[pv][2];

  /*=====================================*/
  /* Populate the the triangle vertices. */
  /*=====================================*/
  TLVertices[0].sx      = D3DVAL( VB->Win[v1][0] );
  TLVertices[0].sy      = D3DVAL( (height - VB->Win[v1][1]) );
  TLVertices[0].sz      = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
  TLVertices[0].tu      = D3DVAL( VB->TexCoord[v1][0] );
  TLVertices[0].tv      = D3DVAL( VB->TexCoord[v1][1] );
  TLVertices[0].rhw     = D3DVAL( (1.0 / VB->Clip[v1][3]) );
  TLVertices[0].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

   TLVertices[1].sx      = D3DVAL( VB->Win[v2][0] );
   TLVertices[1].sy      = D3DVAL( (height - VB->Win[v2][1]) );
   TLVertices[1].sz      = D3DVAL( (VB->Win[v2][2] + ctx->PolygonZoffset) );
   TLVertices[1].tu      = D3DVAL( VB->TexCoord[v2][0] );
   TLVertices[1].tv      = D3DVAL( VB->TexCoord[v2][1] );
   TLVertices[1].rhw     = D3DVAL( (1.0 / VB->Clip[v2][3]) );
   TLVertices[1].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];

   TLVertices[2].sx      = D3DVAL( VB->Win[v3][0] );
   TLVertices[2].sy      = D3DVAL( (height - VB->Win[v3][1]) );
   TLVertices[2].sz      = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
   TLVertices[2].tu      = D3DVAL( VB->TexCoord[v3][0] );
   TLVertices[2].tv      = D3DVAL( VB->TexCoord[v3][1] );
   TLVertices[2].rhw     = D3DVAL( (1.0 / VB->Clip[v3][3]) );
   TLVertices[2].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

   TLVertices[3].sx      = D3DVAL( VB->Win[v3][0] );
   TLVertices[3].sy      = D3DVAL( (height - VB->Win[v3][1]) );
   TLVertices[3].sz      = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
   TLVertices[3].tu      = D3DVAL( VB->TexCoord[v3][0] );
   TLVertices[3].tv      = D3DVAL( VB->TexCoord[v3][1] );
   TLVertices[3].rhw     = D3DVAL( (1.0 / VB->Clip[v3][3]) );
   TLVertices[3].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];

   TLVertices[4].sx      = D3DVAL( VB->Win[v4][0] );
   TLVertices[4].sy      = D3DVAL( (height - VB->Win[v4][1]) );
   TLVertices[4].sz      = D3DVAL( (VB->Win[v4][2] + ctx->PolygonZoffset) );
   TLVertices[4].tu      = D3DVAL( VB->TexCoord[v4][0] );
   TLVertices[4].tv      = D3DVAL( VB->TexCoord[v4][1] );
   TLVertices[4].rhw     = D3DVAL( (1.0 / VB->Clip[v4][3]) );
   TLVertices[4].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v4][3]<<24) | (VB->Color[v4][0]<<16) | (VB->Color[v4][1]<<8) | VB->Color[v4][2];

   TLVertices[5].sx      = D3DVAL( VB->Win[v1][0] );
   TLVertices[5].sy      = D3DVAL( (height - VB->Win[v1][1]) );
   TLVertices[5].sz      = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
   TLVertices[5].tu      = D3DVAL( VB->TexCoord[v1][0] );
   TLVertices[5].tv      = D3DVAL( VB->TexCoord[v1][1] );
   TLVertices[5].rhw     = D3DVAL( (1.0 / VB->Clip[v1][3]) );
   TLVertices[5].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

   /* Draw the two triangles. */
   DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLELIST, &TLVertices[0], 6 );
}
/*===========================================================================*/
/*                                                                           */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void	RenderOneTriangle( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint pv )
{
  D3DMESACONTEXT       	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int                  	height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
  DWORD                	dwPVColor;
  static D3DTLVERTEX   	TLVertices[3];

  DPF(( DBG_FUNC, "RenderOneTriangle" ));
  DPF(( DBG_PRIM_INFO, "RenderTriangle( 1 )" ));

  /*=====================================*/
  /* Populate the the triangle vertices. */
  /*=====================================*/
  if ( ctx->Light.ShadeModel == GL_FLAT )
    dwPVColor = (VB->Color[pv][3]<<24) | (VB->Color[pv][0]<<16) | (VB->Color[pv][1]<<8) | VB->Color[pv][2];

  TLVertices[0].sx      = D3DVAL( VB->Win[v1][0] );
  TLVertices[0].sy      = D3DVAL( (height - VB->Win[v1][1]) );
  TLVertices[0].sz      = D3DVAL( (VB->Win[v1][2] + ctx->PolygonZoffset) );
  TLVertices[0].tu      = D3DVAL( VB->TexCoord[v1][0] );
  TLVertices[0].tv      = D3DVAL( VB->TexCoord[v1][1] );
  TLVertices[0].rhw     = D3DVAL( (1.0 / VB->Clip[v1][3]) );
  TLVertices[0].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                          (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];
  DPF(( DBG_PRIM_INFO, "V1 -> x:%f y:%f z:%f c:%x",
	   TLVertices[0].sx,
	   TLVertices[0].sy,
	   TLVertices[0].sz,
	   TLVertices[0].color ));

  TLVertices[1].sx      = D3DVAL( VB->Win[v2][0] );
  TLVertices[1].sy      = D3DVAL( (height - VB->Win[v2][1]) );
  TLVertices[1].sz      = D3DVAL( (VB->Win[v2][2] + ctx->PolygonZoffset) );
  TLVertices[1].tu      = D3DVAL( VB->TexCoord[v2][0] );
  TLVertices[1].tv      = D3DVAL( VB->TexCoord[v2][1] );
  TLVertices[1].rhw     = D3DVAL( (1.0 / VB->Clip[v2][3]) );
  TLVertices[1].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                          (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];
  DPF(( DBG_PRIM_INFO, "V2 -> x:%f y:%f z:%f c:%x",
	   TLVertices[1].sx,
	   TLVertices[1].sy,
	   TLVertices[1].sz,
	   TLVertices[1].color ));

  TLVertices[2].sx      = D3DVAL( VB->Win[v3][0] );
  TLVertices[2].sy      = D3DVAL( (height - VB->Win[v3][1]) );
  TLVertices[2].sz      = D3DVAL( (VB->Win[v3][2] + ctx->PolygonZoffset) );
  TLVertices[2].tu      = D3DVAL( VB->TexCoord[v3][0] );
  TLVertices[2].tv      = D3DVAL( VB->TexCoord[v3][1] );
  TLVertices[2].rhw     = D3DVAL( (1.0 / VB->Clip[v3][3]) );
  TLVertices[2].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                          (VB->Color[v3][3]<<24) | (VB->Color[v3][0]<<16) | (VB->Color[v3][1]<<8) | VB->Color[v3][2];
  DPF(( DBG_PRIM_INFO, "V3 -> x:%f y:%f z:%f c:%x",
	   TLVertices[2].sx,
	   TLVertices[2].sy,
	   TLVertices[2].sz,
	   TLVertices[2].color ));

  /* Draw the triangle. */
  DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLELIST, &TLVertices[0], 3 );
}
/*===========================================================================*/
/*                                                                           */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
void RenderOneLine( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
  D3DMESACONTEXT     	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  int                  	height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
  DWORD                	dwPVColor;
  static D3DTLVERTEX	TLVertices[2];

  DPF(( DBG_FUNC, "RenderOneLine" ));
  DPF(( DBG_PRIM_INFO, "RenderLine( 1 )" ));

  if ( VB->MonoColor )
    dwPVColor = (pContext->aCurrent<<24) | (pContext->rCurrent<<16) | (pContext->gCurrent<<8) | pContext->bCurrent;
  else 	
    dwPVColor = (VB->Color[pv][3]<<24) | (VB->Color[pv][0]<<16) | (VB->Color[pv][1]<<8) | VB->Color[pv][2];

   TLVertices[0].sx      = D3DVAL( VB->Win[v1][0] );
   TLVertices[0].sy      = D3DVAL( (height - VB->Win[v1][1]) );
   TLVertices[0].sz      = D3DVAL( (VB->Win[v1][2] + ctx->LineZoffset) );
   TLVertices[0].tu      = D3DVAL( VB->TexCoord[v1][0] );
   TLVertices[0].tv      = D3DVAL( VB->TexCoord[v1][1] );
   TLVertices[0].rhw     = D3DVAL( (1.0 / VB->Clip[v1][3]) );
   TLVertices[0].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v1][3]<<24) | (VB->Color[v1][0]<<16) | (VB->Color[v1][1]<<8) | VB->Color[v1][2];

   TLVertices[1].sx      = D3DVAL( VB->Win[v2][0] );
   TLVertices[1].sy      = D3DVAL( (height - VB->Win[v2][1]) );
   TLVertices[1].sz      = D3DVAL( (VB->Win[v2][2] + ctx->LineZoffset) );
   TLVertices[1].tu      = D3DVAL( VB->TexCoord[v2][0] );
   TLVertices[1].tv      = D3DVAL( VB->TexCoord[v2][1] );
   TLVertices[1].rhw     = D3DVAL( (1.0 / VB->Clip[v2][3]) );
   TLVertices[1].color   = (ctx->Light.ShadeModel == GL_FLAT) ? dwPVColor :
                           (VB->Color[v2][3]<<24) | (VB->Color[v2][0]<<16) | (VB->Color[v2][1]<<8) | VB->Color[v2][2];

   /* Draw line from (x0,y0) to (x1,y1) with current pixel color/index */
   DrawPrimitiveHAL( pContext->pShared, D3DPT_LINELIST, &TLVertices[0], 2 );
}
/*===========================================================================*/
/*  This function was written to convert points into triangles.  I did this  */
/* as all card accelerate triangles and most drivers do this anyway.  In hind*/
/* thought this might be a bad idea as some cards do better.                 */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void RenderPointsVB( GLcontext *ctx, GLuint start, GLuint end )
{
  D3DMESACONTEXT 		*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  struct vertex_buffer 	*VB = ctx->VB;
  struct pixel_buffer 	*PB = ctx->PB;
  GLuint 				index;
  GLfloat                radius, z,
                         xmin, ymin,
                         xmax, ymax;
  GLint				cVertex,
                         height = (pContext->pShared->rectW.bottom - pContext->pShared->rectW.top);
  DWORD				dwPVColor;

  DPF(( DBG_FUNC, "RenderPointsVB();" ));

  radius = CLAMP( ctx->Point.Size, MIN_POINT_SIZE, MAX_POINT_SIZE ) * 0.5F;

  for( index = start, cVertex = 0; index <= end; index++ )
  {
    if ( VB->ClipMask[index] == 0 )
    {
	 xmin = D3DVAL( VB->Win[index][0] - radius );
	 xmax = D3DVAL( VB->Win[index][0] + radius );
	 ymin = D3DVAL( height - VB->Win[index][1] - radius );
	 ymax = D3DVAL( height - VB->Win[index][1] + radius );
	 z    = D3DVAL( (VB->Win[index][2] + ctx->PointZoffset) );

	 dwPVColor = (VB->Color[index][3]<<24) |
	             (VB->Color[index][0]<<16) |
	             (VB->Color[index][1]<<8) |
                  VB->Color[index][2];

	 D3DTLVertices[cVertex].sx	  = xmin;
	 D3DTLVertices[cVertex].sy      = ymax;
	 D3DTLVertices[cVertex].sz      = z;
	 D3DTLVertices[cVertex].tu      = 0.0;
	 D3DTLVertices[cVertex].tv      = 0.0;
	 D3DTLVertices[cVertex].rhw     = D3DVAL( (1.0 / VB->Clip[index][3]) );
	 D3DTLVertices[cVertex++].color = dwPVColor;

	 D3DTLVertices[cVertex].sx      = xmin;
	 D3DTLVertices[cVertex].sy      = ymin;
	 D3DTLVertices[cVertex].sz      = z;
	 D3DTLVertices[cVertex].tu      = 0.0;
	 D3DTLVertices[cVertex].tv      = 0.0;
	 D3DTLVertices[cVertex].rhw     = D3DVAL( (1.0 / VB->Clip[index][3]) );
	 D3DTLVertices[cVertex++].color = dwPVColor;

	 D3DTLVertices[cVertex].sx      = xmax;
	 D3DTLVertices[cVertex].sy      = ymin;
	 D3DTLVertices[cVertex].sz      = z;
	 D3DTLVertices[cVertex].tu      = 0.0;
	 D3DTLVertices[cVertex].tv      = 0.0;
	 D3DTLVertices[cVertex].rhw     = D3DVAL( (1.0 / VB->Clip[index][3]) );
	 D3DTLVertices[cVertex++].color = dwPVColor;

	 D3DTLVertices[cVertex].sx      = xmax;
	 D3DTLVertices[cVertex].sy      = ymin;
	 D3DTLVertices[cVertex].sz      = z;
	 D3DTLVertices[cVertex].tu      = 0.0;
	 D3DTLVertices[cVertex].tv      = 0.0;
	 D3DTLVertices[cVertex].rhw     = D3DVAL( (1.0 / VB->Clip[index][3]) );
	 D3DTLVertices[cVertex++].color = dwPVColor;

	 D3DTLVertices[cVertex].sx	  = xmax;
	 D3DTLVertices[cVertex].sy      = ymax;
	 D3DTLVertices[cVertex].sz      = z;
	 D3DTLVertices[cVertex].tu      = 0.0;
	 D3DTLVertices[cVertex].tv      = 0.0;
	 D3DTLVertices[cVertex].rhw     = D3DVAL( (1.0 / VB->Clip[index][3]) );
	 D3DTLVertices[cVertex++].color = dwPVColor;

	 D3DTLVertices[cVertex].sx	  = xmin;
	 D3DTLVertices[cVertex].sy      = ymax;
	 D3DTLVertices[cVertex].sz      = z;
	 D3DTLVertices[cVertex].tu      = 0.0;
	 D3DTLVertices[cVertex].tv      = 0.0;
	 D3DTLVertices[cVertex].rhw     = D3DVAL( (1.0 / VB->Clip[index][3]) );
	 D3DTLVertices[cVertex++].color = dwPVColor;
    }
  }

  /* Render the converted vertex buffer. */
  if ( cVertex )
    DrawPrimitiveHAL( pContext->pShared, D3DPT_TRIANGLELIST, &D3DTLVertices[0], cVertex );
}
/*===========================================================================*/
/*  This gets call before we render any primitives so that the current OGL   */
/* states will be mapped the D3D context.  I'm still not sure how D3D works  */
/* but I'm finding that it doesn't act like a state machine as OGL is.  It   */
/* looks like the state gets set back to the defaults after a DrawPrimitives */
/* or an EndScene.  Also I set states that are the default even though this  */
/* is redundant as the defaults seem screwed up.                             */
/* TODO: make a batch call.                                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void SetRenderStates( GLcontext *ctx )
{
   D3DMESACONTEXT	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DWORD          	dwFunc;
   static	BOOL		bTexture = FALSE;
   static int		texName = -1;

   DPF(( DBG_FUNC, "SetRenderStates();" ));

   if ( g_DBGMask & DBG_STATES )
	DebugRenderStates( ctx, FALSE );

   SetStateHAL( pContext->pShared, D3DRENDERSTATE_CULLMODE, D3DCULL_NONE );
   SetStateHAL( pContext->pShared, D3DRENDERSTATE_DITHERENABLE, (ctx->Color.DitherFlag) ? TRUE : FALSE );

   /*================================================*/
   /* Check too see if there are new TEXTURE states. */
   /*================================================*/
   if ( ctx->Texture._EnabledUnits )
   {
      switch( ctx->Texture.Set[ctx->Texture.CurrentSet].EnvMode )
      {
        case GL_MODULATE:
		if ( ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0][0]->Format == GL_RGBA )
		  dwFunc = pContext->pShared->dwTexFunc[d3dtblend_modulatealpha];
		else
		  dwFunc = pContext->pShared->dwTexFunc[d3dtblend_modulate];
		break;

	   case GL_BLEND:
		dwFunc = pContext->pShared->dwTexFunc[d3dtblend_decalalpha];
		break;

        case GL_REPLACE:
		dwFunc = pContext->pShared->dwTexFunc[d3dtblend_decal];
		break;

        case GL_DECAL:
		if ( ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0][0]->Format == GL_RGBA )
		  dwFunc = pContext->pShared->dwTexFunc[d3dtblend_decalalpha];
		else
		  dwFunc = pContext->pShared->dwTexFunc[d3dtblend_decal];
		break;
      }
      SetStateHAL( pContext->pShared, D3DRENDERSTATE_TEXTUREMAPBLEND, dwFunc );

      switch( ctx->Texture.Set[ctx->Texture.CurrentSet].Current->MagFilter )
      {
	   case GL_NEAREST:
		dwFunc = D3DFILTER_NEAREST;
		break;
        case GL_LINEAR:
		dwFunc = D3DFILTER_LINEAR;
		break;
        case GL_NEAREST_MIPMAP_NEAREST:
		dwFunc = D3DFILTER_MIPNEAREST;
		break;
        case GL_LINEAR_MIPMAP_NEAREST:
		dwFunc = D3DFILTER_LINEARMIPNEAREST;
		break;
        case GL_NEAREST_MIPMAP_LINEAR:
		dwFunc = D3DFILTER_MIPLINEAR;
		break;
        case GL_LINEAR_MIPMAP_LINEAR:
		dwFunc = D3DFILTER_LINEARMIPLINEAR;
		break;
      }
      SetStateHAL( pContext->pShared, D3DRENDERSTATE_TEXTUREMAG, dwFunc );

      switch( ctx->Texture.Set[ctx->Texture.CurrentSet].Current->MinFilter )
      {
	   case GL_NEAREST:
		dwFunc = D3DFILTER_NEAREST;
		break;
        case GL_LINEAR:
		dwFunc = D3DFILTER_LINEAR;
		break;
        case GL_NEAREST_MIPMAP_NEAREST:
		dwFunc = D3DFILTER_MIPNEAREST;
		break;
        case GL_LINEAR_MIPMAP_NEAREST:
		dwFunc = D3DFILTER_LINEARMIPNEAREST;
		break;
        case GL_NEAREST_MIPMAP_LINEAR:
		dwFunc = D3DFILTER_MIPLINEAR;
		break;
        case GL_LINEAR_MIPMAP_LINEAR:
		dwFunc = D3DFILTER_LINEARMIPLINEAR;
		break;
      }
      SetStateHAL( pContext->pShared, D3DRENDERSTATE_TEXTUREMIN, dwFunc  );

	 /* Another hack to cut down on redundant texture binding. */
	 //	 if ( texName != ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Name )
	 //	 {
	   texName = ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Name;
	   CreateTMgrHAL( pContext->pShared,
				   texName,
				   0,
				   ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0][0]->Format,
				   (RECT *)NULL,
				   ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0][0]->Width,
				   ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0][0]->Height,
				   TM_ACTION_BIND,
				   (void *)ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0][0]->Data );
	   //	 }
	 bTexture = TRUE;
   }
   else
   {
	/* This is nasty but should cut down on the number of redundant calls. */
	if ( bTexture == TRUE )
	{
	  DisableTMgrHAL( pContext->pShared );
	  bTexture = FALSE;
	}
   }

   /*===============================================*/
   /* Check too see if there are new RASTER states. */
   /*===============================================*/

   // TODO: no concept of front & back.
   switch( ctx->Polygon.FrontMode )
   {
     case GL_POINT:
	  SetStateHAL( pContext->pShared, D3DRENDERSTATE_FILLMODE, D3DFILL_POINT );
	  break;
     case GL_LINE:
	  SetStateHAL( pContext->pShared, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME );
	  break;
     case GL_FILL:
	  SetStateHAL( pContext->pShared, D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID );
	  break;
   }

   /*************/
   /* Z-Buffer. */
   /*************/
   if ( ctx->Depth.Test == GL_TRUE )
   {
	switch( ctx->Depth.Func )
	{
	  case GL_NEVER:
	    dwFunc = D3DCMP_NEVER;
	    break;
	  case GL_LESS:
	    dwFunc = D3DCMP_LESS;
	    break;
  	  case GL_GEQUAL:
	    dwFunc = D3DCMP_GREATEREQUAL;
	    break;
  	  case GL_LEQUAL:
	    dwFunc = D3DCMP_LESSEQUAL;
	    break;
	  case GL_GREATER:
	    dwFunc = D3DCMP_GREATER;
	    break;
	  case GL_NOTEQUAL:
	    dwFunc = D3DCMP_NOTEQUAL;
	    break;
	  case GL_EQUAL:
	    dwFunc = D3DCMP_EQUAL;
	    break;
	  case GL_ALWAYS:
	    dwFunc = D3DCMP_ALWAYS;
	    break;
	}
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ZFUNC, dwFunc );
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ZENABLE, TRUE );
   }	
   else
   {
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ZENABLE, FALSE );
   }

   /*******************/
   /* Z-Write Enable. */
   /*******************/
   SetStateHAL( pContext->pShared, D3DRENDERSTATE_ZWRITEENABLE , (ctx->Depth.Mask == GL_TRUE) ? TRUE : FALSE );

   /***************/
   /* Alpha test. */
   /***************/
   if ( ctx->Color.AlphaEnabled == GL_TRUE )
   {
	switch( ctx->Color.AlphaFunc )
     {
	  case GL_NEVER:
	    dwFunc = D3DCMP_NEVER;
	    break;
	  case GL_LESS:
	    dwFunc = D3DCMP_LESS;
	    break;
  	  case GL_GEQUAL:
	    dwFunc = D3DCMP_GREATEREQUAL;
	    break;
  	  case GL_LEQUAL:
	    dwFunc = D3DCMP_LESSEQUAL;
	    break;
	  case GL_GREATER:
	    dwFunc = D3DCMP_GREATER;
	    break;
	  case GL_NOTEQUAL:
	    dwFunc = D3DCMP_NOTEQUAL;
	    break;
	  case GL_EQUAL:
	    dwFunc = D3DCMP_EQUAL;
	    break;
	  case GL_ALWAYS:
	    dwFunc = D3DCMP_ALWAYS;
	    break;
	}
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ALPHAFUNC , dwFunc );
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ALPHATESTENABLE, TRUE );
   }
   else
   {
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ALPHATESTENABLE, FALSE );
   }

   /****************/
   /* Alpha blend. */
   /****************/
   if ( ctx->Color.BlendEnabled == GL_TRUE )
   {
	switch( ctx->Color.BlendSrc )
	{
	  case GL_ZERO:	    			
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_zero];
	    break;
       case GL_ONE:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_one];
	    break;
       case GL_DST_COLOR:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_dst_color];
	    break;
       case GL_ONE_MINUS_DST_COLOR:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_one_minus_dst_color];
	    break;
       case GL_SRC_ALPHA:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_src_alpha];
	    break;
       case GL_ONE_MINUS_SRC_ALPHA:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_one_minus_src_alpha];
	    break;
       case GL_DST_ALPHA:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_dst_alpha];
	    break;
       case GL_ONE_MINUS_DST_ALPHA:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_one_minus_dst_alpha];
	    break;
       case GL_SRC_ALPHA_SATURATE:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_src_alpha_saturate];
	    break;
       case GL_CONSTANT_COLOR:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_constant_color];
	    break;
       case GL_ONE_MINUS_CONSTANT_COLOR:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_one_minus_constant_color];
	    break;
       case GL_CONSTANT_ALPHA:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_constant_alpha];
	    break;
       case GL_ONE_MINUS_CONSTANT_ALPHA:
	    dwFunc = pContext->pShared->dwSrcBlendCaps[s_one_minus_constant_alpha];
	    break;
	}
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_SRCBLEND, dwFunc );

	switch( ctx->Color.BlendDst )
	{
       case GL_ZERO:                  	
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_zero];
	    break;
       case GL_ONE:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_one];
	    break;
       case GL_SRC_COLOR:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_src_color];
	    break;
       case GL_ONE_MINUS_SRC_COLOR:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_one_minus_src_color];
	    break;
       case GL_SRC_ALPHA:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_src_alpha];
	    break;
       case GL_ONE_MINUS_SRC_ALPHA:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_one_minus_src_alpha];
	    break;
       case GL_DST_ALPHA:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_dst_alpha];
	    break;
       case GL_ONE_MINUS_DST_ALPHA:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_one_minus_dst_alpha];
	    break;
       case GL_CONSTANT_COLOR:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_constant_color];
	    break;
       case GL_ONE_MINUS_CONSTANT_COLOR:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_one_minus_constant_color];
	    break;
       case GL_CONSTANT_ALPHA:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_constant_alpha];
	    break;
       case GL_ONE_MINUS_CONSTANT_ALPHA:
	    dwFunc = pContext->pShared->dwDestBlendCaps[d_one_minus_constant_alpha];
	    break;
	}
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_DESTBLEND, dwFunc );
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ALPHABLENDENABLE, TRUE );
   }
   else
   {
	SetStateHAL( pContext->pShared, D3DRENDERSTATE_ALPHABLENDENABLE, FALSE );
   }
}
/*===========================================================================*/
/*  If this function is called it will track the changes to the current      */
/* states that I'm setting in Direct3D.  I did this so that the DPF's would  */
/* be under control!                                                         */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void DebugRenderStates( GLcontext *ctx, BOOL bForce )
{
   D3DMESACONTEXT	*pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   DWORD			dwFunc;
   static int		dither = -1,
	               texture = -1,
                    textName = -1,
	               textEnv = -1,
                    textMin = -1,
                    textMag = -1,
                    polyMode = -1,
	               depthTest = -1,
	               depthFunc = -1,
	               depthMask = -1,
	               alphaTest = -1,
                    alphaFunc = -1,
	               blend = -1,
                    blendSrc = -1,
                    blendDest = -1;

   /* Force a displayed update of all current states. */
   if ( bForce )
   {
	dither = texture = textName = textEnv = textMin = textMag = -1;
	polyMode = depthTest = depthFunc = depthMask = -1;
	alphaTest = alphaFunc = blend = blendSrc = blendDest = -1;
   }

   if ( dither != ctx->Color.DitherFlag )
   {
	dither = ctx->Color.DitherFlag;
	DPF(( 0, "\tDither\t\t%s", (dither) ? "ENABLED" : "--------" ));
   }
   if ( depthTest != ctx->Depth.Test )
   {
	depthTest = ctx->Depth.Test;
	DPF(( 0, "\tDepth Test\t%s", (depthTest) ? "ENABLED" : "--------" ));
   }
   if ( alphaTest != ctx->Color.AlphaEnabled )
   {	
	alphaTest = ctx->Color.AlphaEnabled;
	
	DPF(( 0, "\tAlpha Test\t%s", (alphaTest) ? "ENABLED" : "--------" ));
   }
   if ( blend != ctx->Color.BlendEnabled )
   {	
	blend = ctx->Color.BlendEnabled;
	
	DPF(( 0, "\tBlending\t%s", (blend) ? "ENABLED" : "--------" ));
   }

   /*================================================*/
   /* Check too see if there are new TEXTURE states. */
   /*================================================*/
   if ( texture != ctx->Texture._EnabledUnits )
   {
	texture = ctx->Texture._EnabledUnits;
	DPF(( 0, "\tTexture\t\t%s", (texture) ? "ENABLED" : "--------" ));
   }	

   if ( ctx->Texture.Set[ctx->Texture.CurrentSet].Current )
   {
	if ( ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Name != textName )
	{
	  textName = ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Name;
	  DPF(( 0, "\tTexture Name:\t%d", textName ));
	  DPF(( 0, "\tTexture Format:\t%s",
		   (ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0][0]->Format == GL_RGBA) ?
		   "GL_RGBA" : "GLRGB" ));
	}

	if ( textEnv != ctx->Texture.Set[ctx->Texture.CurrentSet].EnvMode )
	{
	  textEnv = ctx->Texture.Set[ctx->Texture.CurrentSet].EnvMode;

	  switch( textEnv )
	  {
       case GL_MODULATE:
	    DPF(( 0, "\tTexture\tMode\tGL_MODULATE" ));
	    break;
       case GL_BLEND:
	    DPF(( 0, "\tTexture\tMode\tGL_BLEND" ));
	    break;
       case GL_REPLACE:
	    DPF(( 0, "\tTexture\tMode\tGL_REPLACE" ));
	    break;
       case GL_DECAL:
	    DPF(( 0, "\tTexture\tMode\tGL_DECAL" ));
	    break;
	  }
	}

	if ( textMag != ctx->Texture.Set[ctx->Texture.CurrentSet].Current->MagFilter )
	{
	  textMag = ctx->Texture.Set[ctx->Texture.CurrentSet].Current->MagFilter;
	
	  switch( textMag )
	  {
       case GL_NEAREST:
	    DPF(( 0, "\tTexture MAG\tGL_NEAREST" ));
	    break;
       case GL_LINEAR:
	    DPF(( 0, "\tTexture MAG\tGL_LINEAR" ));
	    break;
       case GL_NEAREST_MIPMAP_NEAREST:
	    DPF(( 0, "\tTexture MAG\tGL_NEAREST_MIPMAP_NEAREST" ));
	    break;
       case GL_LINEAR_MIPMAP_NEAREST:
	    DPF(( 0, "\tTexture MAG\tGL_LINEAR_MIPMAP_NEAREST" ));
	    break;
       case GL_NEAREST_MIPMAP_LINEAR:
	    DPF(( 0, "\tTexture MAG\tGL_NEAREST_MIPMAP_LINEAR" ));
	    break;
       case GL_LINEAR_MIPMAP_LINEAR:
	    DPF(( 0, "\tTexture MAG\tGL_LINEAR_MIPMAP_LINEAR" ));
	    break;
	  }
	}

	if ( textMin != ctx->Texture.Set[ctx->Texture.CurrentSet].Current->MinFilter )
	{
	  textMin = ctx->Texture.Set[ctx->Texture.CurrentSet].Current->MinFilter;

	  switch( textMin )
	  {
       case GL_NEAREST:
	    DPF(( 0, "\tTexture MIN\tGL_NEAREST" ));
	    break;
       case GL_LINEAR:
	    DPF(( 0, "\tTexture MIN\tGL_LINEAR" ));
	    break;
       case GL_NEAREST_MIPMAP_NEAREST:
	    DPF(( 0, "\tTexture MIN\tGL_NEAREST_MIPMAP_NEAREST" ));
	    break;
       case GL_LINEAR_MIPMAP_NEAREST:
	    DPF(( 0, "\tTexture MIN\tGL_LINEAR_MIPMAP_NEAREST" ));
	    break;
       case GL_NEAREST_MIPMAP_LINEAR:
	    DPF(( 0, "\tTexture MIN\tGL_LINEAR_MIPMAP_LINEAR" ));
	    break;
       case GL_LINEAR_MIPMAP_LINEAR:
	    DPF(( 0, "\tTexture MIN\tGL_LINEAR_MIPMAP_LINEAR" ));
	    break;
	  }
	}	
   }

   if ( ctx->Polygon.FrontMode != polyMode )
   {
	polyMode = ctx->Polygon.FrontMode;

	switch( polyMode )
	{
       case GL_POINT:
	    DPF(( 0, "\tMode\t\tGL_POINT" ));
	    break;
       case GL_LINE:
	    DPF(( 0, "\tMode\t\tGL_LINE" ));
	    break;
       case GL_FILL:
	    DPF(( 0, "\tMode\t\tGL_FILL" ));
	    break;
	}
   }

   if ( depthFunc != ctx->Depth.Func )
   {
	depthFunc = ctx->Depth.Func;

	switch( depthFunc )
	{
       case GL_NEVER:
	    DPF(( 0, "\tDepth Func\tGL_NEVER" ));
	    break;
       case GL_LESS:
	    DPF(( 0, "\tDepth Func\tGL_LESS" ));
	    break;
       case GL_GEQUAL:
	    DPF(( 0, "\tDepth Func\tGL_GEQUAL" ));
	    break;
       case GL_LEQUAL:
	    DPF(( 0, "\tDepth Func\tGL_LEQUAL" ));
	    break;
       case GL_GREATER:
	    DPF(( 0, "\tDepth Func\tGL_GREATER" ));
	    break;
       case GL_NOTEQUAL:
 	    DPF(( 0, "\tDepth Func\tGL_NOTEQUAL" ));
	    break;
       case GL_EQUAL:
	    DPF(( 0, "\tDepth Func\tGL_EQUAL" ));
	    break;
       case GL_ALWAYS:
	    DPF(( 0, "\tDepth Func\tGL_ALWAYS" ));
	    break;
	}
   }	

   if ( depthMask != ctx->Depth.Mask )
   {
	depthMask = ctx->Depth.Mask;
	DPF(( 0, "\tZWrite\t\t%s", (depthMask) ? "ENABLED" : "--------" ));
   }

   if ( alphaFunc != ctx->Color.AlphaFunc )
   {
	alphaFunc = ctx->Color.AlphaFunc;

	switch( alphaFunc )
     {
	  case GL_NEVER:
	    DPF(( 0, "\tAlpha Func\tGL_NEVER" ));
	    break;
	  case GL_LESS:
	    DPF(( 0, "\tAlpha Func\tGL_LESS" ));
	    break;
  	  case GL_GEQUAL:
	    DPF(( 0, "\tAlpha Func\tGL_GEQUAL" ));
	    break;
  	  case GL_LEQUAL:
	    DPF(( 0, "\tAlpha Func\tGL_LEQUAL" ));
	    break;
	  case GL_GREATER:
	    DPF(( 0, "\tAlpha Func\tGL_GREATER" ));
	    break;
	  case GL_NOTEQUAL:
	    DPF(( 0, "\tAlpha Func\tGL_NOTEQUAL" ));
	    break;
	  case GL_EQUAL:
	    DPF(( 0, "\tAlpha Func\tGL_EQUAL" ));
	    break;
	  case GL_ALWAYS:
	    DPF(( 0, "\tAlpha Func\tGL_ALWAYS" ));
	    break;
	}
   }

   if ( blendSrc != ctx->Color.BlendSrc )
   {
	blendSrc = ctx->Color.BlendSrc;

	switch( blendSrc )
	{
	  case GL_ZERO:	    			
	    DPF(( 0, "\tSRC Blend\tGL_ZERO" ));
	    break;
       case GL_ONE:
	    DPF(( 0, "\tSRC Blend\tGL_ONE" ));
	    break;
       case GL_DST_COLOR:
	    DPF(( 0, "\tSRC Blend\tGL_DST_COLOR" ));
	    break;
       case GL_ONE_MINUS_DST_COLOR:
	    DPF(( 0, "\tSRC Blend\tGL_ONE_MINUS_DST_COLOR" ));
	    break;
       case GL_SRC_ALPHA:
	    DPF(( 0, "\tSRC Blend\tGL_SRC_ALPHA" ));
	    break;
       case GL_ONE_MINUS_SRC_ALPHA:
	    DPF(( 0, "\tSRC Blend\tGL_MINUS_SRC_ALPHA" ));
	    break;
       case GL_DST_ALPHA:
	    DPF(( 0, "\tSRC Blend\tGL_DST_ALPHA" ));
	    break;
       case GL_ONE_MINUS_DST_ALPHA:
	    DPF(( 0, "\tSRC Blend\tGL_ONE_MINUS_DST_ALPHA" ));
	    break;
       case GL_SRC_ALPHA_SATURATE:
	    DPF(( 0, "\tSRC Blend\tGL_SRC_ALPHA_SATURATE" ));
	    break;
       case GL_CONSTANT_COLOR:
	    DPF(( 0, "\tSRC Blend\tGL_CONSTANT_COLOR" ));
	    break;
       case GL_ONE_MINUS_CONSTANT_COLOR:
	    DPF(( 0, "\tSRC Blend\tGL_ONE_MINUS_CONSTANT_COLOR" ));
	    break;
       case GL_CONSTANT_ALPHA:
	    DPF(( 0, "\tSRC Blend\tGL_CONSTANT_ALPHA" ));
	    break;
       case GL_ONE_MINUS_CONSTANT_ALPHA:
	    DPF(( 0, "\tSRC Blend\tGL_ONE_MINUS_CONSTANT_ALPHA" ));
	    break;
	}
   }

   if ( blendDest != ctx->Color.BlendDst )
   {
	blendDest = ctx->Color.BlendDst;

	switch( blendDest )
	{
       case GL_ZERO:                  	
	    DPF(( 0, "\tDST Blend\tGL_ZERO" ));
	    break;
       case GL_ONE:
	    DPF(( 0, "\tDST Blend\tGL_ONE" ));
	    break;
       case GL_SRC_COLOR:
	    DPF(( 0, "\tDST Blend\tGL_SRC_COLOR" ));
	    break;
       case GL_ONE_MINUS_SRC_COLOR:
	    DPF(( 0, "\tDST Blend\tGL_ONE_MINUS_SRC_COLOR" ));
	    break;
       case GL_SRC_ALPHA:
	    DPF(( 0, "\tDST Blend\tGL_SRC_ALPHA" ));
	    break;
       case GL_ONE_MINUS_SRC_ALPHA:
	    DPF(( 0, "\tDST Blend\tGL_ONE_MINUS_SRC_ALPHA" ));
	    break;
       case GL_DST_ALPHA:
	    DPF(( 0, "\tDST Blend\tGL_DST_ALPHA" ));
	    break;
       case GL_ONE_MINUS_DST_ALPHA:
	    DPF(( 0, "\tDST Blend\tGL_ONE_MINUS_DST_ALPHA" ));
	    break;
       case GL_CONSTANT_COLOR:
	    DPF(( 0, "\tDST Blend\tGL_CONSTANT_COLOR" ));
	    break;
       case GL_ONE_MINUS_CONSTANT_COLOR:
	    DPF(( 0, "\tDST Blend\tGL_ONE_MINUS_CONSTANT_COLOR" ));
	    break;
       case GL_CONSTANT_ALPHA:
	    DPF(( 0, "\tDST Blend\tGL_CONSTANT_ALPHA" ));
	    break;
       case GL_ONE_MINUS_CONSTANT_ALPHA:
	    DPF(( 0, "\tDST Blend\tGL_ONE_MINUS_CONSTANT_ALPHA" ));
	    break;
	}
   }	
}
