#include "mtypes.h"
#include "debug.h"


void gl_print_state( const char *msg, GLuint state )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & _NEW_MODELVIEW)       ? "ctx->ModelView, " : "",
	   (state & _NEW_PROJECTION)      ? "ctx->Projection, " : "",
	   (state & _NEW_TEXTURE_MATRIX)  ? "ctx->TextureMatrix, " : "",
	   (state & _NEW_COLOR_MATRIX)    ? "ctx->ColorMatrix, " : "",
	   (state & _NEW_ACCUM)           ? "ctx->Accum, " : "",
	   (state & _NEW_COLOR)           ? "ctx->Color, " : "",
	   (state & _NEW_DEPTH)           ? "ctx->Depth, " : "",
	   (state & _NEW_EVAL)            ? "ctx->Eval/EvalMap, " : "",
	   (state & _NEW_FOG)             ? "ctx->Fog, " : "",
	   (state & _NEW_HINT)            ? "ctx->Hint, " : "",
	   (state & _NEW_LIGHT)           ? "ctx->Light, " : "",
	   (state & _NEW_LINE)            ? "ctx->Line, " : "",
	   (state & _NEW_PIXEL)           ? "ctx->Pixel, " : "",
	   (state & _NEW_POINT)           ? "ctx->Point, " : "",
	   (state & _NEW_POLYGON)         ? "ctx->Polygon, " : "",
	   (state & _NEW_POLYGONSTIPPLE)  ? "ctx->PolygonStipple, " : "",
	   (state & _NEW_SCISSOR)         ? "ctx->Scissor, " : "",
	   (state & _NEW_TEXTURE)         ? "ctx->Texture, " : "",
	   (state & _NEW_TRANSFORM)       ? "ctx->Transform, " : "",
	   (state & _NEW_VIEWPORT)        ? "ctx->Viewport, " : "",
	   (state & _NEW_PACKUNPACK)      ? "ctx->Pack/Unpack, " : "",
	   (state & _NEW_ARRAY)           ? "ctx->Array, " : "",
	   (state & _NEW_COLORTABLE)      ? "ctx->{*}ColorTable, " : "",
	   (state & _NEW_RENDERMODE)      ? "ctx->RenderMode, " : "",
	   (state & _NEW_BUFFERS)         ? "ctx->Visual, ctx->DrawBuffer,, " : "");
}


void gl_print_enable_flags( const char *msg, GLuint flags )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   flags,
	   (flags & ENABLE_LIGHT)      ? "light, " : "",
	   (flags & ENABLE_FOG)        ? "fog, " : "",
	   (flags & ENABLE_USERCLIP)   ? "userclip, " : "",
	   (flags & ENABLE_TEXGEN0)    ? "tex-gen-0, " : "",
	   (flags & ENABLE_TEXGEN1)    ? "tex-gen-1, " : "",
	   (flags & ENABLE_TEXGEN2)    ? "tex-gen-2, " : "",
	   (flags & ENABLE_TEXGEN3)    ? "tex-gen-3, " : "",
	   (flags & ENABLE_TEXGEN4)    ? "tex-gen-4, " : "",
	   (flags & ENABLE_TEXGEN5)    ? "tex-gen-5, " : "",
	   (flags & ENABLE_TEXGEN6)    ? "tex-gen-6, " : "",
	   (flags & ENABLE_TEXGEN7)    ? "tex-gen-7, " : "",
	   (flags & ENABLE_TEXMAT0)    ? "tex-mat-0, " : "",
	   (flags & ENABLE_TEXMAT1)    ? "tex-mat-1, " : "",
	   (flags & ENABLE_TEXMAT2)    ? "tex-mat-2, " : "",
	   (flags & ENABLE_TEXMAT3)    ? "tex-mat-3, " : "",
	   (flags & ENABLE_TEXMAT4)    ? "tex-mat-4, " : "",
	   (flags & ENABLE_TEXMAT5)    ? "tex-mat-5, " : "",
	   (flags & ENABLE_TEXMAT6)    ? "tex-mat-6, " : "",
	   (flags & ENABLE_TEXMAT7)    ? "tex-mat-7, " : "",
	   (flags & ENABLE_NORMALIZE)  ? "normalize, " : "",
	   (flags & ENABLE_RESCALE)    ? "rescale, " : "");
}

void gl_print_tri_caps( const char *name, GLuint flags )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   name,
	   flags,
	   (flags & DD_FEEDBACK)            ? "feedback, " : "",
	   (flags & DD_SELECT)              ? "select, " : "",
	   (flags & DD_FLATSHADE)           ? "flat-shade, " : "",
	   (flags & DD_SEPERATE_SPECULAR)   ? "seperate-specular, " : "",
	   (flags & DD_TRI_LIGHT_TWOSIDE)   ? "tri-light-twoside, " : "",
	   (flags & DD_TRI_UNFILLED)        ? "tri-unfilled, " : "",
	   (flags & DD_TRI_STIPPLE)         ? "tri-stipple, " : "",
	   (flags & DD_TRI_OFFSET)          ? "tri-offset, " : "",
	   (flags & DD_TRI_SMOOTH)          ? "tri-smooth, " : "",
	   (flags & DD_LINE_SMOOTH)         ? "line-smooth, " : "",
	   (flags & DD_LINE_STIPPLE)        ? "line-stipple, " : "",
	   (flags & DD_LINE_WIDTH)          ? "line-wide, " : "",
	   (flags & DD_POINT_SMOOTH)        ? "point-smooth, " : "",
	   (flags & DD_POINT_SIZE)          ? "point-size, " : "",
	   (flags & DD_POINT_ATTEN)         ? "point-atten, " : "",
	   (flags & DD_TRI_CULL_FRONT_BACK) ? "cull-all, " : "",
	   (flags & DD_STENCIL)             ? "stencil, " : ""
      );
}
