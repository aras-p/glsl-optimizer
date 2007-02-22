/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x/2000/XP/XBox (Win32)
*
* Description:  GL extensions
*
****************************************************************************/

//#include "../GLDirect.h"
//#include "../gld_log.h"
//#include "../gld_settings.h"

#include <windows.h>
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>

//#include "ddlog.h"
//#include "gld_dx8.h"

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "depth.h"
#include "extensions.h"
#include "macros.h"
#include "matrix.h"
// #include "mem.h"
//#include "mmath.h"
#include "mtypes.h"
#include "texformat.h"
#include "texstore.h"
#include "vbo/vbo.h"
#include "swrast_setup/swrast_setup.h"
#include "swrast_setup/ss_context.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "dglcontext.h"
#include "extensions.h"

// For some reason this is not defined in an above header...
extern void _mesa_enable_imaging_extensions(GLcontext *ctx);

//---------------------------------------------------------------------------
// Hack for the SGIS_multitexture extension that was removed from Mesa
// NOTE: SGIS_multitexture enums also clash with GL_SGIX_async_pixel

	// NOTE: Quake2 ran *slower* with this enabled, so I've
	// disabled it for now.
	// To enable, uncomment:
	//  _mesa_add_extension(ctx, GL_TRUE, szGL_SGIS_multitexture, 0);

//---------------------------------------------------------------------------

enum {
	/* Quake2 GL_SGIS_multitexture */
	GL_SELECTED_TEXTURE_SGIS			= 0x835B,
	GL_SELECTED_TEXTURE_COORD_SET_SGIS	= 0x835C,
	GL_MAX_TEXTURES_SGIS				= 0x835D,
	GL_TEXTURE0_SGIS					= 0x835E,
	GL_TEXTURE1_SGIS					= 0x835F,
	GL_TEXTURE2_SGIS					= 0x8360,
	GL_TEXTURE3_SGIS					= 0x8361,
	GL_TEXTURE_COORD_SET_SOURCE_SGIS	= 0x8363,
};

//---------------------------------------------------------------------------

void APIENTRY gldSelectTextureSGIS(
	GLenum target)
{
	GLenum ARB_target = GL_TEXTURE0_ARB + (target - GL_TEXTURE0_SGIS);
	glActiveTextureARB(ARB_target);
}

//---------------------------------------------------------------------------

void APIENTRY gldMTexCoord2fSGIS(
	GLenum target,
	GLfloat s,
	GLfloat t)
{
	GLenum ARB_target = GL_TEXTURE0_ARB + (target - GL_TEXTURE0_SGIS);
	glMultiTexCoord2fARB(ARB_target, s, t);
}

//---------------------------------------------------------------------------

void APIENTRY gldMTexCoord2fvSGIS(
	GLenum target,
	const GLfloat *v)
{
	GLenum ARB_target = GL_TEXTURE0_ARB + (target - GL_TEXTURE0_SGIS);
	glMultiTexCoord2fvARB(ARB_target, v);
}

//---------------------------------------------------------------------------
// Extensions
//---------------------------------------------------------------------------

typedef struct {
	PROC proc;
	char *name;
}  GLD_extension;

GLD_extension GLD_extList[] = {
#ifdef GL_EXT_polygon_offset
    {	(PROC)glPolygonOffsetEXT,		"glPolygonOffsetEXT"		},
#endif
    {	(PROC)glBlendEquationEXT,		"glBlendEquationEXT"		},
    {	(PROC)glBlendColorEXT,			"glBlendColorExt"			},
    {	(PROC)glVertexPointerEXT,		"glVertexPointerEXT"		},
    {	(PROC)glNormalPointerEXT,		"glNormalPointerEXT"		},
    {	(PROC)glColorPointerEXT,		"glColorPointerEXT"			},
    {	(PROC)glIndexPointerEXT,		"glIndexPointerEXT"			},
    {	(PROC)glTexCoordPointerEXT,		"glTexCoordPointer"			},
    {	(PROC)glEdgeFlagPointerEXT,		"glEdgeFlagPointerEXT"		},
    {	(PROC)glGetPointervEXT,			"glGetPointervEXT"			},
    {	(PROC)glArrayElementEXT,		"glArrayElementEXT"			},
    {	(PROC)glDrawArraysEXT,			"glDrawArrayEXT"			},
    {	(PROC)glAreTexturesResidentEXT,	"glAreTexturesResidentEXT"	},
    {	(PROC)glBindTextureEXT,			"glBindTextureEXT"			},
    {	(PROC)glDeleteTexturesEXT,		"glDeleteTexturesEXT"		},
    {	(PROC)glGenTexturesEXT,			"glGenTexturesEXT"			},
    {	(PROC)glIsTextureEXT,			"glIsTextureEXT"			},
    {	(PROC)glPrioritizeTexturesEXT,	"glPrioritizeTexturesEXT"	},
    {	(PROC)glCopyTexSubImage3DEXT,	"glCopyTexSubImage3DEXT"	},
    {	(PROC)glTexImage3DEXT,			"glTexImage3DEXT"			},
    {	(PROC)glTexSubImage3DEXT,		"glTexSubImage3DEXT"		},
    {	(PROC)glPointParameterfEXT,		"glPointParameterfEXT"		},
    {	(PROC)glPointParameterfvEXT,	"glPointParameterfvEXT"		},

    {	(PROC)glLockArraysEXT,			"glLockArraysEXT"			},
    {	(PROC)glUnlockArraysEXT,		"glUnlockArraysEXT"			},
	{	NULL,							"\0"						}
};

GLD_extension GLD_multitexList[] = {
/*
    {	(PROC)glMultiTexCoord1dSGIS,		"glMTexCoord1dSGIS"			},
    {	(PROC)glMultiTexCoord1dvSGIS,		"glMTexCoord1dvSGIS"		},
    {	(PROC)glMultiTexCoord1fSGIS,		"glMTexCoord1fSGIS"			},
    {	(PROC)glMultiTexCoord1fvSGIS,		"glMTexCoord1fvSGIS"		},
    {	(PROC)glMultiTexCoord1iSGIS,		"glMTexCoord1iSGIS"			},
    {	(PROC)glMultiTexCoord1ivSGIS,		"glMTexCoord1ivSGIS"		},
    {	(PROC)glMultiTexCoord1sSGIS,		"glMTexCoord1sSGIS"			},
    {	(PROC)glMultiTexCoord1svSGIS,		"glMTexCoord1svSGIS"		},
    {	(PROC)glMultiTexCoord2dSGIS,		"glMTexCoord2dSGIS"			},
    {	(PROC)glMultiTexCoord2dvSGIS,		"glMTexCoord2dvSGIS"		},
    {	(PROC)glMultiTexCoord2fSGIS,		"glMTexCoord2fSGIS"			},
    {	(PROC)glMultiTexCoord2fvSGIS,		"glMTexCoord2fvSGIS"		},
    {	(PROC)glMultiTexCoord2iSGIS,		"glMTexCoord2iSGIS"			},
    {	(PROC)glMultiTexCoord2ivSGIS,		"glMTexCoord2ivSGIS"		},
    {	(PROC)glMultiTexCoord2sSGIS,		"glMTexCoord2sSGIS"			},
    {	(PROC)glMultiTexCoord2svSGIS,		"glMTexCoord2svSGIS"		},
    {	(PROC)glMultiTexCoord3dSGIS,		"glMTexCoord3dSGIS"			},
    {	(PROC)glMultiTexCoord3dvSGIS,		"glMTexCoord3dvSGIS"		},
    {	(PROC)glMultiTexCoord3fSGIS,		"glMTexCoord3fSGIS"			},
    {	(PROC)glMultiTexCoord3fvSGIS,		"glMTexCoord3fvSGIS"		},
    {	(PROC)glMultiTexCoord3iSGIS,		"glMTexCoord3iSGIS"			},
    {	(PROC)glMultiTexCoord3ivSGIS,		"glMTexCoord3ivSGIS"		},
    {	(PROC)glMultiTexCoord3sSGIS,		"glMTexCoord3sSGIS"			},
    {	(PROC)glMultiTexCoord3svSGIS,		"glMTexCoord3svSGIS"		},
    {	(PROC)glMultiTexCoord4dSGIS,		"glMTexCoord4dSGIS"			},
    {	(PROC)glMultiTexCoord4dvSGIS,		"glMTexCoord4dvSGIS"		},
    {	(PROC)glMultiTexCoord4fSGIS,		"glMTexCoord4fSGIS"			},
    {	(PROC)glMultiTexCoord4fvSGIS,		"glMTexCoord4fvSGIS"		},
    {	(PROC)glMultiTexCoord4iSGIS,		"glMTexCoord4iSGIS"			},
    {	(PROC)glMultiTexCoord4ivSGIS,		"glMTexCoord4ivSGIS"		},
    {	(PROC)glMultiTexCoord4sSGIS,		"glMTexCoord4sSGIS"			},
    {	(PROC)glMultiTexCoord4svSGIS,		"glMTexCoord4svSGIS"		},
    {	(PROC)glMultiTexCoordPointerSGIS,	"glMTexCoordPointerSGIS"	},
    {	(PROC)glSelectTextureSGIS,			"glSelectTextureSGIS"			},
    {	(PROC)glSelectTextureCoordSetSGIS,	"glSelectTextureCoordSetSGIS"	},
*/
    {	(PROC)glActiveTextureARB,		"glActiveTextureARB"		},
    {	(PROC)glClientActiveTextureARB,	"glClientActiveTextureARB"	},
    {	(PROC)glMultiTexCoord1dARB,		"glMultiTexCoord1dARB"		},
    {	(PROC)glMultiTexCoord1dvARB,	"glMultiTexCoord1dvARB"		},
    {	(PROC)glMultiTexCoord1fARB,		"glMultiTexCoord1fARB"		},
    {	(PROC)glMultiTexCoord1fvARB,	"glMultiTexCoord1fvARB"		},
    {	(PROC)glMultiTexCoord1iARB,		"glMultiTexCoord1iARB"		},
    {	(PROC)glMultiTexCoord1ivARB,	"glMultiTexCoord1ivARB"		},
    {	(PROC)glMultiTexCoord1sARB,		"glMultiTexCoord1sARB"		},
    {	(PROC)glMultiTexCoord1svARB,	"glMultiTexCoord1svARB"		},
    {	(PROC)glMultiTexCoord2dARB,		"glMultiTexCoord2dARB"		},
    {	(PROC)glMultiTexCoord2dvARB,	"glMultiTexCoord2dvARB"		},
    {	(PROC)glMultiTexCoord2fARB,		"glMultiTexCoord2fARB"		},
    {	(PROC)glMultiTexCoord2fvARB,	"glMultiTexCoord2fvARB"		},
    {	(PROC)glMultiTexCoord2iARB,		"glMultiTexCoord2iARB"		},
    {	(PROC)glMultiTexCoord2ivARB,	"glMultiTexCoord2ivARB"		},
    {	(PROC)glMultiTexCoord2sARB,		"glMultiTexCoord2sARB"		},
    {	(PROC)glMultiTexCoord2svARB,	"glMultiTexCoord2svARB"		},
    {	(PROC)glMultiTexCoord3dARB,		"glMultiTexCoord3dARB"		},
    {	(PROC)glMultiTexCoord3dvARB,	"glMultiTexCoord3dvARB"		},
    {	(PROC)glMultiTexCoord3fARB,		"glMultiTexCoord3fARB"		},
    {	(PROC)glMultiTexCoord3fvARB,	"glMultiTexCoord3fvARB"		},
    {	(PROC)glMultiTexCoord3iARB,		"glMultiTexCoord3iARB"		},
    {	(PROC)glMultiTexCoord3ivARB,	"glMultiTexCoord3ivARB"		},
    {	(PROC)glMultiTexCoord3sARB,		"glMultiTexCoord3sARB"		},
    {	(PROC)glMultiTexCoord3svARB,	"glMultiTexCoord3svARB"		},
    {	(PROC)glMultiTexCoord4dARB,		"glMultiTexCoord4dARB"		},
    {	(PROC)glMultiTexCoord4dvARB,	"glMultiTexCoord4dvARB"		},
    {	(PROC)glMultiTexCoord4fARB,		"glMultiTexCoord4fARB"		},
    {	(PROC)glMultiTexCoord4fvARB,	"glMultiTexCoord4fvARB"		},
    {	(PROC)glMultiTexCoord4iARB,		"glMultiTexCoord4iARB"		},
    {	(PROC)glMultiTexCoord4ivARB,	"glMultiTexCoord4ivARB"		},
    {	(PROC)glMultiTexCoord4sARB,		"glMultiTexCoord4sARB"		},
    {	(PROC)glMultiTexCoord4svARB,	"glMultiTexCoord4svARB"		},

	// Descent3 doesn't use correct string, hence this hack
    {	(PROC)glMultiTexCoord4fARB,		"glMultiTexCoord4f"			},

	// Quake2 SGIS multitexture
    {	(PROC)gldSelectTextureSGIS,		"glSelectTextureSGIS"		},
    {	(PROC)gldMTexCoord2fSGIS,		"glMTexCoord2fSGIS"			},
    {	(PROC)gldMTexCoord2fvSGIS,		"glMTexCoord2fvSGIS"		},

	{	NULL,							"\0"						}
};

//---------------------------------------------------------------------------

PROC gldGetProcAddress_DX(
	LPCSTR a)
{
	int		i;
	PROC	proc = NULL;

	for (i=0; GLD_extList[i].proc; i++) {
		if (!strcmp(a, GLD_extList[i].name)) {
			proc = GLD_extList[i].proc;
			break;
		}
	}

	if (glb.bMultitexture) {
		for (i=0; GLD_multitexList[i].proc; i++) {
			if (!strcmp(a, GLD_multitexList[i].name)) {
				proc = GLD_multitexList[i].proc;
				break;
			}
		}
	}

	gldLogPrintf(GLDLOG_INFO, "GetProcAddress: %s (%s)", a, proc ? "OK" : "Failed");

	return proc;
}

//---------------------------------------------------------------------------

void gldEnableExtensions_DX9(
	GLcontext *ctx)
{
	GLuint i;

	// Mesa enables some extensions by default.
	// This table decides which ones we want to switch off again.

	// NOTE: GL_EXT_compiled_vertex_array appears broken.

	const char *gld_disable_extensions[] = {
//		"GL_ARB_transpose_matrix",
//		"GL_EXT_compiled_vertex_array",
//		"GL_EXT_polygon_offset",
//		"GL_EXT_rescale_normal",
		"GL_EXT_texture3D",
//		"GL_NV_texgen_reflection",
		NULL
	};

	const char *gld_multitex_extensions[] = {
		"GL_ARB_multitexture",		// Quake 3
		NULL
	};

	// Quake 2 engines
	const char *szGL_SGIS_multitexture = "GL_SGIS_multitexture";

	const char *gld_enable_extensions[] = {
		"GL_EXT_texture_env_add",	// Quake 3
		"GL_ARB_texture_env_add",	// Quake 3
		NULL
	};
	
	for (i=0; gld_disable_extensions[i]; i++) {
		_mesa_disable_extension(ctx, gld_disable_extensions[i]);
	}
	
	for (i=0; gld_enable_extensions[i]; i++) {
		_mesa_enable_extension(ctx, gld_enable_extensions[i]);
	}

	if (glb.bMultitexture) {	
		for (i=0; gld_multitex_extensions[i]; i++) {
			_mesa_enable_extension(ctx, gld_multitex_extensions[i]);
		}

		// GL_SGIS_multitexture
		// NOTE: Quake2 ran *slower* with this enabled, so I've
		// disabled it for now.
		// Fair bit slower on GeForce256,
		// Much slower on 3dfx Voodoo5 5500.
//		_mesa_add_extension(ctx, GL_TRUE, szGL_SGIS_multitexture, 0);

	}

	_mesa_enable_imaging_extensions(ctx);
	_mesa_enable_1_3_extensions(ctx);
	_mesa_enable_1_4_extensions(ctx);
}

//---------------------------------------------------------------------------
