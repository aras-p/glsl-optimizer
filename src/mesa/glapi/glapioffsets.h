/* $Id: glapioffsets.h,v 1.3 2000/02/11 21:14:28 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * This file defines a static offset for all GL functions within the
 * dispatch table.
 *
 * Eventually a replacement for this file will be available from SGI
 * or the ARB so all vendors have the same info.
 *
 * XXX this file is incomplete - many more extension functions left to add
 */



#ifndef _glextfunc_h_
#define _glextfunc_h_

#define _BASE 0


/* GL 1.1 */
#define _gloffset_Accum					(_BASE +   1)
#define _gloffset_AlphaFunc				(_BASE +   2)
#define _gloffset_Begin					(_BASE +   3)
#define _gloffset_Bitmap				(_BASE +   4)
#define _gloffset_BlendFunc				(_BASE +   5)
#define _gloffset_CallList				(_BASE +   6)
#define _gloffset_CallLists				(_BASE +   7)
#define _gloffset_Clear					(_BASE +   8)
#define _gloffset_ClearAccum				(_BASE +   9)
#define _gloffset_ClearColor				(_BASE +  10)
#define _gloffset_ClearDepth				(_BASE +  11)
#define _gloffset_ClearIndex				(_BASE +  12)
#define _gloffset_ClearStencil				(_BASE +  13)
#define _gloffset_ClipPlane				(_BASE +  14)
#define _gloffset_Color3b				(_BASE +  15)
#define _gloffset_Color3bv				(_BASE +  16)
#define _gloffset_Color3d				(_BASE +  17)
#define _gloffset_Color3dv				(_BASE +  18)
#define _gloffset_Color3f				(_BASE +  19)
#define _gloffset_Color3fv				(_BASE +  20)
#define _gloffset_Color3i				(_BASE +  21)
#define _gloffset_Color3iv				(_BASE +  22)
#define _gloffset_Color3s				(_BASE +  23)
#define _gloffset_Color3sv				(_BASE +  24)
#define _gloffset_Color3ub				(_BASE +  25)
#define _gloffset_Color3ubv				(_BASE +  26)
#define _gloffset_Color3ui				(_BASE +  27)
#define _gloffset_Color3uiv				(_BASE +  28)
#define _gloffset_Color3us				(_BASE +  29)
#define _gloffset_Color3usv				(_BASE +  30)
#define _gloffset_Color4b				(_BASE +  31)
#define _gloffset_Color4bv				(_BASE +  32)
#define _gloffset_Color4d				(_BASE +  33)
#define _gloffset_Color4dv				(_BASE +  34)
#define _gloffset_Color4f				(_BASE +  35)
#define _gloffset_Color4fv				(_BASE +  36)
#define _gloffset_Color4i				(_BASE +  37)
#define _gloffset_Color4iv				(_BASE +  38)
#define _gloffset_Color4s				(_BASE +  39)
#define _gloffset_Color4sv				(_BASE +  40)
#define _gloffset_Color4ub				(_BASE +  41)
#define _gloffset_Color4ubv				(_BASE +  42)
#define _gloffset_Color4ui				(_BASE +  43)
#define _gloffset_Color4uiv				(_BASE +  44)
#define _gloffset_Color4us				(_BASE +  45)
#define _gloffset_Color4usv				(_BASE +  46)
#define _gloffset_ColorMask				(_BASE +  47)
#define _gloffset_ColorMaterial				(_BASE +  48)
#define _gloffset_CopyPixels				(_BASE +  49)
#define _gloffset_CullFace				(_BASE +  50)
#define _gloffset_DeleteLists				(_BASE +  51)
#define _gloffset_DepthFunc				(_BASE +  52)
#define _gloffset_DepthMask				(_BASE +  53)
#define _gloffset_DepthRange				(_BASE +  54)
#define _gloffset_Disable				(_BASE +  55)
#define _gloffset_DrawBuffer				(_BASE +  56)
#define _gloffset_DrawPixels				(_BASE +  57)
#define _gloffset_EdgeFlag				(_BASE +  58)
#define _gloffset_EdgeFlagv				(_BASE +  59)
#define _gloffset_Enable				(_BASE +  60)
#define _gloffset_End					(_BASE +  61)
#define _gloffset_EndList				(_BASE +  62)
#define _gloffset_EvalCoord1d				(_BASE +  63)
#define _gloffset_EvalCoord1dv				(_BASE +  64)
#define _gloffset_EvalCoord1f				(_BASE +  65)
#define _gloffset_EvalCoord1fv				(_BASE +  66)
#define _gloffset_EvalCoord2d				(_BASE +  67)
#define _gloffset_EvalCoord2dv				(_BASE +  68)
#define _gloffset_EvalCoord2f				(_BASE +  69)
#define _gloffset_EvalCoord2fv				(_BASE +  70)
#define _gloffset_EvalMesh1				(_BASE +  71)
#define _gloffset_EvalMesh2				(_BASE +  72)
#define _gloffset_EvalPoint1				(_BASE +  73)
#define _gloffset_EvalPoint2				(_BASE +  74)
#define _gloffset_FeedbackBuffer			(_BASE +  75)
#define _gloffset_Finish				(_BASE +  76)
#define _gloffset_Flush					(_BASE +  77)
#define _gloffset_Fogf					(_BASE +  78)
#define _gloffset_Fogfv					(_BASE +  79)
#define _gloffset_Fogi					(_BASE +  80)
#define _gloffset_Fogiv					(_BASE +  81)
#define _gloffset_FrontFace				(_BASE +  82)
#define _gloffset_Frustum				(_BASE +  83)
#define _gloffset_GenLists				(_BASE +  84)
#define _gloffset_GetBooleanv				(_BASE +  85)
#define _gloffset_GetClipPlane				(_BASE +  86)
#define _gloffset_GetDoublev				(_BASE +  87)
#define _gloffset_GetError				(_BASE +  88)
#define _gloffset_GetFloatv				(_BASE +  89)
#define _gloffset_GetIntegerv				(_BASE +  90)
#define _gloffset_GetLightfv				(_BASE +  91)
#define _gloffset_GetLightiv				(_BASE +  92)
#define _gloffset_GetMapdv				(_BASE +  93)
#define _gloffset_GetMapfv				(_BASE +  94)
#define _gloffset_GetMapiv				(_BASE +  95)
#define _gloffset_GetMaterialfv				(_BASE +  96)
#define _gloffset_GetMaterialiv				(_BASE +  97)
#define _gloffset_GetPixelMapfv				(_BASE +  98)
#define _gloffset_GetPixelMapuiv			(_BASE +  99)
#define _gloffset_GetPixelMapusv			(_BASE + 100)
#define _gloffset_GetPolygonStipple			(_BASE + 101)
#define _gloffset_GetString				(_BASE + 102)
#define _gloffset_GetTexEnvfv				(_BASE + 103)
#define _gloffset_GetTexEnviv				(_BASE + 104)
#define _gloffset_GetTexGendv				(_BASE + 105)
#define _gloffset_GetTexGenfv				(_BASE + 106)
#define _gloffset_GetTexGeniv				(_BASE + 107)
#define _gloffset_GetTexImage				(_BASE + 108)
#define _gloffset_GetTexLevelParameterfv		(_BASE + 109)
#define _gloffset_GetTexLevelParameteriv		(_BASE + 110)
#define _gloffset_GetTexParameterfv			(_BASE + 111)
#define _gloffset_GetTexParameteriv			(_BASE + 112)
#define _gloffset_Hint					(_BASE + 113)
#define _gloffset_IndexMask				(_BASE + 114)
#define _gloffset_Indexd				(_BASE + 115)
#define _gloffset_Indexdv				(_BASE + 116)
#define _gloffset_Indexf				(_BASE + 117)
#define _gloffset_Indexfv				(_BASE + 118)
#define _gloffset_Indexi				(_BASE + 119)
#define _gloffset_Indexiv				(_BASE + 120)
#define _gloffset_Indexs				(_BASE + 121)
#define _gloffset_Indexsv				(_BASE + 122)
#define _gloffset_InitNames				(_BASE + 123)
#define _gloffset_IsEnabled				(_BASE + 124)
#define _gloffset_IsList				(_BASE + 125)
#define _gloffset_LightModelf				(_BASE + 126)
#define _gloffset_LightModelfv				(_BASE + 127)
#define _gloffset_LightModeli				(_BASE + 128)
#define _gloffset_LightModeliv				(_BASE + 129)
#define _gloffset_Lightf				(_BASE + 130)
#define _gloffset_Lightfv				(_BASE + 131)
#define _gloffset_Lighti				(_BASE + 132)
#define _gloffset_Lightiv				(_BASE + 133)
#define _gloffset_LineStipple				(_BASE + 134)
#define _gloffset_LineWidth				(_BASE + 135)
#define _gloffset_ListBase				(_BASE + 136)
#define _gloffset_LoadIdentity				(_BASE + 137)
#define _gloffset_LoadMatrixd				(_BASE + 138)
#define _gloffset_LoadMatrixf				(_BASE + 139)
#define _gloffset_LoadName				(_BASE + 140)
#define _gloffset_LogicOp				(_BASE + 141)
#define _gloffset_Map1d					(_BASE + 142)
#define _gloffset_Map1f					(_BASE + 143)
#define _gloffset_Map2d					(_BASE + 144)
#define _gloffset_Map2f					(_BASE + 145)
#define _gloffset_MapGrid1d				(_BASE + 146)
#define _gloffset_MapGrid1f				(_BASE + 147)
#define _gloffset_MapGrid2d				(_BASE + 148)
#define _gloffset_MapGrid2f				(_BASE + 149)
#define _gloffset_Materialf				(_BASE + 150)
#define _gloffset_Materialfv				(_BASE + 151)
#define _gloffset_Materiali				(_BASE + 152)
#define _gloffset_Materialiv				(_BASE + 153)
#define _gloffset_MatrixMode				(_BASE + 154)
#define _gloffset_MultMatrixd				(_BASE + 155)
#define _gloffset_MultMatrixf				(_BASE + 156)
#define _gloffset_NewList				(_BASE + 157)
#define _gloffset_Normal3b				(_BASE + 158)
#define _gloffset_Normal3bv				(_BASE + 159)
#define _gloffset_Normal3d				(_BASE + 160)
#define _gloffset_Normal3dv				(_BASE + 161)
#define _gloffset_Normal3f				(_BASE + 162)
#define _gloffset_Normal3fv				(_BASE + 163)
#define _gloffset_Normal3i				(_BASE + 164)
#define _gloffset_Normal3iv				(_BASE + 165)
#define _gloffset_Normal3s				(_BASE + 166)
#define _gloffset_Normal3sv				(_BASE + 167)
#define _gloffset_Ortho					(_BASE + 168)
#define _gloffset_PassThrough				(_BASE + 169)
#define _gloffset_PixelMapfv				(_BASE + 170)
#define _gloffset_PixelMapuiv				(_BASE + 171)
#define _gloffset_PixelMapusv				(_BASE + 172)
#define _gloffset_PixelStoref				(_BASE + 173)
#define _gloffset_PixelStorei				(_BASE + 174)
#define _gloffset_PixelTransferf			(_BASE + 175)
#define _gloffset_PixelTransferi			(_BASE + 176)
#define _gloffset_PixelZoom				(_BASE + 177)
#define _gloffset_PointSize				(_BASE + 178)
#define _gloffset_PolygonMode				(_BASE + 179)
#define _gloffset_PolygonOffset				(_BASE + 180)
#define _gloffset_PolygonStipple			(_BASE + 181)
#define _gloffset_PopAttrib				(_BASE + 182)
#define _gloffset_PopMatrix				(_BASE + 183)
#define _gloffset_PopName				(_BASE + 184)
#define _gloffset_PushAttrib				(_BASE + 185)
#define _gloffset_PushMatrix				(_BASE + 186)
#define _gloffset_PushName				(_BASE + 187)
#define _gloffset_RasterPos2d				(_BASE + 188)
#define _gloffset_RasterPos2dv				(_BASE + 189)
#define _gloffset_RasterPos2f				(_BASE + 190)
#define _gloffset_RasterPos2fv				(_BASE + 191)
#define _gloffset_RasterPos2i				(_BASE + 192)
#define _gloffset_RasterPos2iv				(_BASE + 193)
#define _gloffset_RasterPos2s				(_BASE + 194)
#define _gloffset_RasterPos2sv				(_BASE + 195)
#define _gloffset_RasterPos3d				(_BASE + 196)
#define _gloffset_RasterPos3dv				(_BASE + 197)
#define _gloffset_RasterPos3f				(_BASE + 198)
#define _gloffset_RasterPos3fv				(_BASE + 199)
#define _gloffset_RasterPos3i				(_BASE + 200)
#define _gloffset_RasterPos3iv				(_BASE + 201)
#define _gloffset_RasterPos3s				(_BASE + 202)
#define _gloffset_RasterPos3sv				(_BASE + 203)
#define _gloffset_RasterPos4d				(_BASE + 204)
#define _gloffset_RasterPos4dv				(_BASE + 205)
#define _gloffset_RasterPos4f				(_BASE + 206)
#define _gloffset_RasterPos4fv				(_BASE + 207)
#define _gloffset_RasterPos4i				(_BASE + 208)
#define _gloffset_RasterPos4iv				(_BASE + 209)
#define _gloffset_RasterPos4s				(_BASE + 210)
#define _gloffset_RasterPos4sv				(_BASE + 211)
#define _gloffset_ReadBuffer				(_BASE + 212)
#define _gloffset_ReadPixels				(_BASE + 213)
#define _gloffset_Rectd					(_BASE + 214)
#define _gloffset_Rectdv				(_BASE + 215)
#define _gloffset_Rectf					(_BASE + 216)
#define _gloffset_Rectfv				(_BASE + 217)
#define _gloffset_Recti					(_BASE + 218)
#define _gloffset_Rectiv				(_BASE + 219)
#define _gloffset_Rects					(_BASE + 220)
#define _gloffset_Rectsv				(_BASE + 221)
#define _gloffset_RenderMode				(_BASE + 222)
#define _gloffset_Rotated				(_BASE + 223)
#define _gloffset_Rotatef				(_BASE + 224)
#define _gloffset_Scaled				(_BASE + 225)
#define _gloffset_Scalef				(_BASE + 226)
#define _gloffset_Scissor				(_BASE + 227)
#define _gloffset_SelectBuffer				(_BASE + 228)
#define _gloffset_ShadeModel				(_BASE + 229)
#define _gloffset_StencilFunc				(_BASE + 230)
#define _gloffset_StencilMask				(_BASE + 231)
#define _gloffset_StencilOp				(_BASE + 232)
#define _gloffset_TexCoord1d				(_BASE + 233)
#define _gloffset_TexCoord1dv				(_BASE + 234)
#define _gloffset_TexCoord1f				(_BASE + 235)
#define _gloffset_TexCoord1fv				(_BASE + 236)
#define _gloffset_TexCoord1i				(_BASE + 237)
#define _gloffset_TexCoord1iv				(_BASE + 238)
#define _gloffset_TexCoord1s				(_BASE + 239)
#define _gloffset_TexCoord1sv				(_BASE + 240)
#define _gloffset_TexCoord2d				(_BASE + 241)
#define _gloffset_TexCoord2dv				(_BASE + 242)
#define _gloffset_TexCoord2f				(_BASE + 243)
#define _gloffset_TexCoord2fv				(_BASE + 244)
#define _gloffset_TexCoord2i				(_BASE + 245)
#define _gloffset_TexCoord2iv				(_BASE + 246)
#define _gloffset_TexCoord2s				(_BASE + 247)
#define _gloffset_TexCoord2sv				(_BASE + 248)
#define _gloffset_TexCoord3d				(_BASE + 249)
#define _gloffset_TexCoord3dv				(_BASE + 250)
#define _gloffset_TexCoord3f				(_BASE + 251)
#define _gloffset_TexCoord3fv				(_BASE + 252)
#define _gloffset_TexCoord3i				(_BASE + 253)
#define _gloffset_TexCoord3iv				(_BASE + 254)
#define _gloffset_TexCoord3s				(_BASE + 255)
#define _gloffset_TexCoord3sv				(_BASE + 256)
#define _gloffset_TexCoord4d				(_BASE + 257)
#define _gloffset_TexCoord4dv				(_BASE + 258)
#define _gloffset_TexCoord4f				(_BASE + 259)
#define _gloffset_TexCoord4fv				(_BASE + 260)
#define _gloffset_TexCoord4i				(_BASE + 261)
#define _gloffset_TexCoord4iv				(_BASE + 262)
#define _gloffset_TexCoord4s				(_BASE + 263)
#define _gloffset_TexCoord4sv				(_BASE + 264)
#define _gloffset_TexEnvf				(_BASE + 265)
#define _gloffset_TexEnvfv				(_BASE + 266)
#define _gloffset_TexEnvi				(_BASE + 267)
#define _gloffset_TexEnviv				(_BASE + 268)
#define _gloffset_TexGend				(_BASE + 269)
#define _gloffset_TexGendv				(_BASE + 270)
#define _gloffset_TexGenf				(_BASE + 271)
#define _gloffset_TexGenfv				(_BASE + 272)
#define _gloffset_TexGeni				(_BASE + 273)
#define _gloffset_TexGeniv				(_BASE + 274)
#define _gloffset_TexImage1D				(_BASE + 275)
#define _gloffset_TexImage2D				(_BASE + 276)
#define _gloffset_TexParameterf				(_BASE + 277)
#define _gloffset_TexParameterfv			(_BASE + 278)
#define _gloffset_TexParameteri				(_BASE + 279)
#define _gloffset_TexParameteriv			(_BASE + 280)
#define _gloffset_Translated				(_BASE + 281)
#define _gloffset_Translatef				(_BASE + 282)
#define _gloffset_Vertex2d				(_BASE + 283)
#define _gloffset_Vertex2dv				(_BASE + 284)
#define _gloffset_Vertex2f				(_BASE + 285)
#define _gloffset_Vertex2fv				(_BASE + 286)
#define _gloffset_Vertex2i				(_BASE + 287)
#define _gloffset_Vertex2iv				(_BASE + 288)
#define _gloffset_Vertex2s				(_BASE + 289)
#define _gloffset_Vertex2sv				(_BASE + 290)
#define _gloffset_Vertex3d				(_BASE + 291)
#define _gloffset_Vertex3dv				(_BASE + 292)
#define _gloffset_Vertex3f				(_BASE + 293)
#define _gloffset_Vertex3fv				(_BASE + 294)
#define _gloffset_Vertex3i				(_BASE + 295)
#define _gloffset_Vertex3iv				(_BASE + 296)
#define _gloffset_Vertex3s				(_BASE + 297)
#define _gloffset_Vertex3sv				(_BASE + 298)
#define _gloffset_Vertex4d				(_BASE + 299)
#define _gloffset_Vertex4dv				(_BASE + 300)
#define _gloffset_Vertex4f				(_BASE + 301)
#define _gloffset_Vertex4fv				(_BASE + 302)
#define _gloffset_Vertex4i				(_BASE + 303)
#define _gloffset_Vertex4iv				(_BASE + 304)
#define _gloffset_Vertex4s				(_BASE + 305)
#define _gloffset_Vertex4sv				(_BASE + 306)
#define _gloffset_Viewport				(_BASE + 307)

/* GL 1.1 */
#define _gloffset_AreTexturesResident			(_BASE + 308)
#define _gloffset_ArrayElement				(_BASE + 309)
#define _gloffset_BindTexture				(_BASE + 310)
#define _gloffset_ColorPointer				(_BASE + 311)
#define _gloffset_CopyTexImage1D			(_BASE + 312)
#define _gloffset_CopyTexImage2D			(_BASE + 313)
#define _gloffset_CopyTexSubImage1D			(_BASE + 314)
#define _gloffset_CopyTexSubImage2D			(_BASE + 315)
#define _gloffset_DeleteTextures			(_BASE + 316)
#define _gloffset_DisableClientState			(_BASE + 317)
#define _gloffset_DrawArrays				(_BASE + 318)
#define _gloffset_DrawElements				(_BASE + 319)
#define _gloffset_EdgeFlagPointer			(_BASE + 320)
#define _gloffset_EnableClientState			(_BASE + 321)
#define _gloffset_GenTextures				(_BASE + 322)
#define _gloffset_GetPointerv				(_BASE + 323)
#define _gloffset_IndexPointer				(_BASE + 324)
#define _gloffset_Indexub				(_BASE + 325)
#define _gloffset_Indexubv				(_BASE + 326)
#define _gloffset_InterleavedArrays			(_BASE + 327)
#define _gloffset_IsTexture				(_BASE + 328)
#define _gloffset_NormalPointer				(_BASE + 329)
#define _gloffset_PopClientAttrib			(_BASE + 330)
#define _gloffset_PrioritizeTextures			(_BASE + 331)
#define _gloffset_PushClientAttrib			(_BASE + 332)
#define _gloffset_TexCoordPointer			(_BASE + 333)
#define _gloffset_TexSubImage1D				(_BASE + 334)
#define _gloffset_TexSubImage2D				(_BASE + 335)
#define _gloffset_VertexPointer				(_BASE + 336)

/* GL 1.2 */
#define _gloffset_CopyTexSubImage3D			(_BASE + 337)
#define _gloffset_DrawRangeElements			(_BASE + 338)
#define _gloffset_TexImage3D				(_BASE + 339)
#define _gloffset_TexSubImage3D				(_BASE + 340)

/* GL_ARB_imaging */
#define _gloffset_BlendColor				(_BASE + 341)
#define _gloffset_BlendEquation				(_BASE + 342)
#define _gloffset_ColorSubTable				(_BASE + 343)
#define _gloffset_ColorTable				(_BASE + 344)
#define _gloffset_ColorTableParameterfv			(_BASE + 345)
#define _gloffset_ColorTableParameteriv			(_BASE + 346)
#define _gloffset_ConvolutionFilter1D			(_BASE + 347)
#define _gloffset_ConvolutionFilter2D			(_BASE + 348)
#define _gloffset_ConvolutionParameterf			(_BASE + 349)
#define _gloffset_ConvolutionParameterfv		(_BASE + 350)
#define _gloffset_ConvolutionParameteri			(_BASE + 351)
#define _gloffset_ConvolutionParameteriv		(_BASE + 352)
#define _gloffset_CopyColorSubTable			(_BASE + 353)
#define _gloffset_CopyColorTable			(_BASE + 354)
#define _gloffset_CopyConvolutionFilter1D		(_BASE + 355)
#define _gloffset_CopyConvolutionFilter2D		(_BASE + 356)
#define _gloffset_GetColorTable				(_BASE + 357)
#define _gloffset_GetColorTableParameterfv		(_BASE + 358)
#define _gloffset_GetColorTableParameteriv		(_BASE + 359)
#define _gloffset_GetConvolutionFilter			(_BASE + 360)
#define _gloffset_GetConvolutionParameterfv		(_BASE + 361)
#define _gloffset_GetConvolutionParameteriv		(_BASE + 362)
#define _gloffset_GetHistogram				(_BASE + 363)
#define _gloffset_GetHistogramParameterfv		(_BASE + 364)
#define _gloffset_GetHistogramParameteriv		(_BASE + 365)
#define _gloffset_GetMinmax				(_BASE + 366)
#define _gloffset_GetMinmaxParameterfv			(_BASE + 367)
#define _gloffset_GetMinmaxParameteriv			(_BASE + 368)
#define _gloffset_GetSeparableFilter			(_BASE + 369)
#define _gloffset_Histogram				(_BASE + 370)
#define _gloffset_Minmax				(_BASE + 371)
#define _gloffset_ResetHistogram			(_BASE + 372)
#define _gloffset_ResetMinmax				(_BASE + 373)
#define _gloffset_SeparableFilter2D			(_BASE + 374)

/* GL_ARB_multitexture */
#define _gloffset_ActiveTextureARB			(_BASE + 375)
#define _gloffset_ClientActiveTextureARB		(_BASE + 376)
#define _gloffset_MultiTexCoord1dARB			(_BASE + 377)
#define _gloffset_MultiTexCoord1dvARB			(_BASE + 378)
#define _gloffset_MultiTexCoord1fARB			(_BASE + 379)
#define _gloffset_MultiTexCoord1fvARB			(_BASE + 380)
#define _gloffset_MultiTexCoord1iARB			(_BASE + 381)
#define _gloffset_MultiTexCoord1ivARB			(_BASE + 382)
#define _gloffset_MultiTexCoord1sARB			(_BASE + 383)
#define _gloffset_MultiTexCoord1svARB			(_BASE + 384)
#define _gloffset_MultiTexCoord2dARB			(_BASE + 385)
#define _gloffset_MultiTexCoord2dvARB			(_BASE + 386)
#define _gloffset_MultiTexCoord2fARB			(_BASE + 387)
#define _gloffset_MultiTexCoord2fvARB			(_BASE + 388)
#define _gloffset_MultiTexCoord2iARB			(_BASE + 389)
#define _gloffset_MultiTexCoord2ivARB			(_BASE + 390)
#define _gloffset_MultiTexCoord2sARB			(_BASE + 391)
#define _gloffset_MultiTexCoord2svARB			(_BASE + 392)
#define _gloffset_MultiTexCoord3dARB			(_BASE + 393)
#define _gloffset_MultiTexCoord3dvARB			(_BASE + 394)
#define _gloffset_MultiTexCoord3fARB			(_BASE + 395)
#define _gloffset_MultiTexCoord3fvARB			(_BASE + 396)
#define _gloffset_MultiTexCoord3iARB			(_BASE + 397)
#define _gloffset_MultiTexCoord3ivARB			(_BASE + 398)
#define _gloffset_MultiTexCoord3sARB			(_BASE + 399)
#define _gloffset_MultiTexCoord3svARB			(_BASE + 400)
#define _gloffset_MultiTexCoord4dARB			(_BASE + 401)
#define _gloffset_MultiTexCoord4dvARB			(_BASE + 402)
#define _gloffset_MultiTexCoord4fARB			(_BASE + 403)
#define _gloffset_MultiTexCoord4fvARB			(_BASE + 404)
#define _gloffset_MultiTexCoord4iARB			(_BASE + 405)
#define _gloffset_MultiTexCoord4ivARB			(_BASE + 406)
#define _gloffset_MultiTexCoord4sARB			(_BASE + 407)
#define _gloffset_MultiTexCoord4svARB			(_BASE + 408)



#define _EXTBASE (_BASE + 409)


/* 1. GL_EXT_abgr - no functions */

/* 2. GL_EXT_blend_color */
#define _gloffset_BlendColorEXT				(_EXTBASE + 0)

/* 3. GL_EXT_polygon_offset */
#define _gloffset_PolygonOffsetEXT			(_EXTBASE + 1)

/* 4. GL_EXT_texture - no functions */

/* 5. ??? */

/* 6. GL_EXT_texture3D */
#define _gloffset_CopyTexSubImage3DEXT			(_EXTBASE + 2)
#define _gloffset_TexImage3DEXT				(_EXTBASE + 3)
#define _gloffset_TexSubImage3DEXT			(_EXTBASE + 4)

/* 7. GL_SGI_texture_filter4 */
#define _gloffset_GetTexFilterFuncSGIS			(_EXTBASE + 5)
#define _gloffset_TexFilterFuncSGIS			(_EXTBASE + 6)

/* 8. ??? */

/* 9. GL_EXT_subtexture */
#define _gloffset_TexSubImage1DEXT			(_EXTBASE + 7)
#define _gloffset_TexSubImage2DEXT			(_EXTBASE + 8)
/*#define _gloffset_TexSubImage3DEXT*/

/* 10. GL_EXT_copy_texture */
#define _gloffset_CopyTexImage1DEXT			(_EXTBASE + 9)
#define _gloffset_CopyTexImage2DEXT			(_EXTBASE + 10)
#define _gloffset_CopyTexSubImage1DEXT			(_EXTBASE + 11)
#define _gloffset_CopyTexSubImage2DEXT			(_EXTBASE + 12)
/*#define _gloffset_CopyTexSubImage3DEXT*/
                              
/* 11. GL_EXT_histogram */
#define _gloffset_GetHistogramEXT			(_EXTBASE + 13)
#define _gloffset_GetHistogramParameterfvEXT		(_EXTBASE + 15)
#define _gloffset_GetHistogramParameterivEXT		(_EXTBASE + 14)
#define _gloffset_GetMinmaxEXT				(_EXTBASE + 16)
#define _gloffset_GetMinmaxParameterfvEXT		(_EXTBASE + 18)
#define _gloffset_GetMinmaxParameterivEXT		(_EXTBASE + 17)
#define _gloffset_HistogramEXT				(_EXTBASE + 19)
#define _gloffset_MinmaxEXT				(_EXTBASE + 20)
#define _gloffset_ResetHistogramEXT			(_EXTBASE + 21)
#define _gloffset_ResetMinmaxEXT			(_EXTBASE + 22)

/* 12. GL_EXT_convolution */
#define _gloffset_ConvolutionFilter1DEXT		(_EXTBASE + 23)
#define _gloffset_ConvolutionFilter2DEXT		(_EXTBASE + 24)
#define _gloffset_ConvolutionParameterfEXT		(_EXTBASE + 25)
#define _gloffset_ConvolutionParameterfvEXT		(_EXTBASE + 26)
#define _gloffset_ConvolutionParameteriEXT		(_EXTBASE + 27)
#define _gloffset_ConvolutionParameterivEXT		(_EXTBASE + 28)
#define _gloffset_CopyConvolutionFilter1DEXT		(_EXTBASE + 29)
#define _gloffset_CopyConvolutionFilter2DEXT		(_EXTBASE + 30)
#define _gloffset_GetConvolutionFilterEXT		(_EXTBASE + 31)
#define _gloffset_GetConvolutionParameterivEXT		(_EXTBASE + 32)
#define _gloffset_GetConvolutionParameterfvEXT		(_EXTBASE + 33)
#define _gloffset_GetSeparableFilterEXT			(_EXTBASE + 34)
#define _gloffset_SeparableFilter2DEXT			(_EXTBASE + 35)
                    
/* 13. GL_SGI_color_matrix - no functions */

/* 14. GL_SGI_color_table */
#define _gloffset_ColorTableSGI				(_EXTBASE + 36)
#define _gloffset_ColorTableParameterfvSGI		(_EXTBASE + 37)
#define _gloffset_ColorTableParameterivSGI		(_EXTBASE + 38)
#define _gloffset_CopyColorTableSGI			(_EXTBASE + 39)
#define _gloffset_GetColorTableSGI			(_EXTBASE + 40)
#define _gloffset_GetColorTableParameterfvSGI		(_EXTBASE + 41)
#define _gloffset_GetColorTableParameterivSGI		(_EXTBASE + 42)

/* 15. GL_SGIS_pixel_texture */
#define _gloffset_PixelTexGenParameterfSGIS		(_EXTBASE + 43)
#define _gloffset_PixelTexGenParameteriSGIS		(_EXTBASE + 44)
#define _gloffset_GetPixelTexGenParameterfvSGIS		(_EXTBASE + 45)
#define _gloffset_GetPixelTexGenParameterivSGIS		(_EXTBASE + 46)

/* 16. GL_SGIS_texture4D */
#define _gloffset_TexImage4DSGIS			(_EXTBASE + 47)
#define _gloffset_TexSubImage4DSGIS			(_EXTBASE + 48)

/* 17. GL_SGI_texture_color_table - no functions */

/* 18. GL_EXT_cmyka - no functions */

/* 19. ??? */

/* 20. GL_EXT_texture_object */
#define _gloffset_AreTexturesResidentEXT		(_EXTBASE + 49)
#define _gloffset_BindTextureEXT			(_EXTBASE + 50)
#define _gloffset_DeleteTexturesEXT			(_EXTBASE + 51)
#define _gloffset_GenTexturesEXT			(_EXTBASE + 52)
#define _gloffset_IsTextureEXT				(_EXTBASE + 53)
#define _gloffset_PrioritizeTexturesEXT			(_EXTBASE + 54)

/* 21. GL_SGIS_detail_texture */
#define _gloffset_DetailTexFuncSGIS			(_EXTBASE + 55)
#define _gloffset_GetDetailTexFuncSGIS			(_EXTBASE + 56)

/* 22. GL_SGIS_sharpen_texture */
#define _gloffset_GetSharpenTexFuncSGIS			(_EXTBASE + 57)
#define _gloffset_SharpenTexFuncSGIS			(_EXTBASE + 58)

/* 23. GL_EXT_packed_pixels - no functions */

/* 24. GL_SGIS_texture_lod - no functions */

/* 25. GL_SGIS_multisample */
#define _gloffset_SampleMaskSGIS			(_EXTBASE + 54)
#define _gloffset_SamplePatternSGIS			(_EXTBASE + 55)

/* 26. ??? */

/* 27. GL_EXT_rescale_normal - no functions */

/* 28. GLX_EXT_visual_info - no functions */

/* 29. ??? */

/* 30. GL_EXT_vertex_array */
#define _gloffset_ArrayElementEXT			(_EXTBASE + 56)
#define _gloffset_ColorPointerEXT			(_EXTBASE + 57)
#define _gloffset_DrawArraysEXT				(_EXTBASE + 58)
#define _gloffset_EdgeFlagPointerEXT			(_EXTBASE + 59)
#define _gloffset_GetPointervEXT			(_EXTBASE + 60)
#define _gloffset_IndexPointerEXT			(_EXTBASE + 61)
#define _gloffset_NormalPointerEXT			(_EXTBASE + 62)
#define _gloffset_TexCoordPointerEXT			(_EXTBASE + 63)
#define _gloffset_VertexPointerEXT			(_EXTBASE + 64)

/* 31. GL_EXT_misc_attribute - no functions */

/* 32. GL_SGIS_generate_mipmap - no functions */

/* 33. GL_SGIX_clipmap - no functions */

/* 34. GL_SGIX_shadow - no functions */

/* 35. GL_SGIS_texture_edge_clamp - no functions */

/* 36. GL_SGIS_texture_border_clamp - no functions */

/* 37. GL_EXT_blend_minmax */
#define _gloffset_BlendEquationEXT			(_EXTBASE + 65)

/* 38. GL_EXT_blend_subtract - no functions */

/* 39. GL_EXT_blend_logic_op - no functions */

/* 40. GLX_SGI_swap_control - GLX functions */

/* 41. GLX_SGI_video_sync - GLX functions */

/* 42. GLX_SGI_make_current_read - GLX functions */

/* 43. GLX_SGIX_video_source - GLX functions */

/* 44. GLX_EXT_visual_rating - no functions */

/* 45. GL_SGIX_interlace - no functions */

/* 46. ??? */

/* 47. GLX_EXT_import_context - GLX functions */

/* 48. ??? */

/* 49. GLX_SGIX_fbconfig - some GLX functions */

/* 50. GLX_SGIX_pbuffer - GLX functions */

/* 51. GL_SGIS_texture_select - no functions */

/* 52. GL_SGIX_sprite */
#define _gloffset_SpriteParameterfSGIX			(_EXTBASE + 66)
#define _gloffset_SpriteParameterfvSGIX			(_EXTBASE + 67)
#define _gloffset_SpriteParameteriSGIX			(_EXTBASE + 68)
#define _gloffset_SpriteParameterivSGIX			(_EXTBASE + 69)

/* 53. ??? */

/* 54. GL_EXT_point_parameters */
#define _gloffset_PointParameterfEXT			(_EXTBASE + 70)
#define _gloffset_PointParameterfvEXT			(_EXTBASE + 71)

/* 55. GL_SGIX_instruments */
#define _gloffset_InstrumentsBufferSGIX			(_EXTBASE + 72)
#define _gloffset_StartInstrumentsSGIX			(_EXTBASE + 73)
#define _gloffset_StopInstrumentsSGIX			(_EXTBASE + 74)
#define _gloffset_ReadInstrumentsSGIX			(_EXTBASE + 75)
#define _gloffset_PollInstrumentsSGIX			(_EXTBASE + 76)
#define _gloffset_GetInstrumentsSGIX			(_EXTBASE + 77)

/* 56. GL_SGIX_texture_scale_bias - no functions */

/* 57. GL_SGIX_framezoom */
#define _gloffset_FrameZoomSGIX				(_EXTBASE + 78)

/* 58. GL_SGIX_tag_sample_buffer - no functions */

/* 59. ??? */

/* 60. GL_SGIX_reference_plane */
#define _gloffset_ReferencePlaneSGIX			(_EXTBASE + 79)

/* 61. GL_SGIX_flush_raster */
#define _gloffset_FlushRasterSGIX			(_EXTBASE + 80)

/* 62. GLX_SGI_cushion - GLX functions */

/* 63. GL_SGIX_depth_texture - no functions */

/* 64. ??? */

/* 65. GL_SGIX_fog_offset - no functions */

/* 66. GL_HP_image_transform */
#define _gloffset_GetImageTransformParameterfvHP	(_EXTBASE + 81)
#define _gloffset_GetImageTransformParameterivHP	(_EXTBASE + 82)
#define _gloffset_ImageTransformParameterfHP		(_EXTBASE + 83)
#define _gloffset_ImageTransformParameterfvHP		(_EXTBASE + 84)
#define _gloffset_ImageTransformParameteriHP		(_EXTBASE + 85)
#define _gloffset_ImageTransformParameterivHP		(_EXTBASE + 86)

/* 67. GL_HP_convolution_border_modes - no functions */

/* 68. ??? */

/* 69. GL_SGIX_texture_add_env - no functions */

/* 70. ??? */

/* 71. ??? */

/* 72. ??? */

/* 73. ??? */

/* 74. GL_EXT_color_subtable */
#define _gloffset_ColorSubTableEXT			(_EXTBASE + 87)
#define _gloffset_CopyColorSubTableEXT			(_EXTBASE + 88)

/* 75. GLU_EXT_object_space_tess - GLU functions */

/* 76. GL_PGI_vertex_hints - no functions */

/* 77. GL_PGI_misc_hints */
#define _gloffset_HintPGI				(_EXTBASE + 89)

/* 78. GL_EXT_paletted_texture */
/* ColorSubTableEXT already defined */
#define _gloffset_ColorTableEXT				(_EXTBASE + 91)
#define _gloffset_GetColorTableEXT			(_EXTBASE + 92)
#define _gloffset_GetColorTableParameterfvEXT		(_EXTBASE + 93)
#define _gloffset_GetColorTableParameterivEXT		(_EXTBASE + 94)

/* 79. GL_EXT_clip_volume_hint - no functions */

/* 80. GL_SGIX_list_priority */
#define _gloffset_GetListParameterfvSGIX		(_EXTBASE + 95)
#define _gloffset_GetListParameterivSGIX		(_EXTBASE + 96)
#define _gloffset_ListParameterfSGIX			(_EXTBASE + 97)
#define _gloffset_ListParameterfvSGIX			(_EXTBASE + 98)
#define _gloffset_ListParameteriSGIX			(_EXTBASE + 99)
#define _gloffset_ListParameterivSGIX			(_EXTBASE + 100)

/* 81. GL_SGIX_ir_instrument1 - no functions */

/* 82. ??? */

/* 83. GLX_SGIX_video_resize - GLX functions */

/* 84. GL_SGIX_texture_lod_bias - no functions */

/* 85. GLU_SGI_filter4_parameters - GLU functions */

/* 86. GLX_SGIX_dm_buffer - GLX functions */

/* 87. ??? */

/* 88. ??? */

/* 89. ??? */

/* 90. ??? */

/* 91. GLX_SGIX_swap_group - GLX functions */

/* 92. GLX_SGIX_swap_barrier - GLX functions */

/* 93. GL_EXT_index_texture - no functions */

/* 94. GL_EXT_index_material */
#define _gloffset_IndexMaterialEXT			(_EXTBASE + 101)

/* 95. GL_EXT_index_func */
#define _gloffset_IndexFuncEXT				(_EXTBASE + 102)

/* 96. GL_EXT_index_array_formats - no functions */

/* 97. GL_EXT_compiled_vertex_array */
#define _gloffset_LockArraysEXT				(_EXTBASE + 103)
#define _gloffset_UnlockArraysEXT			(_EXTBASE + 104)

/* 98. GL_EXT_cull_vertex */
#define _gloffset_CullParameterfvEXT			(_EXTBASE + 105)
#define _gloffset_CullParameterdvEXT			(_EXTBASE + 106)

/* 99. ??? */

/* 100. GLU_EXT_nurbs_tessellator - GLU functions */

/* 173. GL_EXT/INGR_blend_func_separate */
#define _gloffset_BlendFuncSeparateINGR			(_EXTBASE + 107)

/* GL_MESA_window_pos */
#define _gloffset_WindowPos2dMESA			(_EXTBASE + 108)
#define _gloffset_WindowPos2dvMESA			(_EXTBASE + 109)
#define _gloffset_WindowPos2fMESA			(_EXTBASE + 110)
#define _gloffset_WindowPos2fvMESA			(_EXTBASE + 111)
#define _gloffset_WindowPos2iMESA			(_EXTBASE + 112)
#define _gloffset_WindowPos2ivMESA			(_EXTBASE + 113)
#define _gloffset_WindowPos2sMESA			(_EXTBASE + 114)
#define _gloffset_WindowPos2svMESA			(_EXTBASE + 115)
#define _gloffset_WindowPos3dMESA			(_EXTBASE + 116)
#define _gloffset_WindowPos3dvMESA			(_EXTBASE + 117)
#define _gloffset_WindowPos3fMESA			(_EXTBASE + 118)
#define _gloffset_WindowPos3fvMESA			(_EXTBASE + 119)
#define _gloffset_WindowPos3iMESA			(_EXTBASE + 120)
#define _gloffset_WindowPos3ivMESA			(_EXTBASE + 121)
#define _gloffset_WindowPos3sMESA			(_EXTBASE + 122)
#define _gloffset_WindowPos3svMESA			(_EXTBASE + 123)
#define _gloffset_WindowPos4dMESA			(_EXTBASE + 124)
#define _gloffset_WindowPos4dvMESA			(_EXTBASE + 125)
#define _gloffset_WindowPos4fMESA			(_EXTBASE + 126)
#define _gloffset_WindowPos4fvMESA			(_EXTBASE + 127)
#define _gloffset_WindowPos4iMESA			(_EXTBASE + 128)
#define _gloffset_WindowPos4ivMESA			(_EXTBASE + 129)
#define _gloffset_WindowPos4sMESA			(_EXTBASE + 130)
#define _gloffset_WindowPos4svMESA			(_EXTBASE + 131)

/* GL_MESA_resize_buffers */
#define _gloffset_ResizeBuffersMESA			(_EXTBASE + 132)

/* GL_ARB_transpose_matrix */
#define _gloffset_LoadTransposeMatrixdARB		(_EXTBASE + 133)
#define _gloffset_LoadTransposeMatrixfARB		(_EXTBASE + 134)
#define _gloffset_MultTransposeMatrixdARB		(_EXTBASE + 135)
#define _gloffset_MultTransposeMatrixfARB		(_EXTBASE + 136)



#endif

