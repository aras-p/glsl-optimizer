
/*
 *
 * From: Stephane Rehel <rehel@worldnet.fr>
 * Date: Mon, 31 May 1999 18:40:54 -0400
 * To: Paul Brian <brianp@ra.avid.com>
 * Subject: OpenGL State Dump Function
 * 
 * Here is a function that dumps the current OpenGL state. I wrote it
 * some time ago.
 * 
 * In the attachment:
 *   + the code itself
 *   + its output
 * 
 * I think Mesa is wrong on some getBooleanv(). For example, GL_VERTEX_ARRAY
 * is queried by IsEnabled() (cf. p. 196 of the spec). But on page 193
 * we can read that all the boolean attribs that can be queried by IsEnabled()
 * can also be queried by IsEnabled().
 * 
 * I had duplicated all the enums (LOCAL_*) so that the code can run on any 
 * OpenGL version, even if an enum is not recognized.
 * 
 * The code can be shipped in the public domain.
 * 
 * Stephane.
 */


/*
 * Stephane Rehel
 * Creation: February 5 1999
 */

#include <stdio.h>
#include <GL/gl.h>

/***************************************************************************/

enum {
        /* Data types */
        LOCAL_GL_BYTE                         = 0x1400,
        LOCAL_GL_UNSIGNED_BYTE                = 0x1401,
        LOCAL_GL_SHORT                        = 0x1402,
        LOCAL_GL_UNSIGNED_SHORT               = 0x1403,
        LOCAL_GL_INT                          = 0x1404,
        LOCAL_GL_UNSIGNED_INT                 = 0x1405,
        LOCAL_GL_FLOAT                        = 0x1406,
        LOCAL_GL_DOUBLE                       = 0x140A,
        LOCAL_GL_2_BYTES                      = 0x1407,
        LOCAL_GL_3_BYTES                      = 0x1408,
        LOCAL_GL_4_BYTES                      = 0x1409,

        /* Primitives */
        LOCAL_GL_LINES                        = 0x0001,
        LOCAL_GL_POINTS                       = 0x0000,
        LOCAL_GL_LINE_STRIP                   = 0x0003,
        LOCAL_GL_LINE_LOOP                    = 0x0002,
        LOCAL_GL_TRIANGLES                    = 0x0004,
        LOCAL_GL_TRIANGLE_STRIP               = 0x0005,
        LOCAL_GL_TRIANGLE_FAN                 = 0x0006,
        LOCAL_GL_QUADS                        = 0x0007,
        LOCAL_GL_QUAD_STRIP                   = 0x0008,
        LOCAL_GL_POLYGON                      = 0x0009,
        LOCAL_GL_EDGE_FLAG                    = 0x0B43,

        /* Vertex Arrays */
        LOCAL_GL_VERTEX_ARRAY                 = 0x8074,
        LOCAL_GL_NORMAL_ARRAY                 = 0x8075,
        LOCAL_GL_COLOR_ARRAY                  = 0x8076,
        LOCAL_GL_INDEX_ARRAY                  = 0x8077,
        LOCAL_GL_TEXTURE_COORD_ARRAY          = 0x8078,
        LOCAL_GL_EDGE_FLAG_ARRAY              = 0x8079,
        LOCAL_GL_VERTEX_ARRAY_SIZE            = 0x807A,
        LOCAL_GL_VERTEX_ARRAY_TYPE            = 0x807B,
        LOCAL_GL_VERTEX_ARRAY_STRIDE          = 0x807C,
        LOCAL_GL_NORMAL_ARRAY_TYPE            = 0x807E,
        LOCAL_GL_NORMAL_ARRAY_STRIDE          = 0x807F,
        LOCAL_GL_COLOR_ARRAY_SIZE             = 0x8081,
        LOCAL_GL_COLOR_ARRAY_TYPE             = 0x8082,
        LOCAL_GL_COLOR_ARRAY_STRIDE           = 0x8083,
        LOCAL_GL_INDEX_ARRAY_TYPE             = 0x8085,
        LOCAL_GL_INDEX_ARRAY_STRIDE           = 0x8086,
        LOCAL_GL_TEXTURE_COORD_ARRAY_SIZE     = 0x8088,
        LOCAL_GL_TEXTURE_COORD_ARRAY_TYPE     = 0x8089,
        LOCAL_GL_TEXTURE_COORD_ARRAY_STRIDE   = 0x808A,
        LOCAL_GL_EDGE_FLAG_ARRAY_STRIDE       = 0x808C,
        LOCAL_GL_VERTEX_ARRAY_POINTER         = 0x808E,
        LOCAL_GL_NORMAL_ARRAY_POINTER         = 0x808F,
        LOCAL_GL_COLOR_ARRAY_POINTER          = 0x8090,
        LOCAL_GL_INDEX_ARRAY_POINTER          = 0x8091,
        LOCAL_GL_TEXTURE_COORD_ARRAY_POINTER  = 0x8092,
        LOCAL_GL_EDGE_FLAG_ARRAY_POINTER      = 0x8093,
        LOCAL_GL_V2F                          = 0x2A20,
        LOCAL_GL_V3F                          = 0x2A21,
        LOCAL_GL_C4UB_V2F                     = 0x2A22,
        LOCAL_GL_C4UB_V3F                     = 0x2A23,
        LOCAL_GL_C3F_V3F                      = 0x2A24,
        LOCAL_GL_N3F_V3F                      = 0x2A25,
        LOCAL_GL_C4F_N3F_V3F                  = 0x2A26,
        LOCAL_GL_T2F_V3F                      = 0x2A27,
        LOCAL_GL_T4F_V4F                      = 0x2A28,
        LOCAL_GL_T2F_C4UB_V3F                 = 0x2A29,
        LOCAL_GL_T2F_C3F_V3F                  = 0x2A2A,
        LOCAL_GL_T2F_N3F_V3F                  = 0x2A2B,
        LOCAL_GL_T2F_C4F_N3F_V3F              = 0x2A2C,
        LOCAL_GL_T4F_C4F_N3F_V4F              = 0x2A2D,

        /* Matrix Mode */
        LOCAL_GL_MATRIX_MODE                  = 0x0BA0,
        LOCAL_GL_MODELVIEW                    = 0x1700,
        LOCAL_GL_PROJECTION                   = 0x1701,
        LOCAL_GL_TEXTURE                      = 0x1702,

        /* Points */
        LOCAL_GL_POINT_SMOOTH                 = 0x0B10,
        LOCAL_GL_POINT_SIZE                   = 0x0B11,
        LOCAL_GL_POINT_SIZE_GRANULARITY       = 0x0B13,
        LOCAL_GL_POINT_SIZE_RANGE             = 0x0B12,

        /* Lines */
        LOCAL_GL_LINE_SMOOTH                  = 0x0B20,
        LOCAL_GL_LINE_STIPPLE                 = 0x0B24,
        LOCAL_GL_LINE_STIPPLE_PATTERN         = 0x0B25,
        LOCAL_GL_LINE_STIPPLE_REPEAT          = 0x0B26,
        LOCAL_GL_LINE_WIDTH                   = 0x0B21,
        LOCAL_GL_LINE_WIDTH_GRANULARITY       = 0x0B23,
        LOCAL_GL_LINE_WIDTH_RANGE             = 0x0B22,

        /* Polygons */
        LOCAL_GL_POINT                        = 0x1B00,
        LOCAL_GL_LINE                         = 0x1B01,
        LOCAL_GL_FILL                         = 0x1B02,
        LOCAL_GL_CCW                          = 0x0901,
        LOCAL_GL_CW                           = 0x0900,
        LOCAL_GL_FRONT                        = 0x0404,
        LOCAL_GL_BACK                         = 0x0405,
        LOCAL_GL_CULL_FACE                    = 0x0B44,
        LOCAL_GL_CULL_FACE_MODE               = 0x0B45,
        LOCAL_GL_POLYGON_SMOOTH               = 0x0B41,
        LOCAL_GL_POLYGON_STIPPLE              = 0x0B42,
        LOCAL_GL_FRONT_FACE                   = 0x0B46,
        LOCAL_GL_POLYGON_MODE                 = 0x0B40,
        LOCAL_GL_POLYGON_OFFSET_FACTOR        = 0x8038,
        LOCAL_GL_POLYGON_OFFSET_UNITS         = 0x2A00,
        LOCAL_GL_POLYGON_OFFSET_POINT         = 0x2A01,
        LOCAL_GL_POLYGON_OFFSET_LINE          = 0x2A02,
        LOCAL_GL_POLYGON_OFFSET_FILL          = 0x8037,

        /* Display Lists */
        LOCAL_GL_COMPILE                      = 0x1300,
        LOCAL_GL_COMPILE_AND_EXECUTE          = 0x1301,
        LOCAL_GL_LIST_BASE                    = 0x0B32,
        LOCAL_GL_LIST_INDEX                   = 0x0B33,
        LOCAL_GL_LIST_MODE                    = 0x0B30,

        /* Depth buffer */
        LOCAL_GL_NEVER                        = 0x0200,
        LOCAL_GL_LESS                         = 0x0201,
        LOCAL_GL_GEQUAL                       = 0x0206,
        LOCAL_GL_LEQUAL                       = 0x0203,
        LOCAL_GL_GREATER                      = 0x0204,
        LOCAL_GL_NOTEQUAL                     = 0x0205,
        LOCAL_GL_EQUAL                        = 0x0202,
        LOCAL_GL_ALWAYS                       = 0x0207,
        LOCAL_GL_DEPTH_TEST                   = 0x0B71,
        LOCAL_GL_DEPTH_BITS                   = 0x0D56,
        LOCAL_GL_DEPTH_CLEAR_VALUE            = 0x0B73,
        LOCAL_GL_DEPTH_FUNC                   = 0x0B74,
        LOCAL_GL_DEPTH_RANGE                  = 0x0B70,
        LOCAL_GL_DEPTH_WRITEMASK              = 0x0B72,
        LOCAL_GL_DEPTH_COMPONENT              = 0x1902,

        /* Lighting */
        LOCAL_GL_LIGHTING                     = 0x0B50,
        LOCAL_GL_LIGHT0                       = 0x4000,
        LOCAL_GL_LIGHT1                       = 0x4001,
        LOCAL_GL_LIGHT2                       = 0x4002,
        LOCAL_GL_LIGHT3                       = 0x4003,
        LOCAL_GL_LIGHT4                       = 0x4004,
        LOCAL_GL_LIGHT5                       = 0x4005,
        LOCAL_GL_LIGHT6                       = 0x4006,
        LOCAL_GL_LIGHT7                       = 0x4007,
        LOCAL_GL_SPOT_EXPONENT                = 0x1205,
        LOCAL_GL_SPOT_CUTOFF                  = 0x1206,
        LOCAL_GL_CONSTANT_ATTENUATION         = 0x1207,
        LOCAL_GL_LINEAR_ATTENUATION           = 0x1208,
        LOCAL_GL_QUADRATIC_ATTENUATION        = 0x1209,
        LOCAL_GL_AMBIENT                      = 0x1200,
        LOCAL_GL_DIFFUSE                      = 0x1201,
        LOCAL_GL_SPECULAR                     = 0x1202,
        LOCAL_GL_SHININESS                    = 0x1601,
        LOCAL_GL_EMISSION                     = 0x1600,
        LOCAL_GL_POSITION                     = 0x1203,
        LOCAL_GL_SPOT_DIRECTION               = 0x1204,
        LOCAL_GL_AMBIENT_AND_DIFFUSE          = 0x1602,
        LOCAL_GL_COLOR_INDEXES                = 0x1603,
        LOCAL_GL_LIGHT_MODEL_TWO_SIDE         = 0x0B52,
        LOCAL_GL_LIGHT_MODEL_LOCAL_VIEWER     = 0x0B51,
        LOCAL_GL_LIGHT_MODEL_AMBIENT          = 0x0B53,
        LOCAL_GL_FRONT_AND_BACK               = 0x0408,
        LOCAL_GL_SHADE_MODEL                  = 0x0B54,
        LOCAL_GL_FLAT                         = 0x1D00,
        LOCAL_GL_SMOOTH                       = 0x1D01,
        LOCAL_GL_COLOR_MATERIAL               = 0x0B57,
        LOCAL_GL_COLOR_MATERIAL_FACE          = 0x0B55,
        LOCAL_GL_COLOR_MATERIAL_PARAMETER     = 0x0B56,
        LOCAL_GL_NORMALIZE                    = 0x0BA1,

        /* User clipping planes */
        LOCAL_GL_CLIP_PLANE0                  = 0x3000,
        LOCAL_GL_CLIP_PLANE1                  = 0x3001,
        LOCAL_GL_CLIP_PLANE2                  = 0x3002,
        LOCAL_GL_CLIP_PLANE3                  = 0x3003,
        LOCAL_GL_CLIP_PLANE4                  = 0x3004,
        LOCAL_GL_CLIP_PLANE5                  = 0x3005,

        /* Accumulation buffer */
        LOCAL_GL_ACCUM_RED_BITS               = 0x0D58,
        LOCAL_GL_ACCUM_GREEN_BITS             = 0x0D59,
        LOCAL_GL_ACCUM_BLUE_BITS              = 0x0D5A,
        LOCAL_GL_ACCUM_ALPHA_BITS             = 0x0D5B,
        LOCAL_GL_ACCUM_CLEAR_VALUE            = 0x0B80,
        LOCAL_GL_ACCUM                        = 0x0100,
        LOCAL_GL_ADD                          = 0x0104,
        LOCAL_GL_LOAD                         = 0x0101,
        LOCAL_GL_MULT                         = 0x0103,
        LOCAL_GL_RETURN                       = 0x0102,

        /* Alpha testing */
        LOCAL_GL_ALPHA_TEST                   = 0x0BC0,
        LOCAL_GL_ALPHA_TEST_REF               = 0x0BC2,
        LOCAL_GL_ALPHA_TEST_FUNC              = 0x0BC1,

        /* Blending */
        LOCAL_GL_BLEND                        = 0x0BE2,
        LOCAL_GL_BLEND_SRC                    = 0x0BE1,
        LOCAL_GL_BLEND_DST                    = 0x0BE0,
        LOCAL_GL_ZERO                         = 0,
        LOCAL_GL_ONE                          = 1,
        LOCAL_GL_SRC_COLOR                    = 0x0300,
        LOCAL_GL_ONE_MINUS_SRC_COLOR          = 0x0301,
        LOCAL_GL_DST_COLOR                    = 0x0306,
        LOCAL_GL_ONE_MINUS_DST_COLOR          = 0x0307,
        LOCAL_GL_SRC_ALPHA                    = 0x0302,
        LOCAL_GL_ONE_MINUS_SRC_ALPHA          = 0x0303,
        LOCAL_GL_DST_ALPHA                    = 0x0304,
        LOCAL_GL_ONE_MINUS_DST_ALPHA          = 0x0305,
        LOCAL_GL_SRC_ALPHA_SATURATE           = 0x0308,
        LOCAL_GL_CONSTANT_COLOR               = 0x8001,
        LOCAL_GL_ONE_MINUS_CONSTANT_COLOR     = 0x8002,
        LOCAL_GL_CONSTANT_ALPHA               = 0x8003,
        LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA     = 0x8004,

        /* Render Mode */
        LOCAL_GL_FEEDBACK                     = 0x1C01,
        LOCAL_GL_RENDER                       = 0x1C00,
        LOCAL_GL_SELECT                       = 0x1C02,

        /* Feedback */
        LOCAL_GL_2D                           = 0x0600,
        LOCAL_GL_3D                           = 0x0601,
        LOCAL_GL_3D_COLOR                     = 0x0602,
        LOCAL_GL_3D_COLOR_TEXTURE             = 0x0603,
        LOCAL_GL_4D_COLOR_TEXTURE             = 0x0604,
        LOCAL_GL_POINT_TOKEN                  = 0x0701,
        LOCAL_GL_LINE_TOKEN                   = 0x0702,
        LOCAL_GL_LINE_RESET_TOKEN             = 0x0707,
        LOCAL_GL_POLYGON_TOKEN                = 0x0703,
        LOCAL_GL_BITMAP_TOKEN                 = 0x0704,
        LOCAL_GL_DRAW_PIXEL_TOKEN             = 0x0705,
        LOCAL_GL_COPY_PIXEL_TOKEN             = 0x0706,
        LOCAL_GL_PASS_THROUGH_TOKEN           = 0x0700,
        LOCAL_GL_FEEDBACK_BUFFER_POINTER      = 0x0DF0,
        LOCAL_GL_FEEDBACK_BUFFER_SIZE         = 0x0DF1,
        LOCAL_GL_FEEDBACK_BUFFER_TYPE         = 0x0DF2,

        /* Selection */
        LOCAL_GL_SELECTION_BUFFER_POINTER     = 0x0DF3,
        LOCAL_GL_SELECTION_BUFFER_SIZE        = 0x0DF4,

        /* Fog */
        LOCAL_GL_FOG                          = 0x0B60,
        LOCAL_GL_FOG_MODE                     = 0x0B65,
        LOCAL_GL_FOG_DENSITY                  = 0x0B62,
        LOCAL_GL_FOG_COLOR                    = 0x0B66,
        LOCAL_GL_FOG_INDEX                    = 0x0B61,
        LOCAL_GL_FOG_START                    = 0x0B63,
        LOCAL_GL_FOG_END                      = 0x0B64,
        LOCAL_GL_LINEAR                       = 0x2601,
        LOCAL_GL_EXP                          = 0x0800,
        LOCAL_GL_EXP2                         = 0x0801,

        /* Logic Ops */
        LOCAL_GL_LOGIC_OP                     = 0x0BF1,
        LOCAL_GL_INDEX_LOGIC_OP               = 0x0BF1,
        LOCAL_GL_COLOR_LOGIC_OP               = 0x0BF2,
        LOCAL_GL_LOGIC_OP_MODE                = 0x0BF0,
        LOCAL_GL_CLEAR                        = 0x1500,
        LOCAL_GL_SET                          = 0x150F,
        LOCAL_GL_COPY                         = 0x1503,
        LOCAL_GL_COPY_INVERTED                = 0x150C,
        LOCAL_GL_NOOP                         = 0x1505,
        LOCAL_GL_INVERT                       = 0x150A,
        LOCAL_GL_AND                          = 0x1501,
        LOCAL_GL_NAND                         = 0x150E,
        LOCAL_GL_OR                           = 0x1507,
        LOCAL_GL_NOR                          = 0x1508,
        LOCAL_GL_XOR                          = 0x1506,
        LOCAL_GL_EQUIV                        = 0x1509,
        LOCAL_GL_AND_REVERSE                  = 0x1502,
        LOCAL_GL_AND_INVERTED                 = 0x1504,
        LOCAL_GL_OR_REVERSE                   = 0x150B,
        LOCAL_GL_OR_INVERTED                  = 0x150D,

        /* Stencil */
        LOCAL_GL_STENCIL_TEST                 = 0x0B90,
        LOCAL_GL_STENCIL_WRITEMASK            = 0x0B98,
        LOCAL_GL_STENCIL_BITS                 = 0x0D57,
        LOCAL_GL_STENCIL_FUNC                 = 0x0B92,
        LOCAL_GL_STENCIL_VALUE_MASK           = 0x0B93,
        LOCAL_GL_STENCIL_REF                  = 0x0B97,
        LOCAL_GL_STENCIL_FAIL                 = 0x0B94,
        LOCAL_GL_STENCIL_PASS_DEPTH_PASS      = 0x0B96,
        LOCAL_GL_STENCIL_PASS_DEPTH_FAIL      = 0x0B95,
        LOCAL_GL_STENCIL_CLEAR_VALUE          = 0x0B91,
        LOCAL_GL_STENCIL_INDEX                = 0x1901,
        LOCAL_GL_KEEP                         = 0x1E00,
        LOCAL_GL_REPLACE                      = 0x1E01,
        LOCAL_GL_INCR                         = 0x1E02,
        LOCAL_GL_DECR                         = 0x1E03,

        /* Buffers, Pixel Drawing/Reading */
        LOCAL_GL_NONE                         = 0,
        LOCAL_GL_LEFT                         = 0x0406,
        LOCAL_GL_RIGHT                        = 0x0407,
        /*LOCAL_GL_FRONT                      = 0x0404, */
        /*LOCAL_GL_BACK                       = 0x0405, */
        /*LOCAL_GL_FRONT_AND_BACK             = 0x0408, */
        LOCAL_GL_FRONT_LEFT                   = 0x0400,
        LOCAL_GL_FRONT_RIGHT                  = 0x0401,
        LOCAL_GL_BACK_LEFT                    = 0x0402,
        LOCAL_GL_BACK_RIGHT                   = 0x0403,
        LOCAL_GL_AUX0                         = 0x0409,
        LOCAL_GL_AUX1                         = 0x040A,
        LOCAL_GL_AUX2                         = 0x040B,
        LOCAL_GL_AUX3                         = 0x040C,
        LOCAL_GL_COLOR_INDEX                  = 0x1900,
        LOCAL_GL_RED                          = 0x1903,
        LOCAL_GL_GREEN                        = 0x1904,
        LOCAL_GL_BLUE                         = 0x1905,
        LOCAL_GL_ALPHA                        = 0x1906,
        LOCAL_GL_LUMINANCE                    = 0x1909,
        LOCAL_GL_LUMINANCE_ALPHA              = 0x190A,
        LOCAL_GL_ALPHA_BITS                   = 0x0D55,
        LOCAL_GL_RED_BITS                     = 0x0D52,
        LOCAL_GL_GREEN_BITS                   = 0x0D53,
        LOCAL_GL_BLUE_BITS                    = 0x0D54,
        LOCAL_GL_INDEX_BITS                   = 0x0D51,
        LOCAL_GL_SUBPIXEL_BITS                = 0x0D50,
        LOCAL_GL_AUX_BUFFERS                  = 0x0C00,
        LOCAL_GL_READ_BUFFER                  = 0x0C02,
        LOCAL_GL_DRAW_BUFFER                  = 0x0C01,
        LOCAL_GL_DOUBLEBUFFER                 = 0x0C32,
        LOCAL_GL_STEREO                       = 0x0C33,
        LOCAL_GL_BITMAP                       = 0x1A00,
        LOCAL_GL_COLOR                        = 0x1800,
        LOCAL_GL_DEPTH                        = 0x1801,
        LOCAL_GL_STENCIL                      = 0x1802,
        LOCAL_GL_DITHER                       = 0x0BD0,
        LOCAL_GL_RGB                          = 0x1907,
        LOCAL_GL_RGBA                         = 0x1908,

        /* Implementation limits */
        LOCAL_GL_MAX_LIST_NESTING             = 0x0B31,
        LOCAL_GL_MAX_ATTRIB_STACK_DEPTH       = 0x0D35,
        LOCAL_GL_MAX_MODELVIEW_STACK_DEPTH    = 0x0D36,
        LOCAL_GL_MAX_NAME_STACK_DEPTH         = 0x0D37,
        LOCAL_GL_MAX_PROJECTION_STACK_DEPTH   = 0x0D38,
        LOCAL_GL_MAX_TEXTURE_STACK_DEPTH      = 0x0D39,
        LOCAL_GL_MAX_EVAL_ORDER               = 0x0D30,
        LOCAL_GL_MAX_LIGHTS                   = 0x0D31,
        LOCAL_GL_MAX_CLIP_PLANES              = 0x0D32,
        LOCAL_GL_MAX_TEXTURE_SIZE             = 0x0D33,
        LOCAL_GL_MAX_PIXEL_MAP_TABLE          = 0x0D34,
        LOCAL_GL_MAX_VIEWPORT_DIMS            = 0x0D3A,
        LOCAL_GL_MAX_CLIENT_ATTRIB_STACK_DEPTH= 0x0D3B,

        /* Gets */
        LOCAL_GL_ATTRIB_STACK_DEPTH           = 0x0BB0,
        LOCAL_GL_CLIENT_ATTRIB_STACK_DEPTH    = 0x0BB1,
        LOCAL_GL_COLOR_CLEAR_VALUE            = 0x0C22,
        LOCAL_GL_COLOR_WRITEMASK              = 0x0C23,
        LOCAL_GL_CURRENT_INDEX                = 0x0B01,
        LOCAL_GL_CURRENT_COLOR                = 0x0B00,
        LOCAL_GL_CURRENT_NORMAL               = 0x0B02,
        LOCAL_GL_CURRENT_RASTER_COLOR         = 0x0B04,
        LOCAL_GL_CURRENT_RASTER_DISTANCE      = 0x0B09,
        LOCAL_GL_CURRENT_RASTER_INDEX         = 0x0B05,
        LOCAL_GL_CURRENT_RASTER_POSITION      = 0x0B07,
        LOCAL_GL_CURRENT_RASTER_TEXTURE_COORDS = 0x0B06,
        LOCAL_GL_CURRENT_RASTER_POSITION_VALID = 0x0B08,
        LOCAL_GL_CURRENT_TEXTURE_COORDS       = 0x0B03,
        LOCAL_GL_INDEX_CLEAR_VALUE            = 0x0C20,
        LOCAL_GL_INDEX_MODE                   = 0x0C30,
        LOCAL_GL_INDEX_WRITEMASK              = 0x0C21,
        LOCAL_GL_MODELVIEW_MATRIX             = 0x0BA6,
        LOCAL_GL_MODELVIEW_STACK_DEPTH        = 0x0BA3,
        LOCAL_GL_NAME_STACK_DEPTH             = 0x0D70,
        LOCAL_GL_PROJECTION_MATRIX            = 0x0BA7,
        LOCAL_GL_PROJECTION_STACK_DEPTH       = 0x0BA4,
        LOCAL_GL_RENDER_MODE                  = 0x0C40,
        LOCAL_GL_RGBA_MODE                    = 0x0C31,
        LOCAL_GL_TEXTURE_MATRIX               = 0x0BA8,
        LOCAL_GL_TEXTURE_STACK_DEPTH          = 0x0BA5,
        LOCAL_GL_VIEWPORT                     = 0x0BA2,


        /* Evaluators */
        LOCAL_GL_AUTO_NORMAL                  = 0x0D80,
        LOCAL_GL_MAP1_COLOR_4                 = 0x0D90,
        LOCAL_GL_MAP1_GRID_DOMAIN             = 0x0DD0,
        LOCAL_GL_MAP1_GRID_SEGMENTS           = 0x0DD1,
        LOCAL_GL_MAP1_INDEX                   = 0x0D91,
        LOCAL_GL_MAP1_NORMAL                  = 0x0D92,
        LOCAL_GL_MAP1_TEXTURE_COORD_1         = 0x0D93,
        LOCAL_GL_MAP1_TEXTURE_COORD_2         = 0x0D94,
        LOCAL_GL_MAP1_TEXTURE_COORD_3         = 0x0D95,
        LOCAL_GL_MAP1_TEXTURE_COORD_4         = 0x0D96,
        LOCAL_GL_MAP1_VERTEX_3                = 0x0D97,
        LOCAL_GL_MAP1_VERTEX_4                = 0x0D98,
        LOCAL_GL_MAP2_COLOR_4                 = 0x0DB0,
        LOCAL_GL_MAP2_GRID_DOMAIN             = 0x0DD2,
        LOCAL_GL_MAP2_GRID_SEGMENTS           = 0x0DD3,
        LOCAL_GL_MAP2_INDEX                   = 0x0DB1,
        LOCAL_GL_MAP2_NORMAL                  = 0x0DB2,
        LOCAL_GL_MAP2_TEXTURE_COORD_1         = 0x0DB3,
        LOCAL_GL_MAP2_TEXTURE_COORD_2         = 0x0DB4,
        LOCAL_GL_MAP2_TEXTURE_COORD_3         = 0x0DB5,
        LOCAL_GL_MAP2_TEXTURE_COORD_4         = 0x0DB6,
        LOCAL_GL_MAP2_VERTEX_3                = 0x0DB7,
        LOCAL_GL_MAP2_VERTEX_4                = 0x0DB8,
        LOCAL_GL_COEFF                        = 0x0A00,
        LOCAL_GL_DOMAIN                       = 0x0A02,
        LOCAL_GL_ORDER                        = 0x0A01,

        /* Hints */
        LOCAL_GL_FOG_HINT                     = 0x0C54,
        LOCAL_GL_LINE_SMOOTH_HINT             = 0x0C52,
        LOCAL_GL_PERSPECTIVE_CORRECTION_HINT  = 0x0C50,
        LOCAL_GL_POINT_SMOOTH_HINT            = 0x0C51,
        LOCAL_GL_POLYGON_SMOOTH_HINT          = 0x0C53,
        LOCAL_GL_DONT_CARE                    = 0x1100,
        LOCAL_GL_FASTEST                      = 0x1101,
        LOCAL_GL_NICEST                       = 0x1102,

        /* Scissor box */
        LOCAL_GL_SCISSOR_TEST                 = 0x0C11,
        LOCAL_GL_SCISSOR_BOX                  = 0x0C10,

        /* Pixel Mode / Transfer */
        LOCAL_GL_MAP_COLOR                    = 0x0D10,
        LOCAL_GL_MAP_STENCIL                  = 0x0D11,
        LOCAL_GL_INDEX_SHIFT                  = 0x0D12,
        LOCAL_GL_INDEX_OFFSET                 = 0x0D13,
        LOCAL_GL_RED_SCALE                    = 0x0D14,
        LOCAL_GL_RED_BIAS                     = 0x0D15,
        LOCAL_GL_GREEN_SCALE                  = 0x0D18,
        LOCAL_GL_GREEN_BIAS                   = 0x0D19,
        LOCAL_GL_BLUE_SCALE                   = 0x0D1A,
        LOCAL_GL_BLUE_BIAS                    = 0x0D1B,
        LOCAL_GL_ALPHA_SCALE                  = 0x0D1C,
        LOCAL_GL_ALPHA_BIAS                   = 0x0D1D,
        LOCAL_GL_DEPTH_SCALE                  = 0x0D1E,
        LOCAL_GL_DEPTH_BIAS                   = 0x0D1F,
        LOCAL_GL_PIXEL_MAP_S_TO_S_SIZE        = 0x0CB1,
        LOCAL_GL_PIXEL_MAP_I_TO_I_SIZE        = 0x0CB0,
        LOCAL_GL_PIXEL_MAP_I_TO_R_SIZE        = 0x0CB2,
        LOCAL_GL_PIXEL_MAP_I_TO_G_SIZE        = 0x0CB3,
        LOCAL_GL_PIXEL_MAP_I_TO_B_SIZE        = 0x0CB4,
        LOCAL_GL_PIXEL_MAP_I_TO_A_SIZE        = 0x0CB5,
        LOCAL_GL_PIXEL_MAP_R_TO_R_SIZE        = 0x0CB6,
        LOCAL_GL_PIXEL_MAP_G_TO_G_SIZE        = 0x0CB7,
        LOCAL_GL_PIXEL_MAP_B_TO_B_SIZE        = 0x0CB8,
        LOCAL_GL_PIXEL_MAP_A_TO_A_SIZE        = 0x0CB9,
        LOCAL_GL_PIXEL_MAP_S_TO_S             = 0x0C71,
        LOCAL_GL_PIXEL_MAP_I_TO_I             = 0x0C70,
        LOCAL_GL_PIXEL_MAP_I_TO_R             = 0x0C72,
        LOCAL_GL_PIXEL_MAP_I_TO_G             = 0x0C73,
        LOCAL_GL_PIXEL_MAP_I_TO_B             = 0x0C74,
        LOCAL_GL_PIXEL_MAP_I_TO_A             = 0x0C75,
        LOCAL_GL_PIXEL_MAP_R_TO_R             = 0x0C76,
        LOCAL_GL_PIXEL_MAP_G_TO_G             = 0x0C77,
        LOCAL_GL_PIXEL_MAP_B_TO_B             = 0x0C78,
        LOCAL_GL_PIXEL_MAP_A_TO_A             = 0x0C79,
        LOCAL_GL_PACK_ALIGNMENT               = 0x0D05,
        LOCAL_GL_PACK_LSB_FIRST               = 0x0D01,
        LOCAL_GL_PACK_ROW_LENGTH              = 0x0D02,
        LOCAL_GL_PACK_SKIP_PIXELS             = 0x0D04,
        LOCAL_GL_PACK_SKIP_ROWS               = 0x0D03,
        LOCAL_GL_PACK_SWAP_BYTES              = 0x0D00,
        LOCAL_GL_UNPACK_ALIGNMENT             = 0x0CF5,
        LOCAL_GL_UNPACK_LSB_FIRST             = 0x0CF1,
        LOCAL_GL_UNPACK_ROW_LENGTH            = 0x0CF2,
        LOCAL_GL_UNPACK_SKIP_PIXELS           = 0x0CF4,
        LOCAL_GL_UNPACK_SKIP_ROWS             = 0x0CF3,
        LOCAL_GL_UNPACK_SWAP_BYTES            = 0x0CF0,
        LOCAL_GL_ZOOM_X                       = 0x0D16,
        LOCAL_GL_ZOOM_Y                       = 0x0D17,

        /* Texture mapping */
        LOCAL_GL_TEXTURE_ENV                  = 0x2300,
        LOCAL_GL_TEXTURE_ENV_MODE             = 0x2200,
        LOCAL_GL_TEXTURE_1D                   = 0x0DE0,
        LOCAL_GL_TEXTURE_2D                   = 0x0DE1,
        LOCAL_GL_TEXTURE_WRAP_S               = 0x2802,
        LOCAL_GL_TEXTURE_WRAP_T               = 0x2803,
        LOCAL_GL_TEXTURE_MAG_FILTER           = 0x2800,
        LOCAL_GL_TEXTURE_MIN_FILTER           = 0x2801,
        LOCAL_GL_TEXTURE_ENV_COLOR            = 0x2201,
        LOCAL_GL_TEXTURE_GEN_S                = 0x0C60,
        LOCAL_GL_TEXTURE_GEN_T                = 0x0C61,
        LOCAL_GL_TEXTURE_GEN_MODE             = 0x2500,
        LOCAL_GL_TEXTURE_BORDER_COLOR         = 0x1004,
        LOCAL_GL_TEXTURE_WIDTH                = 0x1000,
        LOCAL_GL_TEXTURE_HEIGHT               = 0x1001,
        LOCAL_GL_TEXTURE_BORDER               = 0x1005,
        LOCAL_GL_TEXTURE_COMPONENTS           = 0x1003,
        LOCAL_GL_TEXTURE_RED_SIZE             = 0x805C,
        LOCAL_GL_TEXTURE_GREEN_SIZE           = 0x805D,
        LOCAL_GL_TEXTURE_BLUE_SIZE            = 0x805E,
        LOCAL_GL_TEXTURE_ALPHA_SIZE           = 0x805F,
        LOCAL_GL_TEXTURE_LUMINANCE_SIZE       = 0x8060,
        LOCAL_GL_TEXTURE_INTENSITY_SIZE       = 0x8061,
        LOCAL_GL_NEAREST_MIPMAP_NEAREST       = 0x2700,
        LOCAL_GL_NEAREST_MIPMAP_LINEAR        = 0x2702,
        LOCAL_GL_LINEAR_MIPMAP_NEAREST        = 0x2701,
        LOCAL_GL_LINEAR_MIPMAP_LINEAR         = 0x2703,
        LOCAL_GL_OBJECT_LINEAR                = 0x2401,
        LOCAL_GL_OBJECT_PLANE                 = 0x2501,
        LOCAL_GL_EYE_LINEAR                   = 0x2400,
        LOCAL_GL_EYE_PLANE                    = 0x2502,
        LOCAL_GL_SPHERE_MAP                   = 0x2402,
        LOCAL_GL_DECAL                        = 0x2101,
        LOCAL_GL_MODULATE                     = 0x2100,
        LOCAL_GL_NEAREST                      = 0x2600,
        LOCAL_GL_REPEAT                       = 0x2901,
        LOCAL_GL_CLAMP                        = 0x2900,
        LOCAL_GL_S                            = 0x2000,
        LOCAL_GL_T                            = 0x2001,
        LOCAL_GL_R                            = 0x2002,
        LOCAL_GL_Q                            = 0x2003,
        LOCAL_GL_TEXTURE_GEN_R                = 0x0C62,
        LOCAL_GL_TEXTURE_GEN_Q                = 0x0C63,

        /* GL 1.1 texturing */
        LOCAL_GL_PROXY_TEXTURE_1D             = 0x8063,
        LOCAL_GL_PROXY_TEXTURE_2D             = 0x8064,
        LOCAL_GL_TEXTURE_PRIORITY             = 0x8066,
        LOCAL_GL_TEXTURE_RESIDENT             = 0x8067,
        LOCAL_GL_TEXTURE_BINDING_1D           = 0x8068,
        LOCAL_GL_TEXTURE_BINDING_2D           = 0x8069,
        LOCAL_GL_TEXTURE_INTERNAL_FORMAT      = 0x1003,

        /* GL 1.2 texturing */
        LOCAL_GL_PACK_SKIP_IMAGES             = 0x806B,
        LOCAL_GL_PACK_IMAGE_HEIGHT            = 0x806C,
        LOCAL_GL_UNPACK_SKIP_IMAGES           = 0x806D,
        LOCAL_GL_UNPACK_IMAGE_HEIGHT          = 0x806E,
        LOCAL_GL_TEXTURE_3D                   = 0x806F,
        LOCAL_GL_PROXY_TEXTURE_3D             = 0x8070,
        LOCAL_GL_TEXTURE_DEPTH                = 0x8071,
        LOCAL_GL_TEXTURE_WRAP_R               = 0x8072,
        LOCAL_GL_MAX_3D_TEXTURE_SIZE          = 0x8073,
        LOCAL_GL_TEXTURE_BINDING_3D           = 0x806A,

        /* Internal texture formats (GL 1.1) */
        LOCAL_GL_ALPHA4                       = 0x803B,
        LOCAL_GL_ALPHA8                       = 0x803C,
        LOCAL_GL_ALPHA12                      = 0x803D,
        LOCAL_GL_ALPHA16                      = 0x803E,
        LOCAL_GL_LUMINANCE4                   = 0x803F,
        LOCAL_GL_LUMINANCE8                   = 0x8040,
        LOCAL_GL_LUMINANCE12                  = 0x8041,
        LOCAL_GL_LUMINANCE16                  = 0x8042,
        LOCAL_GL_LUMINANCE4_ALPHA4            = 0x8043,
        LOCAL_GL_LUMINANCE6_ALPHA2            = 0x8044,
        LOCAL_GL_LUMINANCE8_ALPHA8            = 0x8045,
        LOCAL_GL_LUMINANCE12_ALPHA4           = 0x8046,
        LOCAL_GL_LUMINANCE12_ALPHA12          = 0x8047,
        LOCAL_GL_LUMINANCE16_ALPHA16          = 0x8048,
        LOCAL_GL_INTENSITY                    = 0x8049,
        LOCAL_GL_INTENSITY4                   = 0x804A,
        LOCAL_GL_INTENSITY8                   = 0x804B,
        LOCAL_GL_INTENSITY12                  = 0x804C,
        LOCAL_GL_INTENSITY16                  = 0x804D,
        LOCAL_GL_R3_G3_B2                     = 0x2A10,
        LOCAL_GL_RGB4                         = 0x804F,
        LOCAL_GL_RGB5                         = 0x8050,
        LOCAL_GL_RGB8                         = 0x8051,
        LOCAL_GL_RGB10                        = 0x8052,
        LOCAL_GL_RGB12                        = 0x8053,
        LOCAL_GL_RGB16                        = 0x8054,
        LOCAL_GL_RGBA2                        = 0x8055,
        LOCAL_GL_RGBA4                        = 0x8056,
        LOCAL_GL_RGB5_A1                      = 0x8057,
        LOCAL_GL_RGBA8                        = 0x8058,
        LOCAL_GL_RGB10_A2                     = 0x8059,
        LOCAL_GL_RGBA12                       = 0x805A,
        LOCAL_GL_RGBA16                       = 0x805B,

        /* Utility */
        LOCAL_GL_VENDOR                       = 0x1F00,
        LOCAL_GL_RENDERER                     = 0x1F01,
        LOCAL_GL_VERSION                      = 0x1F02,
        LOCAL_GL_EXTENSIONS                   = 0x1F03,

        /* Errors */
        LOCAL_GL_INVALID_VALUE                = 0x0501,
        LOCAL_GL_INVALID_ENUM                 = 0x0500,
        LOCAL_GL_INVALID_OPERATION            = 0x0502,
        LOCAL_GL_STACK_OVERFLOW               = 0x0503,
        LOCAL_GL_STACK_UNDERFLOW              = 0x0504,
        LOCAL_GL_OUT_OF_MEMORY                = 0x0505,

        /*
         * Extensions
         */

        /* LOCAL_GL_EXT_blend_minmax and LOCAL_GL_EXT_blend_color */
        LOCAL_GL_CONSTANT_COLOR_EXT                   = 0x8001,
        LOCAL_GL_ONE_MINUS_CONSTANT_COLOR_EXT         = 0x8002,
        LOCAL_GL_CONSTANT_ALPHA_EXT                   = 0x8003,
        LOCAL_GL_ONE_MINUS_CONSTANT_ALPHA_EXT         = 0x8004,
        LOCAL_GL_BLEND_EQUATION_EXT                   = 0x8009,
        LOCAL_GL_MIN_EXT                              = 0x8007,
        LOCAL_GL_MAX_EXT                              = 0x8008,
        LOCAL_GL_FUNC_ADD_EXT                         = 0x8006,
        LOCAL_GL_FUNC_SUBTRACT_EXT                    = 0x800A,
        LOCAL_GL_FUNC_REVERSE_SUBTRACT_EXT            = 0x800B,
        LOCAL_GL_BLEND_COLOR_EXT                      = 0x8005,

        /* LOCAL_GL_EXT_polygon_offset */
        LOCAL_GL_POLYGON_OFFSET_EXT                   = 0x8037,
        LOCAL_GL_POLYGON_OFFSET_FACTOR_EXT            = 0x8038,
        LOCAL_GL_POLYGON_OFFSET_BIAS_EXT              = 0x8039,

        /* LOCAL_GL_EXT_vertex_array */
        LOCAL_GL_VERTEX_ARRAY_EXT                     = 0x8074,
        LOCAL_GL_NORMAL_ARRAY_EXT                     = 0x8075,
        LOCAL_GL_COLOR_ARRAY_EXT                      = 0x8076,
        LOCAL_GL_INDEX_ARRAY_EXT                      = 0x8077,
        LOCAL_GL_TEXTURE_COORD_ARRAY_EXT              = 0x8078,
        LOCAL_GL_EDGE_FLAG_ARRAY_EXT                  = 0x8079,
        LOCAL_GL_VERTEX_ARRAY_SIZE_EXT                = 0x807A,
        LOCAL_GL_VERTEX_ARRAY_TYPE_EXT                = 0x807B,
        LOCAL_GL_VERTEX_ARRAY_STRIDE_EXT              = 0x807C,
        LOCAL_GL_VERTEX_ARRAY_COUNT_EXT               = 0x807D,
        LOCAL_GL_NORMAL_ARRAY_TYPE_EXT                = 0x807E,
        LOCAL_GL_NORMAL_ARRAY_STRIDE_EXT              = 0x807F,
        LOCAL_GL_NORMAL_ARRAY_COUNT_EXT               = 0x8080,
        LOCAL_GL_COLOR_ARRAY_SIZE_EXT                 = 0x8081,
        LOCAL_GL_COLOR_ARRAY_TYPE_EXT                 = 0x8082,
        LOCAL_GL_COLOR_ARRAY_STRIDE_EXT               = 0x8083,
        LOCAL_GL_COLOR_ARRAY_COUNT_EXT                = 0x8084,
        LOCAL_GL_INDEX_ARRAY_TYPE_EXT                 = 0x8085,
        LOCAL_GL_INDEX_ARRAY_STRIDE_EXT               = 0x8086,
        LOCAL_GL_INDEX_ARRAY_COUNT_EXT                = 0x8087,
        LOCAL_GL_TEXTURE_COORD_ARRAY_SIZE_EXT         = 0x8088,
        LOCAL_GL_TEXTURE_COORD_ARRAY_TYPE_EXT         = 0x8089,
        LOCAL_GL_TEXTURE_COORD_ARRAY_STRIDE_EXT       = 0x808A,
        LOCAL_GL_TEXTURE_COORD_ARRAY_COUNT_EXT        = 0x808B,
        LOCAL_GL_EDGE_FLAG_ARRAY_STRIDE_EXT           = 0x808C,
        LOCAL_GL_EDGE_FLAG_ARRAY_COUNT_EXT            = 0x808D,
        LOCAL_GL_VERTEX_ARRAY_POINTER_EXT             = 0x808E,
        LOCAL_GL_NORMAL_ARRAY_POINTER_EXT             = 0x808F,
        LOCAL_GL_COLOR_ARRAY_POINTER_EXT              = 0x8090,
        LOCAL_GL_INDEX_ARRAY_POINTER_EXT              = 0x8091,
        LOCAL_GL_TEXTURE_COORD_ARRAY_POINTER_EXT      = 0x8092,
        LOCAL_GL_EDGE_FLAG_ARRAY_POINTER_EXT          = 0x8093,

        /* LOCAL_GL_EXT_texture_object */
        LOCAL_GL_TEXTURE_PRIORITY_EXT                 = 0x8066,
        LOCAL_GL_TEXTURE_RESIDENT_EXT                 = 0x8067,
        LOCAL_GL_TEXTURE_1D_BINDING_EXT               = 0x8068,
        LOCAL_GL_TEXTURE_2D_BINDING_EXT               = 0x8069,

        /* LOCAL_GL_EXT_texture3D */
        LOCAL_GL_PACK_SKIP_IMAGES_EXT                 = 0x806B,
        LOCAL_GL_PACK_IMAGE_HEIGHT_EXT                = 0x806C,
        LOCAL_GL_UNPACK_SKIP_IMAGES_EXT               = 0x806D,
        LOCAL_GL_UNPACK_IMAGE_HEIGHT_EXT              = 0x806E,
        LOCAL_GL_TEXTURE_3D_EXT                       = 0x806F,
        LOCAL_GL_PROXY_TEXTURE_3D_EXT                 = 0x8070,
        LOCAL_GL_TEXTURE_DEPTH_EXT                    = 0x8071,
        LOCAL_GL_TEXTURE_WRAP_R_EXT                   = 0x8072,
        LOCAL_GL_MAX_3D_TEXTURE_SIZE_EXT              = 0x8073,
        LOCAL_GL_TEXTURE_3D_BINDING_EXT               = 0x806A,

        /* LOCAL_GL_EXT_paletted_texture */
        LOCAL_GL_TABLE_TOO_LARGE_EXT                  = 0x8031,
        LOCAL_GL_COLOR_TABLE_FORMAT_EXT               = 0x80D8,
        LOCAL_GL_COLOR_TABLE_WIDTH_EXT                = 0x80D9,
        LOCAL_GL_COLOR_TABLE_RED_SIZE_EXT             = 0x80DA,
        LOCAL_GL_COLOR_TABLE_GREEN_SIZE_EXT           = 0x80DB,
        LOCAL_GL_COLOR_TABLE_BLUE_SIZE_EXT            = 0x80DC,
        LOCAL_GL_COLOR_TABLE_ALPHA_SIZE_EXT           = 0x80DD,
        LOCAL_GL_COLOR_TABLE_LUMINANCE_SIZE_EXT       = 0x80DE,
        LOCAL_GL_COLOR_TABLE_INTENSITY_SIZE_EXT       = 0x80DF,
        LOCAL_GL_TEXTURE_INDEX_SIZE_EXT               = 0x80ED,
        LOCAL_GL_COLOR_INDEX1_EXT                     = 0x80E2,
        LOCAL_GL_COLOR_INDEX2_EXT                     = 0x80E3,
        LOCAL_GL_COLOR_INDEX4_EXT                     = 0x80E4,
        LOCAL_GL_COLOR_INDEX8_EXT                     = 0x80E5,
        LOCAL_GL_COLOR_INDEX12_EXT                    = 0x80E6,
        LOCAL_GL_COLOR_INDEX16_EXT                    = 0x80E7,

        /* LOCAL_GL_EXT_shared_texture_palette */
        LOCAL_GL_SHARED_TEXTURE_PALETTE_EXT           = 0x81FB,

        /* LOCAL_GL_EXT_point_parameters */
        LOCAL_GL_POINT_SIZE_MIN_EXT                   = 0x8126,
        LOCAL_GL_POINT_SIZE_MAX_EXT                   = 0x8127,
        LOCAL_GL_POINT_FADE_THRESHOLD_SIZE_EXT        = 0x8128,
        LOCAL_GL_DISTANCE_ATTENUATION_EXT             = 0x8129,

        /* LOCAL_GL_EXT_rescale_normal */
        LOCAL_GL_RESCALE_NORMAL_EXT                   = 0x803A,

        /* LOCAL_GL_EXT_abgr */
        LOCAL_GL_ABGR_EXT                             = 0x8000,

        /* LOCAL_GL_SGIS_multitexture */
        LOCAL_GL_SELECTED_TEXTURE_SGIS                = 0x835C,
        LOCAL_GL_SELECTED_TEXTURE_COORD_SET_SGIS      = 0x835D,
        LOCAL_GL_MAX_TEXTURES_SGIS                    = 0x835E,
        LOCAL_GL_TEXTURE0_SGIS                        = 0x835F,
        LOCAL_GL_TEXTURE1_SGIS                        = 0x8360,
        LOCAL_GL_TEXTURE2_SGIS                        = 0x8361,
        LOCAL_GL_TEXTURE3_SGIS                        = 0x8362,
        LOCAL_GL_TEXTURE_COORD_SET_SOURCE_SGIS        = 0x8363,

        /* LOCAL_GL_EXT_multitexture */
        LOCAL_GL_SELECTED_TEXTURE_EXT                 = 0x83C0,
        LOCAL_GL_SELECTED_TEXTURE_COORD_SET_EXT       = 0x83C1,
        LOCAL_GL_SELECTED_TEXTURE_TRANSFORM_EXT       = 0x83C2,
        LOCAL_GL_MAX_TEXTURES_EXT                     = 0x83C3,
        LOCAL_GL_MAX_TEXTURE_COORD_SETS_EXT           = 0x83C4,
        LOCAL_GL_TEXTURE_ENV_COORD_SET_EXT            = 0x83C5,
        LOCAL_GL_TEXTURE0_EXT                         = 0x83C6,
        LOCAL_GL_TEXTURE1_EXT                         = 0x83C7,
        LOCAL_GL_TEXTURE2_EXT                         = 0x83C8,
        LOCAL_GL_TEXTURE3_EXT                         = 0x83C9,

        /* LOCAL_GL_SGIS_texture_edge_clamp */
        LOCAL_GL_CLAMP_TO_EDGE_SGIS                   = 0x812F,

        /* OpenGL 1.2 */
        LOCAL_GL_RESCALE_NORMAL                       = 0x803A,
        LOCAL_GL_CLAMP_TO_EDGE                        = 0x812F,
        LOCAL_GL_MAX_ELEMENTS_VERTICES                = 0xF0E8,
        LOCAL_GL_MAX_ELEMENTS_INDICES                 = 0xF0E9,
        LOCAL_GL_BGR                                  = 0x80E0,
        LOCAL_GL_BGRA                                 = 0x80E1,
        LOCAL_GL_UNSIGNED_BYTE_3_3_2                  = 0x8032,
        LOCAL_GL_UNSIGNED_BYTE_2_3_3_REV              = 0x8362,
        LOCAL_GL_UNSIGNED_SHORT_5_6_5                 = 0x8363,
        LOCAL_GL_UNSIGNED_SHORT_5_6_5_REV             = 0x8364,
        LOCAL_GL_UNSIGNED_SHORT_4_4_4_4               = 0x8033,
        LOCAL_GL_UNSIGNED_SHORT_4_4_4_4_REV           = 0x8365,
        LOCAL_GL_UNSIGNED_SHORT_5_5_5_1               = 0x8034,
        LOCAL_GL_UNSIGNED_SHORT_1_5_5_5_REV           = 0x8366,
        LOCAL_GL_UNSIGNED_INT_8_8_8_8                 = 0x8035,
        LOCAL_GL_UNSIGNED_INT_8_8_8_8_REV             = 0x8367,
        LOCAL_GL_UNSIGNED_INT_10_10_10_2              = 0x8036,
        LOCAL_GL_UNSIGNED_INT_2_10_10_10_REV          = 0x8368,
        LOCAL_GL_LIGHT_MODEL_COLOR_CONTROL            = 0x81F8,
        LOCAL_GL_SINGLE_COLOR                         = 0x81F9,
        LOCAL_GL_SEPARATE_SPECULAR_COLOR              = 0x81FA,
        LOCAL_GL_TEXTURE_MIN_LOD                      = 0x813A,
        LOCAL_GL_TEXTURE_MAX_LOD                      = 0x813B,
        LOCAL_GL_TEXTURE_BASE_LEVEL                   = 0x813C,
        LOCAL_GL_TEXTURE_MAX_LEVEL                    = 0x813D
};

typedef struct { GLenum e; const char* name; } ENUM;
#define EDEF(VAR) { (GLenum)(LOCAL_GL_##VAR), #VAR }

static ENUM enums[] =
  {
  EDEF(BYTE),
  EDEF(UNSIGNED_BYTE),
  EDEF(SHORT),
  EDEF(UNSIGNED_SHORT),
  EDEF(INT),
  EDEF(UNSIGNED_INT),
  EDEF(FLOAT),
  EDEF(DOUBLE),
  EDEF(2_BYTES),
  EDEF(3_BYTES),
  EDEF(4_BYTES),
/*
  EDEF(LINES),
  EDEF(POINTS),
  EDEF(LINE_STRIP),
  EDEF(LINE_LOOP),
  EDEF(TRIANGLES),
  EDEF(TRIANGLE_STRIP),
  EDEF(TRIANGLE_FAN),
  EDEF(QUADS),
  EDEF(QUAD_STRIP),
  EDEF(POLYGON),
  EDEF(EDGE_FLAG),
*/
  EDEF(VERTEX_ARRAY),
  EDEF(NORMAL_ARRAY),
  EDEF(COLOR_ARRAY),
  EDEF(INDEX_ARRAY),
  EDEF(TEXTURE_COORD_ARRAY),
  EDEF(EDGE_FLAG_ARRAY),
  EDEF(VERTEX_ARRAY_SIZE),
  EDEF(VERTEX_ARRAY_TYPE),
  EDEF(VERTEX_ARRAY_STRIDE),
  EDEF(NORMAL_ARRAY_TYPE),
  EDEF(NORMAL_ARRAY_STRIDE),
  EDEF(COLOR_ARRAY_SIZE),
  EDEF(COLOR_ARRAY_TYPE),
  EDEF(COLOR_ARRAY_STRIDE),
  EDEF(INDEX_ARRAY_TYPE),
  EDEF(INDEX_ARRAY_STRIDE),
  EDEF(TEXTURE_COORD_ARRAY_SIZE),
  EDEF(TEXTURE_COORD_ARRAY_TYPE),
  EDEF(TEXTURE_COORD_ARRAY_STRIDE),
  EDEF(EDGE_FLAG_ARRAY_STRIDE),
  EDEF(VERTEX_ARRAY_POINTER),
  EDEF(NORMAL_ARRAY_POINTER),
  EDEF(COLOR_ARRAY_POINTER),
  EDEF(INDEX_ARRAY_POINTER),
  EDEF(TEXTURE_COORD_ARRAY_POINTER),
  EDEF(EDGE_FLAG_ARRAY_POINTER),
  EDEF(V2F),
  EDEF(V3F),
  EDEF(C4UB_V2F),
  EDEF(C4UB_V3F),
  EDEF(C3F_V3F),
  EDEF(N3F_V3F),
  EDEF(C4F_N3F_V3F),
  EDEF(T2F_V3F),
  EDEF(T4F_V4F),
  EDEF(T2F_C4UB_V3F),
  EDEF(T2F_C3F_V3F),
  EDEF(T2F_N3F_V3F),
  EDEF(T2F_C4F_N3F_V3F),
  EDEF(T4F_C4F_N3F_V4F),
  EDEF(MATRIX_MODE),
  EDEF(MODELVIEW),
  EDEF(PROJECTION),
  EDEF(TEXTURE),
  EDEF(POINT_SMOOTH),
  EDEF(POINT_SIZE),
  EDEF(POINT_SIZE_GRANULARITY),
  EDEF(POINT_SIZE_RANGE),
  EDEF(LINE_SMOOTH),
  EDEF(LINE_STIPPLE),
  EDEF(LINE_STIPPLE_PATTERN),
  EDEF(LINE_STIPPLE_REPEAT),
  EDEF(LINE_WIDTH),
  EDEF(LINE_WIDTH_GRANULARITY),
  EDEF(LINE_WIDTH_RANGE),
  EDEF(POINT),
  EDEF(LINE),
  EDEF(FILL),
  EDEF(CCW),
  EDEF(CW),
  EDEF(FRONT),
  EDEF(BACK),
  EDEF(CULL_FACE),
  EDEF(CULL_FACE_MODE),
  EDEF(POLYGON_SMOOTH),
  EDEF(POLYGON_STIPPLE),
  EDEF(FRONT_FACE),
  EDEF(POLYGON_MODE),
  EDEF(POLYGON_OFFSET_FACTOR),
  EDEF(POLYGON_OFFSET_UNITS),
  EDEF(POLYGON_OFFSET_POINT),
  EDEF(POLYGON_OFFSET_LINE),
  EDEF(POLYGON_OFFSET_FILL),
  EDEF(COMPILE),
  EDEF(COMPILE_AND_EXECUTE),
  EDEF(LIST_BASE),
  EDEF(LIST_INDEX),
  EDEF(LIST_MODE),
  EDEF(NEVER),
  EDEF(LESS),
  EDEF(GEQUAL),
  EDEF(LEQUAL),
  EDEF(GREATER),
  EDEF(NOTEQUAL),
  EDEF(EQUAL),
  EDEF(ALWAYS),
  EDEF(DEPTH_TEST),
  EDEF(DEPTH_BITS),
  EDEF(DEPTH_CLEAR_VALUE),
  EDEF(DEPTH_FUNC),
  EDEF(DEPTH_RANGE),
  EDEF(DEPTH_WRITEMASK),
  EDEF(DEPTH_COMPONENT),
  EDEF(LIGHTING),
  EDEF(LIGHT0),
  EDEF(LIGHT1),
  EDEF(LIGHT2),
  EDEF(LIGHT3),
  EDEF(LIGHT4),
  EDEF(LIGHT5),
  EDEF(LIGHT6),
  EDEF(LIGHT7),
  EDEF(SPOT_EXPONENT),
  EDEF(SPOT_CUTOFF),
  EDEF(CONSTANT_ATTENUATION),
  EDEF(LINEAR_ATTENUATION),
  EDEF(QUADRATIC_ATTENUATION),
  EDEF(AMBIENT),
  EDEF(DIFFUSE),
  EDEF(SPECULAR),
  EDEF(SHININESS),
  EDEF(EMISSION),
  EDEF(POSITION),
  EDEF(SPOT_DIRECTION),
  EDEF(AMBIENT_AND_DIFFUSE),
  EDEF(COLOR_INDEXES),
  EDEF(LIGHT_MODEL_TWO_SIDE),
  EDEF(LIGHT_MODEL_LOCAL_VIEWER),
  EDEF(LIGHT_MODEL_AMBIENT),
  EDEF(FRONT_AND_BACK),
  EDEF(SHADE_MODEL),
  EDEF(FLAT),
  EDEF(SMOOTH),
  EDEF(COLOR_MATERIAL),
  EDEF(COLOR_MATERIAL_FACE),
  EDEF(COLOR_MATERIAL_PARAMETER),
  EDEF(NORMALIZE),
  EDEF(CLIP_PLANE0),
  EDEF(CLIP_PLANE1),
  EDEF(CLIP_PLANE2),
  EDEF(CLIP_PLANE3),
  EDEF(CLIP_PLANE4),
  EDEF(CLIP_PLANE5),
  EDEF(ACCUM_RED_BITS),
  EDEF(ACCUM_GREEN_BITS),
  EDEF(ACCUM_BLUE_BITS),
  EDEF(ACCUM_ALPHA_BITS),
  EDEF(ACCUM_CLEAR_VALUE),
  EDEF(ACCUM),
  EDEF(ADD),
  EDEF(LOAD),
  EDEF(MULT),
  EDEF(RETURN),
  EDEF(ALPHA_TEST),
  EDEF(ALPHA_TEST_REF),
  EDEF(ALPHA_TEST_FUNC),
  EDEF(BLEND),
  EDEF(BLEND_SRC),
  EDEF(BLEND_DST),
  EDEF(ZERO),
  EDEF(ONE),
  EDEF(SRC_COLOR),
  EDEF(ONE_MINUS_SRC_COLOR),
  EDEF(DST_COLOR),
  EDEF(ONE_MINUS_DST_COLOR),
  EDEF(SRC_ALPHA),
  EDEF(ONE_MINUS_SRC_ALPHA),
  EDEF(DST_ALPHA),
  EDEF(ONE_MINUS_DST_ALPHA),
  EDEF(SRC_ALPHA_SATURATE),
  EDEF(CONSTANT_COLOR),
  EDEF(ONE_MINUS_CONSTANT_COLOR),
  EDEF(CONSTANT_ALPHA),
  EDEF(ONE_MINUS_CONSTANT_ALPHA),
  EDEF(FEEDBACK),
  EDEF(RENDER),
  EDEF(SELECT),
  EDEF(2D),
  EDEF(3D),
  EDEF(3D_COLOR),
  EDEF(3D_COLOR_TEXTURE),
  EDEF(4D_COLOR_TEXTURE),
  EDEF(POINT_TOKEN),
  EDEF(LINE_TOKEN),
  EDEF(LINE_RESET_TOKEN),
  EDEF(POLYGON_TOKEN),
  EDEF(BITMAP_TOKEN),
  EDEF(DRAW_PIXEL_TOKEN),
  EDEF(COPY_PIXEL_TOKEN),
  EDEF(PASS_THROUGH_TOKEN),
  EDEF(FEEDBACK_BUFFER_POINTER),
  EDEF(FEEDBACK_BUFFER_SIZE),
  EDEF(FEEDBACK_BUFFER_TYPE),
  EDEF(SELECTION_BUFFER_POINTER),
  EDEF(SELECTION_BUFFER_SIZE),
  EDEF(FOG),
  EDEF(FOG_MODE),
  EDEF(FOG_DENSITY),
  EDEF(FOG_COLOR),
  EDEF(FOG_INDEX),
  EDEF(FOG_START),
  EDEF(FOG_END),
  EDEF(LINEAR),
  EDEF(EXP),
  EDEF(EXP2),
  EDEF(LOGIC_OP),
  EDEF(INDEX_LOGIC_OP),
  EDEF(COLOR_LOGIC_OP),
  EDEF(LOGIC_OP_MODE),
  EDEF(CLEAR),
  EDEF(SET),
  EDEF(COPY),
  EDEF(COPY_INVERTED),
  EDEF(NOOP),
  EDEF(INVERT),
  EDEF(AND),
  EDEF(NAND),
  EDEF(OR),
  EDEF(NOR),
  EDEF(XOR),
  EDEF(EQUIV),
  EDEF(AND_REVERSE),
  EDEF(AND_INVERTED),
  EDEF(OR_REVERSE),
  EDEF(OR_INVERTED),
  EDEF(STENCIL_TEST),
  EDEF(STENCIL_WRITEMASK),
  EDEF(STENCIL_BITS),
  EDEF(STENCIL_FUNC),
  EDEF(STENCIL_VALUE_MASK),
  EDEF(STENCIL_REF),
  EDEF(STENCIL_FAIL),
  EDEF(STENCIL_PASS_DEPTH_PASS),
  EDEF(STENCIL_PASS_DEPTH_FAIL),
  EDEF(STENCIL_CLEAR_VALUE),
  EDEF(STENCIL_INDEX),
  EDEF(KEEP),
  EDEF(REPLACE),
  EDEF(INCR),
  EDEF(DECR),
  EDEF(NONE),
  EDEF(LEFT),
  EDEF(RIGHT),
  EDEF(FRONT_LEFT),
  EDEF(FRONT_RIGHT),
  EDEF(BACK_LEFT),
  EDEF(BACK_RIGHT),
  EDEF(AUX0),
  EDEF(AUX1),
  EDEF(AUX2),
  EDEF(AUX3),
  EDEF(COLOR_INDEX),
  EDEF(RED),
  EDEF(GREEN),
  EDEF(BLUE),
  EDEF(ALPHA),
  EDEF(LUMINANCE),
  EDEF(LUMINANCE_ALPHA),
  EDEF(ALPHA_BITS),
  EDEF(RED_BITS),
  EDEF(GREEN_BITS),
  EDEF(BLUE_BITS),
  EDEF(INDEX_BITS),
  EDEF(SUBPIXEL_BITS),
  EDEF(AUX_BUFFERS),
  EDEF(READ_BUFFER),
  EDEF(DRAW_BUFFER),
  EDEF(DOUBLEBUFFER),
  EDEF(STEREO),
  EDEF(BITMAP),
  EDEF(COLOR),
  EDEF(DEPTH),
  EDEF(STENCIL),
  EDEF(DITHER),
  EDEF(RGB),
  EDEF(RGBA),
  EDEF(MAX_LIST_NESTING),
  EDEF(MAX_ATTRIB_STACK_DEPTH),
  EDEF(MAX_MODELVIEW_STACK_DEPTH),
  EDEF(MAX_NAME_STACK_DEPTH),
  EDEF(MAX_PROJECTION_STACK_DEPTH),
  EDEF(MAX_TEXTURE_STACK_DEPTH),
  EDEF(MAX_EVAL_ORDER),
  EDEF(MAX_LIGHTS),
  EDEF(MAX_CLIP_PLANES),
  EDEF(MAX_TEXTURE_SIZE),
  EDEF(MAX_PIXEL_MAP_TABLE),
  EDEF(MAX_VIEWPORT_DIMS),
  EDEF(MAX_CLIENT_ATTRIB_STACK_DEPTH),
  EDEF(ATTRIB_STACK_DEPTH),
  EDEF(CLIENT_ATTRIB_STACK_DEPTH),
  EDEF(COLOR_CLEAR_VALUE),
  EDEF(COLOR_WRITEMASK),
  EDEF(CURRENT_INDEX),
  EDEF(CURRENT_COLOR),
  EDEF(CURRENT_NORMAL),
  EDEF(CURRENT_RASTER_COLOR),
  EDEF(CURRENT_RASTER_DISTANCE),
  EDEF(CURRENT_RASTER_INDEX),
  EDEF(CURRENT_RASTER_POSITION),
  EDEF(CURRENT_RASTER_TEXTURE_COORDS),
  EDEF(CURRENT_RASTER_POSITION_VALID),
  EDEF(CURRENT_TEXTURE_COORDS),
  EDEF(INDEX_CLEAR_VALUE),
  EDEF(INDEX_MODE),
  EDEF(INDEX_WRITEMASK),
  EDEF(MODELVIEW_MATRIX),
  EDEF(MODELVIEW_STACK_DEPTH),
  EDEF(NAME_STACK_DEPTH),
  EDEF(PROJECTION_MATRIX),
  EDEF(PROJECTION_STACK_DEPTH),
  EDEF(RENDER_MODE),
  EDEF(RGBA_MODE),
  EDEF(TEXTURE_MATRIX),
  EDEF(TEXTURE_STACK_DEPTH),
  EDEF(VIEWPORT),
  EDEF(AUTO_NORMAL),
  EDEF(MAP1_COLOR_4),
  EDEF(MAP1_GRID_DOMAIN),
  EDEF(MAP1_GRID_SEGMENTS),
  EDEF(MAP1_INDEX),
  EDEF(MAP1_NORMAL),
  EDEF(MAP1_TEXTURE_COORD_1),
  EDEF(MAP1_TEXTURE_COORD_2),
  EDEF(MAP1_TEXTURE_COORD_3),
  EDEF(MAP1_TEXTURE_COORD_4),
  EDEF(MAP1_VERTEX_3),
  EDEF(MAP1_VERTEX_4),
  EDEF(MAP2_COLOR_4),
  EDEF(MAP2_GRID_DOMAIN),
  EDEF(MAP2_GRID_SEGMENTS),
  EDEF(MAP2_INDEX),
  EDEF(MAP2_NORMAL),
  EDEF(MAP2_TEXTURE_COORD_1),
  EDEF(MAP2_TEXTURE_COORD_2),
  EDEF(MAP2_TEXTURE_COORD_3),
  EDEF(MAP2_TEXTURE_COORD_4),
  EDEF(MAP2_VERTEX_3),
  EDEF(MAP2_VERTEX_4),
  EDEF(COEFF),
  EDEF(DOMAIN),
  EDEF(ORDER),
  EDEF(FOG_HINT),
  EDEF(LINE_SMOOTH_HINT),
  EDEF(PERSPECTIVE_CORRECTION_HINT),
  EDEF(POINT_SMOOTH_HINT),
  EDEF(POLYGON_SMOOTH_HINT),
  EDEF(DONT_CARE),
  EDEF(FASTEST),
  EDEF(NICEST),
  EDEF(SCISSOR_TEST),
  EDEF(SCISSOR_BOX),
  EDEF(MAP_COLOR),
  EDEF(MAP_STENCIL),
  EDEF(INDEX_SHIFT),
  EDEF(INDEX_OFFSET),
  EDEF(RED_SCALE),
  EDEF(RED_BIAS),
  EDEF(GREEN_SCALE),
  EDEF(GREEN_BIAS),
  EDEF(BLUE_SCALE),
  EDEF(BLUE_BIAS),
  EDEF(ALPHA_SCALE),
  EDEF(ALPHA_BIAS),
  EDEF(DEPTH_SCALE),
  EDEF(DEPTH_BIAS),
  EDEF(PIXEL_MAP_S_TO_S_SIZE),
  EDEF(PIXEL_MAP_I_TO_I_SIZE),
  EDEF(PIXEL_MAP_I_TO_R_SIZE),
  EDEF(PIXEL_MAP_I_TO_G_SIZE),
  EDEF(PIXEL_MAP_I_TO_B_SIZE),
  EDEF(PIXEL_MAP_I_TO_A_SIZE),
  EDEF(PIXEL_MAP_R_TO_R_SIZE),
  EDEF(PIXEL_MAP_G_TO_G_SIZE),
  EDEF(PIXEL_MAP_B_TO_B_SIZE),
  EDEF(PIXEL_MAP_A_TO_A_SIZE),
  EDEF(PIXEL_MAP_S_TO_S),
  EDEF(PIXEL_MAP_I_TO_I),
  EDEF(PIXEL_MAP_I_TO_R),
  EDEF(PIXEL_MAP_I_TO_G),
  EDEF(PIXEL_MAP_I_TO_B),
  EDEF(PIXEL_MAP_I_TO_A),
  EDEF(PIXEL_MAP_R_TO_R),
  EDEF(PIXEL_MAP_G_TO_G),
  EDEF(PIXEL_MAP_B_TO_B),
  EDEF(PIXEL_MAP_A_TO_A),
  EDEF(PACK_ALIGNMENT),
  EDEF(PACK_LSB_FIRST),
  EDEF(PACK_ROW_LENGTH),
  EDEF(PACK_SKIP_PIXELS),
  EDEF(PACK_SKIP_ROWS),
  EDEF(PACK_SWAP_BYTES),
  EDEF(UNPACK_ALIGNMENT),
  EDEF(UNPACK_LSB_FIRST),
  EDEF(UNPACK_ROW_LENGTH),
  EDEF(UNPACK_SKIP_PIXELS),
  EDEF(UNPACK_SKIP_ROWS),
  EDEF(UNPACK_SWAP_BYTES),
  EDEF(ZOOM_X),
  EDEF(ZOOM_Y),
  EDEF(TEXTURE_ENV),
  EDEF(TEXTURE_ENV_MODE),
  EDEF(TEXTURE_1D),
  EDEF(TEXTURE_2D),
  EDEF(TEXTURE_WRAP_S),
  EDEF(TEXTURE_WRAP_T),
  EDEF(TEXTURE_MAG_FILTER),
  EDEF(TEXTURE_MIN_FILTER),
  EDEF(TEXTURE_ENV_COLOR),
  EDEF(TEXTURE_GEN_S),
  EDEF(TEXTURE_GEN_T),
  EDEF(TEXTURE_GEN_MODE),
  EDEF(TEXTURE_BORDER_COLOR),
  EDEF(TEXTURE_WIDTH),
  EDEF(TEXTURE_HEIGHT),
  EDEF(TEXTURE_BORDER),
  EDEF(TEXTURE_COMPONENTS),
  EDEF(TEXTURE_RED_SIZE),
  EDEF(TEXTURE_GREEN_SIZE),
  EDEF(TEXTURE_BLUE_SIZE),
  EDEF(TEXTURE_ALPHA_SIZE),
  EDEF(TEXTURE_LUMINANCE_SIZE),
  EDEF(TEXTURE_INTENSITY_SIZE),
  EDEF(NEAREST_MIPMAP_NEAREST),
  EDEF(NEAREST_MIPMAP_LINEAR),
  EDEF(LINEAR_MIPMAP_NEAREST),
  EDEF(LINEAR_MIPMAP_LINEAR),
  EDEF(OBJECT_LINEAR),
  EDEF(OBJECT_PLANE),
  EDEF(EYE_LINEAR),
  EDEF(EYE_PLANE),
  EDEF(SPHERE_MAP),
  EDEF(DECAL),
  EDEF(MODULATE),
  EDEF(NEAREST),
  EDEF(REPEAT),
  EDEF(CLAMP),
  EDEF(S),
  EDEF(T),
  EDEF(R),
  EDEF(Q),
  EDEF(TEXTURE_GEN_R),
  EDEF(TEXTURE_GEN_Q),
  EDEF(PROXY_TEXTURE_1D),
  EDEF(PROXY_TEXTURE_2D),
  EDEF(TEXTURE_PRIORITY),
  EDEF(TEXTURE_RESIDENT),
  EDEF(TEXTURE_BINDING_1D),
  EDEF(TEXTURE_BINDING_2D),
  EDEF(TEXTURE_INTERNAL_FORMAT),
  EDEF(PACK_SKIP_IMAGES),
  EDEF(PACK_IMAGE_HEIGHT),
  EDEF(UNPACK_SKIP_IMAGES),
  EDEF(UNPACK_IMAGE_HEIGHT),
  EDEF(TEXTURE_3D),
  EDEF(PROXY_TEXTURE_3D),
  EDEF(TEXTURE_DEPTH),
  EDEF(TEXTURE_WRAP_R),
  EDEF(MAX_3D_TEXTURE_SIZE),
  EDEF(TEXTURE_BINDING_3D),
  EDEF(ALPHA4),
  EDEF(ALPHA8),
  EDEF(ALPHA12),
  EDEF(ALPHA16),
  EDEF(LUMINANCE4),
  EDEF(LUMINANCE8),
  EDEF(LUMINANCE12),
  EDEF(LUMINANCE16),
  EDEF(LUMINANCE4_ALPHA4),
  EDEF(LUMINANCE6_ALPHA2),
  EDEF(LUMINANCE8_ALPHA8),
  EDEF(LUMINANCE12_ALPHA4),
  EDEF(LUMINANCE12_ALPHA12),
  EDEF(LUMINANCE16_ALPHA16),
  EDEF(INTENSITY),
  EDEF(INTENSITY4),
  EDEF(INTENSITY8),
  EDEF(INTENSITY12),
  EDEF(INTENSITY16),
  EDEF(R3_G3_B2),
  EDEF(RGB4),
  EDEF(RGB5),
  EDEF(RGB8),
  EDEF(RGB10),
  EDEF(RGB12),
  EDEF(RGB16),
  EDEF(RGBA2),
  EDEF(RGBA4),
  EDEF(RGB5_A1),
  EDEF(RGBA8),
  EDEF(RGB10_A2),
  EDEF(RGBA12),
  EDEF(RGBA16),
  EDEF(VENDOR),
  EDEF(RENDERER),
  EDEF(VERSION),
  EDEF(EXTENSIONS),
  EDEF(INVALID_VALUE),
  EDEF(INVALID_ENUM),
  EDEF(INVALID_OPERATION),
  EDEF(STACK_OVERFLOW),
  EDEF(STACK_UNDERFLOW),
  EDEF(OUT_OF_MEMORY),

  /* extensions */
  EDEF(CONSTANT_COLOR_EXT),
  EDEF(ONE_MINUS_CONSTANT_COLOR_EXT),
  EDEF(CONSTANT_ALPHA_EXT),
  EDEF(ONE_MINUS_CONSTANT_ALPHA_EXT),
  EDEF(BLEND_EQUATION_EXT),
  EDEF(MIN_EXT),
  EDEF(MAX_EXT),
  EDEF(FUNC_ADD_EXT),
  EDEF(FUNC_SUBTRACT_EXT),
  EDEF(FUNC_REVERSE_SUBTRACT_EXT),
  EDEF(BLEND_COLOR_EXT),
  EDEF(POLYGON_OFFSET_EXT),
  EDEF(POLYGON_OFFSET_FACTOR_EXT),
  EDEF(POLYGON_OFFSET_BIAS_EXT),
  EDEF(VERTEX_ARRAY_EXT),
  EDEF(NORMAL_ARRAY_EXT),
  EDEF(COLOR_ARRAY_EXT),
  EDEF(INDEX_ARRAY_EXT),
  EDEF(TEXTURE_COORD_ARRAY_EXT),
  EDEF(EDGE_FLAG_ARRAY_EXT),
  EDEF(VERTEX_ARRAY_SIZE_EXT),
  EDEF(VERTEX_ARRAY_TYPE_EXT),
  EDEF(VERTEX_ARRAY_STRIDE_EXT),
  EDEF(VERTEX_ARRAY_COUNT_EXT),
  EDEF(NORMAL_ARRAY_TYPE_EXT),
  EDEF(NORMAL_ARRAY_STRIDE_EXT),
  EDEF(NORMAL_ARRAY_COUNT_EXT),
  EDEF(COLOR_ARRAY_SIZE_EXT),
  EDEF(COLOR_ARRAY_TYPE_EXT),
  EDEF(COLOR_ARRAY_STRIDE_EXT),
  EDEF(COLOR_ARRAY_COUNT_EXT),
  EDEF(INDEX_ARRAY_TYPE_EXT),
  EDEF(INDEX_ARRAY_STRIDE_EXT),
  EDEF(INDEX_ARRAY_COUNT_EXT),
  EDEF(TEXTURE_COORD_ARRAY_SIZE_EXT),
  EDEF(TEXTURE_COORD_ARRAY_TYPE_EXT),
  EDEF(TEXTURE_COORD_ARRAY_STRIDE_EXT),
  EDEF(TEXTURE_COORD_ARRAY_COUNT_EXT),
  EDEF(EDGE_FLAG_ARRAY_STRIDE_EXT),
  EDEF(EDGE_FLAG_ARRAY_COUNT_EXT),
  EDEF(VERTEX_ARRAY_POINTER_EXT),
  EDEF(NORMAL_ARRAY_POINTER_EXT),
  EDEF(COLOR_ARRAY_POINTER_EXT),
  EDEF(INDEX_ARRAY_POINTER_EXT),
  EDEF(TEXTURE_COORD_ARRAY_POINTER_EXT),
  EDEF(EDGE_FLAG_ARRAY_POINTER_EXT),
  EDEF(TEXTURE_PRIORITY_EXT),
  EDEF(TEXTURE_RESIDENT_EXT),
  EDEF(TEXTURE_1D_BINDING_EXT),
  EDEF(TEXTURE_2D_BINDING_EXT),
  EDEF(PACK_SKIP_IMAGES_EXT),
  EDEF(PACK_IMAGE_HEIGHT_EXT),
  EDEF(UNPACK_SKIP_IMAGES_EXT),
  EDEF(UNPACK_IMAGE_HEIGHT_EXT),
  EDEF(TEXTURE_3D_EXT),
  EDEF(PROXY_TEXTURE_3D_EXT),
  EDEF(TEXTURE_DEPTH_EXT),
  EDEF(TEXTURE_WRAP_R_EXT),
  EDEF(MAX_3D_TEXTURE_SIZE_EXT),
  EDEF(TEXTURE_3D_BINDING_EXT),
  EDEF(TABLE_TOO_LARGE_EXT),
  EDEF(COLOR_TABLE_FORMAT_EXT),
  EDEF(COLOR_TABLE_WIDTH_EXT),
  EDEF(COLOR_TABLE_RED_SIZE_EXT),
  EDEF(COLOR_TABLE_GREEN_SIZE_EXT),
  EDEF(COLOR_TABLE_BLUE_SIZE_EXT),
  EDEF(COLOR_TABLE_ALPHA_SIZE_EXT),
  EDEF(COLOR_TABLE_LUMINANCE_SIZE_EXT),
  EDEF(COLOR_TABLE_INTENSITY_SIZE_EXT),
  EDEF(TEXTURE_INDEX_SIZE_EXT),
  EDEF(COLOR_INDEX1_EXT),
  EDEF(COLOR_INDEX2_EXT),
  EDEF(COLOR_INDEX4_EXT),
  EDEF(COLOR_INDEX8_EXT),
  EDEF(COLOR_INDEX12_EXT),
  EDEF(COLOR_INDEX16_EXT),
  EDEF(SHARED_TEXTURE_PALETTE_EXT),
  EDEF(POINT_SIZE_MIN_EXT),
  EDEF(POINT_SIZE_MAX_EXT),
  EDEF(POINT_FADE_THRESHOLD_SIZE_EXT),
  EDEF(DISTANCE_ATTENUATION_EXT),
  EDEF(RESCALE_NORMAL_EXT),
  EDEF(ABGR_EXT),
  EDEF(SELECTED_TEXTURE_SGIS),
  EDEF(SELECTED_TEXTURE_COORD_SET_SGIS),
  EDEF(MAX_TEXTURES_SGIS),
  EDEF(TEXTURE0_SGIS),
  EDEF(TEXTURE1_SGIS),
  EDEF(TEXTURE2_SGIS),
  EDEF(TEXTURE3_SGIS),
  EDEF(TEXTURE_COORD_SET_SOURCE_SGIS),
  EDEF(SELECTED_TEXTURE_EXT),
  EDEF(SELECTED_TEXTURE_COORD_SET_EXT),
  EDEF(SELECTED_TEXTURE_TRANSFORM_EXT),
  EDEF(MAX_TEXTURES_EXT),
  EDEF(MAX_TEXTURE_COORD_SETS_EXT),
  EDEF(TEXTURE_ENV_COORD_SET_EXT),
  EDEF(TEXTURE0_EXT),
  EDEF(TEXTURE1_EXT),
  EDEF(TEXTURE2_EXT),
  EDEF(TEXTURE3_EXT),
  EDEF(CLAMP_TO_EDGE_SGIS),
  EDEF(RESCALE_NORMAL),
  EDEF(CLAMP_TO_EDGE),
  EDEF(MAX_ELEMENTS_VERTICES),
  EDEF(MAX_ELEMENTS_INDICES),
  EDEF(BGR),
  EDEF(BGRA),
  EDEF(UNSIGNED_BYTE_3_3_2),
  EDEF(UNSIGNED_BYTE_2_3_3_REV),
  EDEF(UNSIGNED_SHORT_5_6_5),
  EDEF(UNSIGNED_SHORT_5_6_5_REV),
  EDEF(UNSIGNED_SHORT_4_4_4_4),
  EDEF(UNSIGNED_SHORT_4_4_4_4_REV),
  EDEF(UNSIGNED_SHORT_5_5_5_1),
  EDEF(UNSIGNED_SHORT_1_5_5_5_REV),
  EDEF(UNSIGNED_INT_8_8_8_8),
  EDEF(UNSIGNED_INT_8_8_8_8_REV),
  EDEF(UNSIGNED_INT_10_10_10_2),
  EDEF(UNSIGNED_INT_2_10_10_10_REV),
  EDEF(LIGHT_MODEL_COLOR_CONTROL),
  EDEF(SINGLE_COLOR),
  EDEF(SEPARATE_SPECULAR_COLOR),
  EDEF(TEXTURE_MIN_LOD),
  EDEF(TEXTURE_MAX_LOD),
  EDEF(TEXTURE_BASE_LEVEL),
  EDEF(TEXTURE_MAX_LEVEL)
};

#undef EDEF

#define N_ENUMS (sizeof(enums) / sizeof(ENUM))

/***************************************************************************/

static void print_enum_name( FILE* OUT, GLenum e )
{
  int i, found= 0;
  for( i= 0; i < N_ENUMS; ++i )
    {
    if( enums[i].e == e )
      {
      if( found )
        fprintf( OUT, "/" );
      found= 1;
      fprintf( OUT, "%s", enums[i].name );
      }
    }
  if( ! found )
    fprintf( OUT, "*UNKNOWN* [%04x]", (int)e );
  fprintf( OUT, "\n" );
}

#define BOOL_STRING(b) (b ? "true" : "false")

#define VAR_ENUM(VAR)                            \
        {                                        \
        GLint e= 0;                              \
        glGetIntegerv(GL_##VAR,&e);              \
        fprintf( OUT, "%s: ", #VAR );            \
        print_enum_name( OUT, (GLenum) e );      \
        }

#define VAR_FLOAT4(VAR)                          \
        {                                        \
        GLfloat f[4];                            \
        f[0]= f[1]= f[2]= f[3]= 0.0;             \
        glGetFloatv(GL_##VAR,f);                 \
        fprintf( OUT, "%s: [%f %f %f %f]\n",     \
                 #VAR, f[0], f[1], f[2], f[3] ); \
        }

#define VAR_MAT_FLOAT4(VAR)                        \
        {                                          \
        GLfloat f[4];                              \
        f[0]= f[1]= f[2]= f[3]= 0.0;               \
        glGetMaterialfv(GL_FRONT,GL_##VAR,f);      \
        fprintf( OUT, "FRONT_%s: [%f %f %f %f]\n", \
                 #VAR, f[0], f[1], f[2], f[3] );   \
        glGetMaterialfv(GL_BACK,GL_##VAR,f);       \
        fprintf( OUT, " BACK_%s: [%f %f %f %f]\n", \
                 #VAR, f[0], f[1], f[2], f[3] );   \
        }

#define VAR_LIGHT_FLOAT4(LIGHT,VAR)                     \
        {                                               \
        GLfloat f[4];                                   \
        f[0]= f[1]= f[2]= f[3]= 0.0;                    \
        glGetLightfv(GL_LIGHT0+LIGHT,GL_##VAR,f);       \
        fprintf( OUT, "LIGHT%d.%s: [%f %f %f %f]\n",    \
                 LIGHT, #VAR, f[0], f[1], f[2], f[3] ); \
        }

#define VAR_LIGHT_FLOAT3(LIGHT,VAR)               \
        {                                         \
        GLfloat f[3];                             \
        f[0]= f[1]= f[2]= 0.0;                    \
        glGetLightfv(GL_LIGHT0+LIGHT,GL_##VAR,f); \
        fprintf( OUT, "LIGHT%d.%s: [%f %f %f]\n", \
                 LIGHT, #VAR, f[0], f[1], f[2] ); \
        }

#define VAR_FLOAT3(VAR)                    \
        {                                  \
        GLfloat f[3];                      \
        f[0]= f[1]= f[2]= 0.0;             \
        glGetFloatv(GL_##VAR,f) ;          \
        fprintf( OUT, "%s: [%f %f %f]\n",  \
                 #VAR, f[0], f[1], f[2] ); \
        }
#define VAR_FLOAT2(VAR)                \
        {                              \
        GLfloat f[2];                  \
        f[0]= f[1]= 0.0;               \
        glGetFloatv(GL_##VAR,f);       \
        fprintf( OUT, "%s: [%f %f]\n", \
                 #VAR, f[0], f[1] );   \
        }

#define VAR_COLOR(VAR)    VAR_FLOAT4(VAR)
#define VAR_TEXCOORD(VAR) VAR_FLOAT4(VAR)
#define VAR_NORMAL(VAR)   VAR_FLOAT3(VAR)

#define VAR_MAT_COLOR(VAR)         VAR_MAT_FLOAT4(VAR)
#define VAR_LIGHT_COLOR(LIGHT,VAR) VAR_LIGHT_FLOAT4(LIGHT,VAR)

#define VAR_FLOAT(VAR)                       \
        {                                    \
        GLfloat f= 0.0;                      \
        glGetFloatv(GL_##VAR,&f);            \
        fprintf( OUT, "%s: %f\n", #VAR, f ); \
        }

#define VAR_MAT_FLOAT(VAR)                         \
        {                                          \
        GLfloat f= 0.0;                            \
        glGetMaterialfv(GL_FRONT,GL_##VAR,&f);     \
        fprintf( OUT, "FRONT_%s: %f\n", #VAR, f ); \
        glGetMaterialfv(GL_BACK,GL_##VAR,&f);      \
        fprintf( OUT, " BACK_%s: %f\n", #VAR, f ); \
        }

#define VAR_LIGHT_FLOAT(LIGHT,VAR)                 \
        {                                          \
        GLfloat f= 0.0;                            \
        glGetLightfv(GL_LIGHT0+LIGHT,GL_##VAR,&f); \
        fprintf( OUT, "LIGHT%d.%s: %f\n",          \
                 LIGHT, #VAR, f );                 \
        }

#define VAR_INT(VAR)                         \
        {                                    \
        GLint i= 0;                          \
        glGetIntegerv(GL_##VAR,&i);          \
        fprintf( OUT, "%s: %d\n", #VAR, (int)i ); \
        }
#define VAR_INTEGER(VAR) VAR_INT(VAR)
#define VAR_INDEX(VAR)   VAR_INT(VAR)
#define VAR_HEXINT(VAR)                        \
        {                                      \
        GLint i= 0;                            \
        glGetIntegerv(GL_##VAR,&i);            \
        fprintf( OUT, "%s: 0x%04x\n", #VAR, (int)i ); \
        }
#define VAR_INT4(VAR)                            \
        {                                        \
        GLint i[4];                              \
        i[0]= i[1]= i[2]= i[3]= 0;               \
        glGetIntegerv(GL_##VAR,i);               \
        fprintf( OUT, "%s: [%d %d %d %d]\n",     \
                 #VAR, (int)i[0], (int)i[1], (int)i[2], (int)i[3] ); \
        }
#define VAR_BOOL(VAR)                                     \
        {                                                 \
        GLboolean b= 0;                                   \
        glGetBooleanv(GL_##VAR,&b);                       \
        fprintf( OUT, "%s: %s\n", #VAR, BOOL_STRING(b) ); \
        }
#define VAR_BOOL4(VAR)                                    \
        {                                                 \
        GLboolean b[4];                                   \
        b[0]= b[1]= b[2]= b[3]= 0;                        \
        glGetBooleanv(GL_##VAR,b);                        \
        fprintf( OUT, "%s: [%s %s %s %s]\n",              \
                 #VAR,                                    \
                 BOOL_STRING(b[0]),                       \
                 BOOL_STRING(b[1]),                       \
                 BOOL_STRING(b[2]),                       \
                 BOOL_STRING(b[3]) );                     \
        }
#define VAR_PTR(VAR)                         \
        {                                    \
        GLvoid* p= 0;                        \
        glGetPointerv(GL_##VAR,&p);          \
        fprintf( OUT, "%s: %p\n", #VAR, p ); \
        }
#define VAR_MATRIX(VAR)                                    \
        {                                                  \
        GLfloat m[16];                                     \
        int i;                                             \
        for( i= 0; i < 16; ++i ) m[i]= 0.0;                \
        glGetFloatv(GL_##VAR,m);                           \
        fprintf( OUT,                                      \
                 "%s:\n\t[%+.6f %+.6f %+.6f %+.6f]\n\t[%+.6f %+.6f %+.6f 
%+.6f]\n\t[%+.6f %+.6f %+.6f %+.6f]\n\t[%+.6f %+.6f %+.6f %+.6f]\n", \
                 #VAR,                                     \
                 m[0+0*4], m[0+1*4], m[0+2*4], m[0+3*4],   \
                 m[1+0*4], m[1+1*4], m[1+2*4], m[1+3*4],   \
                 m[2+0*4], m[2+1*4], m[2+2*4], m[2+3*4],   \
                 m[3+0*4], m[3+1*4], m[3+2*4], m[3+3*4] ); \
        }

/***************************************************************************/

/*
#define OUT stderr
*/
void dump_opengl_state( FILE* OUT )
{
  int i;
  GLint n_lights= 0;

  glGetIntegerv( GL_MAX_LIGHTS, &n_lights );

  VAR_COLOR(CURRENT_COLOR)
  VAR_INDEX(CURRENT_INDEX)
  VAR_TEXCOORD(CURRENT_TEXTURE_COORDS)
  VAR_NORMAL(CURRENT_NORMAL)
  VAR_FLOAT4(CURRENT_RASTER_POSITION)
  VAR_FLOAT(CURRENT_RASTER_DISTANCE)
  VAR_COLOR(CURRENT_RASTER_COLOR)
  VAR_INDEX(CURRENT_RASTER_INDEX)
  VAR_TEXCOORD(CURRENT_RASTER_TEXTURE_COORDS)
  VAR_BOOL(CURRENT_RASTER_POSITION_VALID)
  VAR_BOOL(EDGE_FLAG)

  VAR_BOOL   (VERTEX_ARRAY)
  VAR_INTEGER(VERTEX_ARRAY_SIZE)
  VAR_ENUM   (VERTEX_ARRAY_TYPE)
  VAR_INTEGER(VERTEX_ARRAY_STRIDE)
  VAR_PTR    (VERTEX_ARRAY_POINTER)

  VAR_BOOL   (NORMAL_ARRAY)
  VAR_ENUM   (NORMAL_ARRAY_TYPE)
  VAR_INTEGER(NORMAL_ARRAY_STRIDE)
  VAR_PTR    (NORMAL_ARRAY_POINTER)

  VAR_BOOL   (COLOR_ARRAY)
  VAR_INTEGER(COLOR_ARRAY_SIZE)
  VAR_ENUM   (COLOR_ARRAY_TYPE)
  VAR_INTEGER(COLOR_ARRAY_STRIDE)
  VAR_PTR    (COLOR_ARRAY_POINTER)

  VAR_BOOL   (INDEX_ARRAY)
  VAR_ENUM   (INDEX_ARRAY_TYPE)
  VAR_INTEGER(INDEX_ARRAY_STRIDE)
  VAR_PTR    (INDEX_ARRAY_POINTER)

  VAR_BOOL   (TEXTURE_COORD_ARRAY)
  VAR_INTEGER(TEXTURE_COORD_ARRAY_SIZE)
  VAR_ENUM   (TEXTURE_COORD_ARRAY_TYPE)
  VAR_INTEGER(TEXTURE_COORD_ARRAY_STRIDE)
  VAR_PTR    (TEXTURE_COORD_ARRAY_POINTER)

  VAR_BOOL   (EDGE_FLAG_ARRAY)
  VAR_INTEGER(EDGE_FLAG_ARRAY_STRIDE)
  VAR_PTR    (EDGE_FLAG_ARRAY_POINTER)

  VAR_MATRIX(MODELVIEW_MATRIX)
  VAR_MATRIX(PROJECTION_MATRIX)
  VAR_MATRIX(TEXTURE_MATRIX)
  VAR_INT4(VIEWPORT)
  VAR_FLOAT2(DEPTH_RANGE)
  VAR_INT(MODELVIEW_STACK_DEPTH)
  VAR_INT(PROJECTION_STACK_DEPTH)
  VAR_INT(TEXTURE_STACK_DEPTH)
  VAR_ENUM(MATRIX_MODE)
  VAR_BOOL(NORMALIZE)
  VAR_BOOL(RESCALE_NORMAL_EXT)
  VAR_BOOL(CLIP_PLANE0)
  VAR_BOOL(CLIP_PLANE1)
  VAR_BOOL(CLIP_PLANE2)
  VAR_BOOL(CLIP_PLANE3)
  VAR_BOOL(CLIP_PLANE4)
  VAR_BOOL(CLIP_PLANE5)
  /* + glGetClipPlane() */

  VAR_COLOR(FOG_COLOR)
  VAR_INDEX(FOG_INDEX)
  VAR_FLOAT(FOG_DENSITY)
  VAR_FLOAT(FOG_START)
  VAR_FLOAT(FOG_END)
  VAR_ENUM(FOG_MODE)
  VAR_BOOL(FOG)
  VAR_ENUM(SHADE_MODEL)

  VAR_BOOL(LIGHTING)
  VAR_BOOL(COLOR_MATERIAL)
  VAR_ENUM(COLOR_MATERIAL_PARAMETER)
  VAR_ENUM(COLOR_MATERIAL_FACE)

  VAR_MAT_COLOR(AMBIENT)
  VAR_MAT_COLOR(DIFFUSE)
  VAR_MAT_COLOR(SPECULAR)
  VAR_MAT_COLOR(EMISSION)
  VAR_MAT_FLOAT(SHININESS)

  VAR_COLOR(LIGHT_MODEL_AMBIENT)
  VAR_BOOL(LIGHT_MODEL_LOCAL_VIEWER)
  VAR_BOOL(LIGHT_MODEL_TWO_SIDE)
/*  VAR_ENUM(LIGHT_MODEL_COLOR_CONTROL)*/

  for( i= 0; i < n_lights; ++i )
    {
    GLboolean b= 0;

    glGetBooleanv( GL_LIGHT0 + i, &b );
    fprintf( OUT, "LIGHT%d: %s\n", i, BOOL_STRING(b) );

    if( ! b )
      continue;

    VAR_LIGHT_COLOR(i,AMBIENT)
    VAR_LIGHT_COLOR(i,DIFFUSE)
    VAR_LIGHT_COLOR(i,SPECULAR)
    VAR_LIGHT_FLOAT4(i,POSITION)
    VAR_LIGHT_FLOAT(i,CONSTANT_ATTENUATION)
    VAR_LIGHT_FLOAT(i,LINEAR_ATTENUATION)
    VAR_LIGHT_FLOAT(i,QUADRATIC_ATTENUATION)
    VAR_LIGHT_FLOAT3(i,SPOT_DIRECTION)
    VAR_LIGHT_FLOAT(i,SPOT_EXPONENT)
    VAR_LIGHT_FLOAT(i,SPOT_CUTOFF)
    /* COLOR_INDEXES */
    }

  VAR_FLOAT(POINT_SIZE)
  VAR_BOOL(POINT_SMOOTH)
  VAR_FLOAT(LINE_WIDTH)
  VAR_BOOL(LINE_SMOOTH)
  VAR_HEXINT(LINE_STIPPLE_PATTERN)
  VAR_INT(LINE_STIPPLE_REPEAT)
  VAR_BOOL(LINE_STIPPLE)
  VAR_BOOL(CULL_FACE)
  VAR_ENUM(CULL_FACE_MODE)
  VAR_ENUM(FRONT_FACE)
  VAR_BOOL(POLYGON_SMOOTH)
  VAR_ENUM(POLYGON_MODE)
  VAR_FLOAT(POLYGON_OFFSET_FACTOR)
  VAR_FLOAT(POLYGON_OFFSET_UNITS)
  VAR_BOOL(POLYGON_OFFSET_POINT)
  VAR_BOOL(POLYGON_OFFSET_LINE)
  VAR_BOOL(POLYGON_OFFSET_FILL)
  /* GetPolygonStipple */
  VAR_BOOL(POLYGON_STIPPLE)

  VAR_BOOL(TEXTURE_1D)
  VAR_BOOL(TEXTURE_2D)
/*  VAR_BOOL(TEXTURE_3D)*/

  VAR_INT(TEXTURE_BINDING_1D)
  VAR_INT(TEXTURE_BINDING_2D)
/*  VAR_INT(TEXTURE_BINDING_3D)*/

  /* GetTexImage() */
  /* GetTexLevelParameter() */
  /* GetTexEnv() */

  VAR_BOOL(TEXTURE_GEN_S)
  VAR_BOOL(TEXTURE_GEN_T)
  VAR_BOOL(TEXTURE_GEN_R)
  VAR_BOOL(TEXTURE_GEN_Q)

  /* GetTexGen() */

  VAR_BOOL(SCISSOR_TEST)
  VAR_INT4(SCISSOR_BOX)
  VAR_BOOL(ALPHA_TEST)
  VAR_ENUM(ALPHA_TEST_FUNC)
  VAR_FLOAT(ALPHA_TEST_REF)
  VAR_BOOL(STENCIL_TEST)
  VAR_ENUM(STENCIL_FUNC)
  VAR_HEXINT(STENCIL_VALUE_MASK)
  VAR_INT(STENCIL_REF)
  VAR_ENUM(STENCIL_FAIL)
  VAR_ENUM(STENCIL_PASS_DEPTH_FAIL)
  VAR_ENUM(STENCIL_PASS_DEPTH_PASS)
  VAR_BOOL(DEPTH_TEST)
  VAR_ENUM(DEPTH_FUNC)
  VAR_BOOL(BLEND)
  VAR_ENUM(BLEND_SRC)
  VAR_ENUM(BLEND_DST)

  VAR_BOOL(DITHER)
  VAR_BOOL(LOGIC_OP) /* INDEX_LOGIC_OP */
  VAR_BOOL(COLOR_LOGIC_OP)

  VAR_ENUM(DRAW_BUFFER)
  VAR_INT(INDEX_WRITEMASK)
  VAR_BOOL4(COLOR_WRITEMASK)
  VAR_BOOL(DEPTH_WRITEMASK)
  VAR_HEXINT(STENCIL_WRITEMASK)
  VAR_COLOR(COLOR_CLEAR_VALUE)
  VAR_INDEX(INDEX_CLEAR_VALUE)
  VAR_FLOAT(DEPTH_CLEAR_VALUE)
  VAR_INT(STENCIL_CLEAR_VALUE)
  VAR_FLOAT(ACCUM_CLEAR_VALUE)

  VAR_BOOL(UNPACK_SWAP_BYTES)
  VAR_BOOL(UNPACK_LSB_FIRST)
#ifdef UNPACK_IMAGE_HEIGHT
  VAR_INT(UNPACK_IMAGE_HEIGHT)
#endif
#ifdef UNPACK_SKIP_IMAGES
  VAR_INT(UNPACK_SKIP_IMAGES)
#endif
  VAR_INT(UNPACK_ROW_LENGTH)
  VAR_INT(UNPACK_SKIP_ROWS)
  VAR_INT(UNPACK_SKIP_PIXELS)
  VAR_INT(UNPACK_ALIGNMENT)

  VAR_BOOL(PACK_SWAP_BYTES)
  VAR_BOOL(PACK_LSB_FIRST)
#ifdef PACK_IMAGE_HEIGHT
  VAR_INT(PACK_IMAGE_HEIGHT)
#endif
#ifdef PACK_SKIP_IMAGES
  VAR_INT(PACK_SKIP_IMAGES)
#endif
  VAR_INT(PACK_ROW_LENGTH)
  VAR_INT(PACK_SKIP_ROWS)
  VAR_INT(PACK_SKIP_PIXELS)
  VAR_INT(PACK_ALIGNMENT)

  VAR_BOOL(MAP_COLOR)
  VAR_BOOL(MAP_STENCIL)
  VAR_INT(INDEX_SHIFT)
  VAR_INT(INDEX_OFFSET)
  VAR_FLOAT(RED_SCALE)
  VAR_FLOAT(GREEN_SCALE)
  VAR_FLOAT(BLUE_SCALE)
  VAR_FLOAT(ALPHA_SCALE)
  VAR_FLOAT(DEPTH_SCALE)
  VAR_FLOAT(RED_BIAS)
  VAR_FLOAT(GREEN_BIAS)
  VAR_FLOAT(BLUE_BIAS)
  VAR_FLOAT(ALPHA_BIAS)
  VAR_FLOAT(DEPTH_BIAS)

  VAR_FLOAT(ZOOM_X)
  VAR_FLOAT(ZOOM_Y)

  VAR_ENUM(READ_BUFFER)

  VAR_BOOL(AUTO_NORMAL)

  VAR_ENUM(PERSPECTIVE_CORRECTION_HINT)
  VAR_ENUM(POINT_SMOOTH_HINT)
  VAR_ENUM(LINE_SMOOTH_HINT)
  VAR_ENUM(POLYGON_SMOOTH_HINT)
  VAR_ENUM(FOG_HINT)

  VAR_INT(MAX_LIGHTS)
  VAR_INT(MAX_CLIP_PLANES)
  VAR_INT(MAX_MODELVIEW_STACK_DEPTH)
  VAR_INT(MAX_PROJECTION_STACK_DEPTH)
  VAR_INT(MAX_TEXTURE_STACK_DEPTH)
  VAR_INT(SUBPIXEL_BITS)
#ifdef GL_MAX_3D_TEXTURE_SIZE
  VAR_INT(MAX_3D_TEXTURE_SIZE)
#endif
  VAR_INT(MAX_TEXTURE_SIZE)
  VAR_INT(MAX_PIXEL_MAP_TABLE)
  VAR_INT(MAX_NAME_STACK_DEPTH)
  VAR_INT(MAX_LIST_NESTING)
  VAR_INT(MAX_EVAL_ORDER)
  VAR_INT(MAX_VIEWPORT_DIMS)
  VAR_INT(MAX_ATTRIB_STACK_DEPTH)
  VAR_INT(MAX_CLIENT_ATTRIB_STACK_DEPTH)
  VAR_INT(AUX_BUFFERS)
  VAR_BOOL(RGBA_MODE)
  VAR_BOOL(INDEX_MODE)
  VAR_BOOL(DOUBLEBUFFER)
  VAR_BOOL(STEREO)
#ifdef GL_ALIASED_POINT_SIZE_RANGE
  VAR_FLOAT2(ALIASED_POINT_SIZE_RANGE)
#endif
#ifdef GL_POINT_SIZE_RANGE
  VAR_FLOAT2(POINT_SIZE_RANGE) /* SMOOTH_POINT_SIZE_RANGE */
#endif
  VAR_FLOAT(POINT_SIZE_GRANULARITY) /* SMOOTH_POINT_SIZE_GRANULARITY */
#ifdef GL_ALIASED_LINE_WIDTH_RANGE
  VAR_FLOAT2(ALIASED_LINE_WIDTH_RANGE)
#endif
  VAR_FLOAT2(LINE_WIDTH_RANGE) /* SMOOTH_LINE_WIDTH_RANGE */
  VAR_FLOAT(LINE_WIDTH_GRANULARITY) /* SMOOTH_LINE_WIDTH_GRANULARITY */

#ifdef GL_MAX_ELEMENTS_INDICES
  VAR_INT(MAX_ELEMENTS_INDICES)
#endif
#ifdef GL_MAX_ELEMENTS_VERTICES
  VAR_INT(MAX_ELEMENTS_VERTICES)
#endif
  VAR_INT(RED_BITS)
  VAR_INT(GREEN_BITS)
  VAR_INT(BLUE_BITS)
  VAR_INT(ALPHA_BITS)
  VAR_INT(INDEX_BITS)
  VAR_INT(DEPTH_BITS)
  VAR_INT(STENCIL_BITS)
  VAR_INT(ACCUM_RED_BITS)
  VAR_INT(ACCUM_GREEN_BITS)
  VAR_INT(ACCUM_BLUE_BITS)
  VAR_INT(ACCUM_ALPHA_BITS)

  VAR_INT(LIST_BASE)
  VAR_INT(LIST_INDEX)
  VAR_ENUM(LIST_MODE)
  VAR_INT(ATTRIB_STACK_DEPTH)
  VAR_INT(CLIENT_ATTRIB_STACK_DEPTH)
  VAR_INT(NAME_STACK_DEPTH)
  VAR_ENUM(RENDER_MODE)
  VAR_PTR(SELECTION_BUFFER_POINTER)
  VAR_INT(SELECTION_BUFFER_SIZE)
  VAR_PTR(FEEDBACK_BUFFER_POINTER)
  VAR_INT(FEEDBACK_BUFFER_SIZE)
  VAR_ENUM(FEEDBACK_BUFFER_TYPE)

  /* glGetError() */
}

/***************************************************************************/

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
   dump_opengl_state(stdout);
   return 0;
}

#endif

