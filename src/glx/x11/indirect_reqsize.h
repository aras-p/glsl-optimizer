/* DO NOT EDIT - This file generated automatically by glX_proto_size.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined( _INDIRECT_REQSIZE_H_ )
#  define _INDIRECT_REQSIZE_H_

#  if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && defined(__ELF__)
#    define HIDDEN  __attribute__((visibility("hidden")))
#  else
#    define HIDDEN
#  endif

#  if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#    define PURE __attribute__((pure))
#  else
#    define PURE
#  endif

extern PURE HIDDEN int __glXCallListsReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXBitmapReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXFogfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXFogivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXLightfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXLightivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXLightModelfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXLightModelivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXMaterialfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXMaterialivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXPolygonStippleReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexParameterfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexParameterivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexImage1DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexImage2DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexEnvfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexEnvivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexGendvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexGenfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexGenivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXMap1dReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXMap1fReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXMap2dReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXMap2fReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXPixelMapfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXPixelMapuivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXPixelMapusvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXDrawPixelsReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXDrawArraysReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXPrioritizeTexturesReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexSubImage1DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexSubImage2DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXColorTableReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXColorTableParameterfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXColorTableParameterivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXColorSubTableReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXConvolutionFilter1DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXConvolutionFilter2DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXConvolutionParameterfvReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXConvolutionParameterivReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXSeparableFilter2DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexImage3DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXTexSubImage3DReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXCompressedTexImage1DARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXCompressedTexImage2DARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXCompressedTexImage3DARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXCompressedTexSubImage1DARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXCompressedTexSubImage2DARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXCompressedTexSubImage3DARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXProgramStringARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXDrawBuffersARBReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXPointParameterfvEXTReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXLoadProgramNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXProgramParameters4dvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXProgramParameters4fvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXRequestResidentProgramsNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs1dvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs1fvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs1svNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs2dvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs2fvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs2svNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs3dvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs3fvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs3svNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs4dvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs4fvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs4svNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXVertexAttribs4ubvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXPointParameterivNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXProgramNamedParameter4dvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXProgramNamedParameter4fvNVReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXDeleteFramebuffersEXTReqSize(const GLbyte *pc, Bool swap);
extern PURE HIDDEN int __glXDeleteRenderbuffersEXTReqSize(const GLbyte *pc, Bool swap);

#  undef HIDDEN
#  undef PURE

#endif /* !defined( _INDIRECT_REQSIZE_H_ ) */
