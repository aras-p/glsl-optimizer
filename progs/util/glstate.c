/* $Id: glstate.c,v 1.1 1999/08/19 00:55:42 jtg Exp $ */

/*
 * Print GL state information (for debugging)
 * Copyright (C) 1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: glstate.c,v $
 * Revision 1.1  1999/08/19 00:55:42  jtg
 * Initial revision
 *
 * Revision 1.4  1999/06/19 01:36:43  brianp
 * more features added
 *
 * Revision 1.3  1999/02/24 05:16:20  brianp
 * added still more records to EnumTable
 *
 * Revision 1.2  1998/11/24 03:47:54  brianp
 * added more records to EnumTable
 *
 * Revision 1.1  1998/11/24 03:41:16  brianp
 * Initial revision
 *
 */



#include <assert.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include "glstate.h"


#define FLOAT       1
#define INT         2
#define DOUBLE      3
#define BOOLEAN     4
#define ENUM        5
#define VOID        6
#define LAST_TOKEN ~0


struct EnumRecord {
   GLenum enumerator;   /* GLenum constant */
   const char *string;  /* string name */
   int getType;         /* INT, FLOAT, DOUBLE, BOOLEAN, ENUM, or VOID */
   int getCount;      /* number of values returned by the glGet*v() call */
};


/* XXX Lots more records to add here!  Help, anyone? */

static struct EnumRecord EnumTable[] = {
   { GL_ACCUM_RED_BITS, "GL_ACCUM_RED_BITS", INT, 1 },
   { GL_ACCUM_GREEN_BITS, "GL_ACCUM_GREEN_BITS", INT, 1 },
   { GL_ACCUM_BLUE_BITS, "GL_ACCUM_BLUE_BITS", INT, 1 },
   { GL_ACCUM_ALPHA_BITS, "GL_ACCUM_ALPHA_BITS", INT, 1 },
   { GL_ACCUM_CLEAR_VALUE, "GL_ACCUM_CLEAR_VALUE", FLOAT, 4 },
   { GL_ALPHA_BIAS, "GL_ALPHA_BIAS", FLOAT, 1 },
   { GL_ALPHA_BITS, "GL_ALPHA_BITS", INT, 1 },
   { GL_ALPHA_SCALE, "GL_ALPHA_SCALE", FLOAT, 1 },
   { GL_ALPHA_TEST, "GL_ALPHA_TEST", BOOLEAN, 1 },
   { GL_ALPHA_TEST_FUNC, "GL_ALPHA_TEST_FUNC", ENUM, 1 },
   { GL_ALWAYS, "GL_ALWAYS", ENUM, 0 },
   { GL_ALPHA_TEST_REF, "GL_ALPHA_TEST_REF", FLOAT, 1 },
   { GL_ATTRIB_STACK_DEPTH, "GL_ATTRIB_STACK_DEPTH", INT, 1 },
   { GL_AUTO_NORMAL, "GL_AUTO_NORMAL", BOOLEAN, 1 },
   { GL_AUX_BUFFERS, "GL_AUX_BUFFERS", INT, 1 },
   { GL_BLEND, "GL_BLEND", BOOLEAN, 1 },
   { GL_BLEND_DST, "GL_BLEND_DST", ENUM, 1 },
   { GL_BLEND_SRC, "GL_BLEND_SRC", ENUM, 1 },
   { GL_BLUE_BIAS, "GL_BLUE_BIAS", FLOAT, 1 },
   { GL_BLUE_BITS, "GL_BLUE_BITS", INT, 1 },
   { GL_BLUE_SCALE, "GL_BLUE_SCALE", FLOAT, 1 },

   { GL_CLAMP_TO_EDGE, "GL_CLAMP_TO_EDGE", ENUM, 0 },
   { GL_CLEAR, "GL_CLEAR", ENUM, 0 },
   { GL_CLIENT_ATTRIB_STACK_DEPTH, "GL_CLIENT_ATTRIB_STACK_DEPTH", INT, 1 },
   { GL_CLIP_PLANE0, "GL_CLIP_PLANE0", BOOLEAN, 1 },
   { GL_CLIP_PLANE1, "GL_CLIP_PLANE1", BOOLEAN, 1 },
   { GL_CLIP_PLANE2, "GL_CLIP_PLANE2", BOOLEAN, 1 },
   { GL_CLIP_PLANE3, "GL_CLIP_PLANE3", BOOLEAN, 1 },
   { GL_CLIP_PLANE4, "GL_CLIP_PLANE4", BOOLEAN, 1 },
   { GL_CLIP_PLANE5, "GL_CLIP_PLANE5", BOOLEAN, 1 },
   { GL_COEFF, "GL_COEEF", ENUM, 0 },
   { GL_COLOR, "GL_COLOR", ENUM, 0 },
   { GL_COLOR_BUFFER_BIT, "GL_COLOR_BUFFER_BIT", ENUM, 0 },
   { GL_COLOR_CLEAR_VALUE, "GL_COLOR_CLEAR_VALUE", FLOAT, 4 },
   { GL_COLOR_INDEX, "GL_COLOR_INDEX", ENUM, 0 },
   { GL_COLOR_MATERIAL, "GL_COLOR_MATERIAL", BOOLEAN, 1 },
   { GL_COLOR_MATERIAL_FACE, "GL_COLOR_MATERIAL_FACE", ENUM, 1 },
   { GL_COLOR_MATERIAL_PARAMETER, "GL_COLOR_MATERIAL_PARAMETER", ENUM, 1 },
   { GL_COLOR_WRITEMASK, "GL_COLOR_WRITEMASK", BOOLEAN, 4 },
   { GL_COMPILE, "GL_COMPILE", ENUM, 0 },
   { GL_COMPILE_AND_EXECUTE, "GL_COMPILE_AND_EXECUTE", ENUM, 0 },
   { GL_COPY, "GL_COPY", ENUM, 0 },
   { GL_COPY_INVERTED, "GL_COPY_INVERTED", ENUM, 0 },
   { GL_COPY_PIXEL_TOKEN, "GL_COPY_PIXEL_TOKEN", ENUM, 0 },
   { GL_CULL_FACE, "GL_CULL_FACE", BOOLEAN, 1 },
   { GL_CULL_FACE_MODE, "GL_CULL_FACE_MODE", ENUM, 1 },
   { GL_CURRENT_BIT, "GL_CURRENT_BIT", ENUM, 0 },
   { GL_CURRENT_COLOR, "GL_CURRENT_COLOR", FLOAT, 4 },
   { GL_CURRENT_INDEX, "GL_CURRENT_INDEX", INT, 1 },
   { GL_CURRENT_NORMAL, "GL_CURRENT_NORMAL", FLOAT, 3 },
   { GL_CURRENT_RASTER_COLOR, "GL_CURRENT_RASTER_COLOR", FLOAT, 4 },
   { GL_CURRENT_RASTER_DISTANCE, "GL_CURRENT_RASTER_DISTANCE", FLOAT, 1 },
   { GL_CURRENT_RASTER_INDEX, "GL_CURRENT_RASTER_INDEX", INT, 1 },
   { GL_CURRENT_RASTER_POSITION, "GL_CURRENT_RASTER_POSITION", FLOAT, 4 },
   { GL_CURRENT_RASTER_TEXTURE_COORDS, "GL_CURRENT_RASTER_TEXTURE_COORDS", FLOAT, 4 },
   { GL_CURRENT_RASTER_POSITION_VALID, "GL_CURRENT_RASTER_POSITION_VALID", BOOLEAN, 1 },
   { GL_CURRENT_TEXTURE_COORDS, "GL_CURRENT_TEXTURE_COORDS", FLOAT, 4 },
   { GL_CW, "GL_CW", ENUM, 0 },
   { GL_CCW, "GL_CCW", ENUM, 0 },

   { GL_DECAL, "GL_DECAL", ENUM, 0 },
   { GL_DECR, "GL_DECR", ENUM, 0 },
   { GL_DEPTH, "GL_DEPTH", ENUM, 0 },
   { GL_DEPTH_BIAS, "GL_DEPTH_BIAS", FLOAT, 1 },
   { GL_DEPTH_BITS, "GL_DEPTH_BITS", INT, 1 },
   { GL_DEPTH_BUFFER_BIT, "GL_DEPTH_BUFFER_BIT", ENUM, 0 },
   { GL_DEPTH_CLEAR_VALUE, "GL_DEPTH_CLEAR_VALUE", FLOAT, 1 },
   { GL_DEPTH_COMPONENT, "GL_DEPTH_COMPONENT", ENUM, 0 },
   { GL_DEPTH_FUNC, "GL_DEPTH_FUNC", ENUM, 1 },
   { GL_DEPTH_RANGE, "GL_DEPTH_RANGE", FLOAT, 2 },
   { GL_DEPTH_SCALE, "GL_DEPTH_SCALE", FLOAT, 1 },
   { GL_DEPTH_TEST, "GL_DEPTH_TEST", ENUM, 1 },
   { GL_DEPTH_WRITEMASK, "GL_DEPTH_WRITEMASK", BOOLEAN, 1 },
   { GL_DIFFUSE, "GL_DIFFUSE", ENUM, 0 },  /*XXX*/
   { GL_DITHER, "GL_DITHER", BOOLEAN, 1 },
   { GL_DOMAIN, "GL_DOMAIN", ENUM, 0 },
   { GL_DONT_CARE, "GL_DONT_CARE", ENUM, 0 },
   { GL_DOUBLE, "GL_DOUBLE", ENUM, 0 },
   { GL_DOUBLEBUFFER, "GL_DOUBLEBUFFER", BOOLEAN, 1},
   { GL_DRAW_BUFFER, "GL_DRAW_BUFFER", ENUM, 1 },
   { GL_DRAW_PIXEL_TOKEN, "GL_DRAW_PIXEL_TOKEN", ENUM, 0 },
   { GL_DST_ALPHA, "GL_DST_ALPHA", ENUM, 0 },
   { GL_DST_COLOR, "GL_DST_COLOR", ENUM, 0 },

   { GL_EDGE_FLAG, "GL_EDGE_FLAG", BOOLEAN, 1 },
   /* XXX GL_EDGE_FLAG_ARRAY_* */
   { GL_EMISSION, "GL_EMISSION", ENUM, 0 }, /* XXX */
   { GL_ENABLE_BIT, "GL_ENABLE_BIT", ENUM, 0 },
   { GL_EQUAL, "GL_EQUAL", ENUM, 0 },
   { GL_EQUIV, "GL_EQUIV", ENUM, 0 },
   { GL_EVAL_BIT, "GL_EVAL_BIT", ENUM, 0 },
   { GL_EXP, "GL_EXP", ENUM, 0 },
   { GL_EXP2, "GL_EXP2", ENUM, 0 },
   { GL_EXTENSIONS, "GL_EXTENSIONS", ENUM, 0 },
   { GL_EYE_LINEAR, "GL_EYE_LINEAR", ENUM, 0 },
   { GL_EYE_PLANE, "GL_EYE_PLANE", ENUM, 0 },

   { GL_FALSE, "GL_FALSE", ENUM, 0 },
   { GL_FASTEST, "GL_FASTEST", ENUM, 0 },
   { GL_FEEDBACK, "GL_FEEDBACK", ENUM, 0 },
   { GL_FEEDBACK_BUFFER_POINTER, "GL_FEEDBACK_BUFFER_POINTER", VOID, 0 },
   { GL_FEEDBACK_BUFFER_SIZE, "GL_FEEDBACK_BUFFER_SIZE", INT, 1 },
   { GL_FEEDBACK_BUFFER_TYPE, "GL_FEEDBACK_BUFFER_TYPE", INT, 1 },
   { GL_FILL, "GL_FILL", ENUM, 0 },
   { GL_FLAT, "GL_FLAT", ENUM, 0 },
   { GL_FLOAT, "GL_FLOAT", ENUM, 0 },
   { GL_FOG, "GL_FOG", BOOLEAN, 1 },
   { GL_FOG_BIT, "GL_FOG_BIT", ENUM, 0 },
   { GL_FOG_COLOR, "GL_FOG_COLOR", FLOAT, 4 },
   { GL_FOG_DENSITY, "GL_FOG_DENSITY", FLOAT, 1 },
   { GL_FOG_END, "GL_FOG_END", FLOAT, 1 },
   { GL_FOG_HINT, "GL_FOG_HINT", ENUM, 1 },
   { GL_FOG_INDEX, "GL_FOG_INDEX", INT, 1 },
   { GL_FOG_MODE, "GL_FOG_MODE", ENUM, 1 },
   { GL_FOG_START, "GL_FOG_START", FLOAT, 1 },
   { GL_FRONT, "GL_FRONT", ENUM, 0 },
   { GL_FRONT_AND_BACK, "GL_FRONT_AND_BACK", ENUM, 0 },
   { GL_FRONT_FACE, "GL_FRONT_FACE", ENUM, 1 },
   { GL_FRONT_LEFT, "GL_FRONT_LEFT", ENUM, 0 },
   { GL_FRONT_RIGHT, "GL_FRONT_RIGHT", ENUM, 0 },

   { GL_GEQUAL, "GL_GEQUAL", ENUM, 0 },
   { GL_GREATER, "GL_GREATER", ENUM, 0 },
   { GL_GREEN, "GL_GREEN", ENUM, 0 },
   { GL_GREEN_BIAS, "GL_GREEN_BIAS", FLOAT, 1 },
   { GL_GREEN_BITS, "GL_GREEN_BITS", INT, 1 },
   { GL_GREEN_SCALE, "GL_GREEN_SCALE", FLOAT, 1 },



   { GL_LESS, "GL_LESS", ENUM, 0 },
   { GL_LEQUAL, "GL_LEQUAL", ENUM, 0 },
   { GL_LIGHTING, "GL_LIGHTING", BOOLEAN, 1 },
   { GL_LINE_SMOOTH, "GL_LINE_SMOOTH", BOOLEAN, 1 },
   { GL_LINE_STIPPLE, "GL_LINE_STIPPLE", BOOLEAN, 1 },
   { GL_LINE_STIPPLE_PATTERN, "GL_LINE_STIPPLE_PATTERN", INT, 1 },
   { GL_LINE_STIPPLE_REPEAT, "GL_LINE_STIPPLE_REPEAT", INT, 1 },
   { GL_LINE_WIDTH, "GL_LINE_WIDTH", FLOAT, 1 },

   { GL_MODELVIEW_MATRIX, "GL_MODELVIEW_MATRIX", DOUBLE, 16 },

   { GL_NEVER, "GL_NEVER", ENUM, 0 },
   { GL_NOTEQUAL, "GL_NOTEQUAL", ENUM, 0 },

   { GL_PROJECTION_MATRIX, "GL_PROJECTION_MATRIX", FLOAT, 16 },

   { GL_PACK_SWAP_BYTES, "GL_PACK_SWAP_BYTES", INT, 1 },
   { GL_PACK_LSB_FIRST, "GL_PACK_LSB_FIRST", INT, 1 },
   { GL_PACK_ROW_LENGTH, "GL_PACK_ROW_LENGTH", INT, 1 },
   { GL_PACK_SKIP_PIXELS, "GL_PACK_SKIP_PIXELS", INT, 1 },
   { GL_PACK_SKIP_ROWS, "GL_PACK_SKIP_ROWS", INT, 1 },
   { GL_PACK_ALIGNMENT, "GL_PACK_ALIGNMENT", INT, 1 },

   { GL_TRUE, "GL_TRUE", ENUM, 0 },

   { GL_UNPACK_SWAP_BYTES, "GL_UNPACK_SWAP_BYTES", INT, 1 },
   { GL_UNPACK_LSB_FIRST, "GL_UNPACK_LSB_FIRST", INT, 1 },
   { GL_UNPACK_ROW_LENGTH, "GL_UNPACK_ROW_LENGTH", INT, 1 },
   { GL_UNPACK_SKIP_PIXELS, "GL_UNPACK_SKIP_PIXELS", INT, 1 },
   { GL_UNPACK_SKIP_ROWS, "GL_UNPACK_SKIP_ROWS", INT, 1 },
   { GL_UNPACK_ALIGNMENT, "GL_UNPACK_ALIGNMENT", INT, 1 },

   { GL_VIEWPORT, "GL_VIEWPORT", INT, 4 },


   /*
    * Extensions
    */

#if defined(GL_EXT_blend_minmax)
   { GL_BLEND_EQUATION_EXT, "GL_BLEND_EQUATION_EXT", ENUM, 1 },
#endif
#if defined(GL_EXT_blend_color)
   { GL_BLEND_COLOR_EXT, "GL_BLEND_COLOR_EXT", FLOAT, 4 },
#endif
#if defined(GL_EXT_point_parameters)
   { GL_DISTANCE_ATTENUATION_EXT, "GL_DISTANCE_ATTENUATION_EXT", FLOAT, 1 },
#endif
#if defined(GL_INGR_blend_func_separate)
   { GL_BLEND_SRC_RGB_INGR, "GL_BLEND_SRC_RGB_INGR", ENUM, 1 },
   { GL_BLEND_DST_RGB_INGR, "GL_BLEND_DST_RGB_INGR", ENUM, 1 },
   { GL_BLEND_SRC_ALPHA_INGR, "GL_BLEND_SRC_ALPHA_INGR", ENUM, 1 },
   { GL_BLEND_DST_ALPHA_INGR, "GL_BLEND_DST_ALPHA_INGR", ENUM, 1 },
#endif


   { LAST_TOKEN, "", 0, 0 }
};


static const struct EnumRecord *FindRecord( GLenum var )
{
   int i;
   for (i = 0; EnumTable[i].enumerator != LAST_TOKEN; i++) {
      if (EnumTable[i].enumerator == var) {
	 return &EnumTable[i];
      }
   }
   return NULL;
}



/*
 * Return the string label for the given enum.
 */
const char *GetEnumString( GLenum var )
{
   const struct EnumRecord *rec = FindRecord(var);
   if (rec)
      return rec->string;
   else
      return NULL;
}



/*
 * Print current value of the given state variable.
 */
void PrintState( int indent, GLenum var )
{
   const struct EnumRecord *rec = FindRecord(var);

   while (indent-- > 0)
      putchar(' ');

   if (rec) {
      if (rec->getCount <= 0) {
         assert(rec->getType == ENUM);
         printf("%s is not a state variable\n", rec->string);
      }
      else {
         switch (rec->getType) {
             case INT:
                {
                   GLint values[100];
                   int i;
                   glGetIntegerv(rec->enumerator, values);
                   printf("%s = ", rec->string);
                   for (i = 0; i < rec->getCount; i++)
                      printf("%d ", values[i]);
                   printf("\n");
                }
                break;
             case FLOAT:
                {
                   GLfloat values[100];
                   int i;
                   glGetFloatv(rec->enumerator, values);
                   printf("%s = ", rec->string);
                   for (i = 0; i < rec->getCount; i++)
                      printf("%f ", values[i]);
                   printf("\n");
                }
                break;
             case DOUBLE:
                {
                   GLdouble values[100];
                   int i;
                   glGetDoublev(rec->enumerator, values);
                   printf("%s = ", rec->string);
                   for (i = 0; i < rec->getCount; i++)
                      printf("%f ", (float) values[i]);
                   printf("\n");
                }
                break;
             case BOOLEAN:
                {
                   GLboolean values[100];
                   int i;
                   glGetBooleanv(rec->enumerator, values);
                   printf("%s = ", rec->string);
                   for (i = 0; i < rec->getCount; i++)
                      printf("%s ", values[i] ? "GL_TRUE" : "GL_FALSE");
                   printf("\n");
                }
                break;
             case ENUM:
                {
                   GLint values[100];
                   int i;
                   glGetIntegerv(rec->enumerator, values);
                   printf("%s = ", rec->string);
                   for (i = 0; i < rec->getCount; i++) {
                      const char *str = GetEnumString((GLenum) values[i]);
                      if (str)
                         printf("%s ", str);
                      else
                         printf("??? ");
                   }
                   printf("\n");
                }
                break;
             case VOID:
                {
                   GLvoid *values[100];
                   int i;
                   glGetPointerv(rec->enumerator, values);
                   printf("%s = ", rec->string);
                   for (i = 0; i < rec->getCount; i++) {
		       printf("%p ", values[i]);
                   }
                   printf("\n");
                }
                break;
             default:
                printf("fatal error in PrintState()\n");
                abort();
         }
      }
   }
   else {
      printf("Unknown GLenum passed to PrintState()\n");
   }
}



/*
 * Print all glPixelStore-related state.
 * NOTE: Should write similar functions for lighting, texturing, etc.
 */
void PrintPixelStoreState( void )
{
   const GLenum enums[] = {
      GL_PACK_SWAP_BYTES,
      GL_PACK_LSB_FIRST,
      GL_PACK_ROW_LENGTH,
      GL_PACK_SKIP_PIXELS,
      GL_PACK_SKIP_ROWS,
      GL_PACK_ALIGNMENT,
      GL_UNPACK_SWAP_BYTES,
      GL_UNPACK_LSB_FIRST,
      GL_UNPACK_ROW_LENGTH,
      GL_UNPACK_SKIP_PIXELS,
      GL_UNPACK_SKIP_ROWS,
      GL_UNPACK_ALIGNMENT,
      0
   };
   int i;
   printf("Pixel pack/unpack state:\n");
   for (i = 0; enums[i]; i++) {
      PrintState(3, enums[i]);
   }
}




/*
 * Print all state for the given attribute group.
 */
void PrintAttribState( GLbitfield attrib )
{
   static const GLenum depth_buffer_enums[] = {
      GL_DEPTH_FUNC,
      GL_DEPTH_CLEAR_VALUE,
      GL_DEPTH_TEST,
      GL_DEPTH_WRITEMASK,
      0
   };
   static const GLenum fog_enums[] = {
      GL_FOG,
      GL_FOG_COLOR,
      GL_FOG_DENSITY,
      GL_FOG_START,
      GL_FOG_END,
      GL_FOG_INDEX,
      GL_FOG_MODE,
      0
   };
   static const GLenum line_enums[] = {
      GL_LINE_SMOOTH,
      GL_LINE_STIPPLE,
      GL_LINE_STIPPLE_PATTERN,
      GL_LINE_STIPPLE_REPEAT,
      GL_LINE_WIDTH,
      0
   };

   const GLenum *enumList = NULL;

   switch (attrib) {
      case GL_DEPTH_BUFFER_BIT:
         enumList = depth_buffer_enums;
         printf("GL_DEPTH_BUFFER_BIT state:\n");
         break;
      case GL_FOG_BIT:
         enumList = fog_enums;
         printf("GL_FOG_BIT state:\n");
         break;
      case GL_LINE_BIT:
         enumList = line_enums;
         printf("GL_LINE_BIT state:\n");
         break;
      default:
         printf("Bad value in PrintAttribState()\n");
   }

   if (enumList) {
      int i;
      for (i = 0; enumList[i]; i++)
         PrintState(3, enumList[i]);
   }
}


/*#define TEST*/
#ifdef TEST

#include <GL/glut.h>

int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 300);
   glutInitDisplayMode(GLUT_RGB);
   glutCreateWindow(argv[0]);
   PrintAttribState(GL_DEPTH_BUFFER_BIT);
   PrintAttribState(GL_FOG_BIT);
   PrintAttribState(GL_LINE_BIT);
   PrintState(0, GL_ALPHA_BITS);
   PrintState(0, GL_VIEWPORT);
   PrintState(0, GL_ALPHA_TEST_FUNC);
   PrintState(0, GL_MODELVIEW_MATRIX);
   PrintState(0, GL_ALWAYS);
   PrintPixelStoreState();
   return 0;
}

#endif
