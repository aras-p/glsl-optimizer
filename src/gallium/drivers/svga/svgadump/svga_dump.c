/**********************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/**
 * @file
 * Dump SVGA commands.
 *
 * Generated automatically from svga3d_reg.h by svga_dump.py.
 */

#include "svga_types.h"
#include "svga_shader_dump.h"
#include "svga3d_reg.h"

#include "util/u_debug.h"
#include "svga_dump.h"

static void
dump_SVGA3dVertexDecl(const SVGA3dVertexDecl *cmd)
{
   switch((*cmd).identity.type) {
   case SVGA3D_DECLTYPE_FLOAT1:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_FLOAT1\n");
      break;
   case SVGA3D_DECLTYPE_FLOAT2:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_FLOAT2\n");
      break;
   case SVGA3D_DECLTYPE_FLOAT3:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_FLOAT3\n");
      break;
   case SVGA3D_DECLTYPE_FLOAT4:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_FLOAT4\n");
      break;
   case SVGA3D_DECLTYPE_D3DCOLOR:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_D3DCOLOR\n");
      break;
   case SVGA3D_DECLTYPE_UBYTE4:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_UBYTE4\n");
      break;
   case SVGA3D_DECLTYPE_SHORT2:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_SHORT2\n");
      break;
   case SVGA3D_DECLTYPE_SHORT4:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_SHORT4\n");
      break;
   case SVGA3D_DECLTYPE_UBYTE4N:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_UBYTE4N\n");
      break;
   case SVGA3D_DECLTYPE_SHORT2N:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_SHORT2N\n");
      break;
   case SVGA3D_DECLTYPE_SHORT4N:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_SHORT4N\n");
      break;
   case SVGA3D_DECLTYPE_USHORT2N:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_USHORT2N\n");
      break;
   case SVGA3D_DECLTYPE_USHORT4N:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_USHORT4N\n");
      break;
   case SVGA3D_DECLTYPE_UDEC3:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_UDEC3\n");
      break;
   case SVGA3D_DECLTYPE_DEC3N:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_DEC3N\n");
      break;
   case SVGA3D_DECLTYPE_FLOAT16_2:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_FLOAT16_2\n");
      break;
   case SVGA3D_DECLTYPE_FLOAT16_4:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_FLOAT16_4\n");
      break;
   case SVGA3D_DECLTYPE_MAX:
      debug_printf("\t\t.identity.type = SVGA3D_DECLTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.identity.type = %i\n", (*cmd).identity.type);
      break;
   }
   switch((*cmd).identity.method) {
   case SVGA3D_DECLMETHOD_DEFAULT:
      debug_printf("\t\t.identity.method = SVGA3D_DECLMETHOD_DEFAULT\n");
      break;
   case SVGA3D_DECLMETHOD_PARTIALU:
      debug_printf("\t\t.identity.method = SVGA3D_DECLMETHOD_PARTIALU\n");
      break;
   case SVGA3D_DECLMETHOD_PARTIALV:
      debug_printf("\t\t.identity.method = SVGA3D_DECLMETHOD_PARTIALV\n");
      break;
   case SVGA3D_DECLMETHOD_CROSSUV:
      debug_printf("\t\t.identity.method = SVGA3D_DECLMETHOD_CROSSUV\n");
      break;
   case SVGA3D_DECLMETHOD_UV:
      debug_printf("\t\t.identity.method = SVGA3D_DECLMETHOD_UV\n");
      break;
   case SVGA3D_DECLMETHOD_LOOKUP:
      debug_printf("\t\t.identity.method = SVGA3D_DECLMETHOD_LOOKUP\n");
      break;
   case SVGA3D_DECLMETHOD_LOOKUPPRESAMPLED:
      debug_printf("\t\t.identity.method = SVGA3D_DECLMETHOD_LOOKUPPRESAMPLED\n");
      break;
   default:
      debug_printf("\t\t.identity.method = %i\n", (*cmd).identity.method);
      break;
   }
   switch((*cmd).identity.usage) {
   case SVGA3D_DECLUSAGE_POSITION:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_POSITION\n");
      break;
   case SVGA3D_DECLUSAGE_BLENDWEIGHT:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_BLENDWEIGHT\n");
      break;
   case SVGA3D_DECLUSAGE_BLENDINDICES:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_BLENDINDICES\n");
      break;
   case SVGA3D_DECLUSAGE_NORMAL:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_NORMAL\n");
      break;
   case SVGA3D_DECLUSAGE_PSIZE:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_PSIZE\n");
      break;
   case SVGA3D_DECLUSAGE_TEXCOORD:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_TEXCOORD\n");
      break;
   case SVGA3D_DECLUSAGE_TANGENT:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_TANGENT\n");
      break;
   case SVGA3D_DECLUSAGE_BINORMAL:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_BINORMAL\n");
      break;
   case SVGA3D_DECLUSAGE_TESSFACTOR:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_TESSFACTOR\n");
      break;
   case SVGA3D_DECLUSAGE_POSITIONT:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_POSITIONT\n");
      break;
   case SVGA3D_DECLUSAGE_COLOR:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_COLOR\n");
      break;
   case SVGA3D_DECLUSAGE_FOG:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_FOG\n");
      break;
   case SVGA3D_DECLUSAGE_DEPTH:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_DEPTH\n");
      break;
   case SVGA3D_DECLUSAGE_SAMPLE:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_SAMPLE\n");
      break;
   case SVGA3D_DECLUSAGE_MAX:
      debug_printf("\t\t.identity.usage = SVGA3D_DECLUSAGE_MAX\n");
      break;
   default:
      debug_printf("\t\t.identity.usage = %i\n", (*cmd).identity.usage);
      break;
   }
   debug_printf("\t\t.identity.usageIndex = %u\n", (*cmd).identity.usageIndex);
   debug_printf("\t\t.array.surfaceId = %u\n", (*cmd).array.surfaceId);
   debug_printf("\t\t.array.offset = %u\n", (*cmd).array.offset);
   debug_printf("\t\t.array.stride = %u\n", (*cmd).array.stride);
   debug_printf("\t\t.rangeHint.first = %u\n", (*cmd).rangeHint.first);
   debug_printf("\t\t.rangeHint.last = %u\n", (*cmd).rangeHint.last);
}

static void
dump_SVGA3dTextureState(const SVGA3dTextureState *cmd)
{
   debug_printf("\t\t.stage = %u\n", (*cmd).stage);
   switch((*cmd).name) {
   case SVGA3D_TS_INVALID:
      debug_printf("\t\t.name = SVGA3D_TS_INVALID\n");
      break;
   case SVGA3D_TS_BIND_TEXTURE:
      debug_printf("\t\t.name = SVGA3D_TS_BIND_TEXTURE\n");
      break;
   case SVGA3D_TS_COLOROP:
      debug_printf("\t\t.name = SVGA3D_TS_COLOROP\n");
      break;
   case SVGA3D_TS_COLORARG1:
      debug_printf("\t\t.name = SVGA3D_TS_COLORARG1\n");
      break;
   case SVGA3D_TS_COLORARG2:
      debug_printf("\t\t.name = SVGA3D_TS_COLORARG2\n");
      break;
   case SVGA3D_TS_ALPHAOP:
      debug_printf("\t\t.name = SVGA3D_TS_ALPHAOP\n");
      break;
   case SVGA3D_TS_ALPHAARG1:
      debug_printf("\t\t.name = SVGA3D_TS_ALPHAARG1\n");
      break;
   case SVGA3D_TS_ALPHAARG2:
      debug_printf("\t\t.name = SVGA3D_TS_ALPHAARG2\n");
      break;
   case SVGA3D_TS_ADDRESSU:
      debug_printf("\t\t.name = SVGA3D_TS_ADDRESSU\n");
      break;
   case SVGA3D_TS_ADDRESSV:
      debug_printf("\t\t.name = SVGA3D_TS_ADDRESSV\n");
      break;
   case SVGA3D_TS_MIPFILTER:
      debug_printf("\t\t.name = SVGA3D_TS_MIPFILTER\n");
      break;
   case SVGA3D_TS_MAGFILTER:
      debug_printf("\t\t.name = SVGA3D_TS_MAGFILTER\n");
      break;
   case SVGA3D_TS_MINFILTER:
      debug_printf("\t\t.name = SVGA3D_TS_MINFILTER\n");
      break;
   case SVGA3D_TS_BORDERCOLOR:
      debug_printf("\t\t.name = SVGA3D_TS_BORDERCOLOR\n");
      break;
   case SVGA3D_TS_TEXCOORDINDEX:
      debug_printf("\t\t.name = SVGA3D_TS_TEXCOORDINDEX\n");
      break;
   case SVGA3D_TS_TEXTURETRANSFORMFLAGS:
      debug_printf("\t\t.name = SVGA3D_TS_TEXTURETRANSFORMFLAGS\n");
      break;
   case SVGA3D_TS_TEXCOORDGEN:
      debug_printf("\t\t.name = SVGA3D_TS_TEXCOORDGEN\n");
      break;
   case SVGA3D_TS_BUMPENVMAT00:
      debug_printf("\t\t.name = SVGA3D_TS_BUMPENVMAT00\n");
      break;
   case SVGA3D_TS_BUMPENVMAT01:
      debug_printf("\t\t.name = SVGA3D_TS_BUMPENVMAT01\n");
      break;
   case SVGA3D_TS_BUMPENVMAT10:
      debug_printf("\t\t.name = SVGA3D_TS_BUMPENVMAT10\n");
      break;
   case SVGA3D_TS_BUMPENVMAT11:
      debug_printf("\t\t.name = SVGA3D_TS_BUMPENVMAT11\n");
      break;
   case SVGA3D_TS_TEXTURE_MIPMAP_LEVEL:
      debug_printf("\t\t.name = SVGA3D_TS_TEXTURE_MIPMAP_LEVEL\n");
      break;
   case SVGA3D_TS_TEXTURE_LOD_BIAS:
      debug_printf("\t\t.name = SVGA3D_TS_TEXTURE_LOD_BIAS\n");
      break;
   case SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL:
      debug_printf("\t\t.name = SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL\n");
      break;
   case SVGA3D_TS_ADDRESSW:
      debug_printf("\t\t.name = SVGA3D_TS_ADDRESSW\n");
      break;
   case SVGA3D_TS_GAMMA:
      debug_printf("\t\t.name = SVGA3D_TS_GAMMA\n");
      break;
   case SVGA3D_TS_BUMPENVLSCALE:
      debug_printf("\t\t.name = SVGA3D_TS_BUMPENVLSCALE\n");
      break;
   case SVGA3D_TS_BUMPENVLOFFSET:
      debug_printf("\t\t.name = SVGA3D_TS_BUMPENVLOFFSET\n");
      break;
   case SVGA3D_TS_COLORARG0:
      debug_printf("\t\t.name = SVGA3D_TS_COLORARG0\n");
      break;
   case SVGA3D_TS_ALPHAARG0:
      debug_printf("\t\t.name = SVGA3D_TS_ALPHAARG0\n");
      break;
   case SVGA3D_TS_MAX:
      debug_printf("\t\t.name = SVGA3D_TS_MAX\n");
      break;
   default:
      debug_printf("\t\t.name = %i\n", (*cmd).name);
      break;
   }
   debug_printf("\t\t.value = %u\n", (*cmd).value);
   debug_printf("\t\t.floatValue = %f\n", (*cmd).floatValue);
}

static void
dump_SVGA3dCopyBox(const SVGA3dCopyBox *cmd)
{
   debug_printf("\t\t.x = %u\n", (*cmd).x);
   debug_printf("\t\t.y = %u\n", (*cmd).y);
   debug_printf("\t\t.z = %u\n", (*cmd).z);
   debug_printf("\t\t.w = %u\n", (*cmd).w);
   debug_printf("\t\t.h = %u\n", (*cmd).h);
   debug_printf("\t\t.d = %u\n", (*cmd).d);
   debug_printf("\t\t.srcx = %u\n", (*cmd).srcx);
   debug_printf("\t\t.srcy = %u\n", (*cmd).srcy);
   debug_printf("\t\t.srcz = %u\n", (*cmd).srcz);
}

static void
dump_SVGA3dCmdSetClipPlane(const SVGA3dCmdSetClipPlane *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.index = %u\n", (*cmd).index);
   debug_printf("\t\t.plane[0] = %f\n", (*cmd).plane[0]);
   debug_printf("\t\t.plane[1] = %f\n", (*cmd).plane[1]);
   debug_printf("\t\t.plane[2] = %f\n", (*cmd).plane[2]);
   debug_printf("\t\t.plane[3] = %f\n", (*cmd).plane[3]);
}

static void
dump_SVGA3dCmdWaitForQuery(const SVGA3dCmdWaitForQuery *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).type) {
   case SVGA3D_QUERYTYPE_OCCLUSION:
      debug_printf("\t\t.type = SVGA3D_QUERYTYPE_OCCLUSION\n");
      break;
   case SVGA3D_QUERYTYPE_MAX:
      debug_printf("\t\t.type = SVGA3D_QUERYTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
   debug_printf("\t\t.guestResult.gmrId = %u\n", (*cmd).guestResult.gmrId);
   debug_printf("\t\t.guestResult.offset = %u\n", (*cmd).guestResult.offset);
}

static void
dump_SVGA3dCmdSetRenderTarget(const SVGA3dCmdSetRenderTarget *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).type) {
   case SVGA3D_RT_DEPTH:
      debug_printf("\t\t.type = SVGA3D_RT_DEPTH\n");
      break;
   case SVGA3D_RT_STENCIL:
      debug_printf("\t\t.type = SVGA3D_RT_STENCIL\n");
      break;
   default:
      debug_printf("\t\t.type = SVGA3D_RT_COLOR%u\n", (*cmd).type - SVGA3D_RT_COLOR0);
      break;
   }
   debug_printf("\t\t.target.sid = %u\n", (*cmd).target.sid);
   debug_printf("\t\t.target.face = %u\n", (*cmd).target.face);
   debug_printf("\t\t.target.mipmap = %u\n", (*cmd).target.mipmap);
}

static void
dump_SVGA3dCmdSetTextureState(const SVGA3dCmdSetTextureState *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
}

static void
dump_SVGA3dCmdSurfaceCopy(const SVGA3dCmdSurfaceCopy *cmd)
{
   debug_printf("\t\t.src.sid = %u\n", (*cmd).src.sid);
   debug_printf("\t\t.src.face = %u\n", (*cmd).src.face);
   debug_printf("\t\t.src.mipmap = %u\n", (*cmd).src.mipmap);
   debug_printf("\t\t.dest.sid = %u\n", (*cmd).dest.sid);
   debug_printf("\t\t.dest.face = %u\n", (*cmd).dest.face);
   debug_printf("\t\t.dest.mipmap = %u\n", (*cmd).dest.mipmap);
}

static void
dump_SVGA3dCmdSetMaterial(const SVGA3dCmdSetMaterial *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).face) {
   case SVGA3D_FACE_INVALID:
      debug_printf("\t\t.face = SVGA3D_FACE_INVALID\n");
      break;
   case SVGA3D_FACE_NONE:
      debug_printf("\t\t.face = SVGA3D_FACE_NONE\n");
      break;
   case SVGA3D_FACE_FRONT:
      debug_printf("\t\t.face = SVGA3D_FACE_FRONT\n");
      break;
   case SVGA3D_FACE_BACK:
      debug_printf("\t\t.face = SVGA3D_FACE_BACK\n");
      break;
   case SVGA3D_FACE_FRONT_BACK:
      debug_printf("\t\t.face = SVGA3D_FACE_FRONT_BACK\n");
      break;
   case SVGA3D_FACE_MAX:
      debug_printf("\t\t.face = SVGA3D_FACE_MAX\n");
      break;
   default:
      debug_printf("\t\t.face = %i\n", (*cmd).face);
      break;
   }
   debug_printf("\t\t.material.diffuse[0] = %f\n", (*cmd).material.diffuse[0]);
   debug_printf("\t\t.material.diffuse[1] = %f\n", (*cmd).material.diffuse[1]);
   debug_printf("\t\t.material.diffuse[2] = %f\n", (*cmd).material.diffuse[2]);
   debug_printf("\t\t.material.diffuse[3] = %f\n", (*cmd).material.diffuse[3]);
   debug_printf("\t\t.material.ambient[0] = %f\n", (*cmd).material.ambient[0]);
   debug_printf("\t\t.material.ambient[1] = %f\n", (*cmd).material.ambient[1]);
   debug_printf("\t\t.material.ambient[2] = %f\n", (*cmd).material.ambient[2]);
   debug_printf("\t\t.material.ambient[3] = %f\n", (*cmd).material.ambient[3]);
   debug_printf("\t\t.material.specular[0] = %f\n", (*cmd).material.specular[0]);
   debug_printf("\t\t.material.specular[1] = %f\n", (*cmd).material.specular[1]);
   debug_printf("\t\t.material.specular[2] = %f\n", (*cmd).material.specular[2]);
   debug_printf("\t\t.material.specular[3] = %f\n", (*cmd).material.specular[3]);
   debug_printf("\t\t.material.emissive[0] = %f\n", (*cmd).material.emissive[0]);
   debug_printf("\t\t.material.emissive[1] = %f\n", (*cmd).material.emissive[1]);
   debug_printf("\t\t.material.emissive[2] = %f\n", (*cmd).material.emissive[2]);
   debug_printf("\t\t.material.emissive[3] = %f\n", (*cmd).material.emissive[3]);
   debug_printf("\t\t.material.shininess = %f\n", (*cmd).material.shininess);
}

static void
dump_SVGA3dCmdSetLightData(const SVGA3dCmdSetLightData *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.index = %u\n", (*cmd).index);
   switch((*cmd).data.type) {
   case SVGA3D_LIGHTTYPE_INVALID:
      debug_printf("\t\t.data.type = SVGA3D_LIGHTTYPE_INVALID\n");
      break;
   case SVGA3D_LIGHTTYPE_POINT:
      debug_printf("\t\t.data.type = SVGA3D_LIGHTTYPE_POINT\n");
      break;
   case SVGA3D_LIGHTTYPE_SPOT1:
      debug_printf("\t\t.data.type = SVGA3D_LIGHTTYPE_SPOT1\n");
      break;
   case SVGA3D_LIGHTTYPE_SPOT2:
      debug_printf("\t\t.data.type = SVGA3D_LIGHTTYPE_SPOT2\n");
      break;
   case SVGA3D_LIGHTTYPE_DIRECTIONAL:
      debug_printf("\t\t.data.type = SVGA3D_LIGHTTYPE_DIRECTIONAL\n");
      break;
   case SVGA3D_LIGHTTYPE_MAX:
      debug_printf("\t\t.data.type = SVGA3D_LIGHTTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.data.type = %i\n", (*cmd).data.type);
      break;
   }
   debug_printf("\t\t.data.inWorldSpace = %u\n", (*cmd).data.inWorldSpace);
   debug_printf("\t\t.data.diffuse[0] = %f\n", (*cmd).data.diffuse[0]);
   debug_printf("\t\t.data.diffuse[1] = %f\n", (*cmd).data.diffuse[1]);
   debug_printf("\t\t.data.diffuse[2] = %f\n", (*cmd).data.diffuse[2]);
   debug_printf("\t\t.data.diffuse[3] = %f\n", (*cmd).data.diffuse[3]);
   debug_printf("\t\t.data.specular[0] = %f\n", (*cmd).data.specular[0]);
   debug_printf("\t\t.data.specular[1] = %f\n", (*cmd).data.specular[1]);
   debug_printf("\t\t.data.specular[2] = %f\n", (*cmd).data.specular[2]);
   debug_printf("\t\t.data.specular[3] = %f\n", (*cmd).data.specular[3]);
   debug_printf("\t\t.data.ambient[0] = %f\n", (*cmd).data.ambient[0]);
   debug_printf("\t\t.data.ambient[1] = %f\n", (*cmd).data.ambient[1]);
   debug_printf("\t\t.data.ambient[2] = %f\n", (*cmd).data.ambient[2]);
   debug_printf("\t\t.data.ambient[3] = %f\n", (*cmd).data.ambient[3]);
   debug_printf("\t\t.data.position[0] = %f\n", (*cmd).data.position[0]);
   debug_printf("\t\t.data.position[1] = %f\n", (*cmd).data.position[1]);
   debug_printf("\t\t.data.position[2] = %f\n", (*cmd).data.position[2]);
   debug_printf("\t\t.data.position[3] = %f\n", (*cmd).data.position[3]);
   debug_printf("\t\t.data.direction[0] = %f\n", (*cmd).data.direction[0]);
   debug_printf("\t\t.data.direction[1] = %f\n", (*cmd).data.direction[1]);
   debug_printf("\t\t.data.direction[2] = %f\n", (*cmd).data.direction[2]);
   debug_printf("\t\t.data.direction[3] = %f\n", (*cmd).data.direction[3]);
   debug_printf("\t\t.data.range = %f\n", (*cmd).data.range);
   debug_printf("\t\t.data.falloff = %f\n", (*cmd).data.falloff);
   debug_printf("\t\t.data.attenuation0 = %f\n", (*cmd).data.attenuation0);
   debug_printf("\t\t.data.attenuation1 = %f\n", (*cmd).data.attenuation1);
   debug_printf("\t\t.data.attenuation2 = %f\n", (*cmd).data.attenuation2);
   debug_printf("\t\t.data.theta = %f\n", (*cmd).data.theta);
   debug_printf("\t\t.data.phi = %f\n", (*cmd).data.phi);
}

static void
dump_SVGA3dCmdSetViewport(const SVGA3dCmdSetViewport *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.rect.x = %u\n", (*cmd).rect.x);
   debug_printf("\t\t.rect.y = %u\n", (*cmd).rect.y);
   debug_printf("\t\t.rect.w = %u\n", (*cmd).rect.w);
   debug_printf("\t\t.rect.h = %u\n", (*cmd).rect.h);
}

static void
dump_SVGA3dCmdSetScissorRect(const SVGA3dCmdSetScissorRect *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.rect.x = %u\n", (*cmd).rect.x);
   debug_printf("\t\t.rect.y = %u\n", (*cmd).rect.y);
   debug_printf("\t\t.rect.w = %u\n", (*cmd).rect.w);
   debug_printf("\t\t.rect.h = %u\n", (*cmd).rect.h);
}

static void
dump_SVGA3dCopyRect(const SVGA3dCopyRect *cmd)
{
   debug_printf("\t\t.x = %u\n", (*cmd).x);
   debug_printf("\t\t.y = %u\n", (*cmd).y);
   debug_printf("\t\t.w = %u\n", (*cmd).w);
   debug_printf("\t\t.h = %u\n", (*cmd).h);
   debug_printf("\t\t.srcx = %u\n", (*cmd).srcx);
   debug_printf("\t\t.srcy = %u\n", (*cmd).srcy);
}

static void
dump_SVGA3dCmdSetShader(const SVGA3dCmdSetShader *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).type) {
   case SVGA3D_SHADERTYPE_COMPILED_DX8:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_COMPILED_DX8\n");
      break;
   case SVGA3D_SHADERTYPE_VS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_VS\n");
      break;
   case SVGA3D_SHADERTYPE_PS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_PS\n");
      break;
   case SVGA3D_SHADERTYPE_MAX:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
   debug_printf("\t\t.shid = %u\n", (*cmd).shid);
}

static void
dump_SVGA3dCmdEndQuery(const SVGA3dCmdEndQuery *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).type) {
   case SVGA3D_QUERYTYPE_OCCLUSION:
      debug_printf("\t\t.type = SVGA3D_QUERYTYPE_OCCLUSION\n");
      break;
   case SVGA3D_QUERYTYPE_MAX:
      debug_printf("\t\t.type = SVGA3D_QUERYTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
   debug_printf("\t\t.guestResult.gmrId = %u\n", (*cmd).guestResult.gmrId);
   debug_printf("\t\t.guestResult.offset = %u\n", (*cmd).guestResult.offset);
}

static void
dump_SVGA3dSize(const SVGA3dSize *cmd)
{
   debug_printf("\t\t.width = %u\n", (*cmd).width);
   debug_printf("\t\t.height = %u\n", (*cmd).height);
   debug_printf("\t\t.depth = %u\n", (*cmd).depth);
}

static void
dump_SVGA3dCmdDestroySurface(const SVGA3dCmdDestroySurface *cmd)
{
   debug_printf("\t\t.sid = %u\n", (*cmd).sid);
}

static void
dump_SVGA3dCmdDefineContext(const SVGA3dCmdDefineContext *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
}

static void
dump_SVGA3dRect(const SVGA3dRect *cmd)
{
   debug_printf("\t\t.x = %u\n", (*cmd).x);
   debug_printf("\t\t.y = %u\n", (*cmd).y);
   debug_printf("\t\t.w = %u\n", (*cmd).w);
   debug_printf("\t\t.h = %u\n", (*cmd).h);
}

static void
dump_SVGA3dCmdBeginQuery(const SVGA3dCmdBeginQuery *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).type) {
   case SVGA3D_QUERYTYPE_OCCLUSION:
      debug_printf("\t\t.type = SVGA3D_QUERYTYPE_OCCLUSION\n");
      break;
   case SVGA3D_QUERYTYPE_MAX:
      debug_printf("\t\t.type = SVGA3D_QUERYTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
}

static void
dump_SVGA3dRenderState(const SVGA3dRenderState *cmd)
{
   switch((*cmd).state) {
   case SVGA3D_RS_INVALID:
      debug_printf("\t\t.state = SVGA3D_RS_INVALID\n");
      break;
   case SVGA3D_RS_ZENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_ZENABLE\n");
      break;
   case SVGA3D_RS_ZWRITEENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_ZWRITEENABLE\n");
      break;
   case SVGA3D_RS_ALPHATESTENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_ALPHATESTENABLE\n");
      break;
   case SVGA3D_RS_DITHERENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_DITHERENABLE\n");
      break;
   case SVGA3D_RS_BLENDENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_BLENDENABLE\n");
      break;
   case SVGA3D_RS_FOGENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_FOGENABLE\n");
      break;
   case SVGA3D_RS_SPECULARENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_SPECULARENABLE\n");
      break;
   case SVGA3D_RS_STENCILENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILENABLE\n");
      break;
   case SVGA3D_RS_LIGHTINGENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_LIGHTINGENABLE\n");
      break;
   case SVGA3D_RS_NORMALIZENORMALS:
      debug_printf("\t\t.state = SVGA3D_RS_NORMALIZENORMALS\n");
      break;
   case SVGA3D_RS_POINTSPRITEENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSPRITEENABLE\n");
      break;
   case SVGA3D_RS_POINTSCALEENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSCALEENABLE\n");
      break;
   case SVGA3D_RS_STENCILREF:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILREF\n");
      break;
   case SVGA3D_RS_STENCILMASK:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILMASK\n");
      break;
   case SVGA3D_RS_STENCILWRITEMASK:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILWRITEMASK\n");
      break;
   case SVGA3D_RS_FOGSTART:
      debug_printf("\t\t.state = SVGA3D_RS_FOGSTART\n");
      break;
   case SVGA3D_RS_FOGEND:
      debug_printf("\t\t.state = SVGA3D_RS_FOGEND\n");
      break;
   case SVGA3D_RS_FOGDENSITY:
      debug_printf("\t\t.state = SVGA3D_RS_FOGDENSITY\n");
      break;
   case SVGA3D_RS_POINTSIZE:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSIZE\n");
      break;
   case SVGA3D_RS_POINTSIZEMIN:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSIZEMIN\n");
      break;
   case SVGA3D_RS_POINTSIZEMAX:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSIZEMAX\n");
      break;
   case SVGA3D_RS_POINTSCALE_A:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSCALE_A\n");
      break;
   case SVGA3D_RS_POINTSCALE_B:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSCALE_B\n");
      break;
   case SVGA3D_RS_POINTSCALE_C:
      debug_printf("\t\t.state = SVGA3D_RS_POINTSCALE_C\n");
      break;
   case SVGA3D_RS_FOGCOLOR:
      debug_printf("\t\t.state = SVGA3D_RS_FOGCOLOR\n");
      break;
   case SVGA3D_RS_AMBIENT:
      debug_printf("\t\t.state = SVGA3D_RS_AMBIENT\n");
      break;
   case SVGA3D_RS_CLIPPLANEENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_CLIPPLANEENABLE\n");
      break;
   case SVGA3D_RS_FOGMODE:
      debug_printf("\t\t.state = SVGA3D_RS_FOGMODE\n");
      break;
   case SVGA3D_RS_FILLMODE:
      debug_printf("\t\t.state = SVGA3D_RS_FILLMODE\n");
      break;
   case SVGA3D_RS_SHADEMODE:
      debug_printf("\t\t.state = SVGA3D_RS_SHADEMODE\n");
      break;
   case SVGA3D_RS_LINEPATTERN:
      debug_printf("\t\t.state = SVGA3D_RS_LINEPATTERN\n");
      break;
   case SVGA3D_RS_SRCBLEND:
      debug_printf("\t\t.state = SVGA3D_RS_SRCBLEND\n");
      break;
   case SVGA3D_RS_DSTBLEND:
      debug_printf("\t\t.state = SVGA3D_RS_DSTBLEND\n");
      break;
   case SVGA3D_RS_BLENDEQUATION:
      debug_printf("\t\t.state = SVGA3D_RS_BLENDEQUATION\n");
      break;
   case SVGA3D_RS_CULLMODE:
      debug_printf("\t\t.state = SVGA3D_RS_CULLMODE\n");
      break;
   case SVGA3D_RS_ZFUNC:
      debug_printf("\t\t.state = SVGA3D_RS_ZFUNC\n");
      break;
   case SVGA3D_RS_ALPHAFUNC:
      debug_printf("\t\t.state = SVGA3D_RS_ALPHAFUNC\n");
      break;
   case SVGA3D_RS_STENCILFUNC:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILFUNC\n");
      break;
   case SVGA3D_RS_STENCILFAIL:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILFAIL\n");
      break;
   case SVGA3D_RS_STENCILZFAIL:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILZFAIL\n");
      break;
   case SVGA3D_RS_STENCILPASS:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILPASS\n");
      break;
   case SVGA3D_RS_ALPHAREF:
      debug_printf("\t\t.state = SVGA3D_RS_ALPHAREF\n");
      break;
   case SVGA3D_RS_FRONTWINDING:
      debug_printf("\t\t.state = SVGA3D_RS_FRONTWINDING\n");
      break;
   case SVGA3D_RS_COORDINATETYPE:
      debug_printf("\t\t.state = SVGA3D_RS_COORDINATETYPE\n");
      break;
   case SVGA3D_RS_ZBIAS:
      debug_printf("\t\t.state = SVGA3D_RS_ZBIAS\n");
      break;
   case SVGA3D_RS_RANGEFOGENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_RANGEFOGENABLE\n");
      break;
   case SVGA3D_RS_COLORWRITEENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_COLORWRITEENABLE\n");
      break;
   case SVGA3D_RS_VERTEXMATERIALENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_VERTEXMATERIALENABLE\n");
      break;
   case SVGA3D_RS_DIFFUSEMATERIALSOURCE:
      debug_printf("\t\t.state = SVGA3D_RS_DIFFUSEMATERIALSOURCE\n");
      break;
   case SVGA3D_RS_SPECULARMATERIALSOURCE:
      debug_printf("\t\t.state = SVGA3D_RS_SPECULARMATERIALSOURCE\n");
      break;
   case SVGA3D_RS_AMBIENTMATERIALSOURCE:
      debug_printf("\t\t.state = SVGA3D_RS_AMBIENTMATERIALSOURCE\n");
      break;
   case SVGA3D_RS_EMISSIVEMATERIALSOURCE:
      debug_printf("\t\t.state = SVGA3D_RS_EMISSIVEMATERIALSOURCE\n");
      break;
   case SVGA3D_RS_TEXTUREFACTOR:
      debug_printf("\t\t.state = SVGA3D_RS_TEXTUREFACTOR\n");
      break;
   case SVGA3D_RS_LOCALVIEWER:
      debug_printf("\t\t.state = SVGA3D_RS_LOCALVIEWER\n");
      break;
   case SVGA3D_RS_SCISSORTESTENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_SCISSORTESTENABLE\n");
      break;
   case SVGA3D_RS_BLENDCOLOR:
      debug_printf("\t\t.state = SVGA3D_RS_BLENDCOLOR\n");
      break;
   case SVGA3D_RS_STENCILENABLE2SIDED:
      debug_printf("\t\t.state = SVGA3D_RS_STENCILENABLE2SIDED\n");
      break;
   case SVGA3D_RS_CCWSTENCILFUNC:
      debug_printf("\t\t.state = SVGA3D_RS_CCWSTENCILFUNC\n");
      break;
   case SVGA3D_RS_CCWSTENCILFAIL:
      debug_printf("\t\t.state = SVGA3D_RS_CCWSTENCILFAIL\n");
      break;
   case SVGA3D_RS_CCWSTENCILZFAIL:
      debug_printf("\t\t.state = SVGA3D_RS_CCWSTENCILZFAIL\n");
      break;
   case SVGA3D_RS_CCWSTENCILPASS:
      debug_printf("\t\t.state = SVGA3D_RS_CCWSTENCILPASS\n");
      break;
   case SVGA3D_RS_VERTEXBLEND:
      debug_printf("\t\t.state = SVGA3D_RS_VERTEXBLEND\n");
      break;
   case SVGA3D_RS_SLOPESCALEDEPTHBIAS:
      debug_printf("\t\t.state = SVGA3D_RS_SLOPESCALEDEPTHBIAS\n");
      break;
   case SVGA3D_RS_DEPTHBIAS:
      debug_printf("\t\t.state = SVGA3D_RS_DEPTHBIAS\n");
      break;
   case SVGA3D_RS_OUTPUTGAMMA:
      debug_printf("\t\t.state = SVGA3D_RS_OUTPUTGAMMA\n");
      break;
   case SVGA3D_RS_ZVISIBLE:
      debug_printf("\t\t.state = SVGA3D_RS_ZVISIBLE\n");
      break;
   case SVGA3D_RS_LASTPIXEL:
      debug_printf("\t\t.state = SVGA3D_RS_LASTPIXEL\n");
      break;
   case SVGA3D_RS_CLIPPING:
      debug_printf("\t\t.state = SVGA3D_RS_CLIPPING\n");
      break;
   case SVGA3D_RS_WRAP0:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP0\n");
      break;
   case SVGA3D_RS_WRAP1:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP1\n");
      break;
   case SVGA3D_RS_WRAP2:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP2\n");
      break;
   case SVGA3D_RS_WRAP3:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP3\n");
      break;
   case SVGA3D_RS_WRAP4:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP4\n");
      break;
   case SVGA3D_RS_WRAP5:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP5\n");
      break;
   case SVGA3D_RS_WRAP6:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP6\n");
      break;
   case SVGA3D_RS_WRAP7:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP7\n");
      break;
   case SVGA3D_RS_WRAP8:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP8\n");
      break;
   case SVGA3D_RS_WRAP9:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP9\n");
      break;
   case SVGA3D_RS_WRAP10:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP10\n");
      break;
   case SVGA3D_RS_WRAP11:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP11\n");
      break;
   case SVGA3D_RS_WRAP12:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP12\n");
      break;
   case SVGA3D_RS_WRAP13:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP13\n");
      break;
   case SVGA3D_RS_WRAP14:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP14\n");
      break;
   case SVGA3D_RS_WRAP15:
      debug_printf("\t\t.state = SVGA3D_RS_WRAP15\n");
      break;
   case SVGA3D_RS_MULTISAMPLEANTIALIAS:
      debug_printf("\t\t.state = SVGA3D_RS_MULTISAMPLEANTIALIAS\n");
      break;
   case SVGA3D_RS_MULTISAMPLEMASK:
      debug_printf("\t\t.state = SVGA3D_RS_MULTISAMPLEMASK\n");
      break;
   case SVGA3D_RS_INDEXEDVERTEXBLENDENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_INDEXEDVERTEXBLENDENABLE\n");
      break;
   case SVGA3D_RS_TWEENFACTOR:
      debug_printf("\t\t.state = SVGA3D_RS_TWEENFACTOR\n");
      break;
   case SVGA3D_RS_ANTIALIASEDLINEENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_ANTIALIASEDLINEENABLE\n");
      break;
   case SVGA3D_RS_COLORWRITEENABLE1:
      debug_printf("\t\t.state = SVGA3D_RS_COLORWRITEENABLE1\n");
      break;
   case SVGA3D_RS_COLORWRITEENABLE2:
      debug_printf("\t\t.state = SVGA3D_RS_COLORWRITEENABLE2\n");
      break;
   case SVGA3D_RS_COLORWRITEENABLE3:
      debug_printf("\t\t.state = SVGA3D_RS_COLORWRITEENABLE3\n");
      break;
   case SVGA3D_RS_SEPARATEALPHABLENDENABLE:
      debug_printf("\t\t.state = SVGA3D_RS_SEPARATEALPHABLENDENABLE\n");
      break;
   case SVGA3D_RS_SRCBLENDALPHA:
      debug_printf("\t\t.state = SVGA3D_RS_SRCBLENDALPHA\n");
      break;
   case SVGA3D_RS_DSTBLENDALPHA:
      debug_printf("\t\t.state = SVGA3D_RS_DSTBLENDALPHA\n");
      break;
   case SVGA3D_RS_BLENDEQUATIONALPHA:
      debug_printf("\t\t.state = SVGA3D_RS_BLENDEQUATIONALPHA\n");
      break;
   case SVGA3D_RS_MAX:
      debug_printf("\t\t.state = SVGA3D_RS_MAX\n");
      break;
   default:
      debug_printf("\t\t.state = %i\n", (*cmd).state);
      break;
   }
   debug_printf("\t\t.uintValue = %u\n", (*cmd).uintValue);
   debug_printf("\t\t.floatValue = %f\n", (*cmd).floatValue);
}

static void
dump_SVGA3dVertexDivisor(const SVGA3dVertexDivisor *cmd)
{
   debug_printf("\t\t.value = %u\n", (*cmd).value);
   debug_printf("\t\t.count = %u\n", (*cmd).count);
   debug_printf("\t\t.indexedData = %u\n", (*cmd).indexedData);
   debug_printf("\t\t.instanceData = %u\n", (*cmd).instanceData);
}

static void
dump_SVGA3dCmdDefineShader(const SVGA3dCmdDefineShader *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.shid = %u\n", (*cmd).shid);
   switch((*cmd).type) {
   case SVGA3D_SHADERTYPE_COMPILED_DX8:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_COMPILED_DX8\n");
      break;
   case SVGA3D_SHADERTYPE_VS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_VS\n");
      break;
   case SVGA3D_SHADERTYPE_PS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_PS\n");
      break;
   case SVGA3D_SHADERTYPE_MAX:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
}

static void
dump_SVGA3dCmdSetShaderConst(const SVGA3dCmdSetShaderConst *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.reg = %u\n", (*cmd).reg);
   switch((*cmd).type) {
   case SVGA3D_SHADERTYPE_COMPILED_DX8:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_COMPILED_DX8\n");
      break;
   case SVGA3D_SHADERTYPE_VS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_VS\n");
      break;
   case SVGA3D_SHADERTYPE_PS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_PS\n");
      break;
   case SVGA3D_SHADERTYPE_MAX:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
   switch((*cmd).ctype) {
   case SVGA3D_CONST_TYPE_FLOAT:
      debug_printf("\t\t.ctype = SVGA3D_CONST_TYPE_FLOAT\n");
      debug_printf("\t\t.values[0] = %f\n", *(const float *)&(*cmd).values[0]);
      debug_printf("\t\t.values[1] = %f\n", *(const float *)&(*cmd).values[1]);
      debug_printf("\t\t.values[2] = %f\n", *(const float *)&(*cmd).values[2]);
      debug_printf("\t\t.values[3] = %f\n", *(const float *)&(*cmd).values[3]);
      break;
   case SVGA3D_CONST_TYPE_INT:
      debug_printf("\t\t.ctype = SVGA3D_CONST_TYPE_INT\n");
      debug_printf("\t\t.values[0] = %u\n", (*cmd).values[0]);
      debug_printf("\t\t.values[1] = %u\n", (*cmd).values[1]);
      debug_printf("\t\t.values[2] = %u\n", (*cmd).values[2]);
      debug_printf("\t\t.values[3] = %u\n", (*cmd).values[3]);
      break;
   case SVGA3D_CONST_TYPE_BOOL:
      debug_printf("\t\t.ctype = SVGA3D_CONST_TYPE_BOOL\n");
      debug_printf("\t\t.values[0] = %u\n", (*cmd).values[0]);
      debug_printf("\t\t.values[1] = %u\n", (*cmd).values[1]);
      debug_printf("\t\t.values[2] = %u\n", (*cmd).values[2]);
      debug_printf("\t\t.values[3] = %u\n", (*cmd).values[3]);
      break;
   default:
      debug_printf("\t\t.ctype = %i\n", (*cmd).ctype);
      debug_printf("\t\t.values[0] = %u\n", (*cmd).values[0]);
      debug_printf("\t\t.values[1] = %u\n", (*cmd).values[1]);
      debug_printf("\t\t.values[2] = %u\n", (*cmd).values[2]);
      debug_printf("\t\t.values[3] = %u\n", (*cmd).values[3]);
      break;
   }
}

static void
dump_SVGA3dCmdSetZRange(const SVGA3dCmdSetZRange *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.zRange.min = %f\n", (*cmd).zRange.min);
   debug_printf("\t\t.zRange.max = %f\n", (*cmd).zRange.max);
}

static void
dump_SVGA3dCmdDrawPrimitives(const SVGA3dCmdDrawPrimitives *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.numVertexDecls = %u\n", (*cmd).numVertexDecls);
   debug_printf("\t\t.numRanges = %u\n", (*cmd).numRanges);
}

static void
dump_SVGA3dCmdSetLightEnabled(const SVGA3dCmdSetLightEnabled *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.index = %u\n", (*cmd).index);
   debug_printf("\t\t.enabled = %u\n", (*cmd).enabled);
}

static void
dump_SVGA3dPrimitiveRange(const SVGA3dPrimitiveRange *cmd)
{
   switch((*cmd).primType) {
   case SVGA3D_PRIMITIVE_INVALID:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_INVALID\n");
      break;
   case SVGA3D_PRIMITIVE_TRIANGLELIST:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_TRIANGLELIST\n");
      break;
   case SVGA3D_PRIMITIVE_POINTLIST:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_POINTLIST\n");
      break;
   case SVGA3D_PRIMITIVE_LINELIST:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_LINELIST\n");
      break;
   case SVGA3D_PRIMITIVE_LINESTRIP:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_LINESTRIP\n");
      break;
   case SVGA3D_PRIMITIVE_TRIANGLESTRIP:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_TRIANGLESTRIP\n");
      break;
   case SVGA3D_PRIMITIVE_TRIANGLEFAN:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_TRIANGLEFAN\n");
      break;
   case SVGA3D_PRIMITIVE_MAX:
      debug_printf("\t\t.primType = SVGA3D_PRIMITIVE_MAX\n");
      break;
   default:
      debug_printf("\t\t.primType = %i\n", (*cmd).primType);
      break;
   }
   debug_printf("\t\t.primitiveCount = %u\n", (*cmd).primitiveCount);
   debug_printf("\t\t.indexArray.surfaceId = %u\n", (*cmd).indexArray.surfaceId);
   debug_printf("\t\t.indexArray.offset = %u\n", (*cmd).indexArray.offset);
   debug_printf("\t\t.indexArray.stride = %u\n", (*cmd).indexArray.stride);
   debug_printf("\t\t.indexWidth = %u\n", (*cmd).indexWidth);
   debug_printf("\t\t.indexBias = %i\n", (*cmd).indexBias);
}

static void
dump_SVGA3dCmdPresent(const SVGA3dCmdPresent *cmd)
{
   debug_printf("\t\t.sid = %u\n", (*cmd).sid);
}

static void
dump_SVGA3dCmdSetRenderState(const SVGA3dCmdSetRenderState *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
}

static void
dump_SVGA3dCmdSurfaceStretchBlt(const SVGA3dCmdSurfaceStretchBlt *cmd)
{
   debug_printf("\t\t.src.sid = %u\n", (*cmd).src.sid);
   debug_printf("\t\t.src.face = %u\n", (*cmd).src.face);
   debug_printf("\t\t.src.mipmap = %u\n", (*cmd).src.mipmap);
   debug_printf("\t\t.dest.sid = %u\n", (*cmd).dest.sid);
   debug_printf("\t\t.dest.face = %u\n", (*cmd).dest.face);
   debug_printf("\t\t.dest.mipmap = %u\n", (*cmd).dest.mipmap);
   debug_printf("\t\t.boxSrc.x = %u\n", (*cmd).boxSrc.x);
   debug_printf("\t\t.boxSrc.y = %u\n", (*cmd).boxSrc.y);
   debug_printf("\t\t.boxSrc.z = %u\n", (*cmd).boxSrc.z);
   debug_printf("\t\t.boxSrc.w = %u\n", (*cmd).boxSrc.w);
   debug_printf("\t\t.boxSrc.h = %u\n", (*cmd).boxSrc.h);
   debug_printf("\t\t.boxSrc.d = %u\n", (*cmd).boxSrc.d);
   debug_printf("\t\t.boxDest.x = %u\n", (*cmd).boxDest.x);
   debug_printf("\t\t.boxDest.y = %u\n", (*cmd).boxDest.y);
   debug_printf("\t\t.boxDest.z = %u\n", (*cmd).boxDest.z);
   debug_printf("\t\t.boxDest.w = %u\n", (*cmd).boxDest.w);
   debug_printf("\t\t.boxDest.h = %u\n", (*cmd).boxDest.h);
   debug_printf("\t\t.boxDest.d = %u\n", (*cmd).boxDest.d);
   switch((*cmd).mode) {
   case SVGA3D_STRETCH_BLT_POINT:
      debug_printf("\t\t.mode = SVGA3D_STRETCH_BLT_POINT\n");
      break;
   case SVGA3D_STRETCH_BLT_LINEAR:
      debug_printf("\t\t.mode = SVGA3D_STRETCH_BLT_LINEAR\n");
      break;
   case SVGA3D_STRETCH_BLT_MAX:
      debug_printf("\t\t.mode = SVGA3D_STRETCH_BLT_MAX\n");
      break;
   default:
      debug_printf("\t\t.mode = %i\n", (*cmd).mode);
      break;
   }
}

static void
dump_SVGA3dCmdSurfaceDMA(const SVGA3dCmdSurfaceDMA *cmd)
{
   debug_printf("\t\t.guest.ptr.gmrId = %u\n", (*cmd).guest.ptr.gmrId);
   debug_printf("\t\t.guest.ptr.offset = %u\n", (*cmd).guest.ptr.offset);
   debug_printf("\t\t.guest.pitch = %u\n", (*cmd).guest.pitch);
   debug_printf("\t\t.host.sid = %u\n", (*cmd).host.sid);
   debug_printf("\t\t.host.face = %u\n", (*cmd).host.face);
   debug_printf("\t\t.host.mipmap = %u\n", (*cmd).host.mipmap);
   switch((*cmd).transfer) {
   case SVGA3D_WRITE_HOST_VRAM:
      debug_printf("\t\t.transfer = SVGA3D_WRITE_HOST_VRAM\n");
      break;
   case SVGA3D_READ_HOST_VRAM:
      debug_printf("\t\t.transfer = SVGA3D_READ_HOST_VRAM\n");
      break;
   default:
      debug_printf("\t\t.transfer = %i\n", (*cmd).transfer);
      break;
   }
}

static void
dump_SVGA3dCmdSurfaceDMASuffix(const SVGA3dCmdSurfaceDMASuffix *cmd)
{
   debug_printf("\t\t.suffixSize = %u\n", (*cmd).suffixSize);
   debug_printf("\t\t.maximumOffset = %u\n", (*cmd).maximumOffset);
   debug_printf("\t\t.flags.discard = %u\n", (*cmd).flags.discard);
   debug_printf("\t\t.flags.unsynchronized = %u\n", (*cmd).flags.unsynchronized);
}

static void
dump_SVGA3dCmdSetTransform(const SVGA3dCmdSetTransform *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).type) {
   case SVGA3D_TRANSFORM_INVALID:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_INVALID\n");
      break;
   case SVGA3D_TRANSFORM_WORLD:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_WORLD\n");
      break;
   case SVGA3D_TRANSFORM_VIEW:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_VIEW\n");
      break;
   case SVGA3D_TRANSFORM_PROJECTION:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_PROJECTION\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE0:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE0\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE1:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE1\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE2:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE2\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE3:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE3\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE4:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE4\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE5:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE5\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE6:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE6\n");
      break;
   case SVGA3D_TRANSFORM_TEXTURE7:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_TEXTURE7\n");
      break;
   case SVGA3D_TRANSFORM_WORLD1:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_WORLD1\n");
      break;
   case SVGA3D_TRANSFORM_WORLD2:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_WORLD2\n");
      break;
   case SVGA3D_TRANSFORM_WORLD3:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_WORLD3\n");
      break;
   case SVGA3D_TRANSFORM_MAX:
      debug_printf("\t\t.type = SVGA3D_TRANSFORM_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
   debug_printf("\t\t.matrix[0] = %f\n", (*cmd).matrix[0]);
   debug_printf("\t\t.matrix[1] = %f\n", (*cmd).matrix[1]);
   debug_printf("\t\t.matrix[2] = %f\n", (*cmd).matrix[2]);
   debug_printf("\t\t.matrix[3] = %f\n", (*cmd).matrix[3]);
   debug_printf("\t\t.matrix[4] = %f\n", (*cmd).matrix[4]);
   debug_printf("\t\t.matrix[5] = %f\n", (*cmd).matrix[5]);
   debug_printf("\t\t.matrix[6] = %f\n", (*cmd).matrix[6]);
   debug_printf("\t\t.matrix[7] = %f\n", (*cmd).matrix[7]);
   debug_printf("\t\t.matrix[8] = %f\n", (*cmd).matrix[8]);
   debug_printf("\t\t.matrix[9] = %f\n", (*cmd).matrix[9]);
   debug_printf("\t\t.matrix[10] = %f\n", (*cmd).matrix[10]);
   debug_printf("\t\t.matrix[11] = %f\n", (*cmd).matrix[11]);
   debug_printf("\t\t.matrix[12] = %f\n", (*cmd).matrix[12]);
   debug_printf("\t\t.matrix[13] = %f\n", (*cmd).matrix[13]);
   debug_printf("\t\t.matrix[14] = %f\n", (*cmd).matrix[14]);
   debug_printf("\t\t.matrix[15] = %f\n", (*cmd).matrix[15]);
}

static void
dump_SVGA3dCmdDestroyShader(const SVGA3dCmdDestroyShader *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   debug_printf("\t\t.shid = %u\n", (*cmd).shid);
   switch((*cmd).type) {
   case SVGA3D_SHADERTYPE_COMPILED_DX8:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_COMPILED_DX8\n");
      break;
   case SVGA3D_SHADERTYPE_VS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_VS\n");
      break;
   case SVGA3D_SHADERTYPE_PS:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_PS\n");
      break;
   case SVGA3D_SHADERTYPE_MAX:
      debug_printf("\t\t.type = SVGA3D_SHADERTYPE_MAX\n");
      break;
   default:
      debug_printf("\t\t.type = %i\n", (*cmd).type);
      break;
   }
}

static void
dump_SVGA3dCmdDestroyContext(const SVGA3dCmdDestroyContext *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
}

static void
dump_SVGA3dCmdClear(const SVGA3dCmdClear *cmd)
{
   debug_printf("\t\t.cid = %u\n", (*cmd).cid);
   switch((*cmd).clearFlag) {
   case SVGA3D_CLEAR_COLOR:
      debug_printf("\t\t.clearFlag = SVGA3D_CLEAR_COLOR\n");
      break;
   case SVGA3D_CLEAR_DEPTH:
      debug_printf("\t\t.clearFlag = SVGA3D_CLEAR_DEPTH\n");
      break;
   case SVGA3D_CLEAR_STENCIL:
      debug_printf("\t\t.clearFlag = SVGA3D_CLEAR_STENCIL\n");
      break;
   default:
      debug_printf("\t\t.clearFlag = %i\n", (*cmd).clearFlag);
      break;
   }
   debug_printf("\t\t.color = %u\n", (*cmd).color);
   debug_printf("\t\t.depth = %f\n", (*cmd).depth);
   debug_printf("\t\t.stencil = %u\n", (*cmd).stencil);
}

static void
dump_SVGA3dCmdDefineSurface(const SVGA3dCmdDefineSurface *cmd)
{
   debug_printf("\t\t.sid = %u\n", (*cmd).sid);
   switch((*cmd).surfaceFlags) {
   case SVGA3D_SURFACE_CUBEMAP:
      debug_printf("\t\t.surfaceFlags = SVGA3D_SURFACE_CUBEMAP\n");
      break;
   case SVGA3D_SURFACE_HINT_STATIC:
      debug_printf("\t\t.surfaceFlags = SVGA3D_SURFACE_HINT_STATIC\n");
      break;
   case SVGA3D_SURFACE_HINT_DYNAMIC:
      debug_printf("\t\t.surfaceFlags = SVGA3D_SURFACE_HINT_DYNAMIC\n");
      break;
   case SVGA3D_SURFACE_HINT_INDEXBUFFER:
      debug_printf("\t\t.surfaceFlags = SVGA3D_SURFACE_HINT_INDEXBUFFER\n");
      break;
   case SVGA3D_SURFACE_HINT_VERTEXBUFFER:
      debug_printf("\t\t.surfaceFlags = SVGA3D_SURFACE_HINT_VERTEXBUFFER\n");
      break;
   default:
      debug_printf("\t\t.surfaceFlags = %i\n", (*cmd).surfaceFlags);
      break;
   }
   switch((*cmd).format) {
   case SVGA3D_FORMAT_INVALID:
      debug_printf("\t\t.format = SVGA3D_FORMAT_INVALID\n");
      break;
   case SVGA3D_X8R8G8B8:
      debug_printf("\t\t.format = SVGA3D_X8R8G8B8\n");
      break;
   case SVGA3D_A8R8G8B8:
      debug_printf("\t\t.format = SVGA3D_A8R8G8B8\n");
      break;
   case SVGA3D_R5G6B5:
      debug_printf("\t\t.format = SVGA3D_R5G6B5\n");
      break;
   case SVGA3D_X1R5G5B5:
      debug_printf("\t\t.format = SVGA3D_X1R5G5B5\n");
      break;
   case SVGA3D_A1R5G5B5:
      debug_printf("\t\t.format = SVGA3D_A1R5G5B5\n");
      break;
   case SVGA3D_A4R4G4B4:
      debug_printf("\t\t.format = SVGA3D_A4R4G4B4\n");
      break;
   case SVGA3D_Z_D32:
      debug_printf("\t\t.format = SVGA3D_Z_D32\n");
      break;
   case SVGA3D_Z_D16:
      debug_printf("\t\t.format = SVGA3D_Z_D16\n");
      break;
   case SVGA3D_Z_D24S8:
      debug_printf("\t\t.format = SVGA3D_Z_D24S8\n");
      break;
   case SVGA3D_Z_D15S1:
      debug_printf("\t\t.format = SVGA3D_Z_D15S1\n");
      break;
   case SVGA3D_LUMINANCE8:
      debug_printf("\t\t.format = SVGA3D_LUMINANCE8\n");
      break;
   case SVGA3D_LUMINANCE4_ALPHA4:
      debug_printf("\t\t.format = SVGA3D_LUMINANCE4_ALPHA4\n");
      break;
   case SVGA3D_LUMINANCE16:
      debug_printf("\t\t.format = SVGA3D_LUMINANCE16\n");
      break;
   case SVGA3D_LUMINANCE8_ALPHA8:
      debug_printf("\t\t.format = SVGA3D_LUMINANCE8_ALPHA8\n");
      break;
   case SVGA3D_DXT1:
      debug_printf("\t\t.format = SVGA3D_DXT1\n");
      break;
   case SVGA3D_DXT2:
      debug_printf("\t\t.format = SVGA3D_DXT2\n");
      break;
   case SVGA3D_DXT3:
      debug_printf("\t\t.format = SVGA3D_DXT3\n");
      break;
   case SVGA3D_DXT4:
      debug_printf("\t\t.format = SVGA3D_DXT4\n");
      break;
   case SVGA3D_DXT5:
      debug_printf("\t\t.format = SVGA3D_DXT5\n");
      break;
   case SVGA3D_BUMPU8V8:
      debug_printf("\t\t.format = SVGA3D_BUMPU8V8\n");
      break;
   case SVGA3D_BUMPL6V5U5:
      debug_printf("\t\t.format = SVGA3D_BUMPL6V5U5\n");
      break;
   case SVGA3D_BUMPX8L8V8U8:
      debug_printf("\t\t.format = SVGA3D_BUMPX8L8V8U8\n");
      break;
   case SVGA3D_BUMPL8V8U8:
      debug_printf("\t\t.format = SVGA3D_BUMPL8V8U8\n");
      break;
   case SVGA3D_ARGB_S10E5:
      debug_printf("\t\t.format = SVGA3D_ARGB_S10E5\n");
      break;
   case SVGA3D_ARGB_S23E8:
      debug_printf("\t\t.format = SVGA3D_ARGB_S23E8\n");
      break;
   case SVGA3D_A2R10G10B10:
      debug_printf("\t\t.format = SVGA3D_A2R10G10B10\n");
      break;
   case SVGA3D_V8U8:
      debug_printf("\t\t.format = SVGA3D_V8U8\n");
      break;
   case SVGA3D_Q8W8V8U8:
      debug_printf("\t\t.format = SVGA3D_Q8W8V8U8\n");
      break;
   case SVGA3D_CxV8U8:
      debug_printf("\t\t.format = SVGA3D_CxV8U8\n");
      break;
   case SVGA3D_X8L8V8U8:
      debug_printf("\t\t.format = SVGA3D_X8L8V8U8\n");
      break;
   case SVGA3D_A2W10V10U10:
      debug_printf("\t\t.format = SVGA3D_A2W10V10U10\n");
      break;
   case SVGA3D_ALPHA8:
      debug_printf("\t\t.format = SVGA3D_ALPHA8\n");
      break;
   case SVGA3D_R_S10E5:
      debug_printf("\t\t.format = SVGA3D_R_S10E5\n");
      break;
   case SVGA3D_R_S23E8:
      debug_printf("\t\t.format = SVGA3D_R_S23E8\n");
      break;
   case SVGA3D_RG_S10E5:
      debug_printf("\t\t.format = SVGA3D_RG_S10E5\n");
      break;
   case SVGA3D_RG_S23E8:
      debug_printf("\t\t.format = SVGA3D_RG_S23E8\n");
      break;
   case SVGA3D_BUFFER:
      debug_printf("\t\t.format = SVGA3D_BUFFER\n");
      break;
   case SVGA3D_Z_D24X8:
      debug_printf("\t\t.format = SVGA3D_Z_D24X8\n");
      break;
   case SVGA3D_FORMAT_MAX:
      debug_printf("\t\t.format = SVGA3D_FORMAT_MAX\n");
      break;
   default:
      debug_printf("\t\t.format = %i\n", (*cmd).format);
      break;
   }
   debug_printf("\t\t.face[0].numMipLevels = %u\n", (*cmd).face[0].numMipLevels);
   debug_printf("\t\t.face[1].numMipLevels = %u\n", (*cmd).face[1].numMipLevels);
   debug_printf("\t\t.face[2].numMipLevels = %u\n", (*cmd).face[2].numMipLevels);
   debug_printf("\t\t.face[3].numMipLevels = %u\n", (*cmd).face[3].numMipLevels);
   debug_printf("\t\t.face[4].numMipLevels = %u\n", (*cmd).face[4].numMipLevels);
   debug_printf("\t\t.face[5].numMipLevels = %u\n", (*cmd).face[5].numMipLevels);
}


void            
svga_dump_commands(const void *commands, uint32_t size)
{
   const uint8_t *next = commands;
   const uint8_t *last = next + size;
   
   assert(size % sizeof(uint32_t) == 0);
   
   while(next < last) {
      const uint32_t cmd_id = *(const uint32_t *)next;

      if(SVGA_3D_CMD_BASE <= cmd_id && cmd_id < SVGA_3D_CMD_MAX) {
         const SVGA3dCmdHeader *header = (const SVGA3dCmdHeader *)next;
         const uint8_t *body = (const uint8_t *)&header[1];

         next = (const uint8_t *)body + header->size;
         if(next > last)
            break;

         switch(cmd_id) {
         case SVGA_3D_CMD_SURFACE_DEFINE:
            debug_printf("\tSVGA_3D_CMD_SURFACE_DEFINE\n");
            {
               const SVGA3dCmdDefineSurface *cmd = (const SVGA3dCmdDefineSurface *)body;
               dump_SVGA3dCmdDefineSurface(cmd);
               body = (const uint8_t *)&cmd[1];
               while(body + sizeof(SVGA3dSize) <= next) {
                  dump_SVGA3dSize((const SVGA3dSize *)body);
                  body += sizeof(SVGA3dSize);
               }
            }
            break;
         case SVGA_3D_CMD_SURFACE_DESTROY:
            debug_printf("\tSVGA_3D_CMD_SURFACE_DESTROY\n");
            {
               const SVGA3dCmdDestroySurface *cmd = (const SVGA3dCmdDestroySurface *)body;
               dump_SVGA3dCmdDestroySurface(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SURFACE_COPY:
            debug_printf("\tSVGA_3D_CMD_SURFACE_COPY\n");
            {
               const SVGA3dCmdSurfaceCopy *cmd = (const SVGA3dCmdSurfaceCopy *)body;
               dump_SVGA3dCmdSurfaceCopy(cmd);
               body = (const uint8_t *)&cmd[1];
               while(body + sizeof(SVGA3dCopyBox) <= next) {
                  dump_SVGA3dCopyBox((const SVGA3dCopyBox *)body);
                  body += sizeof(SVGA3dCopyBox);
               }
            }
            break;
         case SVGA_3D_CMD_SURFACE_STRETCHBLT:
            debug_printf("\tSVGA_3D_CMD_SURFACE_STRETCHBLT\n");
            {
               const SVGA3dCmdSurfaceStretchBlt *cmd = (const SVGA3dCmdSurfaceStretchBlt *)body;
               dump_SVGA3dCmdSurfaceStretchBlt(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SURFACE_DMA:
            debug_printf("\tSVGA_3D_CMD_SURFACE_DMA\n");
            {
               const SVGA3dCmdSurfaceDMA *cmd = (const SVGA3dCmdSurfaceDMA *)body;
               dump_SVGA3dCmdSurfaceDMA(cmd);
               body = (const uint8_t *)&cmd[1];
               while(body + sizeof(SVGA3dCopyBox) <= next) {
                  dump_SVGA3dCopyBox((const SVGA3dCopyBox *)body);
                  body += sizeof(SVGA3dCopyBox);
               }
               while(body + sizeof(SVGA3dCmdSurfaceDMASuffix) <= next) {
                  dump_SVGA3dCmdSurfaceDMASuffix((const SVGA3dCmdSurfaceDMASuffix *)body);
                  body += sizeof(SVGA3dCmdSurfaceDMASuffix);
               }
            }
            break;
         case SVGA_3D_CMD_CONTEXT_DEFINE:
            debug_printf("\tSVGA_3D_CMD_CONTEXT_DEFINE\n");
            {
               const SVGA3dCmdDefineContext *cmd = (const SVGA3dCmdDefineContext *)body;
               dump_SVGA3dCmdDefineContext(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_CONTEXT_DESTROY:
            debug_printf("\tSVGA_3D_CMD_CONTEXT_DESTROY\n");
            {
               const SVGA3dCmdDestroyContext *cmd = (const SVGA3dCmdDestroyContext *)body;
               dump_SVGA3dCmdDestroyContext(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETTRANSFORM:
            debug_printf("\tSVGA_3D_CMD_SETTRANSFORM\n");
            {
               const SVGA3dCmdSetTransform *cmd = (const SVGA3dCmdSetTransform *)body;
               dump_SVGA3dCmdSetTransform(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETZRANGE:
            debug_printf("\tSVGA_3D_CMD_SETZRANGE\n");
            {
               const SVGA3dCmdSetZRange *cmd = (const SVGA3dCmdSetZRange *)body;
               dump_SVGA3dCmdSetZRange(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETRENDERSTATE:
            debug_printf("\tSVGA_3D_CMD_SETRENDERSTATE\n");
            {
               const SVGA3dCmdSetRenderState *cmd = (const SVGA3dCmdSetRenderState *)body;
               dump_SVGA3dCmdSetRenderState(cmd);
               body = (const uint8_t *)&cmd[1];
               while(body + sizeof(SVGA3dRenderState) <= next) {
                  dump_SVGA3dRenderState((const SVGA3dRenderState *)body);
                  body += sizeof(SVGA3dRenderState);
               }
            }
            break;
         case SVGA_3D_CMD_SETRENDERTARGET:
            debug_printf("\tSVGA_3D_CMD_SETRENDERTARGET\n");
            {
               const SVGA3dCmdSetRenderTarget *cmd = (const SVGA3dCmdSetRenderTarget *)body;
               dump_SVGA3dCmdSetRenderTarget(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETTEXTURESTATE:
            debug_printf("\tSVGA_3D_CMD_SETTEXTURESTATE\n");
            {
               const SVGA3dCmdSetTextureState *cmd = (const SVGA3dCmdSetTextureState *)body;
               dump_SVGA3dCmdSetTextureState(cmd);
               body = (const uint8_t *)&cmd[1];
               while(body + sizeof(SVGA3dTextureState) <= next) {
                  dump_SVGA3dTextureState((const SVGA3dTextureState *)body);
                  body += sizeof(SVGA3dTextureState);
               }
            }
            break;
         case SVGA_3D_CMD_SETMATERIAL:
            debug_printf("\tSVGA_3D_CMD_SETMATERIAL\n");
            {
               const SVGA3dCmdSetMaterial *cmd = (const SVGA3dCmdSetMaterial *)body;
               dump_SVGA3dCmdSetMaterial(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETLIGHTDATA:
            debug_printf("\tSVGA_3D_CMD_SETLIGHTDATA\n");
            {
               const SVGA3dCmdSetLightData *cmd = (const SVGA3dCmdSetLightData *)body;
               dump_SVGA3dCmdSetLightData(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETLIGHTENABLED:
            debug_printf("\tSVGA_3D_CMD_SETLIGHTENABLED\n");
            {
               const SVGA3dCmdSetLightEnabled *cmd = (const SVGA3dCmdSetLightEnabled *)body;
               dump_SVGA3dCmdSetLightEnabled(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETVIEWPORT:
            debug_printf("\tSVGA_3D_CMD_SETVIEWPORT\n");
            {
               const SVGA3dCmdSetViewport *cmd = (const SVGA3dCmdSetViewport *)body;
               dump_SVGA3dCmdSetViewport(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SETCLIPPLANE:
            debug_printf("\tSVGA_3D_CMD_SETCLIPPLANE\n");
            {
               const SVGA3dCmdSetClipPlane *cmd = (const SVGA3dCmdSetClipPlane *)body;
               dump_SVGA3dCmdSetClipPlane(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_CLEAR:
            debug_printf("\tSVGA_3D_CMD_CLEAR\n");
            {
               const SVGA3dCmdClear *cmd = (const SVGA3dCmdClear *)body;
               dump_SVGA3dCmdClear(cmd);
               body = (const uint8_t *)&cmd[1];
               while(body + sizeof(SVGA3dRect) <= next) {
                  dump_SVGA3dRect((const SVGA3dRect *)body);
                  body += sizeof(SVGA3dRect);
               }
            }
            break;
         case SVGA_3D_CMD_PRESENT:
            debug_printf("\tSVGA_3D_CMD_PRESENT\n");
            {
               const SVGA3dCmdPresent *cmd = (const SVGA3dCmdPresent *)body;
               dump_SVGA3dCmdPresent(cmd);
               body = (const uint8_t *)&cmd[1];
               while(body + sizeof(SVGA3dCopyRect) <= next) {
                  dump_SVGA3dCopyRect((const SVGA3dCopyRect *)body);
                  body += sizeof(SVGA3dCopyRect);
               }
            }
            break;
         case SVGA_3D_CMD_SHADER_DEFINE:
            debug_printf("\tSVGA_3D_CMD_SHADER_DEFINE\n");
            {
               const SVGA3dCmdDefineShader *cmd = (const SVGA3dCmdDefineShader *)body;
               dump_SVGA3dCmdDefineShader(cmd);
               body = (const uint8_t *)&cmd[1];
               svga_shader_dump((const uint32_t *)body, 
                            (unsigned)(next - body)/sizeof(uint32_t),
                            FALSE );
               body = next;
            }
            break;
         case SVGA_3D_CMD_SHADER_DESTROY:
            debug_printf("\tSVGA_3D_CMD_SHADER_DESTROY\n");
            {
               const SVGA3dCmdDestroyShader *cmd = (const SVGA3dCmdDestroyShader *)body;
               dump_SVGA3dCmdDestroyShader(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SET_SHADER:
            debug_printf("\tSVGA_3D_CMD_SET_SHADER\n");
            {
               const SVGA3dCmdSetShader *cmd = (const SVGA3dCmdSetShader *)body;
               dump_SVGA3dCmdSetShader(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_SET_SHADER_CONST:
            debug_printf("\tSVGA_3D_CMD_SET_SHADER_CONST\n");
            {
               const SVGA3dCmdSetShaderConst *cmd = (const SVGA3dCmdSetShaderConst *)body;
               dump_SVGA3dCmdSetShaderConst(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_DRAW_PRIMITIVES:
            debug_printf("\tSVGA_3D_CMD_DRAW_PRIMITIVES\n");
            {
               const SVGA3dCmdDrawPrimitives *cmd = (const SVGA3dCmdDrawPrimitives *)body;
               unsigned i, j;
               dump_SVGA3dCmdDrawPrimitives(cmd);
               body = (const uint8_t *)&cmd[1];
               for(i = 0; i < cmd->numVertexDecls; ++i) {
                  dump_SVGA3dVertexDecl((const SVGA3dVertexDecl *)body);
                  body += sizeof(SVGA3dVertexDecl);
               }
               for(j = 0; j < cmd->numRanges; ++j) {
                  dump_SVGA3dPrimitiveRange((const SVGA3dPrimitiveRange *)body);
                  body += sizeof(SVGA3dPrimitiveRange);
               }
               while(body + sizeof(SVGA3dVertexDivisor) <= next) {
                  dump_SVGA3dVertexDivisor((const SVGA3dVertexDivisor *)body);
                  body += sizeof(SVGA3dVertexDivisor);
               }
            }
            break;
         case SVGA_3D_CMD_SETSCISSORRECT:
            debug_printf("\tSVGA_3D_CMD_SETSCISSORRECT\n");
            {
               const SVGA3dCmdSetScissorRect *cmd = (const SVGA3dCmdSetScissorRect *)body;
               dump_SVGA3dCmdSetScissorRect(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_BEGIN_QUERY:
            debug_printf("\tSVGA_3D_CMD_BEGIN_QUERY\n");
            {
               const SVGA3dCmdBeginQuery *cmd = (const SVGA3dCmdBeginQuery *)body;
               dump_SVGA3dCmdBeginQuery(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_END_QUERY:
            debug_printf("\tSVGA_3D_CMD_END_QUERY\n");
            {
               const SVGA3dCmdEndQuery *cmd = (const SVGA3dCmdEndQuery *)body;
               dump_SVGA3dCmdEndQuery(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         case SVGA_3D_CMD_WAIT_FOR_QUERY:
            debug_printf("\tSVGA_3D_CMD_WAIT_FOR_QUERY\n");
            {
               const SVGA3dCmdWaitForQuery *cmd = (const SVGA3dCmdWaitForQuery *)body;
               dump_SVGA3dCmdWaitForQuery(cmd);
               body = (const uint8_t *)&cmd[1];
            }
            break;
         default:
            debug_printf("\t0x%08x\n", cmd_id);
            break;
         }

         while(body + sizeof(uint32_t) <= next) {
            debug_printf("\t\t0x%08x\n", *(const uint32_t *)body);
            body += sizeof(uint32_t);
         }
         while(body + sizeof(uint32_t) <= next)
            debug_printf("\t\t0x%02x\n", *body++);
      }
      else if(cmd_id == SVGA_CMD_FENCE) {
         debug_printf("\tSVGA_CMD_FENCE\n");
         debug_printf("\t\t0x%08x\n", ((const uint32_t *)next)[1]);
         next += 2*sizeof(uint32_t);
      }
      else {
         debug_printf("\t0x%08x\n", cmd_id);
         next += sizeof(uint32_t);
      }
   }
}

