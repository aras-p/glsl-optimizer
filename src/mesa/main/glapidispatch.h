/* DO NOT EDIT - This file generated automatically by gl_table.py (from Mesa) script */

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

#if !defined( _GLAPI_DISPATCH_H_ )
#  define _GLAPI_DISPATCH_H_


/* this file should not be included directly in mesa */

/**
 * \file glapidispatch.h
 * Macros for handling GL dispatch tables.
 *
 * For each known GL function, there are 3 macros in this file.  The first
 * macro is named CALL_FuncName and is used to call that GL function using
 * the specified dispatch table.  The other 2 macros, called GET_FuncName
 * can SET_FuncName, are used to get and set the dispatch pointer for the
 * named function in the specified dispatch table.
 */

#define CALL_by_offset(disp, cast, offset, parameters) \
    (*(cast (GET_by_offset(disp, offset)))) parameters
#define GET_by_offset(disp, offset) \
    (offset >= 0) ? (((_glapi_proc *)(disp))[offset]) : NULL
#define SET_by_offset(disp, offset, fn) \
    do { \
        if ( (offset) < 0 ) { \
            /* fprintf( stderr, "[%s:%u] SET_by_offset(%p, %d, %s)!\n", */ \
            /*         __func__, __LINE__, disp, offset, # fn); */ \
            /* abort(); */ \
        } \
        else { \
            ( (_glapi_proc *) (disp) )[offset] = (_glapi_proc) fn; \
        } \
    } while(0)

/* total number of offsets below */
#define _gloffset_COUNT 870

#define _gloffset_NewList 0
#define _gloffset_EndList 1
#define _gloffset_CallList 2
#define _gloffset_CallLists 3
#define _gloffset_DeleteLists 4
#define _gloffset_GenLists 5
#define _gloffset_ListBase 6
#define _gloffset_Begin 7
#define _gloffset_Bitmap 8
#define _gloffset_Color3b 9
#define _gloffset_Color3bv 10
#define _gloffset_Color3d 11
#define _gloffset_Color3dv 12
#define _gloffset_Color3f 13
#define _gloffset_Color3fv 14
#define _gloffset_Color3i 15
#define _gloffset_Color3iv 16
#define _gloffset_Color3s 17
#define _gloffset_Color3sv 18
#define _gloffset_Color3ub 19
#define _gloffset_Color3ubv 20
#define _gloffset_Color3ui 21
#define _gloffset_Color3uiv 22
#define _gloffset_Color3us 23
#define _gloffset_Color3usv 24
#define _gloffset_Color4b 25
#define _gloffset_Color4bv 26
#define _gloffset_Color4d 27
#define _gloffset_Color4dv 28
#define _gloffset_Color4f 29
#define _gloffset_Color4fv 30
#define _gloffset_Color4i 31
#define _gloffset_Color4iv 32
#define _gloffset_Color4s 33
#define _gloffset_Color4sv 34
#define _gloffset_Color4ub 35
#define _gloffset_Color4ubv 36
#define _gloffset_Color4ui 37
#define _gloffset_Color4uiv 38
#define _gloffset_Color4us 39
#define _gloffset_Color4usv 40
#define _gloffset_EdgeFlag 41
#define _gloffset_EdgeFlagv 42
#define _gloffset_End 43
#define _gloffset_Indexd 44
#define _gloffset_Indexdv 45
#define _gloffset_Indexf 46
#define _gloffset_Indexfv 47
#define _gloffset_Indexi 48
#define _gloffset_Indexiv 49
#define _gloffset_Indexs 50
#define _gloffset_Indexsv 51
#define _gloffset_Normal3b 52
#define _gloffset_Normal3bv 53
#define _gloffset_Normal3d 54
#define _gloffset_Normal3dv 55
#define _gloffset_Normal3f 56
#define _gloffset_Normal3fv 57
#define _gloffset_Normal3i 58
#define _gloffset_Normal3iv 59
#define _gloffset_Normal3s 60
#define _gloffset_Normal3sv 61
#define _gloffset_RasterPos2d 62
#define _gloffset_RasterPos2dv 63
#define _gloffset_RasterPos2f 64
#define _gloffset_RasterPos2fv 65
#define _gloffset_RasterPos2i 66
#define _gloffset_RasterPos2iv 67
#define _gloffset_RasterPos2s 68
#define _gloffset_RasterPos2sv 69
#define _gloffset_RasterPos3d 70
#define _gloffset_RasterPos3dv 71
#define _gloffset_RasterPos3f 72
#define _gloffset_RasterPos3fv 73
#define _gloffset_RasterPos3i 74
#define _gloffset_RasterPos3iv 75
#define _gloffset_RasterPos3s 76
#define _gloffset_RasterPos3sv 77
#define _gloffset_RasterPos4d 78
#define _gloffset_RasterPos4dv 79
#define _gloffset_RasterPos4f 80
#define _gloffset_RasterPos4fv 81
#define _gloffset_RasterPos4i 82
#define _gloffset_RasterPos4iv 83
#define _gloffset_RasterPos4s 84
#define _gloffset_RasterPos4sv 85
#define _gloffset_Rectd 86
#define _gloffset_Rectdv 87
#define _gloffset_Rectf 88
#define _gloffset_Rectfv 89
#define _gloffset_Recti 90
#define _gloffset_Rectiv 91
#define _gloffset_Rects 92
#define _gloffset_Rectsv 93
#define _gloffset_TexCoord1d 94
#define _gloffset_TexCoord1dv 95
#define _gloffset_TexCoord1f 96
#define _gloffset_TexCoord1fv 97
#define _gloffset_TexCoord1i 98
#define _gloffset_TexCoord1iv 99
#define _gloffset_TexCoord1s 100
#define _gloffset_TexCoord1sv 101
#define _gloffset_TexCoord2d 102
#define _gloffset_TexCoord2dv 103
#define _gloffset_TexCoord2f 104
#define _gloffset_TexCoord2fv 105
#define _gloffset_TexCoord2i 106
#define _gloffset_TexCoord2iv 107
#define _gloffset_TexCoord2s 108
#define _gloffset_TexCoord2sv 109
#define _gloffset_TexCoord3d 110
#define _gloffset_TexCoord3dv 111
#define _gloffset_TexCoord3f 112
#define _gloffset_TexCoord3fv 113
#define _gloffset_TexCoord3i 114
#define _gloffset_TexCoord3iv 115
#define _gloffset_TexCoord3s 116
#define _gloffset_TexCoord3sv 117
#define _gloffset_TexCoord4d 118
#define _gloffset_TexCoord4dv 119
#define _gloffset_TexCoord4f 120
#define _gloffset_TexCoord4fv 121
#define _gloffset_TexCoord4i 122
#define _gloffset_TexCoord4iv 123
#define _gloffset_TexCoord4s 124
#define _gloffset_TexCoord4sv 125
#define _gloffset_Vertex2d 126
#define _gloffset_Vertex2dv 127
#define _gloffset_Vertex2f 128
#define _gloffset_Vertex2fv 129
#define _gloffset_Vertex2i 130
#define _gloffset_Vertex2iv 131
#define _gloffset_Vertex2s 132
#define _gloffset_Vertex2sv 133
#define _gloffset_Vertex3d 134
#define _gloffset_Vertex3dv 135
#define _gloffset_Vertex3f 136
#define _gloffset_Vertex3fv 137
#define _gloffset_Vertex3i 138
#define _gloffset_Vertex3iv 139
#define _gloffset_Vertex3s 140
#define _gloffset_Vertex3sv 141
#define _gloffset_Vertex4d 142
#define _gloffset_Vertex4dv 143
#define _gloffset_Vertex4f 144
#define _gloffset_Vertex4fv 145
#define _gloffset_Vertex4i 146
#define _gloffset_Vertex4iv 147
#define _gloffset_Vertex4s 148
#define _gloffset_Vertex4sv 149
#define _gloffset_ClipPlane 150
#define _gloffset_ColorMaterial 151
#define _gloffset_CullFace 152
#define _gloffset_Fogf 153
#define _gloffset_Fogfv 154
#define _gloffset_Fogi 155
#define _gloffset_Fogiv 156
#define _gloffset_FrontFace 157
#define _gloffset_Hint 158
#define _gloffset_Lightf 159
#define _gloffset_Lightfv 160
#define _gloffset_Lighti 161
#define _gloffset_Lightiv 162
#define _gloffset_LightModelf 163
#define _gloffset_LightModelfv 164
#define _gloffset_LightModeli 165
#define _gloffset_LightModeliv 166
#define _gloffset_LineStipple 167
#define _gloffset_LineWidth 168
#define _gloffset_Materialf 169
#define _gloffset_Materialfv 170
#define _gloffset_Materiali 171
#define _gloffset_Materialiv 172
#define _gloffset_PointSize 173
#define _gloffset_PolygonMode 174
#define _gloffset_PolygonStipple 175
#define _gloffset_Scissor 176
#define _gloffset_ShadeModel 177
#define _gloffset_TexParameterf 178
#define _gloffset_TexParameterfv 179
#define _gloffset_TexParameteri 180
#define _gloffset_TexParameteriv 181
#define _gloffset_TexImage1D 182
#define _gloffset_TexImage2D 183
#define _gloffset_TexEnvf 184
#define _gloffset_TexEnvfv 185
#define _gloffset_TexEnvi 186
#define _gloffset_TexEnviv 187
#define _gloffset_TexGend 188
#define _gloffset_TexGendv 189
#define _gloffset_TexGenf 190
#define _gloffset_TexGenfv 191
#define _gloffset_TexGeni 192
#define _gloffset_TexGeniv 193
#define _gloffset_FeedbackBuffer 194
#define _gloffset_SelectBuffer 195
#define _gloffset_RenderMode 196
#define _gloffset_InitNames 197
#define _gloffset_LoadName 198
#define _gloffset_PassThrough 199
#define _gloffset_PopName 200
#define _gloffset_PushName 201
#define _gloffset_DrawBuffer 202
#define _gloffset_Clear 203
#define _gloffset_ClearAccum 204
#define _gloffset_ClearIndex 205
#define _gloffset_ClearColor 206
#define _gloffset_ClearStencil 207
#define _gloffset_ClearDepth 208
#define _gloffset_StencilMask 209
#define _gloffset_ColorMask 210
#define _gloffset_DepthMask 211
#define _gloffset_IndexMask 212
#define _gloffset_Accum 213
#define _gloffset_Disable 214
#define _gloffset_Enable 215
#define _gloffset_Finish 216
#define _gloffset_Flush 217
#define _gloffset_PopAttrib 218
#define _gloffset_PushAttrib 219
#define _gloffset_Map1d 220
#define _gloffset_Map1f 221
#define _gloffset_Map2d 222
#define _gloffset_Map2f 223
#define _gloffset_MapGrid1d 224
#define _gloffset_MapGrid1f 225
#define _gloffset_MapGrid2d 226
#define _gloffset_MapGrid2f 227
#define _gloffset_EvalCoord1d 228
#define _gloffset_EvalCoord1dv 229
#define _gloffset_EvalCoord1f 230
#define _gloffset_EvalCoord1fv 231
#define _gloffset_EvalCoord2d 232
#define _gloffset_EvalCoord2dv 233
#define _gloffset_EvalCoord2f 234
#define _gloffset_EvalCoord2fv 235
#define _gloffset_EvalMesh1 236
#define _gloffset_EvalPoint1 237
#define _gloffset_EvalMesh2 238
#define _gloffset_EvalPoint2 239
#define _gloffset_AlphaFunc 240
#define _gloffset_BlendFunc 241
#define _gloffset_LogicOp 242
#define _gloffset_StencilFunc 243
#define _gloffset_StencilOp 244
#define _gloffset_DepthFunc 245
#define _gloffset_PixelZoom 246
#define _gloffset_PixelTransferf 247
#define _gloffset_PixelTransferi 248
#define _gloffset_PixelStoref 249
#define _gloffset_PixelStorei 250
#define _gloffset_PixelMapfv 251
#define _gloffset_PixelMapuiv 252
#define _gloffset_PixelMapusv 253
#define _gloffset_ReadBuffer 254
#define _gloffset_CopyPixels 255
#define _gloffset_ReadPixels 256
#define _gloffset_DrawPixels 257
#define _gloffset_GetBooleanv 258
#define _gloffset_GetClipPlane 259
#define _gloffset_GetDoublev 260
#define _gloffset_GetError 261
#define _gloffset_GetFloatv 262
#define _gloffset_GetIntegerv 263
#define _gloffset_GetLightfv 264
#define _gloffset_GetLightiv 265
#define _gloffset_GetMapdv 266
#define _gloffset_GetMapfv 267
#define _gloffset_GetMapiv 268
#define _gloffset_GetMaterialfv 269
#define _gloffset_GetMaterialiv 270
#define _gloffset_GetPixelMapfv 271
#define _gloffset_GetPixelMapuiv 272
#define _gloffset_GetPixelMapusv 273
#define _gloffset_GetPolygonStipple 274
#define _gloffset_GetString 275
#define _gloffset_GetTexEnvfv 276
#define _gloffset_GetTexEnviv 277
#define _gloffset_GetTexGendv 278
#define _gloffset_GetTexGenfv 279
#define _gloffset_GetTexGeniv 280
#define _gloffset_GetTexImage 281
#define _gloffset_GetTexParameterfv 282
#define _gloffset_GetTexParameteriv 283
#define _gloffset_GetTexLevelParameterfv 284
#define _gloffset_GetTexLevelParameteriv 285
#define _gloffset_IsEnabled 286
#define _gloffset_IsList 287
#define _gloffset_DepthRange 288
#define _gloffset_Frustum 289
#define _gloffset_LoadIdentity 290
#define _gloffset_LoadMatrixf 291
#define _gloffset_LoadMatrixd 292
#define _gloffset_MatrixMode 293
#define _gloffset_MultMatrixf 294
#define _gloffset_MultMatrixd 295
#define _gloffset_Ortho 296
#define _gloffset_PopMatrix 297
#define _gloffset_PushMatrix 298
#define _gloffset_Rotated 299
#define _gloffset_Rotatef 300
#define _gloffset_Scaled 301
#define _gloffset_Scalef 302
#define _gloffset_Translated 303
#define _gloffset_Translatef 304
#define _gloffset_Viewport 305
#define _gloffset_ArrayElement 306
#define _gloffset_BindTexture 307
#define _gloffset_ColorPointer 308
#define _gloffset_DisableClientState 309
#define _gloffset_DrawArrays 310
#define _gloffset_DrawElements 311
#define _gloffset_EdgeFlagPointer 312
#define _gloffset_EnableClientState 313
#define _gloffset_IndexPointer 314
#define _gloffset_Indexub 315
#define _gloffset_Indexubv 316
#define _gloffset_InterleavedArrays 317
#define _gloffset_NormalPointer 318
#define _gloffset_PolygonOffset 319
#define _gloffset_TexCoordPointer 320
#define _gloffset_VertexPointer 321
#define _gloffset_AreTexturesResident 322
#define _gloffset_CopyTexImage1D 323
#define _gloffset_CopyTexImage2D 324
#define _gloffset_CopyTexSubImage1D 325
#define _gloffset_CopyTexSubImage2D 326
#define _gloffset_DeleteTextures 327
#define _gloffset_GenTextures 328
#define _gloffset_GetPointerv 329
#define _gloffset_IsTexture 330
#define _gloffset_PrioritizeTextures 331
#define _gloffset_TexSubImage1D 332
#define _gloffset_TexSubImage2D 333
#define _gloffset_PopClientAttrib 334
#define _gloffset_PushClientAttrib 335
#define _gloffset_BlendColor 336
#define _gloffset_BlendEquation 337
#define _gloffset_DrawRangeElements 338
#define _gloffset_ColorTable 339
#define _gloffset_ColorTableParameterfv 340
#define _gloffset_ColorTableParameteriv 341
#define _gloffset_CopyColorTable 342
#define _gloffset_GetColorTable 343
#define _gloffset_GetColorTableParameterfv 344
#define _gloffset_GetColorTableParameteriv 345
#define _gloffset_ColorSubTable 346
#define _gloffset_CopyColorSubTable 347
#define _gloffset_ConvolutionFilter1D 348
#define _gloffset_ConvolutionFilter2D 349
#define _gloffset_ConvolutionParameterf 350
#define _gloffset_ConvolutionParameterfv 351
#define _gloffset_ConvolutionParameteri 352
#define _gloffset_ConvolutionParameteriv 353
#define _gloffset_CopyConvolutionFilter1D 354
#define _gloffset_CopyConvolutionFilter2D 355
#define _gloffset_GetConvolutionFilter 356
#define _gloffset_GetConvolutionParameterfv 357
#define _gloffset_GetConvolutionParameteriv 358
#define _gloffset_GetSeparableFilter 359
#define _gloffset_SeparableFilter2D 360
#define _gloffset_GetHistogram 361
#define _gloffset_GetHistogramParameterfv 362
#define _gloffset_GetHistogramParameteriv 363
#define _gloffset_GetMinmax 364
#define _gloffset_GetMinmaxParameterfv 365
#define _gloffset_GetMinmaxParameteriv 366
#define _gloffset_Histogram 367
#define _gloffset_Minmax 368
#define _gloffset_ResetHistogram 369
#define _gloffset_ResetMinmax 370
#define _gloffset_TexImage3D 371
#define _gloffset_TexSubImage3D 372
#define _gloffset_CopyTexSubImage3D 373
#define _gloffset_ActiveTextureARB 374
#define _gloffset_ClientActiveTextureARB 375
#define _gloffset_MultiTexCoord1dARB 376
#define _gloffset_MultiTexCoord1dvARB 377
#define _gloffset_MultiTexCoord1fARB 378
#define _gloffset_MultiTexCoord1fvARB 379
#define _gloffset_MultiTexCoord1iARB 380
#define _gloffset_MultiTexCoord1ivARB 381
#define _gloffset_MultiTexCoord1sARB 382
#define _gloffset_MultiTexCoord1svARB 383
#define _gloffset_MultiTexCoord2dARB 384
#define _gloffset_MultiTexCoord2dvARB 385
#define _gloffset_MultiTexCoord2fARB 386
#define _gloffset_MultiTexCoord2fvARB 387
#define _gloffset_MultiTexCoord2iARB 388
#define _gloffset_MultiTexCoord2ivARB 389
#define _gloffset_MultiTexCoord2sARB 390
#define _gloffset_MultiTexCoord2svARB 391
#define _gloffset_MultiTexCoord3dARB 392
#define _gloffset_MultiTexCoord3dvARB 393
#define _gloffset_MultiTexCoord3fARB 394
#define _gloffset_MultiTexCoord3fvARB 395
#define _gloffset_MultiTexCoord3iARB 396
#define _gloffset_MultiTexCoord3ivARB 397
#define _gloffset_MultiTexCoord3sARB 398
#define _gloffset_MultiTexCoord3svARB 399
#define _gloffset_MultiTexCoord4dARB 400
#define _gloffset_MultiTexCoord4dvARB 401
#define _gloffset_MultiTexCoord4fARB 402
#define _gloffset_MultiTexCoord4fvARB 403
#define _gloffset_MultiTexCoord4iARB 404
#define _gloffset_MultiTexCoord4ivARB 405
#define _gloffset_MultiTexCoord4sARB 406
#define _gloffset_MultiTexCoord4svARB 407

#if !defined(_GLAPI_USE_REMAP_TABLE)

#define _gloffset_AttachShader 408
#define _gloffset_CreateProgram 409
#define _gloffset_CreateShader 410
#define _gloffset_DeleteProgram 411
#define _gloffset_DeleteShader 412
#define _gloffset_DetachShader 413
#define _gloffset_GetAttachedShaders 414
#define _gloffset_GetProgramInfoLog 415
#define _gloffset_GetProgramiv 416
#define _gloffset_GetShaderInfoLog 417
#define _gloffset_GetShaderiv 418
#define _gloffset_IsProgram 419
#define _gloffset_IsShader 420
#define _gloffset_StencilFuncSeparate 421
#define _gloffset_StencilMaskSeparate 422
#define _gloffset_StencilOpSeparate 423
#define _gloffset_UniformMatrix2x3fv 424
#define _gloffset_UniformMatrix2x4fv 425
#define _gloffset_UniformMatrix3x2fv 426
#define _gloffset_UniformMatrix3x4fv 427
#define _gloffset_UniformMatrix4x2fv 428
#define _gloffset_UniformMatrix4x3fv 429
#define _gloffset_DrawArraysInstanced 430
#define _gloffset_DrawElementsInstanced 431
#define _gloffset_LoadTransposeMatrixdARB 432
#define _gloffset_LoadTransposeMatrixfARB 433
#define _gloffset_MultTransposeMatrixdARB 434
#define _gloffset_MultTransposeMatrixfARB 435
#define _gloffset_SampleCoverageARB 436
#define _gloffset_CompressedTexImage1DARB 437
#define _gloffset_CompressedTexImage2DARB 438
#define _gloffset_CompressedTexImage3DARB 439
#define _gloffset_CompressedTexSubImage1DARB 440
#define _gloffset_CompressedTexSubImage2DARB 441
#define _gloffset_CompressedTexSubImage3DARB 442
#define _gloffset_GetCompressedTexImageARB 443
#define _gloffset_DisableVertexAttribArrayARB 444
#define _gloffset_EnableVertexAttribArrayARB 445
#define _gloffset_GetProgramEnvParameterdvARB 446
#define _gloffset_GetProgramEnvParameterfvARB 447
#define _gloffset_GetProgramLocalParameterdvARB 448
#define _gloffset_GetProgramLocalParameterfvARB 449
#define _gloffset_GetProgramStringARB 450
#define _gloffset_GetProgramivARB 451
#define _gloffset_GetVertexAttribdvARB 452
#define _gloffset_GetVertexAttribfvARB 453
#define _gloffset_GetVertexAttribivARB 454
#define _gloffset_ProgramEnvParameter4dARB 455
#define _gloffset_ProgramEnvParameter4dvARB 456
#define _gloffset_ProgramEnvParameter4fARB 457
#define _gloffset_ProgramEnvParameter4fvARB 458
#define _gloffset_ProgramLocalParameter4dARB 459
#define _gloffset_ProgramLocalParameter4dvARB 460
#define _gloffset_ProgramLocalParameter4fARB 461
#define _gloffset_ProgramLocalParameter4fvARB 462
#define _gloffset_ProgramStringARB 463
#define _gloffset_VertexAttrib1dARB 464
#define _gloffset_VertexAttrib1dvARB 465
#define _gloffset_VertexAttrib1fARB 466
#define _gloffset_VertexAttrib1fvARB 467
#define _gloffset_VertexAttrib1sARB 468
#define _gloffset_VertexAttrib1svARB 469
#define _gloffset_VertexAttrib2dARB 470
#define _gloffset_VertexAttrib2dvARB 471
#define _gloffset_VertexAttrib2fARB 472
#define _gloffset_VertexAttrib2fvARB 473
#define _gloffset_VertexAttrib2sARB 474
#define _gloffset_VertexAttrib2svARB 475
#define _gloffset_VertexAttrib3dARB 476
#define _gloffset_VertexAttrib3dvARB 477
#define _gloffset_VertexAttrib3fARB 478
#define _gloffset_VertexAttrib3fvARB 479
#define _gloffset_VertexAttrib3sARB 480
#define _gloffset_VertexAttrib3svARB 481
#define _gloffset_VertexAttrib4NbvARB 482
#define _gloffset_VertexAttrib4NivARB 483
#define _gloffset_VertexAttrib4NsvARB 484
#define _gloffset_VertexAttrib4NubARB 485
#define _gloffset_VertexAttrib4NubvARB 486
#define _gloffset_VertexAttrib4NuivARB 487
#define _gloffset_VertexAttrib4NusvARB 488
#define _gloffset_VertexAttrib4bvARB 489
#define _gloffset_VertexAttrib4dARB 490
#define _gloffset_VertexAttrib4dvARB 491
#define _gloffset_VertexAttrib4fARB 492
#define _gloffset_VertexAttrib4fvARB 493
#define _gloffset_VertexAttrib4ivARB 494
#define _gloffset_VertexAttrib4sARB 495
#define _gloffset_VertexAttrib4svARB 496
#define _gloffset_VertexAttrib4ubvARB 497
#define _gloffset_VertexAttrib4uivARB 498
#define _gloffset_VertexAttrib4usvARB 499
#define _gloffset_VertexAttribPointerARB 500
#define _gloffset_BindBufferARB 501
#define _gloffset_BufferDataARB 502
#define _gloffset_BufferSubDataARB 503
#define _gloffset_DeleteBuffersARB 504
#define _gloffset_GenBuffersARB 505
#define _gloffset_GetBufferParameterivARB 506
#define _gloffset_GetBufferPointervARB 507
#define _gloffset_GetBufferSubDataARB 508
#define _gloffset_IsBufferARB 509
#define _gloffset_MapBufferARB 510
#define _gloffset_UnmapBufferARB 511
#define _gloffset_BeginQueryARB 512
#define _gloffset_DeleteQueriesARB 513
#define _gloffset_EndQueryARB 514
#define _gloffset_GenQueriesARB 515
#define _gloffset_GetQueryObjectivARB 516
#define _gloffset_GetQueryObjectuivARB 517
#define _gloffset_GetQueryivARB 518
#define _gloffset_IsQueryARB 519
#define _gloffset_AttachObjectARB 520
#define _gloffset_CompileShaderARB 521
#define _gloffset_CreateProgramObjectARB 522
#define _gloffset_CreateShaderObjectARB 523
#define _gloffset_DeleteObjectARB 524
#define _gloffset_DetachObjectARB 525
#define _gloffset_GetActiveUniformARB 526
#define _gloffset_GetAttachedObjectsARB 527
#define _gloffset_GetHandleARB 528
#define _gloffset_GetInfoLogARB 529
#define _gloffset_GetObjectParameterfvARB 530
#define _gloffset_GetObjectParameterivARB 531
#define _gloffset_GetShaderSourceARB 532
#define _gloffset_GetUniformLocationARB 533
#define _gloffset_GetUniformfvARB 534
#define _gloffset_GetUniformivARB 535
#define _gloffset_LinkProgramARB 536
#define _gloffset_ShaderSourceARB 537
#define _gloffset_Uniform1fARB 538
#define _gloffset_Uniform1fvARB 539
#define _gloffset_Uniform1iARB 540
#define _gloffset_Uniform1ivARB 541
#define _gloffset_Uniform2fARB 542
#define _gloffset_Uniform2fvARB 543
#define _gloffset_Uniform2iARB 544
#define _gloffset_Uniform2ivARB 545
#define _gloffset_Uniform3fARB 546
#define _gloffset_Uniform3fvARB 547
#define _gloffset_Uniform3iARB 548
#define _gloffset_Uniform3ivARB 549
#define _gloffset_Uniform4fARB 550
#define _gloffset_Uniform4fvARB 551
#define _gloffset_Uniform4iARB 552
#define _gloffset_Uniform4ivARB 553
#define _gloffset_UniformMatrix2fvARB 554
#define _gloffset_UniformMatrix3fvARB 555
#define _gloffset_UniformMatrix4fvARB 556
#define _gloffset_UseProgramObjectARB 557
#define _gloffset_ValidateProgramARB 558
#define _gloffset_BindAttribLocationARB 559
#define _gloffset_GetActiveAttribARB 560
#define _gloffset_GetAttribLocationARB 561
#define _gloffset_DrawBuffersARB 562
#define _gloffset_RenderbufferStorageMultisample 563
#define _gloffset_FramebufferTextureARB 564
#define _gloffset_FramebufferTextureFaceARB 565
#define _gloffset_ProgramParameteriARB 566
#define _gloffset_FlushMappedBufferRange 567
#define _gloffset_MapBufferRange 568
#define _gloffset_BindVertexArray 569
#define _gloffset_GenVertexArrays 570
#define _gloffset_CopyBufferSubData 571
#define _gloffset_ClientWaitSync 572
#define _gloffset_DeleteSync 573
#define _gloffset_FenceSync 574
#define _gloffset_GetInteger64v 575
#define _gloffset_GetSynciv 576
#define _gloffset_IsSync 577
#define _gloffset_WaitSync 578
#define _gloffset_DrawElementsBaseVertex 579
#define _gloffset_DrawRangeElementsBaseVertex 580
#define _gloffset_MultiDrawElementsBaseVertex 581
#define _gloffset_BindTransformFeedback 582
#define _gloffset_DeleteTransformFeedbacks 583
#define _gloffset_DrawTransformFeedback 584
#define _gloffset_GenTransformFeedbacks 585
#define _gloffset_IsTransformFeedback 586
#define _gloffset_PauseTransformFeedback 587
#define _gloffset_ResumeTransformFeedback 588
#define _gloffset_PolygonOffsetEXT 589
#define _gloffset_GetPixelTexGenParameterfvSGIS 590
#define _gloffset_GetPixelTexGenParameterivSGIS 591
#define _gloffset_PixelTexGenParameterfSGIS 592
#define _gloffset_PixelTexGenParameterfvSGIS 593
#define _gloffset_PixelTexGenParameteriSGIS 594
#define _gloffset_PixelTexGenParameterivSGIS 595
#define _gloffset_SampleMaskSGIS 596
#define _gloffset_SamplePatternSGIS 597
#define _gloffset_ColorPointerEXT 598
#define _gloffset_EdgeFlagPointerEXT 599
#define _gloffset_IndexPointerEXT 600
#define _gloffset_NormalPointerEXT 601
#define _gloffset_TexCoordPointerEXT 602
#define _gloffset_VertexPointerEXT 603
#define _gloffset_PointParameterfEXT 604
#define _gloffset_PointParameterfvEXT 605
#define _gloffset_LockArraysEXT 606
#define _gloffset_UnlockArraysEXT 607
#define _gloffset_SecondaryColor3bEXT 608
#define _gloffset_SecondaryColor3bvEXT 609
#define _gloffset_SecondaryColor3dEXT 610
#define _gloffset_SecondaryColor3dvEXT 611
#define _gloffset_SecondaryColor3fEXT 612
#define _gloffset_SecondaryColor3fvEXT 613
#define _gloffset_SecondaryColor3iEXT 614
#define _gloffset_SecondaryColor3ivEXT 615
#define _gloffset_SecondaryColor3sEXT 616
#define _gloffset_SecondaryColor3svEXT 617
#define _gloffset_SecondaryColor3ubEXT 618
#define _gloffset_SecondaryColor3ubvEXT 619
#define _gloffset_SecondaryColor3uiEXT 620
#define _gloffset_SecondaryColor3uivEXT 621
#define _gloffset_SecondaryColor3usEXT 622
#define _gloffset_SecondaryColor3usvEXT 623
#define _gloffset_SecondaryColorPointerEXT 624
#define _gloffset_MultiDrawArraysEXT 625
#define _gloffset_MultiDrawElementsEXT 626
#define _gloffset_FogCoordPointerEXT 627
#define _gloffset_FogCoorddEXT 628
#define _gloffset_FogCoorddvEXT 629
#define _gloffset_FogCoordfEXT 630
#define _gloffset_FogCoordfvEXT 631
#define _gloffset_PixelTexGenSGIX 632
#define _gloffset_BlendFuncSeparateEXT 633
#define _gloffset_FlushVertexArrayRangeNV 634
#define _gloffset_VertexArrayRangeNV 635
#define _gloffset_CombinerInputNV 636
#define _gloffset_CombinerOutputNV 637
#define _gloffset_CombinerParameterfNV 638
#define _gloffset_CombinerParameterfvNV 639
#define _gloffset_CombinerParameteriNV 640
#define _gloffset_CombinerParameterivNV 641
#define _gloffset_FinalCombinerInputNV 642
#define _gloffset_GetCombinerInputParameterfvNV 643
#define _gloffset_GetCombinerInputParameterivNV 644
#define _gloffset_GetCombinerOutputParameterfvNV 645
#define _gloffset_GetCombinerOutputParameterivNV 646
#define _gloffset_GetFinalCombinerInputParameterfvNV 647
#define _gloffset_GetFinalCombinerInputParameterivNV 648
#define _gloffset_ResizeBuffersMESA 649
#define _gloffset_WindowPos2dMESA 650
#define _gloffset_WindowPos2dvMESA 651
#define _gloffset_WindowPos2fMESA 652
#define _gloffset_WindowPos2fvMESA 653
#define _gloffset_WindowPos2iMESA 654
#define _gloffset_WindowPos2ivMESA 655
#define _gloffset_WindowPos2sMESA 656
#define _gloffset_WindowPos2svMESA 657
#define _gloffset_WindowPos3dMESA 658
#define _gloffset_WindowPos3dvMESA 659
#define _gloffset_WindowPos3fMESA 660
#define _gloffset_WindowPos3fvMESA 661
#define _gloffset_WindowPos3iMESA 662
#define _gloffset_WindowPos3ivMESA 663
#define _gloffset_WindowPos3sMESA 664
#define _gloffset_WindowPos3svMESA 665
#define _gloffset_WindowPos4dMESA 666
#define _gloffset_WindowPos4dvMESA 667
#define _gloffset_WindowPos4fMESA 668
#define _gloffset_WindowPos4fvMESA 669
#define _gloffset_WindowPos4iMESA 670
#define _gloffset_WindowPos4ivMESA 671
#define _gloffset_WindowPos4sMESA 672
#define _gloffset_WindowPos4svMESA 673
#define _gloffset_MultiModeDrawArraysIBM 674
#define _gloffset_MultiModeDrawElementsIBM 675
#define _gloffset_DeleteFencesNV 676
#define _gloffset_FinishFenceNV 677
#define _gloffset_GenFencesNV 678
#define _gloffset_GetFenceivNV 679
#define _gloffset_IsFenceNV 680
#define _gloffset_SetFenceNV 681
#define _gloffset_TestFenceNV 682
#define _gloffset_AreProgramsResidentNV 683
#define _gloffset_BindProgramNV 684
#define _gloffset_DeleteProgramsNV 685
#define _gloffset_ExecuteProgramNV 686
#define _gloffset_GenProgramsNV 687
#define _gloffset_GetProgramParameterdvNV 688
#define _gloffset_GetProgramParameterfvNV 689
#define _gloffset_GetProgramStringNV 690
#define _gloffset_GetProgramivNV 691
#define _gloffset_GetTrackMatrixivNV 692
#define _gloffset_GetVertexAttribPointervNV 693
#define _gloffset_GetVertexAttribdvNV 694
#define _gloffset_GetVertexAttribfvNV 695
#define _gloffset_GetVertexAttribivNV 696
#define _gloffset_IsProgramNV 697
#define _gloffset_LoadProgramNV 698
#define _gloffset_ProgramParameters4dvNV 699
#define _gloffset_ProgramParameters4fvNV 700
#define _gloffset_RequestResidentProgramsNV 701
#define _gloffset_TrackMatrixNV 702
#define _gloffset_VertexAttrib1dNV 703
#define _gloffset_VertexAttrib1dvNV 704
#define _gloffset_VertexAttrib1fNV 705
#define _gloffset_VertexAttrib1fvNV 706
#define _gloffset_VertexAttrib1sNV 707
#define _gloffset_VertexAttrib1svNV 708
#define _gloffset_VertexAttrib2dNV 709
#define _gloffset_VertexAttrib2dvNV 710
#define _gloffset_VertexAttrib2fNV 711
#define _gloffset_VertexAttrib2fvNV 712
#define _gloffset_VertexAttrib2sNV 713
#define _gloffset_VertexAttrib2svNV 714
#define _gloffset_VertexAttrib3dNV 715
#define _gloffset_VertexAttrib3dvNV 716
#define _gloffset_VertexAttrib3fNV 717
#define _gloffset_VertexAttrib3fvNV 718
#define _gloffset_VertexAttrib3sNV 719
#define _gloffset_VertexAttrib3svNV 720
#define _gloffset_VertexAttrib4dNV 721
#define _gloffset_VertexAttrib4dvNV 722
#define _gloffset_VertexAttrib4fNV 723
#define _gloffset_VertexAttrib4fvNV 724
#define _gloffset_VertexAttrib4sNV 725
#define _gloffset_VertexAttrib4svNV 726
#define _gloffset_VertexAttrib4ubNV 727
#define _gloffset_VertexAttrib4ubvNV 728
#define _gloffset_VertexAttribPointerNV 729
#define _gloffset_VertexAttribs1dvNV 730
#define _gloffset_VertexAttribs1fvNV 731
#define _gloffset_VertexAttribs1svNV 732
#define _gloffset_VertexAttribs2dvNV 733
#define _gloffset_VertexAttribs2fvNV 734
#define _gloffset_VertexAttribs2svNV 735
#define _gloffset_VertexAttribs3dvNV 736
#define _gloffset_VertexAttribs3fvNV 737
#define _gloffset_VertexAttribs3svNV 738
#define _gloffset_VertexAttribs4dvNV 739
#define _gloffset_VertexAttribs4fvNV 740
#define _gloffset_VertexAttribs4svNV 741
#define _gloffset_VertexAttribs4ubvNV 742
#define _gloffset_GetTexBumpParameterfvATI 743
#define _gloffset_GetTexBumpParameterivATI 744
#define _gloffset_TexBumpParameterfvATI 745
#define _gloffset_TexBumpParameterivATI 746
#define _gloffset_AlphaFragmentOp1ATI 747
#define _gloffset_AlphaFragmentOp2ATI 748
#define _gloffset_AlphaFragmentOp3ATI 749
#define _gloffset_BeginFragmentShaderATI 750
#define _gloffset_BindFragmentShaderATI 751
#define _gloffset_ColorFragmentOp1ATI 752
#define _gloffset_ColorFragmentOp2ATI 753
#define _gloffset_ColorFragmentOp3ATI 754
#define _gloffset_DeleteFragmentShaderATI 755
#define _gloffset_EndFragmentShaderATI 756
#define _gloffset_GenFragmentShadersATI 757
#define _gloffset_PassTexCoordATI 758
#define _gloffset_SampleMapATI 759
#define _gloffset_SetFragmentShaderConstantATI 760
#define _gloffset_PointParameteriNV 761
#define _gloffset_PointParameterivNV 762
#define _gloffset_ActiveStencilFaceEXT 763
#define _gloffset_BindVertexArrayAPPLE 764
#define _gloffset_DeleteVertexArraysAPPLE 765
#define _gloffset_GenVertexArraysAPPLE 766
#define _gloffset_IsVertexArrayAPPLE 767
#define _gloffset_GetProgramNamedParameterdvNV 768
#define _gloffset_GetProgramNamedParameterfvNV 769
#define _gloffset_ProgramNamedParameter4dNV 770
#define _gloffset_ProgramNamedParameter4dvNV 771
#define _gloffset_ProgramNamedParameter4fNV 772
#define _gloffset_ProgramNamedParameter4fvNV 773
#define _gloffset_PrimitiveRestartIndexNV 774
#define _gloffset_PrimitiveRestartNV 775
#define _gloffset_DepthBoundsEXT 776
#define _gloffset_BlendEquationSeparateEXT 777
#define _gloffset_BindFramebufferEXT 778
#define _gloffset_BindRenderbufferEXT 779
#define _gloffset_CheckFramebufferStatusEXT 780
#define _gloffset_DeleteFramebuffersEXT 781
#define _gloffset_DeleteRenderbuffersEXT 782
#define _gloffset_FramebufferRenderbufferEXT 783
#define _gloffset_FramebufferTexture1DEXT 784
#define _gloffset_FramebufferTexture2DEXT 785
#define _gloffset_FramebufferTexture3DEXT 786
#define _gloffset_GenFramebuffersEXT 787
#define _gloffset_GenRenderbuffersEXT 788
#define _gloffset_GenerateMipmapEXT 789
#define _gloffset_GetFramebufferAttachmentParameterivEXT 790
#define _gloffset_GetRenderbufferParameterivEXT 791
#define _gloffset_IsFramebufferEXT 792
#define _gloffset_IsRenderbufferEXT 793
#define _gloffset_RenderbufferStorageEXT 794
#define _gloffset_BlitFramebufferEXT 795
#define _gloffset_BufferParameteriAPPLE 796
#define _gloffset_FlushMappedBufferRangeAPPLE 797
#define _gloffset_BindFragDataLocationEXT 798
#define _gloffset_GetFragDataLocationEXT 799
#define _gloffset_GetUniformuivEXT 800
#define _gloffset_GetVertexAttribIivEXT 801
#define _gloffset_GetVertexAttribIuivEXT 802
#define _gloffset_Uniform1uiEXT 803
#define _gloffset_Uniform1uivEXT 804
#define _gloffset_Uniform2uiEXT 805
#define _gloffset_Uniform2uivEXT 806
#define _gloffset_Uniform3uiEXT 807
#define _gloffset_Uniform3uivEXT 808
#define _gloffset_Uniform4uiEXT 809
#define _gloffset_Uniform4uivEXT 810
#define _gloffset_VertexAttribI1iEXT 811
#define _gloffset_VertexAttribI1ivEXT 812
#define _gloffset_VertexAttribI1uiEXT 813
#define _gloffset_VertexAttribI1uivEXT 814
#define _gloffset_VertexAttribI2iEXT 815
#define _gloffset_VertexAttribI2ivEXT 816
#define _gloffset_VertexAttribI2uiEXT 817
#define _gloffset_VertexAttribI2uivEXT 818
#define _gloffset_VertexAttribI3iEXT 819
#define _gloffset_VertexAttribI3ivEXT 820
#define _gloffset_VertexAttribI3uiEXT 821
#define _gloffset_VertexAttribI3uivEXT 822
#define _gloffset_VertexAttribI4bvEXT 823
#define _gloffset_VertexAttribI4iEXT 824
#define _gloffset_VertexAttribI4ivEXT 825
#define _gloffset_VertexAttribI4svEXT 826
#define _gloffset_VertexAttribI4ubvEXT 827
#define _gloffset_VertexAttribI4uiEXT 828
#define _gloffset_VertexAttribI4uivEXT 829
#define _gloffset_VertexAttribI4usvEXT 830
#define _gloffset_VertexAttribIPointerEXT 831
#define _gloffset_FramebufferTextureLayerEXT 832
#define _gloffset_ColorMaskIndexedEXT 833
#define _gloffset_DisableIndexedEXT 834
#define _gloffset_EnableIndexedEXT 835
#define _gloffset_GetBooleanIndexedvEXT 836
#define _gloffset_GetIntegerIndexedvEXT 837
#define _gloffset_IsEnabledIndexedEXT 838
#define _gloffset_ClearColorIiEXT 839
#define _gloffset_ClearColorIuiEXT 840
#define _gloffset_GetTexParameterIivEXT 841
#define _gloffset_GetTexParameterIuivEXT 842
#define _gloffset_TexParameterIivEXT 843
#define _gloffset_TexParameterIuivEXT 844
#define _gloffset_BeginConditionalRenderNV 845
#define _gloffset_EndConditionalRenderNV 846
#define _gloffset_BeginTransformFeedbackEXT 847
#define _gloffset_BindBufferBaseEXT 848
#define _gloffset_BindBufferOffsetEXT 849
#define _gloffset_BindBufferRangeEXT 850
#define _gloffset_EndTransformFeedbackEXT 851
#define _gloffset_GetTransformFeedbackVaryingEXT 852
#define _gloffset_TransformFeedbackVaryingsEXT 853
#define _gloffset_ProvokingVertexEXT 854
#define _gloffset_GetTexParameterPointervAPPLE 855
#define _gloffset_TextureRangeAPPLE 856
#define _gloffset_GetObjectParameterivAPPLE 857
#define _gloffset_ObjectPurgeableAPPLE 858
#define _gloffset_ObjectUnpurgeableAPPLE 859
#define _gloffset_ActiveProgramEXT 860
#define _gloffset_CreateShaderProgramEXT 861
#define _gloffset_UseShaderProgramEXT 862
#define _gloffset_StencilFuncSeparateATI 863
#define _gloffset_ProgramEnvParameters4fvEXT 864
#define _gloffset_ProgramLocalParameters4fvEXT 865
#define _gloffset_GetQueryObjecti64vEXT 866
#define _gloffset_GetQueryObjectui64vEXT 867
#define _gloffset_EGLImageTargetRenderbufferStorageOES 868
#define _gloffset_EGLImageTargetTexture2DOES 869

#else /* !_GLAPI_USE_REMAP_TABLE */

#define driDispatchRemapTable_size 462
extern int driDispatchRemapTable[ driDispatchRemapTable_size ];

#define AttachShader_remap_index 0
#define CreateProgram_remap_index 1
#define CreateShader_remap_index 2
#define DeleteProgram_remap_index 3
#define DeleteShader_remap_index 4
#define DetachShader_remap_index 5
#define GetAttachedShaders_remap_index 6
#define GetProgramInfoLog_remap_index 7
#define GetProgramiv_remap_index 8
#define GetShaderInfoLog_remap_index 9
#define GetShaderiv_remap_index 10
#define IsProgram_remap_index 11
#define IsShader_remap_index 12
#define StencilFuncSeparate_remap_index 13
#define StencilMaskSeparate_remap_index 14
#define StencilOpSeparate_remap_index 15
#define UniformMatrix2x3fv_remap_index 16
#define UniformMatrix2x4fv_remap_index 17
#define UniformMatrix3x2fv_remap_index 18
#define UniformMatrix3x4fv_remap_index 19
#define UniformMatrix4x2fv_remap_index 20
#define UniformMatrix4x3fv_remap_index 21
#define DrawArraysInstanced_remap_index 22
#define DrawElementsInstanced_remap_index 23
#define LoadTransposeMatrixdARB_remap_index 24
#define LoadTransposeMatrixfARB_remap_index 25
#define MultTransposeMatrixdARB_remap_index 26
#define MultTransposeMatrixfARB_remap_index 27
#define SampleCoverageARB_remap_index 28
#define CompressedTexImage1DARB_remap_index 29
#define CompressedTexImage2DARB_remap_index 30
#define CompressedTexImage3DARB_remap_index 31
#define CompressedTexSubImage1DARB_remap_index 32
#define CompressedTexSubImage2DARB_remap_index 33
#define CompressedTexSubImage3DARB_remap_index 34
#define GetCompressedTexImageARB_remap_index 35
#define DisableVertexAttribArrayARB_remap_index 36
#define EnableVertexAttribArrayARB_remap_index 37
#define GetProgramEnvParameterdvARB_remap_index 38
#define GetProgramEnvParameterfvARB_remap_index 39
#define GetProgramLocalParameterdvARB_remap_index 40
#define GetProgramLocalParameterfvARB_remap_index 41
#define GetProgramStringARB_remap_index 42
#define GetProgramivARB_remap_index 43
#define GetVertexAttribdvARB_remap_index 44
#define GetVertexAttribfvARB_remap_index 45
#define GetVertexAttribivARB_remap_index 46
#define ProgramEnvParameter4dARB_remap_index 47
#define ProgramEnvParameter4dvARB_remap_index 48
#define ProgramEnvParameter4fARB_remap_index 49
#define ProgramEnvParameter4fvARB_remap_index 50
#define ProgramLocalParameter4dARB_remap_index 51
#define ProgramLocalParameter4dvARB_remap_index 52
#define ProgramLocalParameter4fARB_remap_index 53
#define ProgramLocalParameter4fvARB_remap_index 54
#define ProgramStringARB_remap_index 55
#define VertexAttrib1dARB_remap_index 56
#define VertexAttrib1dvARB_remap_index 57
#define VertexAttrib1fARB_remap_index 58
#define VertexAttrib1fvARB_remap_index 59
#define VertexAttrib1sARB_remap_index 60
#define VertexAttrib1svARB_remap_index 61
#define VertexAttrib2dARB_remap_index 62
#define VertexAttrib2dvARB_remap_index 63
#define VertexAttrib2fARB_remap_index 64
#define VertexAttrib2fvARB_remap_index 65
#define VertexAttrib2sARB_remap_index 66
#define VertexAttrib2svARB_remap_index 67
#define VertexAttrib3dARB_remap_index 68
#define VertexAttrib3dvARB_remap_index 69
#define VertexAttrib3fARB_remap_index 70
#define VertexAttrib3fvARB_remap_index 71
#define VertexAttrib3sARB_remap_index 72
#define VertexAttrib3svARB_remap_index 73
#define VertexAttrib4NbvARB_remap_index 74
#define VertexAttrib4NivARB_remap_index 75
#define VertexAttrib4NsvARB_remap_index 76
#define VertexAttrib4NubARB_remap_index 77
#define VertexAttrib4NubvARB_remap_index 78
#define VertexAttrib4NuivARB_remap_index 79
#define VertexAttrib4NusvARB_remap_index 80
#define VertexAttrib4bvARB_remap_index 81
#define VertexAttrib4dARB_remap_index 82
#define VertexAttrib4dvARB_remap_index 83
#define VertexAttrib4fARB_remap_index 84
#define VertexAttrib4fvARB_remap_index 85
#define VertexAttrib4ivARB_remap_index 86
#define VertexAttrib4sARB_remap_index 87
#define VertexAttrib4svARB_remap_index 88
#define VertexAttrib4ubvARB_remap_index 89
#define VertexAttrib4uivARB_remap_index 90
#define VertexAttrib4usvARB_remap_index 91
#define VertexAttribPointerARB_remap_index 92
#define BindBufferARB_remap_index 93
#define BufferDataARB_remap_index 94
#define BufferSubDataARB_remap_index 95
#define DeleteBuffersARB_remap_index 96
#define GenBuffersARB_remap_index 97
#define GetBufferParameterivARB_remap_index 98
#define GetBufferPointervARB_remap_index 99
#define GetBufferSubDataARB_remap_index 100
#define IsBufferARB_remap_index 101
#define MapBufferARB_remap_index 102
#define UnmapBufferARB_remap_index 103
#define BeginQueryARB_remap_index 104
#define DeleteQueriesARB_remap_index 105
#define EndQueryARB_remap_index 106
#define GenQueriesARB_remap_index 107
#define GetQueryObjectivARB_remap_index 108
#define GetQueryObjectuivARB_remap_index 109
#define GetQueryivARB_remap_index 110
#define IsQueryARB_remap_index 111
#define AttachObjectARB_remap_index 112
#define CompileShaderARB_remap_index 113
#define CreateProgramObjectARB_remap_index 114
#define CreateShaderObjectARB_remap_index 115
#define DeleteObjectARB_remap_index 116
#define DetachObjectARB_remap_index 117
#define GetActiveUniformARB_remap_index 118
#define GetAttachedObjectsARB_remap_index 119
#define GetHandleARB_remap_index 120
#define GetInfoLogARB_remap_index 121
#define GetObjectParameterfvARB_remap_index 122
#define GetObjectParameterivARB_remap_index 123
#define GetShaderSourceARB_remap_index 124
#define GetUniformLocationARB_remap_index 125
#define GetUniformfvARB_remap_index 126
#define GetUniformivARB_remap_index 127
#define LinkProgramARB_remap_index 128
#define ShaderSourceARB_remap_index 129
#define Uniform1fARB_remap_index 130
#define Uniform1fvARB_remap_index 131
#define Uniform1iARB_remap_index 132
#define Uniform1ivARB_remap_index 133
#define Uniform2fARB_remap_index 134
#define Uniform2fvARB_remap_index 135
#define Uniform2iARB_remap_index 136
#define Uniform2ivARB_remap_index 137
#define Uniform3fARB_remap_index 138
#define Uniform3fvARB_remap_index 139
#define Uniform3iARB_remap_index 140
#define Uniform3ivARB_remap_index 141
#define Uniform4fARB_remap_index 142
#define Uniform4fvARB_remap_index 143
#define Uniform4iARB_remap_index 144
#define Uniform4ivARB_remap_index 145
#define UniformMatrix2fvARB_remap_index 146
#define UniformMatrix3fvARB_remap_index 147
#define UniformMatrix4fvARB_remap_index 148
#define UseProgramObjectARB_remap_index 149
#define ValidateProgramARB_remap_index 150
#define BindAttribLocationARB_remap_index 151
#define GetActiveAttribARB_remap_index 152
#define GetAttribLocationARB_remap_index 153
#define DrawBuffersARB_remap_index 154
#define RenderbufferStorageMultisample_remap_index 155
#define FramebufferTextureARB_remap_index 156
#define FramebufferTextureFaceARB_remap_index 157
#define ProgramParameteriARB_remap_index 158
#define FlushMappedBufferRange_remap_index 159
#define MapBufferRange_remap_index 160
#define BindVertexArray_remap_index 161
#define GenVertexArrays_remap_index 162
#define CopyBufferSubData_remap_index 163
#define ClientWaitSync_remap_index 164
#define DeleteSync_remap_index 165
#define FenceSync_remap_index 166
#define GetInteger64v_remap_index 167
#define GetSynciv_remap_index 168
#define IsSync_remap_index 169
#define WaitSync_remap_index 170
#define DrawElementsBaseVertex_remap_index 171
#define DrawRangeElementsBaseVertex_remap_index 172
#define MultiDrawElementsBaseVertex_remap_index 173
#define BindTransformFeedback_remap_index 174
#define DeleteTransformFeedbacks_remap_index 175
#define DrawTransformFeedback_remap_index 176
#define GenTransformFeedbacks_remap_index 177
#define IsTransformFeedback_remap_index 178
#define PauseTransformFeedback_remap_index 179
#define ResumeTransformFeedback_remap_index 180
#define PolygonOffsetEXT_remap_index 181
#define GetPixelTexGenParameterfvSGIS_remap_index 182
#define GetPixelTexGenParameterivSGIS_remap_index 183
#define PixelTexGenParameterfSGIS_remap_index 184
#define PixelTexGenParameterfvSGIS_remap_index 185
#define PixelTexGenParameteriSGIS_remap_index 186
#define PixelTexGenParameterivSGIS_remap_index 187
#define SampleMaskSGIS_remap_index 188
#define SamplePatternSGIS_remap_index 189
#define ColorPointerEXT_remap_index 190
#define EdgeFlagPointerEXT_remap_index 191
#define IndexPointerEXT_remap_index 192
#define NormalPointerEXT_remap_index 193
#define TexCoordPointerEXT_remap_index 194
#define VertexPointerEXT_remap_index 195
#define PointParameterfEXT_remap_index 196
#define PointParameterfvEXT_remap_index 197
#define LockArraysEXT_remap_index 198
#define UnlockArraysEXT_remap_index 199
#define SecondaryColor3bEXT_remap_index 200
#define SecondaryColor3bvEXT_remap_index 201
#define SecondaryColor3dEXT_remap_index 202
#define SecondaryColor3dvEXT_remap_index 203
#define SecondaryColor3fEXT_remap_index 204
#define SecondaryColor3fvEXT_remap_index 205
#define SecondaryColor3iEXT_remap_index 206
#define SecondaryColor3ivEXT_remap_index 207
#define SecondaryColor3sEXT_remap_index 208
#define SecondaryColor3svEXT_remap_index 209
#define SecondaryColor3ubEXT_remap_index 210
#define SecondaryColor3ubvEXT_remap_index 211
#define SecondaryColor3uiEXT_remap_index 212
#define SecondaryColor3uivEXT_remap_index 213
#define SecondaryColor3usEXT_remap_index 214
#define SecondaryColor3usvEXT_remap_index 215
#define SecondaryColorPointerEXT_remap_index 216
#define MultiDrawArraysEXT_remap_index 217
#define MultiDrawElementsEXT_remap_index 218
#define FogCoordPointerEXT_remap_index 219
#define FogCoorddEXT_remap_index 220
#define FogCoorddvEXT_remap_index 221
#define FogCoordfEXT_remap_index 222
#define FogCoordfvEXT_remap_index 223
#define PixelTexGenSGIX_remap_index 224
#define BlendFuncSeparateEXT_remap_index 225
#define FlushVertexArrayRangeNV_remap_index 226
#define VertexArrayRangeNV_remap_index 227
#define CombinerInputNV_remap_index 228
#define CombinerOutputNV_remap_index 229
#define CombinerParameterfNV_remap_index 230
#define CombinerParameterfvNV_remap_index 231
#define CombinerParameteriNV_remap_index 232
#define CombinerParameterivNV_remap_index 233
#define FinalCombinerInputNV_remap_index 234
#define GetCombinerInputParameterfvNV_remap_index 235
#define GetCombinerInputParameterivNV_remap_index 236
#define GetCombinerOutputParameterfvNV_remap_index 237
#define GetCombinerOutputParameterivNV_remap_index 238
#define GetFinalCombinerInputParameterfvNV_remap_index 239
#define GetFinalCombinerInputParameterivNV_remap_index 240
#define ResizeBuffersMESA_remap_index 241
#define WindowPos2dMESA_remap_index 242
#define WindowPos2dvMESA_remap_index 243
#define WindowPos2fMESA_remap_index 244
#define WindowPos2fvMESA_remap_index 245
#define WindowPos2iMESA_remap_index 246
#define WindowPos2ivMESA_remap_index 247
#define WindowPos2sMESA_remap_index 248
#define WindowPos2svMESA_remap_index 249
#define WindowPos3dMESA_remap_index 250
#define WindowPos3dvMESA_remap_index 251
#define WindowPos3fMESA_remap_index 252
#define WindowPos3fvMESA_remap_index 253
#define WindowPos3iMESA_remap_index 254
#define WindowPos3ivMESA_remap_index 255
#define WindowPos3sMESA_remap_index 256
#define WindowPos3svMESA_remap_index 257
#define WindowPos4dMESA_remap_index 258
#define WindowPos4dvMESA_remap_index 259
#define WindowPos4fMESA_remap_index 260
#define WindowPos4fvMESA_remap_index 261
#define WindowPos4iMESA_remap_index 262
#define WindowPos4ivMESA_remap_index 263
#define WindowPos4sMESA_remap_index 264
#define WindowPos4svMESA_remap_index 265
#define MultiModeDrawArraysIBM_remap_index 266
#define MultiModeDrawElementsIBM_remap_index 267
#define DeleteFencesNV_remap_index 268
#define FinishFenceNV_remap_index 269
#define GenFencesNV_remap_index 270
#define GetFenceivNV_remap_index 271
#define IsFenceNV_remap_index 272
#define SetFenceNV_remap_index 273
#define TestFenceNV_remap_index 274
#define AreProgramsResidentNV_remap_index 275
#define BindProgramNV_remap_index 276
#define DeleteProgramsNV_remap_index 277
#define ExecuteProgramNV_remap_index 278
#define GenProgramsNV_remap_index 279
#define GetProgramParameterdvNV_remap_index 280
#define GetProgramParameterfvNV_remap_index 281
#define GetProgramStringNV_remap_index 282
#define GetProgramivNV_remap_index 283
#define GetTrackMatrixivNV_remap_index 284
#define GetVertexAttribPointervNV_remap_index 285
#define GetVertexAttribdvNV_remap_index 286
#define GetVertexAttribfvNV_remap_index 287
#define GetVertexAttribivNV_remap_index 288
#define IsProgramNV_remap_index 289
#define LoadProgramNV_remap_index 290
#define ProgramParameters4dvNV_remap_index 291
#define ProgramParameters4fvNV_remap_index 292
#define RequestResidentProgramsNV_remap_index 293
#define TrackMatrixNV_remap_index 294
#define VertexAttrib1dNV_remap_index 295
#define VertexAttrib1dvNV_remap_index 296
#define VertexAttrib1fNV_remap_index 297
#define VertexAttrib1fvNV_remap_index 298
#define VertexAttrib1sNV_remap_index 299
#define VertexAttrib1svNV_remap_index 300
#define VertexAttrib2dNV_remap_index 301
#define VertexAttrib2dvNV_remap_index 302
#define VertexAttrib2fNV_remap_index 303
#define VertexAttrib2fvNV_remap_index 304
#define VertexAttrib2sNV_remap_index 305
#define VertexAttrib2svNV_remap_index 306
#define VertexAttrib3dNV_remap_index 307
#define VertexAttrib3dvNV_remap_index 308
#define VertexAttrib3fNV_remap_index 309
#define VertexAttrib3fvNV_remap_index 310
#define VertexAttrib3sNV_remap_index 311
#define VertexAttrib3svNV_remap_index 312
#define VertexAttrib4dNV_remap_index 313
#define VertexAttrib4dvNV_remap_index 314
#define VertexAttrib4fNV_remap_index 315
#define VertexAttrib4fvNV_remap_index 316
#define VertexAttrib4sNV_remap_index 317
#define VertexAttrib4svNV_remap_index 318
#define VertexAttrib4ubNV_remap_index 319
#define VertexAttrib4ubvNV_remap_index 320
#define VertexAttribPointerNV_remap_index 321
#define VertexAttribs1dvNV_remap_index 322
#define VertexAttribs1fvNV_remap_index 323
#define VertexAttribs1svNV_remap_index 324
#define VertexAttribs2dvNV_remap_index 325
#define VertexAttribs2fvNV_remap_index 326
#define VertexAttribs2svNV_remap_index 327
#define VertexAttribs3dvNV_remap_index 328
#define VertexAttribs3fvNV_remap_index 329
#define VertexAttribs3svNV_remap_index 330
#define VertexAttribs4dvNV_remap_index 331
#define VertexAttribs4fvNV_remap_index 332
#define VertexAttribs4svNV_remap_index 333
#define VertexAttribs4ubvNV_remap_index 334
#define GetTexBumpParameterfvATI_remap_index 335
#define GetTexBumpParameterivATI_remap_index 336
#define TexBumpParameterfvATI_remap_index 337
#define TexBumpParameterivATI_remap_index 338
#define AlphaFragmentOp1ATI_remap_index 339
#define AlphaFragmentOp2ATI_remap_index 340
#define AlphaFragmentOp3ATI_remap_index 341
#define BeginFragmentShaderATI_remap_index 342
#define BindFragmentShaderATI_remap_index 343
#define ColorFragmentOp1ATI_remap_index 344
#define ColorFragmentOp2ATI_remap_index 345
#define ColorFragmentOp3ATI_remap_index 346
#define DeleteFragmentShaderATI_remap_index 347
#define EndFragmentShaderATI_remap_index 348
#define GenFragmentShadersATI_remap_index 349
#define PassTexCoordATI_remap_index 350
#define SampleMapATI_remap_index 351
#define SetFragmentShaderConstantATI_remap_index 352
#define PointParameteriNV_remap_index 353
#define PointParameterivNV_remap_index 354
#define ActiveStencilFaceEXT_remap_index 355
#define BindVertexArrayAPPLE_remap_index 356
#define DeleteVertexArraysAPPLE_remap_index 357
#define GenVertexArraysAPPLE_remap_index 358
#define IsVertexArrayAPPLE_remap_index 359
#define GetProgramNamedParameterdvNV_remap_index 360
#define GetProgramNamedParameterfvNV_remap_index 361
#define ProgramNamedParameter4dNV_remap_index 362
#define ProgramNamedParameter4dvNV_remap_index 363
#define ProgramNamedParameter4fNV_remap_index 364
#define ProgramNamedParameter4fvNV_remap_index 365
#define PrimitiveRestartIndexNV_remap_index 366
#define PrimitiveRestartNV_remap_index 367
#define DepthBoundsEXT_remap_index 368
#define BlendEquationSeparateEXT_remap_index 369
#define BindFramebufferEXT_remap_index 370
#define BindRenderbufferEXT_remap_index 371
#define CheckFramebufferStatusEXT_remap_index 372
#define DeleteFramebuffersEXT_remap_index 373
#define DeleteRenderbuffersEXT_remap_index 374
#define FramebufferRenderbufferEXT_remap_index 375
#define FramebufferTexture1DEXT_remap_index 376
#define FramebufferTexture2DEXT_remap_index 377
#define FramebufferTexture3DEXT_remap_index 378
#define GenFramebuffersEXT_remap_index 379
#define GenRenderbuffersEXT_remap_index 380
#define GenerateMipmapEXT_remap_index 381
#define GetFramebufferAttachmentParameterivEXT_remap_index 382
#define GetRenderbufferParameterivEXT_remap_index 383
#define IsFramebufferEXT_remap_index 384
#define IsRenderbufferEXT_remap_index 385
#define RenderbufferStorageEXT_remap_index 386
#define BlitFramebufferEXT_remap_index 387
#define BufferParameteriAPPLE_remap_index 388
#define FlushMappedBufferRangeAPPLE_remap_index 389
#define BindFragDataLocationEXT_remap_index 390
#define GetFragDataLocationEXT_remap_index 391
#define GetUniformuivEXT_remap_index 392
#define GetVertexAttribIivEXT_remap_index 393
#define GetVertexAttribIuivEXT_remap_index 394
#define Uniform1uiEXT_remap_index 395
#define Uniform1uivEXT_remap_index 396
#define Uniform2uiEXT_remap_index 397
#define Uniform2uivEXT_remap_index 398
#define Uniform3uiEXT_remap_index 399
#define Uniform3uivEXT_remap_index 400
#define Uniform4uiEXT_remap_index 401
#define Uniform4uivEXT_remap_index 402
#define VertexAttribI1iEXT_remap_index 403
#define VertexAttribI1ivEXT_remap_index 404
#define VertexAttribI1uiEXT_remap_index 405
#define VertexAttribI1uivEXT_remap_index 406
#define VertexAttribI2iEXT_remap_index 407
#define VertexAttribI2ivEXT_remap_index 408
#define VertexAttribI2uiEXT_remap_index 409
#define VertexAttribI2uivEXT_remap_index 410
#define VertexAttribI3iEXT_remap_index 411
#define VertexAttribI3ivEXT_remap_index 412
#define VertexAttribI3uiEXT_remap_index 413
#define VertexAttribI3uivEXT_remap_index 414
#define VertexAttribI4bvEXT_remap_index 415
#define VertexAttribI4iEXT_remap_index 416
#define VertexAttribI4ivEXT_remap_index 417
#define VertexAttribI4svEXT_remap_index 418
#define VertexAttribI4ubvEXT_remap_index 419
#define VertexAttribI4uiEXT_remap_index 420
#define VertexAttribI4uivEXT_remap_index 421
#define VertexAttribI4usvEXT_remap_index 422
#define VertexAttribIPointerEXT_remap_index 423
#define FramebufferTextureLayerEXT_remap_index 424
#define ColorMaskIndexedEXT_remap_index 425
#define DisableIndexedEXT_remap_index 426
#define EnableIndexedEXT_remap_index 427
#define GetBooleanIndexedvEXT_remap_index 428
#define GetIntegerIndexedvEXT_remap_index 429
#define IsEnabledIndexedEXT_remap_index 430
#define ClearColorIiEXT_remap_index 431
#define ClearColorIuiEXT_remap_index 432
#define GetTexParameterIivEXT_remap_index 433
#define GetTexParameterIuivEXT_remap_index 434
#define TexParameterIivEXT_remap_index 435
#define TexParameterIuivEXT_remap_index 436
#define BeginConditionalRenderNV_remap_index 437
#define EndConditionalRenderNV_remap_index 438
#define BeginTransformFeedbackEXT_remap_index 439
#define BindBufferBaseEXT_remap_index 440
#define BindBufferOffsetEXT_remap_index 441
#define BindBufferRangeEXT_remap_index 442
#define EndTransformFeedbackEXT_remap_index 443
#define GetTransformFeedbackVaryingEXT_remap_index 444
#define TransformFeedbackVaryingsEXT_remap_index 445
#define ProvokingVertexEXT_remap_index 446
#define GetTexParameterPointervAPPLE_remap_index 447
#define TextureRangeAPPLE_remap_index 448
#define GetObjectParameterivAPPLE_remap_index 449
#define ObjectPurgeableAPPLE_remap_index 450
#define ObjectUnpurgeableAPPLE_remap_index 451
#define ActiveProgramEXT_remap_index 452
#define CreateShaderProgramEXT_remap_index 453
#define UseShaderProgramEXT_remap_index 454
#define StencilFuncSeparateATI_remap_index 455
#define ProgramEnvParameters4fvEXT_remap_index 456
#define ProgramLocalParameters4fvEXT_remap_index 457
#define GetQueryObjecti64vEXT_remap_index 458
#define GetQueryObjectui64vEXT_remap_index 459
#define EGLImageTargetRenderbufferStorageOES_remap_index 460
#define EGLImageTargetTexture2DOES_remap_index 461

#define _gloffset_AttachShader driDispatchRemapTable[AttachShader_remap_index]
#define _gloffset_CreateProgram driDispatchRemapTable[CreateProgram_remap_index]
#define _gloffset_CreateShader driDispatchRemapTable[CreateShader_remap_index]
#define _gloffset_DeleteProgram driDispatchRemapTable[DeleteProgram_remap_index]
#define _gloffset_DeleteShader driDispatchRemapTable[DeleteShader_remap_index]
#define _gloffset_DetachShader driDispatchRemapTable[DetachShader_remap_index]
#define _gloffset_GetAttachedShaders driDispatchRemapTable[GetAttachedShaders_remap_index]
#define _gloffset_GetProgramInfoLog driDispatchRemapTable[GetProgramInfoLog_remap_index]
#define _gloffset_GetProgramiv driDispatchRemapTable[GetProgramiv_remap_index]
#define _gloffset_GetShaderInfoLog driDispatchRemapTable[GetShaderInfoLog_remap_index]
#define _gloffset_GetShaderiv driDispatchRemapTable[GetShaderiv_remap_index]
#define _gloffset_IsProgram driDispatchRemapTable[IsProgram_remap_index]
#define _gloffset_IsShader driDispatchRemapTable[IsShader_remap_index]
#define _gloffset_StencilFuncSeparate driDispatchRemapTable[StencilFuncSeparate_remap_index]
#define _gloffset_StencilMaskSeparate driDispatchRemapTable[StencilMaskSeparate_remap_index]
#define _gloffset_StencilOpSeparate driDispatchRemapTable[StencilOpSeparate_remap_index]
#define _gloffset_UniformMatrix2x3fv driDispatchRemapTable[UniformMatrix2x3fv_remap_index]
#define _gloffset_UniformMatrix2x4fv driDispatchRemapTable[UniformMatrix2x4fv_remap_index]
#define _gloffset_UniformMatrix3x2fv driDispatchRemapTable[UniformMatrix3x2fv_remap_index]
#define _gloffset_UniformMatrix3x4fv driDispatchRemapTable[UniformMatrix3x4fv_remap_index]
#define _gloffset_UniformMatrix4x2fv driDispatchRemapTable[UniformMatrix4x2fv_remap_index]
#define _gloffset_UniformMatrix4x3fv driDispatchRemapTable[UniformMatrix4x3fv_remap_index]
#define _gloffset_DrawArraysInstanced driDispatchRemapTable[DrawArraysInstanced_remap_index]
#define _gloffset_DrawElementsInstanced driDispatchRemapTable[DrawElementsInstanced_remap_index]
#define _gloffset_LoadTransposeMatrixdARB driDispatchRemapTable[LoadTransposeMatrixdARB_remap_index]
#define _gloffset_LoadTransposeMatrixfARB driDispatchRemapTable[LoadTransposeMatrixfARB_remap_index]
#define _gloffset_MultTransposeMatrixdARB driDispatchRemapTable[MultTransposeMatrixdARB_remap_index]
#define _gloffset_MultTransposeMatrixfARB driDispatchRemapTable[MultTransposeMatrixfARB_remap_index]
#define _gloffset_SampleCoverageARB driDispatchRemapTable[SampleCoverageARB_remap_index]
#define _gloffset_CompressedTexImage1DARB driDispatchRemapTable[CompressedTexImage1DARB_remap_index]
#define _gloffset_CompressedTexImage2DARB driDispatchRemapTable[CompressedTexImage2DARB_remap_index]
#define _gloffset_CompressedTexImage3DARB driDispatchRemapTable[CompressedTexImage3DARB_remap_index]
#define _gloffset_CompressedTexSubImage1DARB driDispatchRemapTable[CompressedTexSubImage1DARB_remap_index]
#define _gloffset_CompressedTexSubImage2DARB driDispatchRemapTable[CompressedTexSubImage2DARB_remap_index]
#define _gloffset_CompressedTexSubImage3DARB driDispatchRemapTable[CompressedTexSubImage3DARB_remap_index]
#define _gloffset_GetCompressedTexImageARB driDispatchRemapTable[GetCompressedTexImageARB_remap_index]
#define _gloffset_DisableVertexAttribArrayARB driDispatchRemapTable[DisableVertexAttribArrayARB_remap_index]
#define _gloffset_EnableVertexAttribArrayARB driDispatchRemapTable[EnableVertexAttribArrayARB_remap_index]
#define _gloffset_GetProgramEnvParameterdvARB driDispatchRemapTable[GetProgramEnvParameterdvARB_remap_index]
#define _gloffset_GetProgramEnvParameterfvARB driDispatchRemapTable[GetProgramEnvParameterfvARB_remap_index]
#define _gloffset_GetProgramLocalParameterdvARB driDispatchRemapTable[GetProgramLocalParameterdvARB_remap_index]
#define _gloffset_GetProgramLocalParameterfvARB driDispatchRemapTable[GetProgramLocalParameterfvARB_remap_index]
#define _gloffset_GetProgramStringARB driDispatchRemapTable[GetProgramStringARB_remap_index]
#define _gloffset_GetProgramivARB driDispatchRemapTable[GetProgramivARB_remap_index]
#define _gloffset_GetVertexAttribdvARB driDispatchRemapTable[GetVertexAttribdvARB_remap_index]
#define _gloffset_GetVertexAttribfvARB driDispatchRemapTable[GetVertexAttribfvARB_remap_index]
#define _gloffset_GetVertexAttribivARB driDispatchRemapTable[GetVertexAttribivARB_remap_index]
#define _gloffset_ProgramEnvParameter4dARB driDispatchRemapTable[ProgramEnvParameter4dARB_remap_index]
#define _gloffset_ProgramEnvParameter4dvARB driDispatchRemapTable[ProgramEnvParameter4dvARB_remap_index]
#define _gloffset_ProgramEnvParameter4fARB driDispatchRemapTable[ProgramEnvParameter4fARB_remap_index]
#define _gloffset_ProgramEnvParameter4fvARB driDispatchRemapTable[ProgramEnvParameter4fvARB_remap_index]
#define _gloffset_ProgramLocalParameter4dARB driDispatchRemapTable[ProgramLocalParameter4dARB_remap_index]
#define _gloffset_ProgramLocalParameter4dvARB driDispatchRemapTable[ProgramLocalParameter4dvARB_remap_index]
#define _gloffset_ProgramLocalParameter4fARB driDispatchRemapTable[ProgramLocalParameter4fARB_remap_index]
#define _gloffset_ProgramLocalParameter4fvARB driDispatchRemapTable[ProgramLocalParameter4fvARB_remap_index]
#define _gloffset_ProgramStringARB driDispatchRemapTable[ProgramStringARB_remap_index]
#define _gloffset_VertexAttrib1dARB driDispatchRemapTable[VertexAttrib1dARB_remap_index]
#define _gloffset_VertexAttrib1dvARB driDispatchRemapTable[VertexAttrib1dvARB_remap_index]
#define _gloffset_VertexAttrib1fARB driDispatchRemapTable[VertexAttrib1fARB_remap_index]
#define _gloffset_VertexAttrib1fvARB driDispatchRemapTable[VertexAttrib1fvARB_remap_index]
#define _gloffset_VertexAttrib1sARB driDispatchRemapTable[VertexAttrib1sARB_remap_index]
#define _gloffset_VertexAttrib1svARB driDispatchRemapTable[VertexAttrib1svARB_remap_index]
#define _gloffset_VertexAttrib2dARB driDispatchRemapTable[VertexAttrib2dARB_remap_index]
#define _gloffset_VertexAttrib2dvARB driDispatchRemapTable[VertexAttrib2dvARB_remap_index]
#define _gloffset_VertexAttrib2fARB driDispatchRemapTable[VertexAttrib2fARB_remap_index]
#define _gloffset_VertexAttrib2fvARB driDispatchRemapTable[VertexAttrib2fvARB_remap_index]
#define _gloffset_VertexAttrib2sARB driDispatchRemapTable[VertexAttrib2sARB_remap_index]
#define _gloffset_VertexAttrib2svARB driDispatchRemapTable[VertexAttrib2svARB_remap_index]
#define _gloffset_VertexAttrib3dARB driDispatchRemapTable[VertexAttrib3dARB_remap_index]
#define _gloffset_VertexAttrib3dvARB driDispatchRemapTable[VertexAttrib3dvARB_remap_index]
#define _gloffset_VertexAttrib3fARB driDispatchRemapTable[VertexAttrib3fARB_remap_index]
#define _gloffset_VertexAttrib3fvARB driDispatchRemapTable[VertexAttrib3fvARB_remap_index]
#define _gloffset_VertexAttrib3sARB driDispatchRemapTable[VertexAttrib3sARB_remap_index]
#define _gloffset_VertexAttrib3svARB driDispatchRemapTable[VertexAttrib3svARB_remap_index]
#define _gloffset_VertexAttrib4NbvARB driDispatchRemapTable[VertexAttrib4NbvARB_remap_index]
#define _gloffset_VertexAttrib4NivARB driDispatchRemapTable[VertexAttrib4NivARB_remap_index]
#define _gloffset_VertexAttrib4NsvARB driDispatchRemapTable[VertexAttrib4NsvARB_remap_index]
#define _gloffset_VertexAttrib4NubARB driDispatchRemapTable[VertexAttrib4NubARB_remap_index]
#define _gloffset_VertexAttrib4NubvARB driDispatchRemapTable[VertexAttrib4NubvARB_remap_index]
#define _gloffset_VertexAttrib4NuivARB driDispatchRemapTable[VertexAttrib4NuivARB_remap_index]
#define _gloffset_VertexAttrib4NusvARB driDispatchRemapTable[VertexAttrib4NusvARB_remap_index]
#define _gloffset_VertexAttrib4bvARB driDispatchRemapTable[VertexAttrib4bvARB_remap_index]
#define _gloffset_VertexAttrib4dARB driDispatchRemapTable[VertexAttrib4dARB_remap_index]
#define _gloffset_VertexAttrib4dvARB driDispatchRemapTable[VertexAttrib4dvARB_remap_index]
#define _gloffset_VertexAttrib4fARB driDispatchRemapTable[VertexAttrib4fARB_remap_index]
#define _gloffset_VertexAttrib4fvARB driDispatchRemapTable[VertexAttrib4fvARB_remap_index]
#define _gloffset_VertexAttrib4ivARB driDispatchRemapTable[VertexAttrib4ivARB_remap_index]
#define _gloffset_VertexAttrib4sARB driDispatchRemapTable[VertexAttrib4sARB_remap_index]
#define _gloffset_VertexAttrib4svARB driDispatchRemapTable[VertexAttrib4svARB_remap_index]
#define _gloffset_VertexAttrib4ubvARB driDispatchRemapTable[VertexAttrib4ubvARB_remap_index]
#define _gloffset_VertexAttrib4uivARB driDispatchRemapTable[VertexAttrib4uivARB_remap_index]
#define _gloffset_VertexAttrib4usvARB driDispatchRemapTable[VertexAttrib4usvARB_remap_index]
#define _gloffset_VertexAttribPointerARB driDispatchRemapTable[VertexAttribPointerARB_remap_index]
#define _gloffset_BindBufferARB driDispatchRemapTable[BindBufferARB_remap_index]
#define _gloffset_BufferDataARB driDispatchRemapTable[BufferDataARB_remap_index]
#define _gloffset_BufferSubDataARB driDispatchRemapTable[BufferSubDataARB_remap_index]
#define _gloffset_DeleteBuffersARB driDispatchRemapTable[DeleteBuffersARB_remap_index]
#define _gloffset_GenBuffersARB driDispatchRemapTable[GenBuffersARB_remap_index]
#define _gloffset_GetBufferParameterivARB driDispatchRemapTable[GetBufferParameterivARB_remap_index]
#define _gloffset_GetBufferPointervARB driDispatchRemapTable[GetBufferPointervARB_remap_index]
#define _gloffset_GetBufferSubDataARB driDispatchRemapTable[GetBufferSubDataARB_remap_index]
#define _gloffset_IsBufferARB driDispatchRemapTable[IsBufferARB_remap_index]
#define _gloffset_MapBufferARB driDispatchRemapTable[MapBufferARB_remap_index]
#define _gloffset_UnmapBufferARB driDispatchRemapTable[UnmapBufferARB_remap_index]
#define _gloffset_BeginQueryARB driDispatchRemapTable[BeginQueryARB_remap_index]
#define _gloffset_DeleteQueriesARB driDispatchRemapTable[DeleteQueriesARB_remap_index]
#define _gloffset_EndQueryARB driDispatchRemapTable[EndQueryARB_remap_index]
#define _gloffset_GenQueriesARB driDispatchRemapTable[GenQueriesARB_remap_index]
#define _gloffset_GetQueryObjectivARB driDispatchRemapTable[GetQueryObjectivARB_remap_index]
#define _gloffset_GetQueryObjectuivARB driDispatchRemapTable[GetQueryObjectuivARB_remap_index]
#define _gloffset_GetQueryivARB driDispatchRemapTable[GetQueryivARB_remap_index]
#define _gloffset_IsQueryARB driDispatchRemapTable[IsQueryARB_remap_index]
#define _gloffset_AttachObjectARB driDispatchRemapTable[AttachObjectARB_remap_index]
#define _gloffset_CompileShaderARB driDispatchRemapTable[CompileShaderARB_remap_index]
#define _gloffset_CreateProgramObjectARB driDispatchRemapTable[CreateProgramObjectARB_remap_index]
#define _gloffset_CreateShaderObjectARB driDispatchRemapTable[CreateShaderObjectARB_remap_index]
#define _gloffset_DeleteObjectARB driDispatchRemapTable[DeleteObjectARB_remap_index]
#define _gloffset_DetachObjectARB driDispatchRemapTable[DetachObjectARB_remap_index]
#define _gloffset_GetActiveUniformARB driDispatchRemapTable[GetActiveUniformARB_remap_index]
#define _gloffset_GetAttachedObjectsARB driDispatchRemapTable[GetAttachedObjectsARB_remap_index]
#define _gloffset_GetHandleARB driDispatchRemapTable[GetHandleARB_remap_index]
#define _gloffset_GetInfoLogARB driDispatchRemapTable[GetInfoLogARB_remap_index]
#define _gloffset_GetObjectParameterfvARB driDispatchRemapTable[GetObjectParameterfvARB_remap_index]
#define _gloffset_GetObjectParameterivARB driDispatchRemapTable[GetObjectParameterivARB_remap_index]
#define _gloffset_GetShaderSourceARB driDispatchRemapTable[GetShaderSourceARB_remap_index]
#define _gloffset_GetUniformLocationARB driDispatchRemapTable[GetUniformLocationARB_remap_index]
#define _gloffset_GetUniformfvARB driDispatchRemapTable[GetUniformfvARB_remap_index]
#define _gloffset_GetUniformivARB driDispatchRemapTable[GetUniformivARB_remap_index]
#define _gloffset_LinkProgramARB driDispatchRemapTable[LinkProgramARB_remap_index]
#define _gloffset_ShaderSourceARB driDispatchRemapTable[ShaderSourceARB_remap_index]
#define _gloffset_Uniform1fARB driDispatchRemapTable[Uniform1fARB_remap_index]
#define _gloffset_Uniform1fvARB driDispatchRemapTable[Uniform1fvARB_remap_index]
#define _gloffset_Uniform1iARB driDispatchRemapTable[Uniform1iARB_remap_index]
#define _gloffset_Uniform1ivARB driDispatchRemapTable[Uniform1ivARB_remap_index]
#define _gloffset_Uniform2fARB driDispatchRemapTable[Uniform2fARB_remap_index]
#define _gloffset_Uniform2fvARB driDispatchRemapTable[Uniform2fvARB_remap_index]
#define _gloffset_Uniform2iARB driDispatchRemapTable[Uniform2iARB_remap_index]
#define _gloffset_Uniform2ivARB driDispatchRemapTable[Uniform2ivARB_remap_index]
#define _gloffset_Uniform3fARB driDispatchRemapTable[Uniform3fARB_remap_index]
#define _gloffset_Uniform3fvARB driDispatchRemapTable[Uniform3fvARB_remap_index]
#define _gloffset_Uniform3iARB driDispatchRemapTable[Uniform3iARB_remap_index]
#define _gloffset_Uniform3ivARB driDispatchRemapTable[Uniform3ivARB_remap_index]
#define _gloffset_Uniform4fARB driDispatchRemapTable[Uniform4fARB_remap_index]
#define _gloffset_Uniform4fvARB driDispatchRemapTable[Uniform4fvARB_remap_index]
#define _gloffset_Uniform4iARB driDispatchRemapTable[Uniform4iARB_remap_index]
#define _gloffset_Uniform4ivARB driDispatchRemapTable[Uniform4ivARB_remap_index]
#define _gloffset_UniformMatrix2fvARB driDispatchRemapTable[UniformMatrix2fvARB_remap_index]
#define _gloffset_UniformMatrix3fvARB driDispatchRemapTable[UniformMatrix3fvARB_remap_index]
#define _gloffset_UniformMatrix4fvARB driDispatchRemapTable[UniformMatrix4fvARB_remap_index]
#define _gloffset_UseProgramObjectARB driDispatchRemapTable[UseProgramObjectARB_remap_index]
#define _gloffset_ValidateProgramARB driDispatchRemapTable[ValidateProgramARB_remap_index]
#define _gloffset_BindAttribLocationARB driDispatchRemapTable[BindAttribLocationARB_remap_index]
#define _gloffset_GetActiveAttribARB driDispatchRemapTable[GetActiveAttribARB_remap_index]
#define _gloffset_GetAttribLocationARB driDispatchRemapTable[GetAttribLocationARB_remap_index]
#define _gloffset_DrawBuffersARB driDispatchRemapTable[DrawBuffersARB_remap_index]
#define _gloffset_RenderbufferStorageMultisample driDispatchRemapTable[RenderbufferStorageMultisample_remap_index]
#define _gloffset_FramebufferTextureARB driDispatchRemapTable[FramebufferTextureARB_remap_index]
#define _gloffset_FramebufferTextureFaceARB driDispatchRemapTable[FramebufferTextureFaceARB_remap_index]
#define _gloffset_ProgramParameteriARB driDispatchRemapTable[ProgramParameteriARB_remap_index]
#define _gloffset_FlushMappedBufferRange driDispatchRemapTable[FlushMappedBufferRange_remap_index]
#define _gloffset_MapBufferRange driDispatchRemapTable[MapBufferRange_remap_index]
#define _gloffset_BindVertexArray driDispatchRemapTable[BindVertexArray_remap_index]
#define _gloffset_GenVertexArrays driDispatchRemapTable[GenVertexArrays_remap_index]
#define _gloffset_CopyBufferSubData driDispatchRemapTable[CopyBufferSubData_remap_index]
#define _gloffset_ClientWaitSync driDispatchRemapTable[ClientWaitSync_remap_index]
#define _gloffset_DeleteSync driDispatchRemapTable[DeleteSync_remap_index]
#define _gloffset_FenceSync driDispatchRemapTable[FenceSync_remap_index]
#define _gloffset_GetInteger64v driDispatchRemapTable[GetInteger64v_remap_index]
#define _gloffset_GetSynciv driDispatchRemapTable[GetSynciv_remap_index]
#define _gloffset_IsSync driDispatchRemapTable[IsSync_remap_index]
#define _gloffset_WaitSync driDispatchRemapTable[WaitSync_remap_index]
#define _gloffset_DrawElementsBaseVertex driDispatchRemapTable[DrawElementsBaseVertex_remap_index]
#define _gloffset_DrawRangeElementsBaseVertex driDispatchRemapTable[DrawRangeElementsBaseVertex_remap_index]
#define _gloffset_MultiDrawElementsBaseVertex driDispatchRemapTable[MultiDrawElementsBaseVertex_remap_index]
#define _gloffset_BindTransformFeedback driDispatchRemapTable[BindTransformFeedback_remap_index]
#define _gloffset_DeleteTransformFeedbacks driDispatchRemapTable[DeleteTransformFeedbacks_remap_index]
#define _gloffset_DrawTransformFeedback driDispatchRemapTable[DrawTransformFeedback_remap_index]
#define _gloffset_GenTransformFeedbacks driDispatchRemapTable[GenTransformFeedbacks_remap_index]
#define _gloffset_IsTransformFeedback driDispatchRemapTable[IsTransformFeedback_remap_index]
#define _gloffset_PauseTransformFeedback driDispatchRemapTable[PauseTransformFeedback_remap_index]
#define _gloffset_ResumeTransformFeedback driDispatchRemapTable[ResumeTransformFeedback_remap_index]
#define _gloffset_PolygonOffsetEXT driDispatchRemapTable[PolygonOffsetEXT_remap_index]
#define _gloffset_GetPixelTexGenParameterfvSGIS driDispatchRemapTable[GetPixelTexGenParameterfvSGIS_remap_index]
#define _gloffset_GetPixelTexGenParameterivSGIS driDispatchRemapTable[GetPixelTexGenParameterivSGIS_remap_index]
#define _gloffset_PixelTexGenParameterfSGIS driDispatchRemapTable[PixelTexGenParameterfSGIS_remap_index]
#define _gloffset_PixelTexGenParameterfvSGIS driDispatchRemapTable[PixelTexGenParameterfvSGIS_remap_index]
#define _gloffset_PixelTexGenParameteriSGIS driDispatchRemapTable[PixelTexGenParameteriSGIS_remap_index]
#define _gloffset_PixelTexGenParameterivSGIS driDispatchRemapTable[PixelTexGenParameterivSGIS_remap_index]
#define _gloffset_SampleMaskSGIS driDispatchRemapTable[SampleMaskSGIS_remap_index]
#define _gloffset_SamplePatternSGIS driDispatchRemapTable[SamplePatternSGIS_remap_index]
#define _gloffset_ColorPointerEXT driDispatchRemapTable[ColorPointerEXT_remap_index]
#define _gloffset_EdgeFlagPointerEXT driDispatchRemapTable[EdgeFlagPointerEXT_remap_index]
#define _gloffset_IndexPointerEXT driDispatchRemapTable[IndexPointerEXT_remap_index]
#define _gloffset_NormalPointerEXT driDispatchRemapTable[NormalPointerEXT_remap_index]
#define _gloffset_TexCoordPointerEXT driDispatchRemapTable[TexCoordPointerEXT_remap_index]
#define _gloffset_VertexPointerEXT driDispatchRemapTable[VertexPointerEXT_remap_index]
#define _gloffset_PointParameterfEXT driDispatchRemapTable[PointParameterfEXT_remap_index]
#define _gloffset_PointParameterfvEXT driDispatchRemapTable[PointParameterfvEXT_remap_index]
#define _gloffset_LockArraysEXT driDispatchRemapTable[LockArraysEXT_remap_index]
#define _gloffset_UnlockArraysEXT driDispatchRemapTable[UnlockArraysEXT_remap_index]
#define _gloffset_SecondaryColor3bEXT driDispatchRemapTable[SecondaryColor3bEXT_remap_index]
#define _gloffset_SecondaryColor3bvEXT driDispatchRemapTable[SecondaryColor3bvEXT_remap_index]
#define _gloffset_SecondaryColor3dEXT driDispatchRemapTable[SecondaryColor3dEXT_remap_index]
#define _gloffset_SecondaryColor3dvEXT driDispatchRemapTable[SecondaryColor3dvEXT_remap_index]
#define _gloffset_SecondaryColor3fEXT driDispatchRemapTable[SecondaryColor3fEXT_remap_index]
#define _gloffset_SecondaryColor3fvEXT driDispatchRemapTable[SecondaryColor3fvEXT_remap_index]
#define _gloffset_SecondaryColor3iEXT driDispatchRemapTable[SecondaryColor3iEXT_remap_index]
#define _gloffset_SecondaryColor3ivEXT driDispatchRemapTable[SecondaryColor3ivEXT_remap_index]
#define _gloffset_SecondaryColor3sEXT driDispatchRemapTable[SecondaryColor3sEXT_remap_index]
#define _gloffset_SecondaryColor3svEXT driDispatchRemapTable[SecondaryColor3svEXT_remap_index]
#define _gloffset_SecondaryColor3ubEXT driDispatchRemapTable[SecondaryColor3ubEXT_remap_index]
#define _gloffset_SecondaryColor3ubvEXT driDispatchRemapTable[SecondaryColor3ubvEXT_remap_index]
#define _gloffset_SecondaryColor3uiEXT driDispatchRemapTable[SecondaryColor3uiEXT_remap_index]
#define _gloffset_SecondaryColor3uivEXT driDispatchRemapTable[SecondaryColor3uivEXT_remap_index]
#define _gloffset_SecondaryColor3usEXT driDispatchRemapTable[SecondaryColor3usEXT_remap_index]
#define _gloffset_SecondaryColor3usvEXT driDispatchRemapTable[SecondaryColor3usvEXT_remap_index]
#define _gloffset_SecondaryColorPointerEXT driDispatchRemapTable[SecondaryColorPointerEXT_remap_index]
#define _gloffset_MultiDrawArraysEXT driDispatchRemapTable[MultiDrawArraysEXT_remap_index]
#define _gloffset_MultiDrawElementsEXT driDispatchRemapTable[MultiDrawElementsEXT_remap_index]
#define _gloffset_FogCoordPointerEXT driDispatchRemapTable[FogCoordPointerEXT_remap_index]
#define _gloffset_FogCoorddEXT driDispatchRemapTable[FogCoorddEXT_remap_index]
#define _gloffset_FogCoorddvEXT driDispatchRemapTable[FogCoorddvEXT_remap_index]
#define _gloffset_FogCoordfEXT driDispatchRemapTable[FogCoordfEXT_remap_index]
#define _gloffset_FogCoordfvEXT driDispatchRemapTable[FogCoordfvEXT_remap_index]
#define _gloffset_PixelTexGenSGIX driDispatchRemapTable[PixelTexGenSGIX_remap_index]
#define _gloffset_BlendFuncSeparateEXT driDispatchRemapTable[BlendFuncSeparateEXT_remap_index]
#define _gloffset_FlushVertexArrayRangeNV driDispatchRemapTable[FlushVertexArrayRangeNV_remap_index]
#define _gloffset_VertexArrayRangeNV driDispatchRemapTable[VertexArrayRangeNV_remap_index]
#define _gloffset_CombinerInputNV driDispatchRemapTable[CombinerInputNV_remap_index]
#define _gloffset_CombinerOutputNV driDispatchRemapTable[CombinerOutputNV_remap_index]
#define _gloffset_CombinerParameterfNV driDispatchRemapTable[CombinerParameterfNV_remap_index]
#define _gloffset_CombinerParameterfvNV driDispatchRemapTable[CombinerParameterfvNV_remap_index]
#define _gloffset_CombinerParameteriNV driDispatchRemapTable[CombinerParameteriNV_remap_index]
#define _gloffset_CombinerParameterivNV driDispatchRemapTable[CombinerParameterivNV_remap_index]
#define _gloffset_FinalCombinerInputNV driDispatchRemapTable[FinalCombinerInputNV_remap_index]
#define _gloffset_GetCombinerInputParameterfvNV driDispatchRemapTable[GetCombinerInputParameterfvNV_remap_index]
#define _gloffset_GetCombinerInputParameterivNV driDispatchRemapTable[GetCombinerInputParameterivNV_remap_index]
#define _gloffset_GetCombinerOutputParameterfvNV driDispatchRemapTable[GetCombinerOutputParameterfvNV_remap_index]
#define _gloffset_GetCombinerOutputParameterivNV driDispatchRemapTable[GetCombinerOutputParameterivNV_remap_index]
#define _gloffset_GetFinalCombinerInputParameterfvNV driDispatchRemapTable[GetFinalCombinerInputParameterfvNV_remap_index]
#define _gloffset_GetFinalCombinerInputParameterivNV driDispatchRemapTable[GetFinalCombinerInputParameterivNV_remap_index]
#define _gloffset_ResizeBuffersMESA driDispatchRemapTable[ResizeBuffersMESA_remap_index]
#define _gloffset_WindowPos2dMESA driDispatchRemapTable[WindowPos2dMESA_remap_index]
#define _gloffset_WindowPos2dvMESA driDispatchRemapTable[WindowPos2dvMESA_remap_index]
#define _gloffset_WindowPos2fMESA driDispatchRemapTable[WindowPos2fMESA_remap_index]
#define _gloffset_WindowPos2fvMESA driDispatchRemapTable[WindowPos2fvMESA_remap_index]
#define _gloffset_WindowPos2iMESA driDispatchRemapTable[WindowPos2iMESA_remap_index]
#define _gloffset_WindowPos2ivMESA driDispatchRemapTable[WindowPos2ivMESA_remap_index]
#define _gloffset_WindowPos2sMESA driDispatchRemapTable[WindowPos2sMESA_remap_index]
#define _gloffset_WindowPos2svMESA driDispatchRemapTable[WindowPos2svMESA_remap_index]
#define _gloffset_WindowPos3dMESA driDispatchRemapTable[WindowPos3dMESA_remap_index]
#define _gloffset_WindowPos3dvMESA driDispatchRemapTable[WindowPos3dvMESA_remap_index]
#define _gloffset_WindowPos3fMESA driDispatchRemapTable[WindowPos3fMESA_remap_index]
#define _gloffset_WindowPos3fvMESA driDispatchRemapTable[WindowPos3fvMESA_remap_index]
#define _gloffset_WindowPos3iMESA driDispatchRemapTable[WindowPos3iMESA_remap_index]
#define _gloffset_WindowPos3ivMESA driDispatchRemapTable[WindowPos3ivMESA_remap_index]
#define _gloffset_WindowPos3sMESA driDispatchRemapTable[WindowPos3sMESA_remap_index]
#define _gloffset_WindowPos3svMESA driDispatchRemapTable[WindowPos3svMESA_remap_index]
#define _gloffset_WindowPos4dMESA driDispatchRemapTable[WindowPos4dMESA_remap_index]
#define _gloffset_WindowPos4dvMESA driDispatchRemapTable[WindowPos4dvMESA_remap_index]
#define _gloffset_WindowPos4fMESA driDispatchRemapTable[WindowPos4fMESA_remap_index]
#define _gloffset_WindowPos4fvMESA driDispatchRemapTable[WindowPos4fvMESA_remap_index]
#define _gloffset_WindowPos4iMESA driDispatchRemapTable[WindowPos4iMESA_remap_index]
#define _gloffset_WindowPos4ivMESA driDispatchRemapTable[WindowPos4ivMESA_remap_index]
#define _gloffset_WindowPos4sMESA driDispatchRemapTable[WindowPos4sMESA_remap_index]
#define _gloffset_WindowPos4svMESA driDispatchRemapTable[WindowPos4svMESA_remap_index]
#define _gloffset_MultiModeDrawArraysIBM driDispatchRemapTable[MultiModeDrawArraysIBM_remap_index]
#define _gloffset_MultiModeDrawElementsIBM driDispatchRemapTable[MultiModeDrawElementsIBM_remap_index]
#define _gloffset_DeleteFencesNV driDispatchRemapTable[DeleteFencesNV_remap_index]
#define _gloffset_FinishFenceNV driDispatchRemapTable[FinishFenceNV_remap_index]
#define _gloffset_GenFencesNV driDispatchRemapTable[GenFencesNV_remap_index]
#define _gloffset_GetFenceivNV driDispatchRemapTable[GetFenceivNV_remap_index]
#define _gloffset_IsFenceNV driDispatchRemapTable[IsFenceNV_remap_index]
#define _gloffset_SetFenceNV driDispatchRemapTable[SetFenceNV_remap_index]
#define _gloffset_TestFenceNV driDispatchRemapTable[TestFenceNV_remap_index]
#define _gloffset_AreProgramsResidentNV driDispatchRemapTable[AreProgramsResidentNV_remap_index]
#define _gloffset_BindProgramNV driDispatchRemapTable[BindProgramNV_remap_index]
#define _gloffset_DeleteProgramsNV driDispatchRemapTable[DeleteProgramsNV_remap_index]
#define _gloffset_ExecuteProgramNV driDispatchRemapTable[ExecuteProgramNV_remap_index]
#define _gloffset_GenProgramsNV driDispatchRemapTable[GenProgramsNV_remap_index]
#define _gloffset_GetProgramParameterdvNV driDispatchRemapTable[GetProgramParameterdvNV_remap_index]
#define _gloffset_GetProgramParameterfvNV driDispatchRemapTable[GetProgramParameterfvNV_remap_index]
#define _gloffset_GetProgramStringNV driDispatchRemapTable[GetProgramStringNV_remap_index]
#define _gloffset_GetProgramivNV driDispatchRemapTable[GetProgramivNV_remap_index]
#define _gloffset_GetTrackMatrixivNV driDispatchRemapTable[GetTrackMatrixivNV_remap_index]
#define _gloffset_GetVertexAttribPointervNV driDispatchRemapTable[GetVertexAttribPointervNV_remap_index]
#define _gloffset_GetVertexAttribdvNV driDispatchRemapTable[GetVertexAttribdvNV_remap_index]
#define _gloffset_GetVertexAttribfvNV driDispatchRemapTable[GetVertexAttribfvNV_remap_index]
#define _gloffset_GetVertexAttribivNV driDispatchRemapTable[GetVertexAttribivNV_remap_index]
#define _gloffset_IsProgramNV driDispatchRemapTable[IsProgramNV_remap_index]
#define _gloffset_LoadProgramNV driDispatchRemapTable[LoadProgramNV_remap_index]
#define _gloffset_ProgramParameters4dvNV driDispatchRemapTable[ProgramParameters4dvNV_remap_index]
#define _gloffset_ProgramParameters4fvNV driDispatchRemapTable[ProgramParameters4fvNV_remap_index]
#define _gloffset_RequestResidentProgramsNV driDispatchRemapTable[RequestResidentProgramsNV_remap_index]
#define _gloffset_TrackMatrixNV driDispatchRemapTable[TrackMatrixNV_remap_index]
#define _gloffset_VertexAttrib1dNV driDispatchRemapTable[VertexAttrib1dNV_remap_index]
#define _gloffset_VertexAttrib1dvNV driDispatchRemapTable[VertexAttrib1dvNV_remap_index]
#define _gloffset_VertexAttrib1fNV driDispatchRemapTable[VertexAttrib1fNV_remap_index]
#define _gloffset_VertexAttrib1fvNV driDispatchRemapTable[VertexAttrib1fvNV_remap_index]
#define _gloffset_VertexAttrib1sNV driDispatchRemapTable[VertexAttrib1sNV_remap_index]
#define _gloffset_VertexAttrib1svNV driDispatchRemapTable[VertexAttrib1svNV_remap_index]
#define _gloffset_VertexAttrib2dNV driDispatchRemapTable[VertexAttrib2dNV_remap_index]
#define _gloffset_VertexAttrib2dvNV driDispatchRemapTable[VertexAttrib2dvNV_remap_index]
#define _gloffset_VertexAttrib2fNV driDispatchRemapTable[VertexAttrib2fNV_remap_index]
#define _gloffset_VertexAttrib2fvNV driDispatchRemapTable[VertexAttrib2fvNV_remap_index]
#define _gloffset_VertexAttrib2sNV driDispatchRemapTable[VertexAttrib2sNV_remap_index]
#define _gloffset_VertexAttrib2svNV driDispatchRemapTable[VertexAttrib2svNV_remap_index]
#define _gloffset_VertexAttrib3dNV driDispatchRemapTable[VertexAttrib3dNV_remap_index]
#define _gloffset_VertexAttrib3dvNV driDispatchRemapTable[VertexAttrib3dvNV_remap_index]
#define _gloffset_VertexAttrib3fNV driDispatchRemapTable[VertexAttrib3fNV_remap_index]
#define _gloffset_VertexAttrib3fvNV driDispatchRemapTable[VertexAttrib3fvNV_remap_index]
#define _gloffset_VertexAttrib3sNV driDispatchRemapTable[VertexAttrib3sNV_remap_index]
#define _gloffset_VertexAttrib3svNV driDispatchRemapTable[VertexAttrib3svNV_remap_index]
#define _gloffset_VertexAttrib4dNV driDispatchRemapTable[VertexAttrib4dNV_remap_index]
#define _gloffset_VertexAttrib4dvNV driDispatchRemapTable[VertexAttrib4dvNV_remap_index]
#define _gloffset_VertexAttrib4fNV driDispatchRemapTable[VertexAttrib4fNV_remap_index]
#define _gloffset_VertexAttrib4fvNV driDispatchRemapTable[VertexAttrib4fvNV_remap_index]
#define _gloffset_VertexAttrib4sNV driDispatchRemapTable[VertexAttrib4sNV_remap_index]
#define _gloffset_VertexAttrib4svNV driDispatchRemapTable[VertexAttrib4svNV_remap_index]
#define _gloffset_VertexAttrib4ubNV driDispatchRemapTable[VertexAttrib4ubNV_remap_index]
#define _gloffset_VertexAttrib4ubvNV driDispatchRemapTable[VertexAttrib4ubvNV_remap_index]
#define _gloffset_VertexAttribPointerNV driDispatchRemapTable[VertexAttribPointerNV_remap_index]
#define _gloffset_VertexAttribs1dvNV driDispatchRemapTable[VertexAttribs1dvNV_remap_index]
#define _gloffset_VertexAttribs1fvNV driDispatchRemapTable[VertexAttribs1fvNV_remap_index]
#define _gloffset_VertexAttribs1svNV driDispatchRemapTable[VertexAttribs1svNV_remap_index]
#define _gloffset_VertexAttribs2dvNV driDispatchRemapTable[VertexAttribs2dvNV_remap_index]
#define _gloffset_VertexAttribs2fvNV driDispatchRemapTable[VertexAttribs2fvNV_remap_index]
#define _gloffset_VertexAttribs2svNV driDispatchRemapTable[VertexAttribs2svNV_remap_index]
#define _gloffset_VertexAttribs3dvNV driDispatchRemapTable[VertexAttribs3dvNV_remap_index]
#define _gloffset_VertexAttribs3fvNV driDispatchRemapTable[VertexAttribs3fvNV_remap_index]
#define _gloffset_VertexAttribs3svNV driDispatchRemapTable[VertexAttribs3svNV_remap_index]
#define _gloffset_VertexAttribs4dvNV driDispatchRemapTable[VertexAttribs4dvNV_remap_index]
#define _gloffset_VertexAttribs4fvNV driDispatchRemapTable[VertexAttribs4fvNV_remap_index]
#define _gloffset_VertexAttribs4svNV driDispatchRemapTable[VertexAttribs4svNV_remap_index]
#define _gloffset_VertexAttribs4ubvNV driDispatchRemapTable[VertexAttribs4ubvNV_remap_index]
#define _gloffset_GetTexBumpParameterfvATI driDispatchRemapTable[GetTexBumpParameterfvATI_remap_index]
#define _gloffset_GetTexBumpParameterivATI driDispatchRemapTable[GetTexBumpParameterivATI_remap_index]
#define _gloffset_TexBumpParameterfvATI driDispatchRemapTable[TexBumpParameterfvATI_remap_index]
#define _gloffset_TexBumpParameterivATI driDispatchRemapTable[TexBumpParameterivATI_remap_index]
#define _gloffset_AlphaFragmentOp1ATI driDispatchRemapTable[AlphaFragmentOp1ATI_remap_index]
#define _gloffset_AlphaFragmentOp2ATI driDispatchRemapTable[AlphaFragmentOp2ATI_remap_index]
#define _gloffset_AlphaFragmentOp3ATI driDispatchRemapTable[AlphaFragmentOp3ATI_remap_index]
#define _gloffset_BeginFragmentShaderATI driDispatchRemapTable[BeginFragmentShaderATI_remap_index]
#define _gloffset_BindFragmentShaderATI driDispatchRemapTable[BindFragmentShaderATI_remap_index]
#define _gloffset_ColorFragmentOp1ATI driDispatchRemapTable[ColorFragmentOp1ATI_remap_index]
#define _gloffset_ColorFragmentOp2ATI driDispatchRemapTable[ColorFragmentOp2ATI_remap_index]
#define _gloffset_ColorFragmentOp3ATI driDispatchRemapTable[ColorFragmentOp3ATI_remap_index]
#define _gloffset_DeleteFragmentShaderATI driDispatchRemapTable[DeleteFragmentShaderATI_remap_index]
#define _gloffset_EndFragmentShaderATI driDispatchRemapTable[EndFragmentShaderATI_remap_index]
#define _gloffset_GenFragmentShadersATI driDispatchRemapTable[GenFragmentShadersATI_remap_index]
#define _gloffset_PassTexCoordATI driDispatchRemapTable[PassTexCoordATI_remap_index]
#define _gloffset_SampleMapATI driDispatchRemapTable[SampleMapATI_remap_index]
#define _gloffset_SetFragmentShaderConstantATI driDispatchRemapTable[SetFragmentShaderConstantATI_remap_index]
#define _gloffset_PointParameteriNV driDispatchRemapTable[PointParameteriNV_remap_index]
#define _gloffset_PointParameterivNV driDispatchRemapTable[PointParameterivNV_remap_index]
#define _gloffset_ActiveStencilFaceEXT driDispatchRemapTable[ActiveStencilFaceEXT_remap_index]
#define _gloffset_BindVertexArrayAPPLE driDispatchRemapTable[BindVertexArrayAPPLE_remap_index]
#define _gloffset_DeleteVertexArraysAPPLE driDispatchRemapTable[DeleteVertexArraysAPPLE_remap_index]
#define _gloffset_GenVertexArraysAPPLE driDispatchRemapTable[GenVertexArraysAPPLE_remap_index]
#define _gloffset_IsVertexArrayAPPLE driDispatchRemapTable[IsVertexArrayAPPLE_remap_index]
#define _gloffset_GetProgramNamedParameterdvNV driDispatchRemapTable[GetProgramNamedParameterdvNV_remap_index]
#define _gloffset_GetProgramNamedParameterfvNV driDispatchRemapTable[GetProgramNamedParameterfvNV_remap_index]
#define _gloffset_ProgramNamedParameter4dNV driDispatchRemapTable[ProgramNamedParameter4dNV_remap_index]
#define _gloffset_ProgramNamedParameter4dvNV driDispatchRemapTable[ProgramNamedParameter4dvNV_remap_index]
#define _gloffset_ProgramNamedParameter4fNV driDispatchRemapTable[ProgramNamedParameter4fNV_remap_index]
#define _gloffset_ProgramNamedParameter4fvNV driDispatchRemapTable[ProgramNamedParameter4fvNV_remap_index]
#define _gloffset_PrimitiveRestartIndexNV driDispatchRemapTable[PrimitiveRestartIndexNV_remap_index]
#define _gloffset_PrimitiveRestartNV driDispatchRemapTable[PrimitiveRestartNV_remap_index]
#define _gloffset_DepthBoundsEXT driDispatchRemapTable[DepthBoundsEXT_remap_index]
#define _gloffset_BlendEquationSeparateEXT driDispatchRemapTable[BlendEquationSeparateEXT_remap_index]
#define _gloffset_BindFramebufferEXT driDispatchRemapTable[BindFramebufferEXT_remap_index]
#define _gloffset_BindRenderbufferEXT driDispatchRemapTable[BindRenderbufferEXT_remap_index]
#define _gloffset_CheckFramebufferStatusEXT driDispatchRemapTable[CheckFramebufferStatusEXT_remap_index]
#define _gloffset_DeleteFramebuffersEXT driDispatchRemapTable[DeleteFramebuffersEXT_remap_index]
#define _gloffset_DeleteRenderbuffersEXT driDispatchRemapTable[DeleteRenderbuffersEXT_remap_index]
#define _gloffset_FramebufferRenderbufferEXT driDispatchRemapTable[FramebufferRenderbufferEXT_remap_index]
#define _gloffset_FramebufferTexture1DEXT driDispatchRemapTable[FramebufferTexture1DEXT_remap_index]
#define _gloffset_FramebufferTexture2DEXT driDispatchRemapTable[FramebufferTexture2DEXT_remap_index]
#define _gloffset_FramebufferTexture3DEXT driDispatchRemapTable[FramebufferTexture3DEXT_remap_index]
#define _gloffset_GenFramebuffersEXT driDispatchRemapTable[GenFramebuffersEXT_remap_index]
#define _gloffset_GenRenderbuffersEXT driDispatchRemapTable[GenRenderbuffersEXT_remap_index]
#define _gloffset_GenerateMipmapEXT driDispatchRemapTable[GenerateMipmapEXT_remap_index]
#define _gloffset_GetFramebufferAttachmentParameterivEXT driDispatchRemapTable[GetFramebufferAttachmentParameterivEXT_remap_index]
#define _gloffset_GetRenderbufferParameterivEXT driDispatchRemapTable[GetRenderbufferParameterivEXT_remap_index]
#define _gloffset_IsFramebufferEXT driDispatchRemapTable[IsFramebufferEXT_remap_index]
#define _gloffset_IsRenderbufferEXT driDispatchRemapTable[IsRenderbufferEXT_remap_index]
#define _gloffset_RenderbufferStorageEXT driDispatchRemapTable[RenderbufferStorageEXT_remap_index]
#define _gloffset_BlitFramebufferEXT driDispatchRemapTable[BlitFramebufferEXT_remap_index]
#define _gloffset_BufferParameteriAPPLE driDispatchRemapTable[BufferParameteriAPPLE_remap_index]
#define _gloffset_FlushMappedBufferRangeAPPLE driDispatchRemapTable[FlushMappedBufferRangeAPPLE_remap_index]
#define _gloffset_BindFragDataLocationEXT driDispatchRemapTable[BindFragDataLocationEXT_remap_index]
#define _gloffset_GetFragDataLocationEXT driDispatchRemapTable[GetFragDataLocationEXT_remap_index]
#define _gloffset_GetUniformuivEXT driDispatchRemapTable[GetUniformuivEXT_remap_index]
#define _gloffset_GetVertexAttribIivEXT driDispatchRemapTable[GetVertexAttribIivEXT_remap_index]
#define _gloffset_GetVertexAttribIuivEXT driDispatchRemapTable[GetVertexAttribIuivEXT_remap_index]
#define _gloffset_Uniform1uiEXT driDispatchRemapTable[Uniform1uiEXT_remap_index]
#define _gloffset_Uniform1uivEXT driDispatchRemapTable[Uniform1uivEXT_remap_index]
#define _gloffset_Uniform2uiEXT driDispatchRemapTable[Uniform2uiEXT_remap_index]
#define _gloffset_Uniform2uivEXT driDispatchRemapTable[Uniform2uivEXT_remap_index]
#define _gloffset_Uniform3uiEXT driDispatchRemapTable[Uniform3uiEXT_remap_index]
#define _gloffset_Uniform3uivEXT driDispatchRemapTable[Uniform3uivEXT_remap_index]
#define _gloffset_Uniform4uiEXT driDispatchRemapTable[Uniform4uiEXT_remap_index]
#define _gloffset_Uniform4uivEXT driDispatchRemapTable[Uniform4uivEXT_remap_index]
#define _gloffset_VertexAttribI1iEXT driDispatchRemapTable[VertexAttribI1iEXT_remap_index]
#define _gloffset_VertexAttribI1ivEXT driDispatchRemapTable[VertexAttribI1ivEXT_remap_index]
#define _gloffset_VertexAttribI1uiEXT driDispatchRemapTable[VertexAttribI1uiEXT_remap_index]
#define _gloffset_VertexAttribI1uivEXT driDispatchRemapTable[VertexAttribI1uivEXT_remap_index]
#define _gloffset_VertexAttribI2iEXT driDispatchRemapTable[VertexAttribI2iEXT_remap_index]
#define _gloffset_VertexAttribI2ivEXT driDispatchRemapTable[VertexAttribI2ivEXT_remap_index]
#define _gloffset_VertexAttribI2uiEXT driDispatchRemapTable[VertexAttribI2uiEXT_remap_index]
#define _gloffset_VertexAttribI2uivEXT driDispatchRemapTable[VertexAttribI2uivEXT_remap_index]
#define _gloffset_VertexAttribI3iEXT driDispatchRemapTable[VertexAttribI3iEXT_remap_index]
#define _gloffset_VertexAttribI3ivEXT driDispatchRemapTable[VertexAttribI3ivEXT_remap_index]
#define _gloffset_VertexAttribI3uiEXT driDispatchRemapTable[VertexAttribI3uiEXT_remap_index]
#define _gloffset_VertexAttribI3uivEXT driDispatchRemapTable[VertexAttribI3uivEXT_remap_index]
#define _gloffset_VertexAttribI4bvEXT driDispatchRemapTable[VertexAttribI4bvEXT_remap_index]
#define _gloffset_VertexAttribI4iEXT driDispatchRemapTable[VertexAttribI4iEXT_remap_index]
#define _gloffset_VertexAttribI4ivEXT driDispatchRemapTable[VertexAttribI4ivEXT_remap_index]
#define _gloffset_VertexAttribI4svEXT driDispatchRemapTable[VertexAttribI4svEXT_remap_index]
#define _gloffset_VertexAttribI4ubvEXT driDispatchRemapTable[VertexAttribI4ubvEXT_remap_index]
#define _gloffset_VertexAttribI4uiEXT driDispatchRemapTable[VertexAttribI4uiEXT_remap_index]
#define _gloffset_VertexAttribI4uivEXT driDispatchRemapTable[VertexAttribI4uivEXT_remap_index]
#define _gloffset_VertexAttribI4usvEXT driDispatchRemapTable[VertexAttribI4usvEXT_remap_index]
#define _gloffset_VertexAttribIPointerEXT driDispatchRemapTable[VertexAttribIPointerEXT_remap_index]
#define _gloffset_FramebufferTextureLayerEXT driDispatchRemapTable[FramebufferTextureLayerEXT_remap_index]
#define _gloffset_ColorMaskIndexedEXT driDispatchRemapTable[ColorMaskIndexedEXT_remap_index]
#define _gloffset_DisableIndexedEXT driDispatchRemapTable[DisableIndexedEXT_remap_index]
#define _gloffset_EnableIndexedEXT driDispatchRemapTable[EnableIndexedEXT_remap_index]
#define _gloffset_GetBooleanIndexedvEXT driDispatchRemapTable[GetBooleanIndexedvEXT_remap_index]
#define _gloffset_GetIntegerIndexedvEXT driDispatchRemapTable[GetIntegerIndexedvEXT_remap_index]
#define _gloffset_IsEnabledIndexedEXT driDispatchRemapTable[IsEnabledIndexedEXT_remap_index]
#define _gloffset_ClearColorIiEXT driDispatchRemapTable[ClearColorIiEXT_remap_index]
#define _gloffset_ClearColorIuiEXT driDispatchRemapTable[ClearColorIuiEXT_remap_index]
#define _gloffset_GetTexParameterIivEXT driDispatchRemapTable[GetTexParameterIivEXT_remap_index]
#define _gloffset_GetTexParameterIuivEXT driDispatchRemapTable[GetTexParameterIuivEXT_remap_index]
#define _gloffset_TexParameterIivEXT driDispatchRemapTable[TexParameterIivEXT_remap_index]
#define _gloffset_TexParameterIuivEXT driDispatchRemapTable[TexParameterIuivEXT_remap_index]
#define _gloffset_BeginConditionalRenderNV driDispatchRemapTable[BeginConditionalRenderNV_remap_index]
#define _gloffset_EndConditionalRenderNV driDispatchRemapTable[EndConditionalRenderNV_remap_index]
#define _gloffset_BeginTransformFeedbackEXT driDispatchRemapTable[BeginTransformFeedbackEXT_remap_index]
#define _gloffset_BindBufferBaseEXT driDispatchRemapTable[BindBufferBaseEXT_remap_index]
#define _gloffset_BindBufferOffsetEXT driDispatchRemapTable[BindBufferOffsetEXT_remap_index]
#define _gloffset_BindBufferRangeEXT driDispatchRemapTable[BindBufferRangeEXT_remap_index]
#define _gloffset_EndTransformFeedbackEXT driDispatchRemapTable[EndTransformFeedbackEXT_remap_index]
#define _gloffset_GetTransformFeedbackVaryingEXT driDispatchRemapTable[GetTransformFeedbackVaryingEXT_remap_index]
#define _gloffset_TransformFeedbackVaryingsEXT driDispatchRemapTable[TransformFeedbackVaryingsEXT_remap_index]
#define _gloffset_ProvokingVertexEXT driDispatchRemapTable[ProvokingVertexEXT_remap_index]
#define _gloffset_GetTexParameterPointervAPPLE driDispatchRemapTable[GetTexParameterPointervAPPLE_remap_index]
#define _gloffset_TextureRangeAPPLE driDispatchRemapTable[TextureRangeAPPLE_remap_index]
#define _gloffset_GetObjectParameterivAPPLE driDispatchRemapTable[GetObjectParameterivAPPLE_remap_index]
#define _gloffset_ObjectPurgeableAPPLE driDispatchRemapTable[ObjectPurgeableAPPLE_remap_index]
#define _gloffset_ObjectUnpurgeableAPPLE driDispatchRemapTable[ObjectUnpurgeableAPPLE_remap_index]
#define _gloffset_ActiveProgramEXT driDispatchRemapTable[ActiveProgramEXT_remap_index]
#define _gloffset_CreateShaderProgramEXT driDispatchRemapTable[CreateShaderProgramEXT_remap_index]
#define _gloffset_UseShaderProgramEXT driDispatchRemapTable[UseShaderProgramEXT_remap_index]
#define _gloffset_StencilFuncSeparateATI driDispatchRemapTable[StencilFuncSeparateATI_remap_index]
#define _gloffset_ProgramEnvParameters4fvEXT driDispatchRemapTable[ProgramEnvParameters4fvEXT_remap_index]
#define _gloffset_ProgramLocalParameters4fvEXT driDispatchRemapTable[ProgramLocalParameters4fvEXT_remap_index]
#define _gloffset_GetQueryObjecti64vEXT driDispatchRemapTable[GetQueryObjecti64vEXT_remap_index]
#define _gloffset_GetQueryObjectui64vEXT driDispatchRemapTable[GetQueryObjectui64vEXT_remap_index]
#define _gloffset_EGLImageTargetRenderbufferStorageOES driDispatchRemapTable[EGLImageTargetRenderbufferStorageOES_remap_index]
#define _gloffset_EGLImageTargetTexture2DOES driDispatchRemapTable[EGLImageTargetTexture2DOES_remap_index]

#endif /* _GLAPI_USE_REMAP_TABLE */

#define CALL_NewList(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum)), _gloffset_NewList, parameters)
#define GET_NewList(disp) GET_by_offset(disp, _gloffset_NewList)
#define SET_NewList(disp, fn) SET_by_offset(disp, _gloffset_NewList, fn)
#define CALL_EndList(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndList, parameters)
#define GET_EndList(disp) GET_by_offset(disp, _gloffset_EndList)
#define SET_EndList(disp, fn) SET_by_offset(disp, _gloffset_EndList, fn)
#define CALL_CallList(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_CallList, parameters)
#define GET_CallList(disp) GET_by_offset(disp, _gloffset_CallList)
#define SET_CallList(disp, fn) SET_by_offset(disp, _gloffset_CallList, fn)
#define CALL_CallLists(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLenum, const GLvoid *)), _gloffset_CallLists, parameters)
#define GET_CallLists(disp) GET_by_offset(disp, _gloffset_CallLists)
#define SET_CallLists(disp, fn) SET_by_offset(disp, _gloffset_CallLists, fn)
#define CALL_DeleteLists(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei)), _gloffset_DeleteLists, parameters)
#define GET_DeleteLists(disp) GET_by_offset(disp, _gloffset_DeleteLists)
#define SET_DeleteLists(disp, fn) SET_by_offset(disp, _gloffset_DeleteLists, fn)
#define CALL_GenLists(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLsizei)), _gloffset_GenLists, parameters)
#define GET_GenLists(disp) GET_by_offset(disp, _gloffset_GenLists)
#define SET_GenLists(disp, fn) SET_by_offset(disp, _gloffset_GenLists, fn)
#define CALL_ListBase(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_ListBase, parameters)
#define GET_ListBase(disp) GET_by_offset(disp, _gloffset_ListBase)
#define SET_ListBase(disp, fn) SET_by_offset(disp, _gloffset_ListBase, fn)
#define CALL_Begin(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_Begin, parameters)
#define GET_Begin(disp) GET_by_offset(disp, _gloffset_Begin)
#define SET_Begin(disp, fn) SET_by_offset(disp, _gloffset_Begin, fn)
#define CALL_Bitmap(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *)), _gloffset_Bitmap, parameters)
#define GET_Bitmap(disp) GET_by_offset(disp, _gloffset_Bitmap)
#define SET_Bitmap(disp, fn) SET_by_offset(disp, _gloffset_Bitmap, fn)
#define CALL_Color3b(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte)), _gloffset_Color3b, parameters)
#define GET_Color3b(disp) GET_by_offset(disp, _gloffset_Color3b)
#define SET_Color3b(disp, fn) SET_by_offset(disp, _gloffset_Color3b, fn)
#define CALL_Color3bv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_Color3bv, parameters)
#define GET_Color3bv(disp) GET_by_offset(disp, _gloffset_Color3bv)
#define SET_Color3bv(disp, fn) SET_by_offset(disp, _gloffset_Color3bv, fn)
#define CALL_Color3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Color3d, parameters)
#define GET_Color3d(disp) GET_by_offset(disp, _gloffset_Color3d)
#define SET_Color3d(disp, fn) SET_by_offset(disp, _gloffset_Color3d, fn)
#define CALL_Color3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Color3dv, parameters)
#define GET_Color3dv(disp) GET_by_offset(disp, _gloffset_Color3dv)
#define SET_Color3dv(disp, fn) SET_by_offset(disp, _gloffset_Color3dv, fn)
#define CALL_Color3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Color3f, parameters)
#define GET_Color3f(disp) GET_by_offset(disp, _gloffset_Color3f)
#define SET_Color3f(disp, fn) SET_by_offset(disp, _gloffset_Color3f, fn)
#define CALL_Color3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Color3fv, parameters)
#define GET_Color3fv(disp) GET_by_offset(disp, _gloffset_Color3fv)
#define SET_Color3fv(disp, fn) SET_by_offset(disp, _gloffset_Color3fv, fn)
#define CALL_Color3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Color3i, parameters)
#define GET_Color3i(disp) GET_by_offset(disp, _gloffset_Color3i)
#define SET_Color3i(disp, fn) SET_by_offset(disp, _gloffset_Color3i, fn)
#define CALL_Color3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Color3iv, parameters)
#define GET_Color3iv(disp) GET_by_offset(disp, _gloffset_Color3iv)
#define SET_Color3iv(disp, fn) SET_by_offset(disp, _gloffset_Color3iv, fn)
#define CALL_Color3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_Color3s, parameters)
#define GET_Color3s(disp) GET_by_offset(disp, _gloffset_Color3s)
#define SET_Color3s(disp, fn) SET_by_offset(disp, _gloffset_Color3s, fn)
#define CALL_Color3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Color3sv, parameters)
#define GET_Color3sv(disp) GET_by_offset(disp, _gloffset_Color3sv)
#define SET_Color3sv(disp, fn) SET_by_offset(disp, _gloffset_Color3sv, fn)
#define CALL_Color3ub(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte, GLubyte, GLubyte)), _gloffset_Color3ub, parameters)
#define GET_Color3ub(disp) GET_by_offset(disp, _gloffset_Color3ub)
#define SET_Color3ub(disp, fn) SET_by_offset(disp, _gloffset_Color3ub, fn)
#define CALL_Color3ubv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_Color3ubv, parameters)
#define GET_Color3ubv(disp) GET_by_offset(disp, _gloffset_Color3ubv)
#define SET_Color3ubv(disp, fn) SET_by_offset(disp, _gloffset_Color3ubv, fn)
#define CALL_Color3ui(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint)), _gloffset_Color3ui, parameters)
#define GET_Color3ui(disp) GET_by_offset(disp, _gloffset_Color3ui)
#define SET_Color3ui(disp, fn) SET_by_offset(disp, _gloffset_Color3ui, fn)
#define CALL_Color3uiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLuint *)), _gloffset_Color3uiv, parameters)
#define GET_Color3uiv(disp) GET_by_offset(disp, _gloffset_Color3uiv)
#define SET_Color3uiv(disp, fn) SET_by_offset(disp, _gloffset_Color3uiv, fn)
#define CALL_Color3us(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLushort, GLushort, GLushort)), _gloffset_Color3us, parameters)
#define GET_Color3us(disp) GET_by_offset(disp, _gloffset_Color3us)
#define SET_Color3us(disp, fn) SET_by_offset(disp, _gloffset_Color3us, fn)
#define CALL_Color3usv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLushort *)), _gloffset_Color3usv, parameters)
#define GET_Color3usv(disp) GET_by_offset(disp, _gloffset_Color3usv)
#define SET_Color3usv(disp, fn) SET_by_offset(disp, _gloffset_Color3usv, fn)
#define CALL_Color4b(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte, GLbyte)), _gloffset_Color4b, parameters)
#define GET_Color4b(disp) GET_by_offset(disp, _gloffset_Color4b)
#define SET_Color4b(disp, fn) SET_by_offset(disp, _gloffset_Color4b, fn)
#define CALL_Color4bv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_Color4bv, parameters)
#define GET_Color4bv(disp) GET_by_offset(disp, _gloffset_Color4bv)
#define SET_Color4bv(disp, fn) SET_by_offset(disp, _gloffset_Color4bv, fn)
#define CALL_Color4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Color4d, parameters)
#define GET_Color4d(disp) GET_by_offset(disp, _gloffset_Color4d)
#define SET_Color4d(disp, fn) SET_by_offset(disp, _gloffset_Color4d, fn)
#define CALL_Color4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Color4dv, parameters)
#define GET_Color4dv(disp) GET_by_offset(disp, _gloffset_Color4dv)
#define SET_Color4dv(disp, fn) SET_by_offset(disp, _gloffset_Color4dv, fn)
#define CALL_Color4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Color4f, parameters)
#define GET_Color4f(disp) GET_by_offset(disp, _gloffset_Color4f)
#define SET_Color4f(disp, fn) SET_by_offset(disp, _gloffset_Color4f, fn)
#define CALL_Color4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Color4fv, parameters)
#define GET_Color4fv(disp) GET_by_offset(disp, _gloffset_Color4fv)
#define SET_Color4fv(disp, fn) SET_by_offset(disp, _gloffset_Color4fv, fn)
#define CALL_Color4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Color4i, parameters)
#define GET_Color4i(disp) GET_by_offset(disp, _gloffset_Color4i)
#define SET_Color4i(disp, fn) SET_by_offset(disp, _gloffset_Color4i, fn)
#define CALL_Color4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Color4iv, parameters)
#define GET_Color4iv(disp) GET_by_offset(disp, _gloffset_Color4iv)
#define SET_Color4iv(disp, fn) SET_by_offset(disp, _gloffset_Color4iv, fn)
#define CALL_Color4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_Color4s, parameters)
#define GET_Color4s(disp) GET_by_offset(disp, _gloffset_Color4s)
#define SET_Color4s(disp, fn) SET_by_offset(disp, _gloffset_Color4s, fn)
#define CALL_Color4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Color4sv, parameters)
#define GET_Color4sv(disp) GET_by_offset(disp, _gloffset_Color4sv)
#define SET_Color4sv(disp, fn) SET_by_offset(disp, _gloffset_Color4sv, fn)
#define CALL_Color4ub(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte, GLubyte, GLubyte, GLubyte)), _gloffset_Color4ub, parameters)
#define GET_Color4ub(disp) GET_by_offset(disp, _gloffset_Color4ub)
#define SET_Color4ub(disp, fn) SET_by_offset(disp, _gloffset_Color4ub, fn)
#define CALL_Color4ubv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_Color4ubv, parameters)
#define GET_Color4ubv(disp) GET_by_offset(disp, _gloffset_Color4ubv)
#define SET_Color4ubv(disp, fn) SET_by_offset(disp, _gloffset_Color4ubv, fn)
#define CALL_Color4ui(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint)), _gloffset_Color4ui, parameters)
#define GET_Color4ui(disp) GET_by_offset(disp, _gloffset_Color4ui)
#define SET_Color4ui(disp, fn) SET_by_offset(disp, _gloffset_Color4ui, fn)
#define CALL_Color4uiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLuint *)), _gloffset_Color4uiv, parameters)
#define GET_Color4uiv(disp) GET_by_offset(disp, _gloffset_Color4uiv)
#define SET_Color4uiv(disp, fn) SET_by_offset(disp, _gloffset_Color4uiv, fn)
#define CALL_Color4us(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLushort, GLushort, GLushort, GLushort)), _gloffset_Color4us, parameters)
#define GET_Color4us(disp) GET_by_offset(disp, _gloffset_Color4us)
#define SET_Color4us(disp, fn) SET_by_offset(disp, _gloffset_Color4us, fn)
#define CALL_Color4usv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLushort *)), _gloffset_Color4usv, parameters)
#define GET_Color4usv(disp) GET_by_offset(disp, _gloffset_Color4usv)
#define SET_Color4usv(disp, fn) SET_by_offset(disp, _gloffset_Color4usv, fn)
#define CALL_EdgeFlag(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLboolean)), _gloffset_EdgeFlag, parameters)
#define GET_EdgeFlag(disp) GET_by_offset(disp, _gloffset_EdgeFlag)
#define SET_EdgeFlag(disp, fn) SET_by_offset(disp, _gloffset_EdgeFlag, fn)
#define CALL_EdgeFlagv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLboolean *)), _gloffset_EdgeFlagv, parameters)
#define GET_EdgeFlagv(disp) GET_by_offset(disp, _gloffset_EdgeFlagv)
#define SET_EdgeFlagv(disp, fn) SET_by_offset(disp, _gloffset_EdgeFlagv, fn)
#define CALL_End(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_End, parameters)
#define GET_End(disp) GET_by_offset(disp, _gloffset_End)
#define SET_End(disp, fn) SET_by_offset(disp, _gloffset_End, fn)
#define CALL_Indexd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_Indexd, parameters)
#define GET_Indexd(disp) GET_by_offset(disp, _gloffset_Indexd)
#define SET_Indexd(disp, fn) SET_by_offset(disp, _gloffset_Indexd, fn)
#define CALL_Indexdv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Indexdv, parameters)
#define GET_Indexdv(disp) GET_by_offset(disp, _gloffset_Indexdv)
#define SET_Indexdv(disp, fn) SET_by_offset(disp, _gloffset_Indexdv, fn)
#define CALL_Indexf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_Indexf, parameters)
#define GET_Indexf(disp) GET_by_offset(disp, _gloffset_Indexf)
#define SET_Indexf(disp, fn) SET_by_offset(disp, _gloffset_Indexf, fn)
#define CALL_Indexfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Indexfv, parameters)
#define GET_Indexfv(disp) GET_by_offset(disp, _gloffset_Indexfv)
#define SET_Indexfv(disp, fn) SET_by_offset(disp, _gloffset_Indexfv, fn)
#define CALL_Indexi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_Indexi, parameters)
#define GET_Indexi(disp) GET_by_offset(disp, _gloffset_Indexi)
#define SET_Indexi(disp, fn) SET_by_offset(disp, _gloffset_Indexi, fn)
#define CALL_Indexiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Indexiv, parameters)
#define GET_Indexiv(disp) GET_by_offset(disp, _gloffset_Indexiv)
#define SET_Indexiv(disp, fn) SET_by_offset(disp, _gloffset_Indexiv, fn)
#define CALL_Indexs(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort)), _gloffset_Indexs, parameters)
#define GET_Indexs(disp) GET_by_offset(disp, _gloffset_Indexs)
#define SET_Indexs(disp, fn) SET_by_offset(disp, _gloffset_Indexs, fn)
#define CALL_Indexsv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Indexsv, parameters)
#define GET_Indexsv(disp) GET_by_offset(disp, _gloffset_Indexsv)
#define SET_Indexsv(disp, fn) SET_by_offset(disp, _gloffset_Indexsv, fn)
#define CALL_Normal3b(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte)), _gloffset_Normal3b, parameters)
#define GET_Normal3b(disp) GET_by_offset(disp, _gloffset_Normal3b)
#define SET_Normal3b(disp, fn) SET_by_offset(disp, _gloffset_Normal3b, fn)
#define CALL_Normal3bv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_Normal3bv, parameters)
#define GET_Normal3bv(disp) GET_by_offset(disp, _gloffset_Normal3bv)
#define SET_Normal3bv(disp, fn) SET_by_offset(disp, _gloffset_Normal3bv, fn)
#define CALL_Normal3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Normal3d, parameters)
#define GET_Normal3d(disp) GET_by_offset(disp, _gloffset_Normal3d)
#define SET_Normal3d(disp, fn) SET_by_offset(disp, _gloffset_Normal3d, fn)
#define CALL_Normal3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Normal3dv, parameters)
#define GET_Normal3dv(disp) GET_by_offset(disp, _gloffset_Normal3dv)
#define SET_Normal3dv(disp, fn) SET_by_offset(disp, _gloffset_Normal3dv, fn)
#define CALL_Normal3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Normal3f, parameters)
#define GET_Normal3f(disp) GET_by_offset(disp, _gloffset_Normal3f)
#define SET_Normal3f(disp, fn) SET_by_offset(disp, _gloffset_Normal3f, fn)
#define CALL_Normal3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Normal3fv, parameters)
#define GET_Normal3fv(disp) GET_by_offset(disp, _gloffset_Normal3fv)
#define SET_Normal3fv(disp, fn) SET_by_offset(disp, _gloffset_Normal3fv, fn)
#define CALL_Normal3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Normal3i, parameters)
#define GET_Normal3i(disp) GET_by_offset(disp, _gloffset_Normal3i)
#define SET_Normal3i(disp, fn) SET_by_offset(disp, _gloffset_Normal3i, fn)
#define CALL_Normal3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Normal3iv, parameters)
#define GET_Normal3iv(disp) GET_by_offset(disp, _gloffset_Normal3iv)
#define SET_Normal3iv(disp, fn) SET_by_offset(disp, _gloffset_Normal3iv, fn)
#define CALL_Normal3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_Normal3s, parameters)
#define GET_Normal3s(disp) GET_by_offset(disp, _gloffset_Normal3s)
#define SET_Normal3s(disp, fn) SET_by_offset(disp, _gloffset_Normal3s, fn)
#define CALL_Normal3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Normal3sv, parameters)
#define GET_Normal3sv(disp) GET_by_offset(disp, _gloffset_Normal3sv)
#define SET_Normal3sv(disp, fn) SET_by_offset(disp, _gloffset_Normal3sv, fn)
#define CALL_RasterPos2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_RasterPos2d, parameters)
#define GET_RasterPos2d(disp) GET_by_offset(disp, _gloffset_RasterPos2d)
#define SET_RasterPos2d(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2d, fn)
#define CALL_RasterPos2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_RasterPos2dv, parameters)
#define GET_RasterPos2dv(disp) GET_by_offset(disp, _gloffset_RasterPos2dv)
#define SET_RasterPos2dv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2dv, fn)
#define CALL_RasterPos2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_RasterPos2f, parameters)
#define GET_RasterPos2f(disp) GET_by_offset(disp, _gloffset_RasterPos2f)
#define SET_RasterPos2f(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2f, fn)
#define CALL_RasterPos2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_RasterPos2fv, parameters)
#define GET_RasterPos2fv(disp) GET_by_offset(disp, _gloffset_RasterPos2fv)
#define SET_RasterPos2fv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2fv, fn)
#define CALL_RasterPos2i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_RasterPos2i, parameters)
#define GET_RasterPos2i(disp) GET_by_offset(disp, _gloffset_RasterPos2i)
#define SET_RasterPos2i(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2i, fn)
#define CALL_RasterPos2iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_RasterPos2iv, parameters)
#define GET_RasterPos2iv(disp) GET_by_offset(disp, _gloffset_RasterPos2iv)
#define SET_RasterPos2iv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2iv, fn)
#define CALL_RasterPos2s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_RasterPos2s, parameters)
#define GET_RasterPos2s(disp) GET_by_offset(disp, _gloffset_RasterPos2s)
#define SET_RasterPos2s(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2s, fn)
#define CALL_RasterPos2sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_RasterPos2sv, parameters)
#define GET_RasterPos2sv(disp) GET_by_offset(disp, _gloffset_RasterPos2sv)
#define SET_RasterPos2sv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos2sv, fn)
#define CALL_RasterPos3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_RasterPos3d, parameters)
#define GET_RasterPos3d(disp) GET_by_offset(disp, _gloffset_RasterPos3d)
#define SET_RasterPos3d(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3d, fn)
#define CALL_RasterPos3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_RasterPos3dv, parameters)
#define GET_RasterPos3dv(disp) GET_by_offset(disp, _gloffset_RasterPos3dv)
#define SET_RasterPos3dv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3dv, fn)
#define CALL_RasterPos3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_RasterPos3f, parameters)
#define GET_RasterPos3f(disp) GET_by_offset(disp, _gloffset_RasterPos3f)
#define SET_RasterPos3f(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3f, fn)
#define CALL_RasterPos3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_RasterPos3fv, parameters)
#define GET_RasterPos3fv(disp) GET_by_offset(disp, _gloffset_RasterPos3fv)
#define SET_RasterPos3fv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3fv, fn)
#define CALL_RasterPos3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_RasterPos3i, parameters)
#define GET_RasterPos3i(disp) GET_by_offset(disp, _gloffset_RasterPos3i)
#define SET_RasterPos3i(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3i, fn)
#define CALL_RasterPos3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_RasterPos3iv, parameters)
#define GET_RasterPos3iv(disp) GET_by_offset(disp, _gloffset_RasterPos3iv)
#define SET_RasterPos3iv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3iv, fn)
#define CALL_RasterPos3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_RasterPos3s, parameters)
#define GET_RasterPos3s(disp) GET_by_offset(disp, _gloffset_RasterPos3s)
#define SET_RasterPos3s(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3s, fn)
#define CALL_RasterPos3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_RasterPos3sv, parameters)
#define GET_RasterPos3sv(disp) GET_by_offset(disp, _gloffset_RasterPos3sv)
#define SET_RasterPos3sv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos3sv, fn)
#define CALL_RasterPos4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_RasterPos4d, parameters)
#define GET_RasterPos4d(disp) GET_by_offset(disp, _gloffset_RasterPos4d)
#define SET_RasterPos4d(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4d, fn)
#define CALL_RasterPos4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_RasterPos4dv, parameters)
#define GET_RasterPos4dv(disp) GET_by_offset(disp, _gloffset_RasterPos4dv)
#define SET_RasterPos4dv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4dv, fn)
#define CALL_RasterPos4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_RasterPos4f, parameters)
#define GET_RasterPos4f(disp) GET_by_offset(disp, _gloffset_RasterPos4f)
#define SET_RasterPos4f(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4f, fn)
#define CALL_RasterPos4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_RasterPos4fv, parameters)
#define GET_RasterPos4fv(disp) GET_by_offset(disp, _gloffset_RasterPos4fv)
#define SET_RasterPos4fv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4fv, fn)
#define CALL_RasterPos4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_RasterPos4i, parameters)
#define GET_RasterPos4i(disp) GET_by_offset(disp, _gloffset_RasterPos4i)
#define SET_RasterPos4i(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4i, fn)
#define CALL_RasterPos4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_RasterPos4iv, parameters)
#define GET_RasterPos4iv(disp) GET_by_offset(disp, _gloffset_RasterPos4iv)
#define SET_RasterPos4iv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4iv, fn)
#define CALL_RasterPos4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_RasterPos4s, parameters)
#define GET_RasterPos4s(disp) GET_by_offset(disp, _gloffset_RasterPos4s)
#define SET_RasterPos4s(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4s, fn)
#define CALL_RasterPos4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_RasterPos4sv, parameters)
#define GET_RasterPos4sv(disp) GET_by_offset(disp, _gloffset_RasterPos4sv)
#define SET_RasterPos4sv(disp, fn) SET_by_offset(disp, _gloffset_RasterPos4sv, fn)
#define CALL_Rectd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Rectd, parameters)
#define GET_Rectd(disp) GET_by_offset(disp, _gloffset_Rectd)
#define SET_Rectd(disp, fn) SET_by_offset(disp, _gloffset_Rectd, fn)
#define CALL_Rectdv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *, const GLdouble *)), _gloffset_Rectdv, parameters)
#define GET_Rectdv(disp) GET_by_offset(disp, _gloffset_Rectdv)
#define SET_Rectdv(disp, fn) SET_by_offset(disp, _gloffset_Rectdv, fn)
#define CALL_Rectf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Rectf, parameters)
#define GET_Rectf(disp) GET_by_offset(disp, _gloffset_Rectf)
#define SET_Rectf(disp, fn) SET_by_offset(disp, _gloffset_Rectf, fn)
#define CALL_Rectfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *, const GLfloat *)), _gloffset_Rectfv, parameters)
#define GET_Rectfv(disp) GET_by_offset(disp, _gloffset_Rectfv)
#define SET_Rectfv(disp, fn) SET_by_offset(disp, _gloffset_Rectfv, fn)
#define CALL_Recti(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Recti, parameters)
#define GET_Recti(disp) GET_by_offset(disp, _gloffset_Recti)
#define SET_Recti(disp, fn) SET_by_offset(disp, _gloffset_Recti, fn)
#define CALL_Rectiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *, const GLint *)), _gloffset_Rectiv, parameters)
#define GET_Rectiv(disp) GET_by_offset(disp, _gloffset_Rectiv)
#define SET_Rectiv(disp, fn) SET_by_offset(disp, _gloffset_Rectiv, fn)
#define CALL_Rects(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_Rects, parameters)
#define GET_Rects(disp) GET_by_offset(disp, _gloffset_Rects)
#define SET_Rects(disp, fn) SET_by_offset(disp, _gloffset_Rects, fn)
#define CALL_Rectsv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *, const GLshort *)), _gloffset_Rectsv, parameters)
#define GET_Rectsv(disp) GET_by_offset(disp, _gloffset_Rectsv)
#define SET_Rectsv(disp, fn) SET_by_offset(disp, _gloffset_Rectsv, fn)
#define CALL_TexCoord1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_TexCoord1d, parameters)
#define GET_TexCoord1d(disp) GET_by_offset(disp, _gloffset_TexCoord1d)
#define SET_TexCoord1d(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1d, fn)
#define CALL_TexCoord1dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord1dv, parameters)
#define GET_TexCoord1dv(disp) GET_by_offset(disp, _gloffset_TexCoord1dv)
#define SET_TexCoord1dv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1dv, fn)
#define CALL_TexCoord1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_TexCoord1f, parameters)
#define GET_TexCoord1f(disp) GET_by_offset(disp, _gloffset_TexCoord1f)
#define SET_TexCoord1f(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1f, fn)
#define CALL_TexCoord1fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord1fv, parameters)
#define GET_TexCoord1fv(disp) GET_by_offset(disp, _gloffset_TexCoord1fv)
#define SET_TexCoord1fv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1fv, fn)
#define CALL_TexCoord1i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_TexCoord1i, parameters)
#define GET_TexCoord1i(disp) GET_by_offset(disp, _gloffset_TexCoord1i)
#define SET_TexCoord1i(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1i, fn)
#define CALL_TexCoord1iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord1iv, parameters)
#define GET_TexCoord1iv(disp) GET_by_offset(disp, _gloffset_TexCoord1iv)
#define SET_TexCoord1iv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1iv, fn)
#define CALL_TexCoord1s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort)), _gloffset_TexCoord1s, parameters)
#define GET_TexCoord1s(disp) GET_by_offset(disp, _gloffset_TexCoord1s)
#define SET_TexCoord1s(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1s, fn)
#define CALL_TexCoord1sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord1sv, parameters)
#define GET_TexCoord1sv(disp) GET_by_offset(disp, _gloffset_TexCoord1sv)
#define SET_TexCoord1sv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord1sv, fn)
#define CALL_TexCoord2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_TexCoord2d, parameters)
#define GET_TexCoord2d(disp) GET_by_offset(disp, _gloffset_TexCoord2d)
#define SET_TexCoord2d(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2d, fn)
#define CALL_TexCoord2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord2dv, parameters)
#define GET_TexCoord2dv(disp) GET_by_offset(disp, _gloffset_TexCoord2dv)
#define SET_TexCoord2dv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2dv, fn)
#define CALL_TexCoord2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_TexCoord2f, parameters)
#define GET_TexCoord2f(disp) GET_by_offset(disp, _gloffset_TexCoord2f)
#define SET_TexCoord2f(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2f, fn)
#define CALL_TexCoord2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord2fv, parameters)
#define GET_TexCoord2fv(disp) GET_by_offset(disp, _gloffset_TexCoord2fv)
#define SET_TexCoord2fv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2fv, fn)
#define CALL_TexCoord2i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_TexCoord2i, parameters)
#define GET_TexCoord2i(disp) GET_by_offset(disp, _gloffset_TexCoord2i)
#define SET_TexCoord2i(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2i, fn)
#define CALL_TexCoord2iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord2iv, parameters)
#define GET_TexCoord2iv(disp) GET_by_offset(disp, _gloffset_TexCoord2iv)
#define SET_TexCoord2iv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2iv, fn)
#define CALL_TexCoord2s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_TexCoord2s, parameters)
#define GET_TexCoord2s(disp) GET_by_offset(disp, _gloffset_TexCoord2s)
#define SET_TexCoord2s(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2s, fn)
#define CALL_TexCoord2sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord2sv, parameters)
#define GET_TexCoord2sv(disp) GET_by_offset(disp, _gloffset_TexCoord2sv)
#define SET_TexCoord2sv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord2sv, fn)
#define CALL_TexCoord3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_TexCoord3d, parameters)
#define GET_TexCoord3d(disp) GET_by_offset(disp, _gloffset_TexCoord3d)
#define SET_TexCoord3d(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3d, fn)
#define CALL_TexCoord3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord3dv, parameters)
#define GET_TexCoord3dv(disp) GET_by_offset(disp, _gloffset_TexCoord3dv)
#define SET_TexCoord3dv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3dv, fn)
#define CALL_TexCoord3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_TexCoord3f, parameters)
#define GET_TexCoord3f(disp) GET_by_offset(disp, _gloffset_TexCoord3f)
#define SET_TexCoord3f(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3f, fn)
#define CALL_TexCoord3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord3fv, parameters)
#define GET_TexCoord3fv(disp) GET_by_offset(disp, _gloffset_TexCoord3fv)
#define SET_TexCoord3fv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3fv, fn)
#define CALL_TexCoord3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_TexCoord3i, parameters)
#define GET_TexCoord3i(disp) GET_by_offset(disp, _gloffset_TexCoord3i)
#define SET_TexCoord3i(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3i, fn)
#define CALL_TexCoord3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord3iv, parameters)
#define GET_TexCoord3iv(disp) GET_by_offset(disp, _gloffset_TexCoord3iv)
#define SET_TexCoord3iv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3iv, fn)
#define CALL_TexCoord3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_TexCoord3s, parameters)
#define GET_TexCoord3s(disp) GET_by_offset(disp, _gloffset_TexCoord3s)
#define SET_TexCoord3s(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3s, fn)
#define CALL_TexCoord3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord3sv, parameters)
#define GET_TexCoord3sv(disp) GET_by_offset(disp, _gloffset_TexCoord3sv)
#define SET_TexCoord3sv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord3sv, fn)
#define CALL_TexCoord4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_TexCoord4d, parameters)
#define GET_TexCoord4d(disp) GET_by_offset(disp, _gloffset_TexCoord4d)
#define SET_TexCoord4d(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4d, fn)
#define CALL_TexCoord4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord4dv, parameters)
#define GET_TexCoord4dv(disp) GET_by_offset(disp, _gloffset_TexCoord4dv)
#define SET_TexCoord4dv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4dv, fn)
#define CALL_TexCoord4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_TexCoord4f, parameters)
#define GET_TexCoord4f(disp) GET_by_offset(disp, _gloffset_TexCoord4f)
#define SET_TexCoord4f(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4f, fn)
#define CALL_TexCoord4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord4fv, parameters)
#define GET_TexCoord4fv(disp) GET_by_offset(disp, _gloffset_TexCoord4fv)
#define SET_TexCoord4fv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4fv, fn)
#define CALL_TexCoord4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_TexCoord4i, parameters)
#define GET_TexCoord4i(disp) GET_by_offset(disp, _gloffset_TexCoord4i)
#define SET_TexCoord4i(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4i, fn)
#define CALL_TexCoord4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord4iv, parameters)
#define GET_TexCoord4iv(disp) GET_by_offset(disp, _gloffset_TexCoord4iv)
#define SET_TexCoord4iv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4iv, fn)
#define CALL_TexCoord4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_TexCoord4s, parameters)
#define GET_TexCoord4s(disp) GET_by_offset(disp, _gloffset_TexCoord4s)
#define SET_TexCoord4s(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4s, fn)
#define CALL_TexCoord4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord4sv, parameters)
#define GET_TexCoord4sv(disp) GET_by_offset(disp, _gloffset_TexCoord4sv)
#define SET_TexCoord4sv(disp, fn) SET_by_offset(disp, _gloffset_TexCoord4sv, fn)
#define CALL_Vertex2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_Vertex2d, parameters)
#define GET_Vertex2d(disp) GET_by_offset(disp, _gloffset_Vertex2d)
#define SET_Vertex2d(disp, fn) SET_by_offset(disp, _gloffset_Vertex2d, fn)
#define CALL_Vertex2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Vertex2dv, parameters)
#define GET_Vertex2dv(disp) GET_by_offset(disp, _gloffset_Vertex2dv)
#define SET_Vertex2dv(disp, fn) SET_by_offset(disp, _gloffset_Vertex2dv, fn)
#define CALL_Vertex2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_Vertex2f, parameters)
#define GET_Vertex2f(disp) GET_by_offset(disp, _gloffset_Vertex2f)
#define SET_Vertex2f(disp, fn) SET_by_offset(disp, _gloffset_Vertex2f, fn)
#define CALL_Vertex2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Vertex2fv, parameters)
#define GET_Vertex2fv(disp) GET_by_offset(disp, _gloffset_Vertex2fv)
#define SET_Vertex2fv(disp, fn) SET_by_offset(disp, _gloffset_Vertex2fv, fn)
#define CALL_Vertex2i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_Vertex2i, parameters)
#define GET_Vertex2i(disp) GET_by_offset(disp, _gloffset_Vertex2i)
#define SET_Vertex2i(disp, fn) SET_by_offset(disp, _gloffset_Vertex2i, fn)
#define CALL_Vertex2iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Vertex2iv, parameters)
#define GET_Vertex2iv(disp) GET_by_offset(disp, _gloffset_Vertex2iv)
#define SET_Vertex2iv(disp, fn) SET_by_offset(disp, _gloffset_Vertex2iv, fn)
#define CALL_Vertex2s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_Vertex2s, parameters)
#define GET_Vertex2s(disp) GET_by_offset(disp, _gloffset_Vertex2s)
#define SET_Vertex2s(disp, fn) SET_by_offset(disp, _gloffset_Vertex2s, fn)
#define CALL_Vertex2sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Vertex2sv, parameters)
#define GET_Vertex2sv(disp) GET_by_offset(disp, _gloffset_Vertex2sv)
#define SET_Vertex2sv(disp, fn) SET_by_offset(disp, _gloffset_Vertex2sv, fn)
#define CALL_Vertex3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Vertex3d, parameters)
#define GET_Vertex3d(disp) GET_by_offset(disp, _gloffset_Vertex3d)
#define SET_Vertex3d(disp, fn) SET_by_offset(disp, _gloffset_Vertex3d, fn)
#define CALL_Vertex3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Vertex3dv, parameters)
#define GET_Vertex3dv(disp) GET_by_offset(disp, _gloffset_Vertex3dv)
#define SET_Vertex3dv(disp, fn) SET_by_offset(disp, _gloffset_Vertex3dv, fn)
#define CALL_Vertex3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Vertex3f, parameters)
#define GET_Vertex3f(disp) GET_by_offset(disp, _gloffset_Vertex3f)
#define SET_Vertex3f(disp, fn) SET_by_offset(disp, _gloffset_Vertex3f, fn)
#define CALL_Vertex3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Vertex3fv, parameters)
#define GET_Vertex3fv(disp) GET_by_offset(disp, _gloffset_Vertex3fv)
#define SET_Vertex3fv(disp, fn) SET_by_offset(disp, _gloffset_Vertex3fv, fn)
#define CALL_Vertex3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Vertex3i, parameters)
#define GET_Vertex3i(disp) GET_by_offset(disp, _gloffset_Vertex3i)
#define SET_Vertex3i(disp, fn) SET_by_offset(disp, _gloffset_Vertex3i, fn)
#define CALL_Vertex3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Vertex3iv, parameters)
#define GET_Vertex3iv(disp) GET_by_offset(disp, _gloffset_Vertex3iv)
#define SET_Vertex3iv(disp, fn) SET_by_offset(disp, _gloffset_Vertex3iv, fn)
#define CALL_Vertex3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_Vertex3s, parameters)
#define GET_Vertex3s(disp) GET_by_offset(disp, _gloffset_Vertex3s)
#define SET_Vertex3s(disp, fn) SET_by_offset(disp, _gloffset_Vertex3s, fn)
#define CALL_Vertex3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Vertex3sv, parameters)
#define GET_Vertex3sv(disp) GET_by_offset(disp, _gloffset_Vertex3sv)
#define SET_Vertex3sv(disp, fn) SET_by_offset(disp, _gloffset_Vertex3sv, fn)
#define CALL_Vertex4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Vertex4d, parameters)
#define GET_Vertex4d(disp) GET_by_offset(disp, _gloffset_Vertex4d)
#define SET_Vertex4d(disp, fn) SET_by_offset(disp, _gloffset_Vertex4d, fn)
#define CALL_Vertex4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Vertex4dv, parameters)
#define GET_Vertex4dv(disp) GET_by_offset(disp, _gloffset_Vertex4dv)
#define SET_Vertex4dv(disp, fn) SET_by_offset(disp, _gloffset_Vertex4dv, fn)
#define CALL_Vertex4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Vertex4f, parameters)
#define GET_Vertex4f(disp) GET_by_offset(disp, _gloffset_Vertex4f)
#define SET_Vertex4f(disp, fn) SET_by_offset(disp, _gloffset_Vertex4f, fn)
#define CALL_Vertex4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Vertex4fv, parameters)
#define GET_Vertex4fv(disp) GET_by_offset(disp, _gloffset_Vertex4fv)
#define SET_Vertex4fv(disp, fn) SET_by_offset(disp, _gloffset_Vertex4fv, fn)
#define CALL_Vertex4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Vertex4i, parameters)
#define GET_Vertex4i(disp) GET_by_offset(disp, _gloffset_Vertex4i)
#define SET_Vertex4i(disp, fn) SET_by_offset(disp, _gloffset_Vertex4i, fn)
#define CALL_Vertex4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Vertex4iv, parameters)
#define GET_Vertex4iv(disp) GET_by_offset(disp, _gloffset_Vertex4iv)
#define SET_Vertex4iv(disp, fn) SET_by_offset(disp, _gloffset_Vertex4iv, fn)
#define CALL_Vertex4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_Vertex4s, parameters)
#define GET_Vertex4s(disp) GET_by_offset(disp, _gloffset_Vertex4s)
#define SET_Vertex4s(disp, fn) SET_by_offset(disp, _gloffset_Vertex4s, fn)
#define CALL_Vertex4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Vertex4sv, parameters)
#define GET_Vertex4sv(disp) GET_by_offset(disp, _gloffset_Vertex4sv)
#define SET_Vertex4sv(disp, fn) SET_by_offset(disp, _gloffset_Vertex4sv, fn)
#define CALL_ClipPlane(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_ClipPlane, parameters)
#define GET_ClipPlane(disp) GET_by_offset(disp, _gloffset_ClipPlane)
#define SET_ClipPlane(disp, fn) SET_by_offset(disp, _gloffset_ClipPlane, fn)
#define CALL_ColorMaterial(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_ColorMaterial, parameters)
#define GET_ColorMaterial(disp) GET_by_offset(disp, _gloffset_ColorMaterial)
#define SET_ColorMaterial(disp, fn) SET_by_offset(disp, _gloffset_ColorMaterial, fn)
#define CALL_CullFace(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_CullFace, parameters)
#define GET_CullFace(disp) GET_by_offset(disp, _gloffset_CullFace)
#define SET_CullFace(disp, fn) SET_by_offset(disp, _gloffset_CullFace, fn)
#define CALL_Fogf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_Fogf, parameters)
#define GET_Fogf(disp) GET_by_offset(disp, _gloffset_Fogf)
#define SET_Fogf(disp, fn) SET_by_offset(disp, _gloffset_Fogf, fn)
#define CALL_Fogfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_Fogfv, parameters)
#define GET_Fogfv(disp) GET_by_offset(disp, _gloffset_Fogfv)
#define SET_Fogfv(disp, fn) SET_by_offset(disp, _gloffset_Fogfv, fn)
#define CALL_Fogi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_Fogi, parameters)
#define GET_Fogi(disp) GET_by_offset(disp, _gloffset_Fogi)
#define SET_Fogi(disp, fn) SET_by_offset(disp, _gloffset_Fogi, fn)
#define CALL_Fogiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_Fogiv, parameters)
#define GET_Fogiv(disp) GET_by_offset(disp, _gloffset_Fogiv)
#define SET_Fogiv(disp, fn) SET_by_offset(disp, _gloffset_Fogiv, fn)
#define CALL_FrontFace(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_FrontFace, parameters)
#define GET_FrontFace(disp) GET_by_offset(disp, _gloffset_FrontFace)
#define SET_FrontFace(disp, fn) SET_by_offset(disp, _gloffset_FrontFace, fn)
#define CALL_Hint(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_Hint, parameters)
#define GET_Hint(disp) GET_by_offset(disp, _gloffset_Hint)
#define SET_Hint(disp, fn) SET_by_offset(disp, _gloffset_Hint, fn)
#define CALL_Lightf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_Lightf, parameters)
#define GET_Lightf(disp) GET_by_offset(disp, _gloffset_Lightf)
#define SET_Lightf(disp, fn) SET_by_offset(disp, _gloffset_Lightf, fn)
#define CALL_Lightfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_Lightfv, parameters)
#define GET_Lightfv(disp) GET_by_offset(disp, _gloffset_Lightfv)
#define SET_Lightfv(disp, fn) SET_by_offset(disp, _gloffset_Lightfv, fn)
#define CALL_Lighti(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_Lighti, parameters)
#define GET_Lighti(disp) GET_by_offset(disp, _gloffset_Lighti)
#define SET_Lighti(disp, fn) SET_by_offset(disp, _gloffset_Lighti, fn)
#define CALL_Lightiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_Lightiv, parameters)
#define GET_Lightiv(disp) GET_by_offset(disp, _gloffset_Lightiv)
#define SET_Lightiv(disp, fn) SET_by_offset(disp, _gloffset_Lightiv, fn)
#define CALL_LightModelf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_LightModelf, parameters)
#define GET_LightModelf(disp) GET_by_offset(disp, _gloffset_LightModelf)
#define SET_LightModelf(disp, fn) SET_by_offset(disp, _gloffset_LightModelf, fn)
#define CALL_LightModelfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_LightModelfv, parameters)
#define GET_LightModelfv(disp) GET_by_offset(disp, _gloffset_LightModelfv)
#define SET_LightModelfv(disp, fn) SET_by_offset(disp, _gloffset_LightModelfv, fn)
#define CALL_LightModeli(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_LightModeli, parameters)
#define GET_LightModeli(disp) GET_by_offset(disp, _gloffset_LightModeli)
#define SET_LightModeli(disp, fn) SET_by_offset(disp, _gloffset_LightModeli, fn)
#define CALL_LightModeliv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_LightModeliv, parameters)
#define GET_LightModeliv(disp) GET_by_offset(disp, _gloffset_LightModeliv)
#define SET_LightModeliv(disp, fn) SET_by_offset(disp, _gloffset_LightModeliv, fn)
#define CALL_LineStipple(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLushort)), _gloffset_LineStipple, parameters)
#define GET_LineStipple(disp) GET_by_offset(disp, _gloffset_LineStipple)
#define SET_LineStipple(disp, fn) SET_by_offset(disp, _gloffset_LineStipple, fn)
#define CALL_LineWidth(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_LineWidth, parameters)
#define GET_LineWidth(disp) GET_by_offset(disp, _gloffset_LineWidth)
#define SET_LineWidth(disp, fn) SET_by_offset(disp, _gloffset_LineWidth, fn)
#define CALL_Materialf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_Materialf, parameters)
#define GET_Materialf(disp) GET_by_offset(disp, _gloffset_Materialf)
#define SET_Materialf(disp, fn) SET_by_offset(disp, _gloffset_Materialf, fn)
#define CALL_Materialfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_Materialfv, parameters)
#define GET_Materialfv(disp) GET_by_offset(disp, _gloffset_Materialfv)
#define SET_Materialfv(disp, fn) SET_by_offset(disp, _gloffset_Materialfv, fn)
#define CALL_Materiali(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_Materiali, parameters)
#define GET_Materiali(disp) GET_by_offset(disp, _gloffset_Materiali)
#define SET_Materiali(disp, fn) SET_by_offset(disp, _gloffset_Materiali, fn)
#define CALL_Materialiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_Materialiv, parameters)
#define GET_Materialiv(disp) GET_by_offset(disp, _gloffset_Materialiv)
#define SET_Materialiv(disp, fn) SET_by_offset(disp, _gloffset_Materialiv, fn)
#define CALL_PointSize(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_PointSize, parameters)
#define GET_PointSize(disp) GET_by_offset(disp, _gloffset_PointSize)
#define SET_PointSize(disp, fn) SET_by_offset(disp, _gloffset_PointSize, fn)
#define CALL_PolygonMode(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_PolygonMode, parameters)
#define GET_PolygonMode(disp) GET_by_offset(disp, _gloffset_PolygonMode)
#define SET_PolygonMode(disp, fn) SET_by_offset(disp, _gloffset_PolygonMode, fn)
#define CALL_PolygonStipple(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_PolygonStipple, parameters)
#define GET_PolygonStipple(disp) GET_by_offset(disp, _gloffset_PolygonStipple)
#define SET_PolygonStipple(disp, fn) SET_by_offset(disp, _gloffset_PolygonStipple, fn)
#define CALL_Scissor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei)), _gloffset_Scissor, parameters)
#define GET_Scissor(disp) GET_by_offset(disp, _gloffset_Scissor)
#define SET_Scissor(disp, fn) SET_by_offset(disp, _gloffset_Scissor, fn)
#define CALL_ShadeModel(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ShadeModel, parameters)
#define GET_ShadeModel(disp) GET_by_offset(disp, _gloffset_ShadeModel)
#define SET_ShadeModel(disp, fn) SET_by_offset(disp, _gloffset_ShadeModel, fn)
#define CALL_TexParameterf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_TexParameterf, parameters)
#define GET_TexParameterf(disp) GET_by_offset(disp, _gloffset_TexParameterf)
#define SET_TexParameterf(disp, fn) SET_by_offset(disp, _gloffset_TexParameterf, fn)
#define CALL_TexParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_TexParameterfv, parameters)
#define GET_TexParameterfv(disp) GET_by_offset(disp, _gloffset_TexParameterfv)
#define SET_TexParameterfv(disp, fn) SET_by_offset(disp, _gloffset_TexParameterfv, fn)
#define CALL_TexParameteri(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_TexParameteri, parameters)
#define GET_TexParameteri(disp) GET_by_offset(disp, _gloffset_TexParameteri)
#define SET_TexParameteri(disp, fn) SET_by_offset(disp, _gloffset_TexParameteri, fn)
#define CALL_TexParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexParameteriv, parameters)
#define GET_TexParameteriv(disp) GET_by_offset(disp, _gloffset_TexParameteriv)
#define SET_TexParameteriv(disp, fn) SET_by_offset(disp, _gloffset_TexParameteriv, fn)
#define CALL_TexImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *)), _gloffset_TexImage1D, parameters)
#define GET_TexImage1D(disp) GET_by_offset(disp, _gloffset_TexImage1D)
#define SET_TexImage1D(disp, fn) SET_by_offset(disp, _gloffset_TexImage1D, fn)
#define CALL_TexImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)), _gloffset_TexImage2D, parameters)
#define GET_TexImage2D(disp) GET_by_offset(disp, _gloffset_TexImage2D)
#define SET_TexImage2D(disp, fn) SET_by_offset(disp, _gloffset_TexImage2D, fn)
#define CALL_TexEnvf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_TexEnvf, parameters)
#define GET_TexEnvf(disp) GET_by_offset(disp, _gloffset_TexEnvf)
#define SET_TexEnvf(disp, fn) SET_by_offset(disp, _gloffset_TexEnvf, fn)
#define CALL_TexEnvfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_TexEnvfv, parameters)
#define GET_TexEnvfv(disp) GET_by_offset(disp, _gloffset_TexEnvfv)
#define SET_TexEnvfv(disp, fn) SET_by_offset(disp, _gloffset_TexEnvfv, fn)
#define CALL_TexEnvi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_TexEnvi, parameters)
#define GET_TexEnvi(disp) GET_by_offset(disp, _gloffset_TexEnvi)
#define SET_TexEnvi(disp, fn) SET_by_offset(disp, _gloffset_TexEnvi, fn)
#define CALL_TexEnviv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexEnviv, parameters)
#define GET_TexEnviv(disp) GET_by_offset(disp, _gloffset_TexEnviv)
#define SET_TexEnviv(disp, fn) SET_by_offset(disp, _gloffset_TexEnviv, fn)
#define CALL_TexGend(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLdouble)), _gloffset_TexGend, parameters)
#define GET_TexGend(disp) GET_by_offset(disp, _gloffset_TexGend)
#define SET_TexGend(disp, fn) SET_by_offset(disp, _gloffset_TexGend, fn)
#define CALL_TexGendv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLdouble *)), _gloffset_TexGendv, parameters)
#define GET_TexGendv(disp) GET_by_offset(disp, _gloffset_TexGendv)
#define SET_TexGendv(disp, fn) SET_by_offset(disp, _gloffset_TexGendv, fn)
#define CALL_TexGenf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_TexGenf, parameters)
#define GET_TexGenf(disp) GET_by_offset(disp, _gloffset_TexGenf)
#define SET_TexGenf(disp, fn) SET_by_offset(disp, _gloffset_TexGenf, fn)
#define CALL_TexGenfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_TexGenfv, parameters)
#define GET_TexGenfv(disp) GET_by_offset(disp, _gloffset_TexGenfv)
#define SET_TexGenfv(disp, fn) SET_by_offset(disp, _gloffset_TexGenfv, fn)
#define CALL_TexGeni(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_TexGeni, parameters)
#define GET_TexGeni(disp) GET_by_offset(disp, _gloffset_TexGeni)
#define SET_TexGeni(disp, fn) SET_by_offset(disp, _gloffset_TexGeni, fn)
#define CALL_TexGeniv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexGeniv, parameters)
#define GET_TexGeniv(disp) GET_by_offset(disp, _gloffset_TexGeniv)
#define SET_TexGeniv(disp, fn) SET_by_offset(disp, _gloffset_TexGeniv, fn)
#define CALL_FeedbackBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLenum, GLfloat *)), _gloffset_FeedbackBuffer, parameters)
#define GET_FeedbackBuffer(disp) GET_by_offset(disp, _gloffset_FeedbackBuffer)
#define SET_FeedbackBuffer(disp, fn) SET_by_offset(disp, _gloffset_FeedbackBuffer, fn)
#define CALL_SelectBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_SelectBuffer, parameters)
#define GET_SelectBuffer(disp) GET_by_offset(disp, _gloffset_SelectBuffer)
#define SET_SelectBuffer(disp, fn) SET_by_offset(disp, _gloffset_SelectBuffer, fn)
#define CALL_RenderMode(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLenum)), _gloffset_RenderMode, parameters)
#define GET_RenderMode(disp) GET_by_offset(disp, _gloffset_RenderMode)
#define SET_RenderMode(disp, fn) SET_by_offset(disp, _gloffset_RenderMode, fn)
#define CALL_InitNames(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_InitNames, parameters)
#define GET_InitNames(disp) GET_by_offset(disp, _gloffset_InitNames)
#define SET_InitNames(disp, fn) SET_by_offset(disp, _gloffset_InitNames, fn)
#define CALL_LoadName(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_LoadName, parameters)
#define GET_LoadName(disp) GET_by_offset(disp, _gloffset_LoadName)
#define SET_LoadName(disp, fn) SET_by_offset(disp, _gloffset_LoadName, fn)
#define CALL_PassThrough(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_PassThrough, parameters)
#define GET_PassThrough(disp) GET_by_offset(disp, _gloffset_PassThrough)
#define SET_PassThrough(disp, fn) SET_by_offset(disp, _gloffset_PassThrough, fn)
#define CALL_PopName(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopName, parameters)
#define GET_PopName(disp) GET_by_offset(disp, _gloffset_PopName)
#define SET_PopName(disp, fn) SET_by_offset(disp, _gloffset_PopName, fn)
#define CALL_PushName(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_PushName, parameters)
#define GET_PushName(disp) GET_by_offset(disp, _gloffset_PushName)
#define SET_PushName(disp, fn) SET_by_offset(disp, _gloffset_PushName, fn)
#define CALL_DrawBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_DrawBuffer, parameters)
#define GET_DrawBuffer(disp) GET_by_offset(disp, _gloffset_DrawBuffer)
#define SET_DrawBuffer(disp, fn) SET_by_offset(disp, _gloffset_DrawBuffer, fn)
#define CALL_Clear(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbitfield)), _gloffset_Clear, parameters)
#define GET_Clear(disp) GET_by_offset(disp, _gloffset_Clear)
#define SET_Clear(disp, fn) SET_by_offset(disp, _gloffset_Clear, fn)
#define CALL_ClearAccum(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ClearAccum, parameters)
#define GET_ClearAccum(disp) GET_by_offset(disp, _gloffset_ClearAccum)
#define SET_ClearAccum(disp, fn) SET_by_offset(disp, _gloffset_ClearAccum, fn)
#define CALL_ClearIndex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_ClearIndex, parameters)
#define GET_ClearIndex(disp) GET_by_offset(disp, _gloffset_ClearIndex)
#define SET_ClearIndex(disp, fn) SET_by_offset(disp, _gloffset_ClearIndex, fn)
#define CALL_ClearColor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLclampf, GLclampf, GLclampf)), _gloffset_ClearColor, parameters)
#define GET_ClearColor(disp) GET_by_offset(disp, _gloffset_ClearColor)
#define SET_ClearColor(disp, fn) SET_by_offset(disp, _gloffset_ClearColor, fn)
#define CALL_ClearStencil(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_ClearStencil, parameters)
#define GET_ClearStencil(disp) GET_by_offset(disp, _gloffset_ClearStencil)
#define SET_ClearStencil(disp, fn) SET_by_offset(disp, _gloffset_ClearStencil, fn)
#define CALL_ClearDepth(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampd)), _gloffset_ClearDepth, parameters)
#define GET_ClearDepth(disp) GET_by_offset(disp, _gloffset_ClearDepth)
#define SET_ClearDepth(disp, fn) SET_by_offset(disp, _gloffset_ClearDepth, fn)
#define CALL_StencilMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_StencilMask, parameters)
#define GET_StencilMask(disp) GET_by_offset(disp, _gloffset_StencilMask)
#define SET_StencilMask(disp, fn) SET_by_offset(disp, _gloffset_StencilMask, fn)
#define CALL_ColorMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLboolean, GLboolean, GLboolean, GLboolean)), _gloffset_ColorMask, parameters)
#define GET_ColorMask(disp) GET_by_offset(disp, _gloffset_ColorMask)
#define SET_ColorMask(disp, fn) SET_by_offset(disp, _gloffset_ColorMask, fn)
#define CALL_DepthMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLboolean)), _gloffset_DepthMask, parameters)
#define GET_DepthMask(disp) GET_by_offset(disp, _gloffset_DepthMask)
#define SET_DepthMask(disp, fn) SET_by_offset(disp, _gloffset_DepthMask, fn)
#define CALL_IndexMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_IndexMask, parameters)
#define GET_IndexMask(disp) GET_by_offset(disp, _gloffset_IndexMask)
#define SET_IndexMask(disp, fn) SET_by_offset(disp, _gloffset_IndexMask, fn)
#define CALL_Accum(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_Accum, parameters)
#define GET_Accum(disp) GET_by_offset(disp, _gloffset_Accum)
#define SET_Accum(disp, fn) SET_by_offset(disp, _gloffset_Accum, fn)
#define CALL_Disable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_Disable, parameters)
#define GET_Disable(disp) GET_by_offset(disp, _gloffset_Disable)
#define SET_Disable(disp, fn) SET_by_offset(disp, _gloffset_Disable, fn)
#define CALL_Enable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_Enable, parameters)
#define GET_Enable(disp) GET_by_offset(disp, _gloffset_Enable)
#define SET_Enable(disp, fn) SET_by_offset(disp, _gloffset_Enable, fn)
#define CALL_Finish(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_Finish, parameters)
#define GET_Finish(disp) GET_by_offset(disp, _gloffset_Finish)
#define SET_Finish(disp, fn) SET_by_offset(disp, _gloffset_Finish, fn)
#define CALL_Flush(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_Flush, parameters)
#define GET_Flush(disp) GET_by_offset(disp, _gloffset_Flush)
#define SET_Flush(disp, fn) SET_by_offset(disp, _gloffset_Flush, fn)
#define CALL_PopAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopAttrib, parameters)
#define GET_PopAttrib(disp) GET_by_offset(disp, _gloffset_PopAttrib)
#define SET_PopAttrib(disp, fn) SET_by_offset(disp, _gloffset_PopAttrib, fn)
#define CALL_PushAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbitfield)), _gloffset_PushAttrib, parameters)
#define GET_PushAttrib(disp) GET_by_offset(disp, _gloffset_PushAttrib)
#define SET_PushAttrib(disp, fn) SET_by_offset(disp, _gloffset_PushAttrib, fn)
#define CALL_Map1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *)), _gloffset_Map1d, parameters)
#define GET_Map1d(disp) GET_by_offset(disp, _gloffset_Map1d)
#define SET_Map1d(disp, fn) SET_by_offset(disp, _gloffset_Map1d, fn)
#define CALL_Map1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *)), _gloffset_Map1f, parameters)
#define GET_Map1f(disp) GET_by_offset(disp, _gloffset_Map1f)
#define SET_Map1f(disp, fn) SET_by_offset(disp, _gloffset_Map1f, fn)
#define CALL_Map2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *)), _gloffset_Map2d, parameters)
#define GET_Map2d(disp) GET_by_offset(disp, _gloffset_Map2d)
#define SET_Map2d(disp, fn) SET_by_offset(disp, _gloffset_Map2d, fn)
#define CALL_Map2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *)), _gloffset_Map2f, parameters)
#define GET_Map2f(disp) GET_by_offset(disp, _gloffset_Map2f)
#define SET_Map2f(disp, fn) SET_by_offset(disp, _gloffset_Map2f, fn)
#define CALL_MapGrid1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLdouble, GLdouble)), _gloffset_MapGrid1d, parameters)
#define GET_MapGrid1d(disp) GET_by_offset(disp, _gloffset_MapGrid1d)
#define SET_MapGrid1d(disp, fn) SET_by_offset(disp, _gloffset_MapGrid1d, fn)
#define CALL_MapGrid1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat)), _gloffset_MapGrid1f, parameters)
#define GET_MapGrid1f(disp) GET_by_offset(disp, _gloffset_MapGrid1f)
#define SET_MapGrid1f(disp, fn) SET_by_offset(disp, _gloffset_MapGrid1f, fn)
#define CALL_MapGrid2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble)), _gloffset_MapGrid2d, parameters)
#define GET_MapGrid2d(disp) GET_by_offset(disp, _gloffset_MapGrid2d)
#define SET_MapGrid2d(disp, fn) SET_by_offset(disp, _gloffset_MapGrid2d, fn)
#define CALL_MapGrid2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat)), _gloffset_MapGrid2f, parameters)
#define GET_MapGrid2f(disp) GET_by_offset(disp, _gloffset_MapGrid2f)
#define SET_MapGrid2f(disp, fn) SET_by_offset(disp, _gloffset_MapGrid2f, fn)
#define CALL_EvalCoord1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_EvalCoord1d, parameters)
#define GET_EvalCoord1d(disp) GET_by_offset(disp, _gloffset_EvalCoord1d)
#define SET_EvalCoord1d(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord1d, fn)
#define CALL_EvalCoord1dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_EvalCoord1dv, parameters)
#define GET_EvalCoord1dv(disp) GET_by_offset(disp, _gloffset_EvalCoord1dv)
#define SET_EvalCoord1dv(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord1dv, fn)
#define CALL_EvalCoord1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_EvalCoord1f, parameters)
#define GET_EvalCoord1f(disp) GET_by_offset(disp, _gloffset_EvalCoord1f)
#define SET_EvalCoord1f(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord1f, fn)
#define CALL_EvalCoord1fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_EvalCoord1fv, parameters)
#define GET_EvalCoord1fv(disp) GET_by_offset(disp, _gloffset_EvalCoord1fv)
#define SET_EvalCoord1fv(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord1fv, fn)
#define CALL_EvalCoord2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_EvalCoord2d, parameters)
#define GET_EvalCoord2d(disp) GET_by_offset(disp, _gloffset_EvalCoord2d)
#define SET_EvalCoord2d(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord2d, fn)
#define CALL_EvalCoord2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_EvalCoord2dv, parameters)
#define GET_EvalCoord2dv(disp) GET_by_offset(disp, _gloffset_EvalCoord2dv)
#define SET_EvalCoord2dv(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord2dv, fn)
#define CALL_EvalCoord2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_EvalCoord2f, parameters)
#define GET_EvalCoord2f(disp) GET_by_offset(disp, _gloffset_EvalCoord2f)
#define SET_EvalCoord2f(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord2f, fn)
#define CALL_EvalCoord2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_EvalCoord2fv, parameters)
#define GET_EvalCoord2fv(disp) GET_by_offset(disp, _gloffset_EvalCoord2fv)
#define SET_EvalCoord2fv(disp, fn) SET_by_offset(disp, _gloffset_EvalCoord2fv, fn)
#define CALL_EvalMesh1(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint)), _gloffset_EvalMesh1, parameters)
#define GET_EvalMesh1(disp) GET_by_offset(disp, _gloffset_EvalMesh1)
#define SET_EvalMesh1(disp, fn) SET_by_offset(disp, _gloffset_EvalMesh1, fn)
#define CALL_EvalPoint1(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_EvalPoint1, parameters)
#define GET_EvalPoint1(disp) GET_by_offset(disp, _gloffset_EvalPoint1)
#define SET_EvalPoint1(disp, fn) SET_by_offset(disp, _gloffset_EvalPoint1, fn)
#define CALL_EvalMesh2(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint)), _gloffset_EvalMesh2, parameters)
#define GET_EvalMesh2(disp) GET_by_offset(disp, _gloffset_EvalMesh2)
#define SET_EvalMesh2(disp, fn) SET_by_offset(disp, _gloffset_EvalMesh2, fn)
#define CALL_EvalPoint2(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_EvalPoint2, parameters)
#define GET_EvalPoint2(disp) GET_by_offset(disp, _gloffset_EvalPoint2)
#define SET_EvalPoint2(disp, fn) SET_by_offset(disp, _gloffset_EvalPoint2, fn)
#define CALL_AlphaFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLclampf)), _gloffset_AlphaFunc, parameters)
#define GET_AlphaFunc(disp) GET_by_offset(disp, _gloffset_AlphaFunc)
#define SET_AlphaFunc(disp, fn) SET_by_offset(disp, _gloffset_AlphaFunc, fn)
#define CALL_BlendFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_BlendFunc, parameters)
#define GET_BlendFunc(disp) GET_by_offset(disp, _gloffset_BlendFunc)
#define SET_BlendFunc(disp, fn) SET_by_offset(disp, _gloffset_BlendFunc, fn)
#define CALL_LogicOp(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_LogicOp, parameters)
#define GET_LogicOp(disp) GET_by_offset(disp, _gloffset_LogicOp)
#define SET_LogicOp(disp, fn) SET_by_offset(disp, _gloffset_LogicOp, fn)
#define CALL_StencilFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLuint)), _gloffset_StencilFunc, parameters)
#define GET_StencilFunc(disp) GET_by_offset(disp, _gloffset_StencilFunc)
#define SET_StencilFunc(disp, fn) SET_by_offset(disp, _gloffset_StencilFunc, fn)
#define CALL_StencilOp(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum)), _gloffset_StencilOp, parameters)
#define GET_StencilOp(disp) GET_by_offset(disp, _gloffset_StencilOp)
#define SET_StencilOp(disp, fn) SET_by_offset(disp, _gloffset_StencilOp, fn)
#define CALL_DepthFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_DepthFunc, parameters)
#define GET_DepthFunc(disp) GET_by_offset(disp, _gloffset_DepthFunc)
#define SET_DepthFunc(disp, fn) SET_by_offset(disp, _gloffset_DepthFunc, fn)
#define CALL_PixelZoom(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_PixelZoom, parameters)
#define GET_PixelZoom(disp) GET_by_offset(disp, _gloffset_PixelZoom)
#define SET_PixelZoom(disp, fn) SET_by_offset(disp, _gloffset_PixelZoom, fn)
#define CALL_PixelTransferf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PixelTransferf, parameters)
#define GET_PixelTransferf(disp) GET_by_offset(disp, _gloffset_PixelTransferf)
#define SET_PixelTransferf(disp, fn) SET_by_offset(disp, _gloffset_PixelTransferf, fn)
#define CALL_PixelTransferi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PixelTransferi, parameters)
#define GET_PixelTransferi(disp) GET_by_offset(disp, _gloffset_PixelTransferi)
#define SET_PixelTransferi(disp, fn) SET_by_offset(disp, _gloffset_PixelTransferi, fn)
#define CALL_PixelStoref(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PixelStoref, parameters)
#define GET_PixelStoref(disp) GET_by_offset(disp, _gloffset_PixelStoref)
#define SET_PixelStoref(disp, fn) SET_by_offset(disp, _gloffset_PixelStoref, fn)
#define CALL_PixelStorei(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PixelStorei, parameters)
#define GET_PixelStorei(disp) GET_by_offset(disp, _gloffset_PixelStorei)
#define SET_PixelStorei(disp, fn) SET_by_offset(disp, _gloffset_PixelStorei, fn)
#define CALL_PixelMapfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLfloat *)), _gloffset_PixelMapfv, parameters)
#define GET_PixelMapfv(disp) GET_by_offset(disp, _gloffset_PixelMapfv)
#define SET_PixelMapfv(disp, fn) SET_by_offset(disp, _gloffset_PixelMapfv, fn)
#define CALL_PixelMapuiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLuint *)), _gloffset_PixelMapuiv, parameters)
#define GET_PixelMapuiv(disp) GET_by_offset(disp, _gloffset_PixelMapuiv)
#define SET_PixelMapuiv(disp, fn) SET_by_offset(disp, _gloffset_PixelMapuiv, fn)
#define CALL_PixelMapusv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLushort *)), _gloffset_PixelMapusv, parameters)
#define GET_PixelMapusv(disp) GET_by_offset(disp, _gloffset_PixelMapusv)
#define SET_PixelMapusv(disp, fn) SET_by_offset(disp, _gloffset_PixelMapusv, fn)
#define CALL_ReadBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ReadBuffer, parameters)
#define GET_ReadBuffer(disp) GET_by_offset(disp, _gloffset_ReadBuffer)
#define SET_ReadBuffer(disp, fn) SET_by_offset(disp, _gloffset_ReadBuffer, fn)
#define CALL_CopyPixels(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei, GLenum)), _gloffset_CopyPixels, parameters)
#define GET_CopyPixels(disp) GET_by_offset(disp, _gloffset_CopyPixels)
#define SET_CopyPixels(disp, fn) SET_by_offset(disp, _gloffset_CopyPixels, fn)
#define CALL_ReadPixels(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *)), _gloffset_ReadPixels, parameters)
#define GET_ReadPixels(disp) GET_by_offset(disp, _gloffset_ReadPixels)
#define SET_ReadPixels(disp, fn) SET_by_offset(disp, _gloffset_ReadPixels, fn)
#define CALL_DrawPixels(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_DrawPixels, parameters)
#define GET_DrawPixels(disp) GET_by_offset(disp, _gloffset_DrawPixels)
#define SET_DrawPixels(disp, fn) SET_by_offset(disp, _gloffset_DrawPixels, fn)
#define CALL_GetBooleanv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean *)), _gloffset_GetBooleanv, parameters)
#define GET_GetBooleanv(disp) GET_by_offset(disp, _gloffset_GetBooleanv)
#define SET_GetBooleanv(disp, fn) SET_by_offset(disp, _gloffset_GetBooleanv, fn)
#define CALL_GetClipPlane(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble *)), _gloffset_GetClipPlane, parameters)
#define GET_GetClipPlane(disp) GET_by_offset(disp, _gloffset_GetClipPlane)
#define SET_GetClipPlane(disp, fn) SET_by_offset(disp, _gloffset_GetClipPlane, fn)
#define CALL_GetDoublev(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble *)), _gloffset_GetDoublev, parameters)
#define GET_GetDoublev(disp) GET_by_offset(disp, _gloffset_GetDoublev)
#define SET_GetDoublev(disp, fn) SET_by_offset(disp, _gloffset_GetDoublev, fn)
#define CALL_GetError(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(void)), _gloffset_GetError, parameters)
#define GET_GetError(disp) GET_by_offset(disp, _gloffset_GetError)
#define SET_GetError(disp, fn) SET_by_offset(disp, _gloffset_GetError, fn)
#define CALL_GetFloatv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetFloatv, parameters)
#define GET_GetFloatv(disp) GET_by_offset(disp, _gloffset_GetFloatv)
#define SET_GetFloatv(disp, fn) SET_by_offset(disp, _gloffset_GetFloatv, fn)
#define CALL_GetIntegerv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *)), _gloffset_GetIntegerv, parameters)
#define GET_GetIntegerv(disp) GET_by_offset(disp, _gloffset_GetIntegerv)
#define SET_GetIntegerv(disp, fn) SET_by_offset(disp, _gloffset_GetIntegerv, fn)
#define CALL_GetLightfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetLightfv, parameters)
#define GET_GetLightfv(disp) GET_by_offset(disp, _gloffset_GetLightfv)
#define SET_GetLightfv(disp, fn) SET_by_offset(disp, _gloffset_GetLightfv, fn)
#define CALL_GetLightiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetLightiv, parameters)
#define GET_GetLightiv(disp) GET_by_offset(disp, _gloffset_GetLightiv)
#define SET_GetLightiv(disp, fn) SET_by_offset(disp, _gloffset_GetLightiv, fn)
#define CALL_GetMapdv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLdouble *)), _gloffset_GetMapdv, parameters)
#define GET_GetMapdv(disp) GET_by_offset(disp, _gloffset_GetMapdv)
#define SET_GetMapdv(disp, fn) SET_by_offset(disp, _gloffset_GetMapdv, fn)
#define CALL_GetMapfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetMapfv, parameters)
#define GET_GetMapfv(disp) GET_by_offset(disp, _gloffset_GetMapfv)
#define SET_GetMapfv(disp, fn) SET_by_offset(disp, _gloffset_GetMapfv, fn)
#define CALL_GetMapiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetMapiv, parameters)
#define GET_GetMapiv(disp) GET_by_offset(disp, _gloffset_GetMapiv)
#define SET_GetMapiv(disp, fn) SET_by_offset(disp, _gloffset_GetMapiv, fn)
#define CALL_GetMaterialfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetMaterialfv, parameters)
#define GET_GetMaterialfv(disp) GET_by_offset(disp, _gloffset_GetMaterialfv)
#define SET_GetMaterialfv(disp, fn) SET_by_offset(disp, _gloffset_GetMaterialfv, fn)
#define CALL_GetMaterialiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetMaterialiv, parameters)
#define GET_GetMaterialiv(disp) GET_by_offset(disp, _gloffset_GetMaterialiv)
#define SET_GetMaterialiv(disp, fn) SET_by_offset(disp, _gloffset_GetMaterialiv, fn)
#define CALL_GetPixelMapfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetPixelMapfv, parameters)
#define GET_GetPixelMapfv(disp) GET_by_offset(disp, _gloffset_GetPixelMapfv)
#define SET_GetPixelMapfv(disp, fn) SET_by_offset(disp, _gloffset_GetPixelMapfv, fn)
#define CALL_GetPixelMapuiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint *)), _gloffset_GetPixelMapuiv, parameters)
#define GET_GetPixelMapuiv(disp) GET_by_offset(disp, _gloffset_GetPixelMapuiv)
#define SET_GetPixelMapuiv(disp, fn) SET_by_offset(disp, _gloffset_GetPixelMapuiv, fn)
#define CALL_GetPixelMapusv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLushort *)), _gloffset_GetPixelMapusv, parameters)
#define GET_GetPixelMapusv(disp) GET_by_offset(disp, _gloffset_GetPixelMapusv)
#define SET_GetPixelMapusv(disp, fn) SET_by_offset(disp, _gloffset_GetPixelMapusv, fn)
#define CALL_GetPolygonStipple(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte *)), _gloffset_GetPolygonStipple, parameters)
#define GET_GetPolygonStipple(disp) GET_by_offset(disp, _gloffset_GetPolygonStipple)
#define SET_GetPolygonStipple(disp, fn) SET_by_offset(disp, _gloffset_GetPolygonStipple, fn)
#define CALL_GetString(disp, parameters) CALL_by_offset(disp, (const GLubyte * (GLAPIENTRYP)(GLenum)), _gloffset_GetString, parameters)
#define GET_GetString(disp) GET_by_offset(disp, _gloffset_GetString)
#define SET_GetString(disp, fn) SET_by_offset(disp, _gloffset_GetString, fn)
#define CALL_GetTexEnvfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetTexEnvfv, parameters)
#define GET_GetTexEnvfv(disp) GET_by_offset(disp, _gloffset_GetTexEnvfv)
#define SET_GetTexEnvfv(disp, fn) SET_by_offset(disp, _gloffset_GetTexEnvfv, fn)
#define CALL_GetTexEnviv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexEnviv, parameters)
#define GET_GetTexEnviv(disp) GET_by_offset(disp, _gloffset_GetTexEnviv)
#define SET_GetTexEnviv(disp, fn) SET_by_offset(disp, _gloffset_GetTexEnviv, fn)
#define CALL_GetTexGendv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLdouble *)), _gloffset_GetTexGendv, parameters)
#define GET_GetTexGendv(disp) GET_by_offset(disp, _gloffset_GetTexGendv)
#define SET_GetTexGendv(disp, fn) SET_by_offset(disp, _gloffset_GetTexGendv, fn)
#define CALL_GetTexGenfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetTexGenfv, parameters)
#define GET_GetTexGenfv(disp) GET_by_offset(disp, _gloffset_GetTexGenfv)
#define SET_GetTexGenfv(disp, fn) SET_by_offset(disp, _gloffset_GetTexGenfv, fn)
#define CALL_GetTexGeniv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexGeniv, parameters)
#define GET_GetTexGeniv(disp) GET_by_offset(disp, _gloffset_GetTexGeniv)
#define SET_GetTexGeniv(disp, fn) SET_by_offset(disp, _gloffset_GetTexGeniv, fn)
#define CALL_GetTexImage(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLenum, GLvoid *)), _gloffset_GetTexImage, parameters)
#define GET_GetTexImage(disp) GET_by_offset(disp, _gloffset_GetTexImage)
#define SET_GetTexImage(disp, fn) SET_by_offset(disp, _gloffset_GetTexImage, fn)
#define CALL_GetTexParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetTexParameterfv, parameters)
#define GET_GetTexParameterfv(disp) GET_by_offset(disp, _gloffset_GetTexParameterfv)
#define SET_GetTexParameterfv(disp, fn) SET_by_offset(disp, _gloffset_GetTexParameterfv, fn)
#define CALL_GetTexParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexParameteriv, parameters)
#define GET_GetTexParameteriv(disp) GET_by_offset(disp, _gloffset_GetTexParameteriv)
#define SET_GetTexParameteriv(disp, fn) SET_by_offset(disp, _gloffset_GetTexParameteriv, fn)
#define CALL_GetTexLevelParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLfloat *)), _gloffset_GetTexLevelParameterfv, parameters)
#define GET_GetTexLevelParameterfv(disp) GET_by_offset(disp, _gloffset_GetTexLevelParameterfv)
#define SET_GetTexLevelParameterfv(disp, fn) SET_by_offset(disp, _gloffset_GetTexLevelParameterfv, fn)
#define CALL_GetTexLevelParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLint *)), _gloffset_GetTexLevelParameteriv, parameters)
#define GET_GetTexLevelParameteriv(disp) GET_by_offset(disp, _gloffset_GetTexLevelParameteriv)
#define SET_GetTexLevelParameteriv(disp, fn) SET_by_offset(disp, _gloffset_GetTexLevelParameteriv, fn)
#define CALL_IsEnabled(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLenum)), _gloffset_IsEnabled, parameters)
#define GET_IsEnabled(disp) GET_by_offset(disp, _gloffset_IsEnabled)
#define SET_IsEnabled(disp, fn) SET_by_offset(disp, _gloffset_IsEnabled, fn)
#define CALL_IsList(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsList, parameters)
#define GET_IsList(disp) GET_by_offset(disp, _gloffset_IsList)
#define SET_IsList(disp, fn) SET_by_offset(disp, _gloffset_IsList, fn)
#define CALL_DepthRange(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampd, GLclampd)), _gloffset_DepthRange, parameters)
#define GET_DepthRange(disp) GET_by_offset(disp, _gloffset_DepthRange)
#define SET_DepthRange(disp, fn) SET_by_offset(disp, _gloffset_DepthRange, fn)
#define CALL_Frustum(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Frustum, parameters)
#define GET_Frustum(disp) GET_by_offset(disp, _gloffset_Frustum)
#define SET_Frustum(disp, fn) SET_by_offset(disp, _gloffset_Frustum, fn)
#define CALL_LoadIdentity(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_LoadIdentity, parameters)
#define GET_LoadIdentity(disp) GET_by_offset(disp, _gloffset_LoadIdentity)
#define SET_LoadIdentity(disp, fn) SET_by_offset(disp, _gloffset_LoadIdentity, fn)
#define CALL_LoadMatrixf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_LoadMatrixf, parameters)
#define GET_LoadMatrixf(disp) GET_by_offset(disp, _gloffset_LoadMatrixf)
#define SET_LoadMatrixf(disp, fn) SET_by_offset(disp, _gloffset_LoadMatrixf, fn)
#define CALL_LoadMatrixd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_LoadMatrixd, parameters)
#define GET_LoadMatrixd(disp) GET_by_offset(disp, _gloffset_LoadMatrixd)
#define SET_LoadMatrixd(disp, fn) SET_by_offset(disp, _gloffset_LoadMatrixd, fn)
#define CALL_MatrixMode(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_MatrixMode, parameters)
#define GET_MatrixMode(disp) GET_by_offset(disp, _gloffset_MatrixMode)
#define SET_MatrixMode(disp, fn) SET_by_offset(disp, _gloffset_MatrixMode, fn)
#define CALL_MultMatrixf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_MultMatrixf, parameters)
#define GET_MultMatrixf(disp) GET_by_offset(disp, _gloffset_MultMatrixf)
#define SET_MultMatrixf(disp, fn) SET_by_offset(disp, _gloffset_MultMatrixf, fn)
#define CALL_MultMatrixd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_MultMatrixd, parameters)
#define GET_MultMatrixd(disp) GET_by_offset(disp, _gloffset_MultMatrixd)
#define SET_MultMatrixd(disp, fn) SET_by_offset(disp, _gloffset_MultMatrixd, fn)
#define CALL_Ortho(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Ortho, parameters)
#define GET_Ortho(disp) GET_by_offset(disp, _gloffset_Ortho)
#define SET_Ortho(disp, fn) SET_by_offset(disp, _gloffset_Ortho, fn)
#define CALL_PopMatrix(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopMatrix, parameters)
#define GET_PopMatrix(disp) GET_by_offset(disp, _gloffset_PopMatrix)
#define SET_PopMatrix(disp, fn) SET_by_offset(disp, _gloffset_PopMatrix, fn)
#define CALL_PushMatrix(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PushMatrix, parameters)
#define GET_PushMatrix(disp) GET_by_offset(disp, _gloffset_PushMatrix)
#define SET_PushMatrix(disp, fn) SET_by_offset(disp, _gloffset_PushMatrix, fn)
#define CALL_Rotated(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Rotated, parameters)
#define GET_Rotated(disp) GET_by_offset(disp, _gloffset_Rotated)
#define SET_Rotated(disp, fn) SET_by_offset(disp, _gloffset_Rotated, fn)
#define CALL_Rotatef(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Rotatef, parameters)
#define GET_Rotatef(disp) GET_by_offset(disp, _gloffset_Rotatef)
#define SET_Rotatef(disp, fn) SET_by_offset(disp, _gloffset_Rotatef, fn)
#define CALL_Scaled(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Scaled, parameters)
#define GET_Scaled(disp) GET_by_offset(disp, _gloffset_Scaled)
#define SET_Scaled(disp, fn) SET_by_offset(disp, _gloffset_Scaled, fn)
#define CALL_Scalef(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Scalef, parameters)
#define GET_Scalef(disp) GET_by_offset(disp, _gloffset_Scalef)
#define SET_Scalef(disp, fn) SET_by_offset(disp, _gloffset_Scalef, fn)
#define CALL_Translated(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Translated, parameters)
#define GET_Translated(disp) GET_by_offset(disp, _gloffset_Translated)
#define SET_Translated(disp, fn) SET_by_offset(disp, _gloffset_Translated, fn)
#define CALL_Translatef(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Translatef, parameters)
#define GET_Translatef(disp) GET_by_offset(disp, _gloffset_Translatef)
#define SET_Translatef(disp, fn) SET_by_offset(disp, _gloffset_Translatef, fn)
#define CALL_Viewport(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei)), _gloffset_Viewport, parameters)
#define GET_Viewport(disp) GET_by_offset(disp, _gloffset_Viewport)
#define SET_Viewport(disp, fn) SET_by_offset(disp, _gloffset_Viewport, fn)
#define CALL_ArrayElement(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_ArrayElement, parameters)
#define GET_ArrayElement(disp) GET_by_offset(disp, _gloffset_ArrayElement)
#define SET_ArrayElement(disp, fn) SET_by_offset(disp, _gloffset_ArrayElement, fn)
#define CALL_BindTexture(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindTexture, parameters)
#define GET_BindTexture(disp) GET_by_offset(disp, _gloffset_BindTexture)
#define SET_BindTexture(disp, fn) SET_by_offset(disp, _gloffset_BindTexture, fn)
#define CALL_ColorPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_ColorPointer, parameters)
#define GET_ColorPointer(disp) GET_by_offset(disp, _gloffset_ColorPointer)
#define SET_ColorPointer(disp, fn) SET_by_offset(disp, _gloffset_ColorPointer, fn)
#define CALL_DisableClientState(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_DisableClientState, parameters)
#define GET_DisableClientState(disp) GET_by_offset(disp, _gloffset_DisableClientState)
#define SET_DisableClientState(disp, fn) SET_by_offset(disp, _gloffset_DisableClientState, fn)
#define CALL_DrawArrays(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLsizei)), _gloffset_DrawArrays, parameters)
#define GET_DrawArrays(disp) GET_by_offset(disp, _gloffset_DrawArrays)
#define SET_DrawArrays(disp, fn) SET_by_offset(disp, _gloffset_DrawArrays, fn)
#define CALL_DrawElements(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, const GLvoid *)), _gloffset_DrawElements, parameters)
#define GET_DrawElements(disp) GET_by_offset(disp, _gloffset_DrawElements)
#define SET_DrawElements(disp, fn) SET_by_offset(disp, _gloffset_DrawElements, fn)
#define CALL_EdgeFlagPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLvoid *)), _gloffset_EdgeFlagPointer, parameters)
#define GET_EdgeFlagPointer(disp) GET_by_offset(disp, _gloffset_EdgeFlagPointer)
#define SET_EdgeFlagPointer(disp, fn) SET_by_offset(disp, _gloffset_EdgeFlagPointer, fn)
#define CALL_EnableClientState(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_EnableClientState, parameters)
#define GET_EnableClientState(disp) GET_by_offset(disp, _gloffset_EnableClientState)
#define SET_EnableClientState(disp, fn) SET_by_offset(disp, _gloffset_EnableClientState, fn)
#define CALL_IndexPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_IndexPointer, parameters)
#define GET_IndexPointer(disp) GET_by_offset(disp, _gloffset_IndexPointer)
#define SET_IndexPointer(disp, fn) SET_by_offset(disp, _gloffset_IndexPointer, fn)
#define CALL_Indexub(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte)), _gloffset_Indexub, parameters)
#define GET_Indexub(disp) GET_by_offset(disp, _gloffset_Indexub)
#define SET_Indexub(disp, fn) SET_by_offset(disp, _gloffset_Indexub, fn)
#define CALL_Indexubv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_Indexubv, parameters)
#define GET_Indexubv(disp) GET_by_offset(disp, _gloffset_Indexubv)
#define SET_Indexubv(disp, fn) SET_by_offset(disp, _gloffset_Indexubv, fn)
#define CALL_InterleavedArrays(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_InterleavedArrays, parameters)
#define GET_InterleavedArrays(disp) GET_by_offset(disp, _gloffset_InterleavedArrays)
#define SET_InterleavedArrays(disp, fn) SET_by_offset(disp, _gloffset_InterleavedArrays, fn)
#define CALL_NormalPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_NormalPointer, parameters)
#define GET_NormalPointer(disp) GET_by_offset(disp, _gloffset_NormalPointer)
#define SET_NormalPointer(disp, fn) SET_by_offset(disp, _gloffset_NormalPointer, fn)
#define CALL_PolygonOffset(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_PolygonOffset, parameters)
#define GET_PolygonOffset(disp) GET_by_offset(disp, _gloffset_PolygonOffset)
#define SET_PolygonOffset(disp, fn) SET_by_offset(disp, _gloffset_PolygonOffset, fn)
#define CALL_TexCoordPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_TexCoordPointer, parameters)
#define GET_TexCoordPointer(disp) GET_by_offset(disp, _gloffset_TexCoordPointer)
#define SET_TexCoordPointer(disp, fn) SET_by_offset(disp, _gloffset_TexCoordPointer, fn)
#define CALL_VertexPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_VertexPointer, parameters)
#define GET_VertexPointer(disp) GET_by_offset(disp, _gloffset_VertexPointer)
#define SET_VertexPointer(disp, fn) SET_by_offset(disp, _gloffset_VertexPointer, fn)
#define CALL_AreTexturesResident(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLsizei, const GLuint *, GLboolean *)), _gloffset_AreTexturesResident, parameters)
#define GET_AreTexturesResident(disp) GET_by_offset(disp, _gloffset_AreTexturesResident)
#define SET_AreTexturesResident(disp, fn) SET_by_offset(disp, _gloffset_AreTexturesResident, fn)
#define CALL_CopyTexImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint)), _gloffset_CopyTexImage1D, parameters)
#define GET_CopyTexImage1D(disp) GET_by_offset(disp, _gloffset_CopyTexImage1D)
#define SET_CopyTexImage1D(disp, fn) SET_by_offset(disp, _gloffset_CopyTexImage1D, fn)
#define CALL_CopyTexImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)), _gloffset_CopyTexImage2D, parameters)
#define GET_CopyTexImage2D(disp) GET_by_offset(disp, _gloffset_CopyTexImage2D)
#define SET_CopyTexImage2D(disp, fn) SET_by_offset(disp, _gloffset_CopyTexImage2D, fn)
#define CALL_CopyTexSubImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei)), _gloffset_CopyTexSubImage1D, parameters)
#define GET_CopyTexSubImage1D(disp) GET_by_offset(disp, _gloffset_CopyTexSubImage1D)
#define SET_CopyTexSubImage1D(disp, fn) SET_by_offset(disp, _gloffset_CopyTexSubImage1D, fn)
#define CALL_CopyTexSubImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)), _gloffset_CopyTexSubImage2D, parameters)
#define GET_CopyTexSubImage2D(disp) GET_by_offset(disp, _gloffset_CopyTexSubImage2D)
#define SET_CopyTexSubImage2D(disp, fn) SET_by_offset(disp, _gloffset_CopyTexSubImage2D, fn)
#define CALL_DeleteTextures(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteTextures, parameters)
#define GET_DeleteTextures(disp) GET_by_offset(disp, _gloffset_DeleteTextures)
#define SET_DeleteTextures(disp, fn) SET_by_offset(disp, _gloffset_DeleteTextures, fn)
#define CALL_GenTextures(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenTextures, parameters)
#define GET_GenTextures(disp) GET_by_offset(disp, _gloffset_GenTextures)
#define SET_GenTextures(disp, fn) SET_by_offset(disp, _gloffset_GenTextures, fn)
#define CALL_GetPointerv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLvoid **)), _gloffset_GetPointerv, parameters)
#define GET_GetPointerv(disp) GET_by_offset(disp, _gloffset_GetPointerv)
#define SET_GetPointerv(disp, fn) SET_by_offset(disp, _gloffset_GetPointerv, fn)
#define CALL_IsTexture(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsTexture, parameters)
#define GET_IsTexture(disp) GET_by_offset(disp, _gloffset_IsTexture)
#define SET_IsTexture(disp, fn) SET_by_offset(disp, _gloffset_IsTexture, fn)
#define CALL_PrioritizeTextures(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *, const GLclampf *)), _gloffset_PrioritizeTextures, parameters)
#define GET_PrioritizeTextures(disp) GET_by_offset(disp, _gloffset_PrioritizeTextures)
#define SET_PrioritizeTextures(disp, fn) SET_by_offset(disp, _gloffset_PrioritizeTextures, fn)
#define CALL_TexSubImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_TexSubImage1D, parameters)
#define GET_TexSubImage1D(disp) GET_by_offset(disp, _gloffset_TexSubImage1D)
#define SET_TexSubImage1D(disp, fn) SET_by_offset(disp, _gloffset_TexSubImage1D, fn)
#define CALL_TexSubImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_TexSubImage2D, parameters)
#define GET_TexSubImage2D(disp) GET_by_offset(disp, _gloffset_TexSubImage2D)
#define SET_TexSubImage2D(disp, fn) SET_by_offset(disp, _gloffset_TexSubImage2D, fn)
#define CALL_PopClientAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopClientAttrib, parameters)
#define GET_PopClientAttrib(disp) GET_by_offset(disp, _gloffset_PopClientAttrib)
#define SET_PopClientAttrib(disp, fn) SET_by_offset(disp, _gloffset_PopClientAttrib, fn)
#define CALL_PushClientAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbitfield)), _gloffset_PushClientAttrib, parameters)
#define GET_PushClientAttrib(disp) GET_by_offset(disp, _gloffset_PushClientAttrib)
#define SET_PushClientAttrib(disp, fn) SET_by_offset(disp, _gloffset_PushClientAttrib, fn)
#define CALL_BlendColor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLclampf, GLclampf, GLclampf)), _gloffset_BlendColor, parameters)
#define GET_BlendColor(disp) GET_by_offset(disp, _gloffset_BlendColor)
#define SET_BlendColor(disp, fn) SET_by_offset(disp, _gloffset_BlendColor, fn)
#define CALL_BlendEquation(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_BlendEquation, parameters)
#define GET_BlendEquation(disp) GET_by_offset(disp, _gloffset_BlendEquation)
#define SET_BlendEquation(disp, fn) SET_by_offset(disp, _gloffset_BlendEquation, fn)
#define CALL_DrawRangeElements(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *)), _gloffset_DrawRangeElements, parameters)
#define GET_DrawRangeElements(disp) GET_by_offset(disp, _gloffset_DrawRangeElements)
#define SET_DrawRangeElements(disp, fn) SET_by_offset(disp, _gloffset_DrawRangeElements, fn)
#define CALL_ColorTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ColorTable, parameters)
#define GET_ColorTable(disp) GET_by_offset(disp, _gloffset_ColorTable)
#define SET_ColorTable(disp, fn) SET_by_offset(disp, _gloffset_ColorTable, fn)
#define CALL_ColorTableParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_ColorTableParameterfv, parameters)
#define GET_ColorTableParameterfv(disp) GET_by_offset(disp, _gloffset_ColorTableParameterfv)
#define SET_ColorTableParameterfv(disp, fn) SET_by_offset(disp, _gloffset_ColorTableParameterfv, fn)
#define CALL_ColorTableParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_ColorTableParameteriv, parameters)
#define GET_ColorTableParameteriv(disp) GET_by_offset(disp, _gloffset_ColorTableParameteriv)
#define SET_ColorTableParameteriv(disp, fn) SET_by_offset(disp, _gloffset_ColorTableParameteriv, fn)
#define CALL_CopyColorTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLint, GLsizei)), _gloffset_CopyColorTable, parameters)
#define GET_CopyColorTable(disp) GET_by_offset(disp, _gloffset_CopyColorTable)
#define SET_CopyColorTable(disp, fn) SET_by_offset(disp, _gloffset_CopyColorTable, fn)
#define CALL_GetColorTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLvoid *)), _gloffset_GetColorTable, parameters)
#define GET_GetColorTable(disp) GET_by_offset(disp, _gloffset_GetColorTable)
#define SET_GetColorTable(disp, fn) SET_by_offset(disp, _gloffset_GetColorTable, fn)
#define CALL_GetColorTableParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetColorTableParameterfv, parameters)
#define GET_GetColorTableParameterfv(disp) GET_by_offset(disp, _gloffset_GetColorTableParameterfv)
#define SET_GetColorTableParameterfv(disp, fn) SET_by_offset(disp, _gloffset_GetColorTableParameterfv, fn)
#define CALL_GetColorTableParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetColorTableParameteriv, parameters)
#define GET_GetColorTableParameteriv(disp) GET_by_offset(disp, _gloffset_GetColorTableParameteriv)
#define SET_GetColorTableParameteriv(disp, fn) SET_by_offset(disp, _gloffset_GetColorTableParameteriv, fn)
#define CALL_ColorSubTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ColorSubTable, parameters)
#define GET_ColorSubTable(disp) GET_by_offset(disp, _gloffset_ColorSubTable)
#define SET_ColorSubTable(disp, fn) SET_by_offset(disp, _gloffset_ColorSubTable, fn)
#define CALL_CopyColorSubTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLint, GLint, GLsizei)), _gloffset_CopyColorSubTable, parameters)
#define GET_CopyColorSubTable(disp) GET_by_offset(disp, _gloffset_CopyColorSubTable)
#define SET_CopyColorSubTable(disp, fn) SET_by_offset(disp, _gloffset_CopyColorSubTable, fn)
#define CALL_ConvolutionFilter1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ConvolutionFilter1D, parameters)
#define GET_ConvolutionFilter1D(disp) GET_by_offset(disp, _gloffset_ConvolutionFilter1D)
#define SET_ConvolutionFilter1D(disp, fn) SET_by_offset(disp, _gloffset_ConvolutionFilter1D, fn)
#define CALL_ConvolutionFilter2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ConvolutionFilter2D, parameters)
#define GET_ConvolutionFilter2D(disp) GET_by_offset(disp, _gloffset_ConvolutionFilter2D)
#define SET_ConvolutionFilter2D(disp, fn) SET_by_offset(disp, _gloffset_ConvolutionFilter2D, fn)
#define CALL_ConvolutionParameterf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_ConvolutionParameterf, parameters)
#define GET_ConvolutionParameterf(disp) GET_by_offset(disp, _gloffset_ConvolutionParameterf)
#define SET_ConvolutionParameterf(disp, fn) SET_by_offset(disp, _gloffset_ConvolutionParameterf, fn)
#define CALL_ConvolutionParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_ConvolutionParameterfv, parameters)
#define GET_ConvolutionParameterfv(disp) GET_by_offset(disp, _gloffset_ConvolutionParameterfv)
#define SET_ConvolutionParameterfv(disp, fn) SET_by_offset(disp, _gloffset_ConvolutionParameterfv, fn)
#define CALL_ConvolutionParameteri(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_ConvolutionParameteri, parameters)
#define GET_ConvolutionParameteri(disp) GET_by_offset(disp, _gloffset_ConvolutionParameteri)
#define SET_ConvolutionParameteri(disp, fn) SET_by_offset(disp, _gloffset_ConvolutionParameteri, fn)
#define CALL_ConvolutionParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_ConvolutionParameteriv, parameters)
#define GET_ConvolutionParameteriv(disp) GET_by_offset(disp, _gloffset_ConvolutionParameteriv)
#define SET_ConvolutionParameteriv(disp, fn) SET_by_offset(disp, _gloffset_ConvolutionParameteriv, fn)
#define CALL_CopyConvolutionFilter1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLint, GLsizei)), _gloffset_CopyConvolutionFilter1D, parameters)
#define GET_CopyConvolutionFilter1D(disp) GET_by_offset(disp, _gloffset_CopyConvolutionFilter1D)
#define SET_CopyConvolutionFilter1D(disp, fn) SET_by_offset(disp, _gloffset_CopyConvolutionFilter1D, fn)
#define CALL_CopyConvolutionFilter2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei)), _gloffset_CopyConvolutionFilter2D, parameters)
#define GET_CopyConvolutionFilter2D(disp) GET_by_offset(disp, _gloffset_CopyConvolutionFilter2D)
#define SET_CopyConvolutionFilter2D(disp, fn) SET_by_offset(disp, _gloffset_CopyConvolutionFilter2D, fn)
#define CALL_GetConvolutionFilter(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLvoid *)), _gloffset_GetConvolutionFilter, parameters)
#define GET_GetConvolutionFilter(disp) GET_by_offset(disp, _gloffset_GetConvolutionFilter)
#define SET_GetConvolutionFilter(disp, fn) SET_by_offset(disp, _gloffset_GetConvolutionFilter, fn)
#define CALL_GetConvolutionParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetConvolutionParameterfv, parameters)
#define GET_GetConvolutionParameterfv(disp) GET_by_offset(disp, _gloffset_GetConvolutionParameterfv)
#define SET_GetConvolutionParameterfv(disp, fn) SET_by_offset(disp, _gloffset_GetConvolutionParameterfv, fn)
#define CALL_GetConvolutionParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetConvolutionParameteriv, parameters)
#define GET_GetConvolutionParameteriv(disp) GET_by_offset(disp, _gloffset_GetConvolutionParameteriv)
#define SET_GetConvolutionParameteriv(disp, fn) SET_by_offset(disp, _gloffset_GetConvolutionParameteriv, fn)
#define CALL_GetSeparableFilter(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *)), _gloffset_GetSeparableFilter, parameters)
#define GET_GetSeparableFilter(disp) GET_by_offset(disp, _gloffset_GetSeparableFilter)
#define SET_GetSeparableFilter(disp, fn) SET_by_offset(disp, _gloffset_GetSeparableFilter, fn)
#define CALL_SeparableFilter2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *)), _gloffset_SeparableFilter2D, parameters)
#define GET_SeparableFilter2D(disp) GET_by_offset(disp, _gloffset_SeparableFilter2D)
#define SET_SeparableFilter2D(disp, fn) SET_by_offset(disp, _gloffset_SeparableFilter2D, fn)
#define CALL_GetHistogram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)), _gloffset_GetHistogram, parameters)
#define GET_GetHistogram(disp) GET_by_offset(disp, _gloffset_GetHistogram)
#define SET_GetHistogram(disp, fn) SET_by_offset(disp, _gloffset_GetHistogram, fn)
#define CALL_GetHistogramParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetHistogramParameterfv, parameters)
#define GET_GetHistogramParameterfv(disp) GET_by_offset(disp, _gloffset_GetHistogramParameterfv)
#define SET_GetHistogramParameterfv(disp, fn) SET_by_offset(disp, _gloffset_GetHistogramParameterfv, fn)
#define CALL_GetHistogramParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetHistogramParameteriv, parameters)
#define GET_GetHistogramParameteriv(disp) GET_by_offset(disp, _gloffset_GetHistogramParameteriv)
#define SET_GetHistogramParameteriv(disp, fn) SET_by_offset(disp, _gloffset_GetHistogramParameteriv, fn)
#define CALL_GetMinmax(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)), _gloffset_GetMinmax, parameters)
#define GET_GetMinmax(disp) GET_by_offset(disp, _gloffset_GetMinmax)
#define SET_GetMinmax(disp, fn) SET_by_offset(disp, _gloffset_GetMinmax, fn)
#define CALL_GetMinmaxParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetMinmaxParameterfv, parameters)
#define GET_GetMinmaxParameterfv(disp) GET_by_offset(disp, _gloffset_GetMinmaxParameterfv)
#define SET_GetMinmaxParameterfv(disp, fn) SET_by_offset(disp, _gloffset_GetMinmaxParameterfv, fn)
#define CALL_GetMinmaxParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetMinmaxParameteriv, parameters)
#define GET_GetMinmaxParameteriv(disp) GET_by_offset(disp, _gloffset_GetMinmaxParameteriv)
#define SET_GetMinmaxParameteriv(disp, fn) SET_by_offset(disp, _gloffset_GetMinmaxParameteriv, fn)
#define CALL_Histogram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, GLboolean)), _gloffset_Histogram, parameters)
#define GET_Histogram(disp) GET_by_offset(disp, _gloffset_Histogram)
#define SET_Histogram(disp, fn) SET_by_offset(disp, _gloffset_Histogram, fn)
#define CALL_Minmax(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLboolean)), _gloffset_Minmax, parameters)
#define GET_Minmax(disp) GET_by_offset(disp, _gloffset_Minmax)
#define SET_Minmax(disp, fn) SET_by_offset(disp, _gloffset_Minmax, fn)
#define CALL_ResetHistogram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ResetHistogram, parameters)
#define GET_ResetHistogram(disp) GET_by_offset(disp, _gloffset_ResetHistogram)
#define SET_ResetHistogram(disp, fn) SET_by_offset(disp, _gloffset_ResetHistogram, fn)
#define CALL_ResetMinmax(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ResetMinmax, parameters)
#define GET_ResetMinmax(disp) GET_by_offset(disp, _gloffset_ResetMinmax)
#define SET_ResetMinmax(disp, fn) SET_by_offset(disp, _gloffset_ResetMinmax, fn)
#define CALL_TexImage3D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)), _gloffset_TexImage3D, parameters)
#define GET_TexImage3D(disp) GET_by_offset(disp, _gloffset_TexImage3D)
#define SET_TexImage3D(disp, fn) SET_by_offset(disp, _gloffset_TexImage3D, fn)
#define CALL_TexSubImage3D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_TexSubImage3D, parameters)
#define GET_TexSubImage3D(disp) GET_by_offset(disp, _gloffset_TexSubImage3D)
#define SET_TexSubImage3D(disp, fn) SET_by_offset(disp, _gloffset_TexSubImage3D, fn)
#define CALL_CopyTexSubImage3D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)), _gloffset_CopyTexSubImage3D, parameters)
#define GET_CopyTexSubImage3D(disp) GET_by_offset(disp, _gloffset_CopyTexSubImage3D)
#define SET_CopyTexSubImage3D(disp, fn) SET_by_offset(disp, _gloffset_CopyTexSubImage3D, fn)
#define CALL_ActiveTextureARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ActiveTextureARB, parameters)
#define GET_ActiveTextureARB(disp) GET_by_offset(disp, _gloffset_ActiveTextureARB)
#define SET_ActiveTextureARB(disp, fn) SET_by_offset(disp, _gloffset_ActiveTextureARB, fn)
#define CALL_ClientActiveTextureARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ClientActiveTextureARB, parameters)
#define GET_ClientActiveTextureARB(disp) GET_by_offset(disp, _gloffset_ClientActiveTextureARB)
#define SET_ClientActiveTextureARB(disp, fn) SET_by_offset(disp, _gloffset_ClientActiveTextureARB, fn)
#define CALL_MultiTexCoord1dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble)), _gloffset_MultiTexCoord1dARB, parameters)
#define GET_MultiTexCoord1dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1dARB)
#define SET_MultiTexCoord1dARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1dARB, fn)
#define CALL_MultiTexCoord1dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord1dvARB, parameters)
#define GET_MultiTexCoord1dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1dvARB)
#define SET_MultiTexCoord1dvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1dvARB, fn)
#define CALL_MultiTexCoord1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_MultiTexCoord1fARB, parameters)
#define GET_MultiTexCoord1fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1fARB)
#define SET_MultiTexCoord1fARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1fARB, fn)
#define CALL_MultiTexCoord1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord1fvARB, parameters)
#define GET_MultiTexCoord1fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1fvARB)
#define SET_MultiTexCoord1fvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1fvARB, fn)
#define CALL_MultiTexCoord1iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_MultiTexCoord1iARB, parameters)
#define GET_MultiTexCoord1iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1iARB)
#define SET_MultiTexCoord1iARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1iARB, fn)
#define CALL_MultiTexCoord1ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord1ivARB, parameters)
#define GET_MultiTexCoord1ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1ivARB)
#define SET_MultiTexCoord1ivARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1ivARB, fn)
#define CALL_MultiTexCoord1sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort)), _gloffset_MultiTexCoord1sARB, parameters)
#define GET_MultiTexCoord1sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1sARB)
#define SET_MultiTexCoord1sARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1sARB, fn)
#define CALL_MultiTexCoord1svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord1svARB, parameters)
#define GET_MultiTexCoord1svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1svARB)
#define SET_MultiTexCoord1svARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord1svARB, fn)
#define CALL_MultiTexCoord2dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble)), _gloffset_MultiTexCoord2dARB, parameters)
#define GET_MultiTexCoord2dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2dARB)
#define SET_MultiTexCoord2dARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2dARB, fn)
#define CALL_MultiTexCoord2dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord2dvARB, parameters)
#define GET_MultiTexCoord2dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2dvARB)
#define SET_MultiTexCoord2dvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2dvARB, fn)
#define CALL_MultiTexCoord2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat)), _gloffset_MultiTexCoord2fARB, parameters)
#define GET_MultiTexCoord2fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2fARB)
#define SET_MultiTexCoord2fARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2fARB, fn)
#define CALL_MultiTexCoord2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord2fvARB, parameters)
#define GET_MultiTexCoord2fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2fvARB)
#define SET_MultiTexCoord2fvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2fvARB, fn)
#define CALL_MultiTexCoord2iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint)), _gloffset_MultiTexCoord2iARB, parameters)
#define GET_MultiTexCoord2iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2iARB)
#define SET_MultiTexCoord2iARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2iARB, fn)
#define CALL_MultiTexCoord2ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord2ivARB, parameters)
#define GET_MultiTexCoord2ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2ivARB)
#define SET_MultiTexCoord2ivARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2ivARB, fn)
#define CALL_MultiTexCoord2sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort, GLshort)), _gloffset_MultiTexCoord2sARB, parameters)
#define GET_MultiTexCoord2sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2sARB)
#define SET_MultiTexCoord2sARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2sARB, fn)
#define CALL_MultiTexCoord2svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord2svARB, parameters)
#define GET_MultiTexCoord2svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2svARB)
#define SET_MultiTexCoord2svARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord2svARB, fn)
#define CALL_MultiTexCoord3dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLdouble)), _gloffset_MultiTexCoord3dARB, parameters)
#define GET_MultiTexCoord3dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3dARB)
#define SET_MultiTexCoord3dARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3dARB, fn)
#define CALL_MultiTexCoord3dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord3dvARB, parameters)
#define GET_MultiTexCoord3dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3dvARB)
#define SET_MultiTexCoord3dvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3dvARB, fn)
#define CALL_MultiTexCoord3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLfloat)), _gloffset_MultiTexCoord3fARB, parameters)
#define GET_MultiTexCoord3fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3fARB)
#define SET_MultiTexCoord3fARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3fARB, fn)
#define CALL_MultiTexCoord3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord3fvARB, parameters)
#define GET_MultiTexCoord3fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3fvARB)
#define SET_MultiTexCoord3fvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3fvARB, fn)
#define CALL_MultiTexCoord3iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint)), _gloffset_MultiTexCoord3iARB, parameters)
#define GET_MultiTexCoord3iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3iARB)
#define SET_MultiTexCoord3iARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3iARB, fn)
#define CALL_MultiTexCoord3ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord3ivARB, parameters)
#define GET_MultiTexCoord3ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3ivARB)
#define SET_MultiTexCoord3ivARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3ivARB, fn)
#define CALL_MultiTexCoord3sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort, GLshort, GLshort)), _gloffset_MultiTexCoord3sARB, parameters)
#define GET_MultiTexCoord3sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3sARB)
#define SET_MultiTexCoord3sARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3sARB, fn)
#define CALL_MultiTexCoord3svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord3svARB, parameters)
#define GET_MultiTexCoord3svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3svARB)
#define SET_MultiTexCoord3svARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord3svARB, fn)
#define CALL_MultiTexCoord4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_MultiTexCoord4dARB, parameters)
#define GET_MultiTexCoord4dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4dARB)
#define SET_MultiTexCoord4dARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4dARB, fn)
#define CALL_MultiTexCoord4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord4dvARB, parameters)
#define GET_MultiTexCoord4dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4dvARB)
#define SET_MultiTexCoord4dvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4dvARB, fn)
#define CALL_MultiTexCoord4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_MultiTexCoord4fARB, parameters)
#define GET_MultiTexCoord4fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4fARB)
#define SET_MultiTexCoord4fARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4fARB, fn)
#define CALL_MultiTexCoord4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord4fvARB, parameters)
#define GET_MultiTexCoord4fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4fvARB)
#define SET_MultiTexCoord4fvARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4fvARB, fn)
#define CALL_MultiTexCoord4iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint)), _gloffset_MultiTexCoord4iARB, parameters)
#define GET_MultiTexCoord4iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4iARB)
#define SET_MultiTexCoord4iARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4iARB, fn)
#define CALL_MultiTexCoord4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord4ivARB, parameters)
#define GET_MultiTexCoord4ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4ivARB)
#define SET_MultiTexCoord4ivARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4ivARB, fn)
#define CALL_MultiTexCoord4sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort, GLshort, GLshort, GLshort)), _gloffset_MultiTexCoord4sARB, parameters)
#define GET_MultiTexCoord4sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4sARB)
#define SET_MultiTexCoord4sARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4sARB, fn)
#define CALL_MultiTexCoord4svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord4svARB, parameters)
#define GET_MultiTexCoord4svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4svARB)
#define SET_MultiTexCoord4svARB(disp, fn) SET_by_offset(disp, _gloffset_MultiTexCoord4svARB, fn)
#define CALL_AttachShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_AttachShader, parameters)
#define GET_AttachShader(disp) GET_by_offset(disp, _gloffset_AttachShader)
#define SET_AttachShader(disp, fn) SET_by_offset(disp, _gloffset_AttachShader, fn)
#define CALL_CreateProgram(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(void)), _gloffset_CreateProgram, parameters)
#define GET_CreateProgram(disp) GET_by_offset(disp, _gloffset_CreateProgram)
#define SET_CreateProgram(disp, fn) SET_by_offset(disp, _gloffset_CreateProgram, fn)
#define CALL_CreateShader(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLenum)), _gloffset_CreateShader, parameters)
#define GET_CreateShader(disp) GET_by_offset(disp, _gloffset_CreateShader)
#define SET_CreateShader(disp, fn) SET_by_offset(disp, _gloffset_CreateShader, fn)
#define CALL_DeleteProgram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DeleteProgram, parameters)
#define GET_DeleteProgram(disp) GET_by_offset(disp, _gloffset_DeleteProgram)
#define SET_DeleteProgram(disp, fn) SET_by_offset(disp, _gloffset_DeleteProgram, fn)
#define CALL_DeleteShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DeleteShader, parameters)
#define GET_DeleteShader(disp) GET_by_offset(disp, _gloffset_DeleteShader)
#define SET_DeleteShader(disp, fn) SET_by_offset(disp, _gloffset_DeleteShader, fn)
#define CALL_DetachShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_DetachShader, parameters)
#define GET_DetachShader(disp) GET_by_offset(disp, _gloffset_DetachShader)
#define SET_DetachShader(disp, fn) SET_by_offset(disp, _gloffset_DetachShader, fn)
#define CALL_GetAttachedShaders(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLuint *)), _gloffset_GetAttachedShaders, parameters)
#define GET_GetAttachedShaders(disp) GET_by_offset(disp, _gloffset_GetAttachedShaders)
#define SET_GetAttachedShaders(disp, fn) SET_by_offset(disp, _gloffset_GetAttachedShaders, fn)
#define CALL_GetProgramInfoLog(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLchar *)), _gloffset_GetProgramInfoLog, parameters)
#define GET_GetProgramInfoLog(disp) GET_by_offset(disp, _gloffset_GetProgramInfoLog)
#define SET_GetProgramInfoLog(disp, fn) SET_by_offset(disp, _gloffset_GetProgramInfoLog, fn)
#define CALL_GetProgramiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetProgramiv, parameters)
#define GET_GetProgramiv(disp) GET_by_offset(disp, _gloffset_GetProgramiv)
#define SET_GetProgramiv(disp, fn) SET_by_offset(disp, _gloffset_GetProgramiv, fn)
#define CALL_GetShaderInfoLog(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLchar *)), _gloffset_GetShaderInfoLog, parameters)
#define GET_GetShaderInfoLog(disp) GET_by_offset(disp, _gloffset_GetShaderInfoLog)
#define SET_GetShaderInfoLog(disp, fn) SET_by_offset(disp, _gloffset_GetShaderInfoLog, fn)
#define CALL_GetShaderiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetShaderiv, parameters)
#define GET_GetShaderiv(disp) GET_by_offset(disp, _gloffset_GetShaderiv)
#define SET_GetShaderiv(disp, fn) SET_by_offset(disp, _gloffset_GetShaderiv, fn)
#define CALL_IsProgram(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsProgram, parameters)
#define GET_IsProgram(disp) GET_by_offset(disp, _gloffset_IsProgram)
#define SET_IsProgram(disp, fn) SET_by_offset(disp, _gloffset_IsProgram, fn)
#define CALL_IsShader(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsShader, parameters)
#define GET_IsShader(disp) GET_by_offset(disp, _gloffset_IsShader)
#define SET_IsShader(disp, fn) SET_by_offset(disp, _gloffset_IsShader, fn)
#define CALL_StencilFuncSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLuint)), _gloffset_StencilFuncSeparate, parameters)
#define GET_StencilFuncSeparate(disp) GET_by_offset(disp, _gloffset_StencilFuncSeparate)
#define SET_StencilFuncSeparate(disp, fn) SET_by_offset(disp, _gloffset_StencilFuncSeparate, fn)
#define CALL_StencilMaskSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_StencilMaskSeparate, parameters)
#define GET_StencilMaskSeparate(disp) GET_by_offset(disp, _gloffset_StencilMaskSeparate)
#define SET_StencilMaskSeparate(disp, fn) SET_by_offset(disp, _gloffset_StencilMaskSeparate, fn)
#define CALL_StencilOpSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), _gloffset_StencilOpSeparate, parameters)
#define GET_StencilOpSeparate(disp) GET_by_offset(disp, _gloffset_StencilOpSeparate)
#define SET_StencilOpSeparate(disp, fn) SET_by_offset(disp, _gloffset_StencilOpSeparate, fn)
#define CALL_UniformMatrix2x3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix2x3fv, parameters)
#define GET_UniformMatrix2x3fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix2x3fv)
#define SET_UniformMatrix2x3fv(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix2x3fv, fn)
#define CALL_UniformMatrix2x4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix2x4fv, parameters)
#define GET_UniformMatrix2x4fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix2x4fv)
#define SET_UniformMatrix2x4fv(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix2x4fv, fn)
#define CALL_UniformMatrix3x2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix3x2fv, parameters)
#define GET_UniformMatrix3x2fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix3x2fv)
#define SET_UniformMatrix3x2fv(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix3x2fv, fn)
#define CALL_UniformMatrix3x4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix3x4fv, parameters)
#define GET_UniformMatrix3x4fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix3x4fv)
#define SET_UniformMatrix3x4fv(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix3x4fv, fn)
#define CALL_UniformMatrix4x2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix4x2fv, parameters)
#define GET_UniformMatrix4x2fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix4x2fv)
#define SET_UniformMatrix4x2fv(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix4x2fv, fn)
#define CALL_UniformMatrix4x3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix4x3fv, parameters)
#define GET_UniformMatrix4x3fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix4x3fv)
#define SET_UniformMatrix4x3fv(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix4x3fv, fn)
#define CALL_DrawArraysInstanced(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLsizei, GLsizei)), _gloffset_DrawArraysInstanced, parameters)
#define GET_DrawArraysInstanced(disp) GET_by_offset(disp, _gloffset_DrawArraysInstanced)
#define SET_DrawArraysInstanced(disp, fn) SET_by_offset(disp, _gloffset_DrawArraysInstanced, fn)
#define CALL_DrawElementsInstanced(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei)), _gloffset_DrawElementsInstanced, parameters)
#define GET_DrawElementsInstanced(disp) GET_by_offset(disp, _gloffset_DrawElementsInstanced)
#define SET_DrawElementsInstanced(disp, fn) SET_by_offset(disp, _gloffset_DrawElementsInstanced, fn)
#define CALL_LoadTransposeMatrixdARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_LoadTransposeMatrixdARB, parameters)
#define GET_LoadTransposeMatrixdARB(disp) GET_by_offset(disp, _gloffset_LoadTransposeMatrixdARB)
#define SET_LoadTransposeMatrixdARB(disp, fn) SET_by_offset(disp, _gloffset_LoadTransposeMatrixdARB, fn)
#define CALL_LoadTransposeMatrixfARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_LoadTransposeMatrixfARB, parameters)
#define GET_LoadTransposeMatrixfARB(disp) GET_by_offset(disp, _gloffset_LoadTransposeMatrixfARB)
#define SET_LoadTransposeMatrixfARB(disp, fn) SET_by_offset(disp, _gloffset_LoadTransposeMatrixfARB, fn)
#define CALL_MultTransposeMatrixdARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_MultTransposeMatrixdARB, parameters)
#define GET_MultTransposeMatrixdARB(disp) GET_by_offset(disp, _gloffset_MultTransposeMatrixdARB)
#define SET_MultTransposeMatrixdARB(disp, fn) SET_by_offset(disp, _gloffset_MultTransposeMatrixdARB, fn)
#define CALL_MultTransposeMatrixfARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_MultTransposeMatrixfARB, parameters)
#define GET_MultTransposeMatrixfARB(disp) GET_by_offset(disp, _gloffset_MultTransposeMatrixfARB)
#define SET_MultTransposeMatrixfARB(disp, fn) SET_by_offset(disp, _gloffset_MultTransposeMatrixfARB, fn)
#define CALL_SampleCoverageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLboolean)), _gloffset_SampleCoverageARB, parameters)
#define GET_SampleCoverageARB(disp) GET_by_offset(disp, _gloffset_SampleCoverageARB)
#define SET_SampleCoverageARB(disp, fn) SET_by_offset(disp, _gloffset_SampleCoverageARB, fn)
#define CALL_CompressedTexImage1DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *)), _gloffset_CompressedTexImage1DARB, parameters)
#define GET_CompressedTexImage1DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexImage1DARB)
#define SET_CompressedTexImage1DARB(disp, fn) SET_by_offset(disp, _gloffset_CompressedTexImage1DARB, fn)
#define CALL_CompressedTexImage2DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)), _gloffset_CompressedTexImage2DARB, parameters)
#define GET_CompressedTexImage2DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexImage2DARB)
#define SET_CompressedTexImage2DARB(disp, fn) SET_by_offset(disp, _gloffset_CompressedTexImage2DARB, fn)
#define CALL_CompressedTexImage3DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)), _gloffset_CompressedTexImage3DARB, parameters)
#define GET_CompressedTexImage3DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexImage3DARB)
#define SET_CompressedTexImage3DARB(disp, fn) SET_by_offset(disp, _gloffset_CompressedTexImage3DARB, fn)
#define CALL_CompressedTexSubImage1DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *)), _gloffset_CompressedTexSubImage1DARB, parameters)
#define GET_CompressedTexSubImage1DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexSubImage1DARB)
#define SET_CompressedTexSubImage1DARB(disp, fn) SET_by_offset(disp, _gloffset_CompressedTexSubImage1DARB, fn)
#define CALL_CompressedTexSubImage2DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)), _gloffset_CompressedTexSubImage2DARB, parameters)
#define GET_CompressedTexSubImage2DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexSubImage2DARB)
#define SET_CompressedTexSubImage2DARB(disp, fn) SET_by_offset(disp, _gloffset_CompressedTexSubImage2DARB, fn)
#define CALL_CompressedTexSubImage3DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)), _gloffset_CompressedTexSubImage3DARB, parameters)
#define GET_CompressedTexSubImage3DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexSubImage3DARB)
#define SET_CompressedTexSubImage3DARB(disp, fn) SET_by_offset(disp, _gloffset_CompressedTexSubImage3DARB, fn)
#define CALL_GetCompressedTexImageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLvoid *)), _gloffset_GetCompressedTexImageARB, parameters)
#define GET_GetCompressedTexImageARB(disp) GET_by_offset(disp, _gloffset_GetCompressedTexImageARB)
#define SET_GetCompressedTexImageARB(disp, fn) SET_by_offset(disp, _gloffset_GetCompressedTexImageARB, fn)
#define CALL_DisableVertexAttribArrayARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DisableVertexAttribArrayARB, parameters)
#define GET_DisableVertexAttribArrayARB(disp) GET_by_offset(disp, _gloffset_DisableVertexAttribArrayARB)
#define SET_DisableVertexAttribArrayARB(disp, fn) SET_by_offset(disp, _gloffset_DisableVertexAttribArrayARB, fn)
#define CALL_EnableVertexAttribArrayARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_EnableVertexAttribArrayARB, parameters)
#define GET_EnableVertexAttribArrayARB(disp) GET_by_offset(disp, _gloffset_EnableVertexAttribArrayARB)
#define SET_EnableVertexAttribArrayARB(disp, fn) SET_by_offset(disp, _gloffset_EnableVertexAttribArrayARB, fn)
#define CALL_GetProgramEnvParameterdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble *)), _gloffset_GetProgramEnvParameterdvARB, parameters)
#define GET_GetProgramEnvParameterdvARB(disp) GET_by_offset(disp, _gloffset_GetProgramEnvParameterdvARB)
#define SET_GetProgramEnvParameterdvARB(disp, fn) SET_by_offset(disp, _gloffset_GetProgramEnvParameterdvARB, fn)
#define CALL_GetProgramEnvParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat *)), _gloffset_GetProgramEnvParameterfvARB, parameters)
#define GET_GetProgramEnvParameterfvARB(disp) GET_by_offset(disp, _gloffset_GetProgramEnvParameterfvARB)
#define SET_GetProgramEnvParameterfvARB(disp, fn) SET_by_offset(disp, _gloffset_GetProgramEnvParameterfvARB, fn)
#define CALL_GetProgramLocalParameterdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble *)), _gloffset_GetProgramLocalParameterdvARB, parameters)
#define GET_GetProgramLocalParameterdvARB(disp) GET_by_offset(disp, _gloffset_GetProgramLocalParameterdvARB)
#define SET_GetProgramLocalParameterdvARB(disp, fn) SET_by_offset(disp, _gloffset_GetProgramLocalParameterdvARB, fn)
#define CALL_GetProgramLocalParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat *)), _gloffset_GetProgramLocalParameterfvARB, parameters)
#define GET_GetProgramLocalParameterfvARB(disp) GET_by_offset(disp, _gloffset_GetProgramLocalParameterfvARB)
#define SET_GetProgramLocalParameterfvARB(disp, fn) SET_by_offset(disp, _gloffset_GetProgramLocalParameterfvARB, fn)
#define CALL_GetProgramStringARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid *)), _gloffset_GetProgramStringARB, parameters)
#define GET_GetProgramStringARB(disp) GET_by_offset(disp, _gloffset_GetProgramStringARB)
#define SET_GetProgramStringARB(disp, fn) SET_by_offset(disp, _gloffset_GetProgramStringARB, fn)
#define CALL_GetProgramivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetProgramivARB, parameters)
#define GET_GetProgramivARB(disp) GET_by_offset(disp, _gloffset_GetProgramivARB)
#define SET_GetProgramivARB(disp, fn) SET_by_offset(disp, _gloffset_GetProgramivARB, fn)
#define CALL_GetVertexAttribdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLdouble *)), _gloffset_GetVertexAttribdvARB, parameters)
#define GET_GetVertexAttribdvARB(disp) GET_by_offset(disp, _gloffset_GetVertexAttribdvARB)
#define SET_GetVertexAttribdvARB(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribdvARB, fn)
#define CALL_GetVertexAttribfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat *)), _gloffset_GetVertexAttribfvARB, parameters)
#define GET_GetVertexAttribfvARB(disp) GET_by_offset(disp, _gloffset_GetVertexAttribfvARB)
#define SET_GetVertexAttribfvARB(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribfvARB, fn)
#define CALL_GetVertexAttribivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetVertexAttribivARB, parameters)
#define GET_GetVertexAttribivARB(disp) GET_by_offset(disp, _gloffset_GetVertexAttribivARB)
#define SET_GetVertexAttribivARB(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribivARB, fn)
#define CALL_ProgramEnvParameter4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_ProgramEnvParameter4dARB, parameters)
#define GET_ProgramEnvParameter4dARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4dARB)
#define SET_ProgramEnvParameter4dARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramEnvParameter4dARB, fn)
#define CALL_ProgramEnvParameter4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLdouble *)), _gloffset_ProgramEnvParameter4dvARB, parameters)
#define GET_ProgramEnvParameter4dvARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4dvARB)
#define SET_ProgramEnvParameter4dvARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramEnvParameter4dvARB, fn)
#define CALL_ProgramEnvParameter4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ProgramEnvParameter4fARB, parameters)
#define GET_ProgramEnvParameter4fARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4fARB)
#define SET_ProgramEnvParameter4fARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramEnvParameter4fARB, fn)
#define CALL_ProgramEnvParameter4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), _gloffset_ProgramEnvParameter4fvARB, parameters)
#define GET_ProgramEnvParameter4fvARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4fvARB)
#define SET_ProgramEnvParameter4fvARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramEnvParameter4fvARB, fn)
#define CALL_ProgramLocalParameter4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_ProgramLocalParameter4dARB, parameters)
#define GET_ProgramLocalParameter4dARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4dARB)
#define SET_ProgramLocalParameter4dARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramLocalParameter4dARB, fn)
#define CALL_ProgramLocalParameter4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLdouble *)), _gloffset_ProgramLocalParameter4dvARB, parameters)
#define GET_ProgramLocalParameter4dvARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4dvARB)
#define SET_ProgramLocalParameter4dvARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramLocalParameter4dvARB, fn)
#define CALL_ProgramLocalParameter4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ProgramLocalParameter4fARB, parameters)
#define GET_ProgramLocalParameter4fARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4fARB)
#define SET_ProgramLocalParameter4fARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramLocalParameter4fARB, fn)
#define CALL_ProgramLocalParameter4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), _gloffset_ProgramLocalParameter4fvARB, parameters)
#define GET_ProgramLocalParameter4fvARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4fvARB)
#define SET_ProgramLocalParameter4fvARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramLocalParameter4fvARB, fn)
#define CALL_ProgramStringARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, const GLvoid *)), _gloffset_ProgramStringARB, parameters)
#define GET_ProgramStringARB(disp) GET_by_offset(disp, _gloffset_ProgramStringARB)
#define SET_ProgramStringARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramStringARB, fn)
#define CALL_VertexAttrib1dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble)), _gloffset_VertexAttrib1dARB, parameters)
#define GET_VertexAttrib1dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dARB)
#define SET_VertexAttrib1dARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1dARB, fn)
#define CALL_VertexAttrib1dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib1dvARB, parameters)
#define GET_VertexAttrib1dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dvARB)
#define SET_VertexAttrib1dvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1dvARB, fn)
#define CALL_VertexAttrib1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat)), _gloffset_VertexAttrib1fARB, parameters)
#define GET_VertexAttrib1fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fARB)
#define SET_VertexAttrib1fARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1fARB, fn)
#define CALL_VertexAttrib1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib1fvARB, parameters)
#define GET_VertexAttrib1fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fvARB)
#define SET_VertexAttrib1fvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1fvARB, fn)
#define CALL_VertexAttrib1sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort)), _gloffset_VertexAttrib1sARB, parameters)
#define GET_VertexAttrib1sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1sARB)
#define SET_VertexAttrib1sARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1sARB, fn)
#define CALL_VertexAttrib1svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib1svARB, parameters)
#define GET_VertexAttrib1svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1svARB)
#define SET_VertexAttrib1svARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1svARB, fn)
#define CALL_VertexAttrib2dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble)), _gloffset_VertexAttrib2dARB, parameters)
#define GET_VertexAttrib2dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dARB)
#define SET_VertexAttrib2dARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2dARB, fn)
#define CALL_VertexAttrib2dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib2dvARB, parameters)
#define GET_VertexAttrib2dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dvARB)
#define SET_VertexAttrib2dvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2dvARB, fn)
#define CALL_VertexAttrib2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat)), _gloffset_VertexAttrib2fARB, parameters)
#define GET_VertexAttrib2fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fARB)
#define SET_VertexAttrib2fARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2fARB, fn)
#define CALL_VertexAttrib2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib2fvARB, parameters)
#define GET_VertexAttrib2fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fvARB)
#define SET_VertexAttrib2fvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2fvARB, fn)
#define CALL_VertexAttrib2sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort)), _gloffset_VertexAttrib2sARB, parameters)
#define GET_VertexAttrib2sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2sARB)
#define SET_VertexAttrib2sARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2sARB, fn)
#define CALL_VertexAttrib2svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib2svARB, parameters)
#define GET_VertexAttrib2svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2svARB)
#define SET_VertexAttrib2svARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2svARB, fn)
#define CALL_VertexAttrib3dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib3dARB, parameters)
#define GET_VertexAttrib3dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dARB)
#define SET_VertexAttrib3dARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3dARB, fn)
#define CALL_VertexAttrib3dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib3dvARB, parameters)
#define GET_VertexAttrib3dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dvARB)
#define SET_VertexAttrib3dvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3dvARB, fn)
#define CALL_VertexAttrib3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib3fARB, parameters)
#define GET_VertexAttrib3fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fARB)
#define SET_VertexAttrib3fARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3fARB, fn)
#define CALL_VertexAttrib3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib3fvARB, parameters)
#define GET_VertexAttrib3fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fvARB)
#define SET_VertexAttrib3fvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3fvARB, fn)
#define CALL_VertexAttrib3sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib3sARB, parameters)
#define GET_VertexAttrib3sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3sARB)
#define SET_VertexAttrib3sARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3sARB, fn)
#define CALL_VertexAttrib3svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib3svARB, parameters)
#define GET_VertexAttrib3svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3svARB)
#define SET_VertexAttrib3svARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3svARB, fn)
#define CALL_VertexAttrib4NbvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), _gloffset_VertexAttrib4NbvARB, parameters)
#define GET_VertexAttrib4NbvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NbvARB)
#define SET_VertexAttrib4NbvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4NbvARB, fn)
#define CALL_VertexAttrib4NivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttrib4NivARB, parameters)
#define GET_VertexAttrib4NivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NivARB)
#define SET_VertexAttrib4NivARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4NivARB, fn)
#define CALL_VertexAttrib4NsvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib4NsvARB, parameters)
#define GET_VertexAttrib4NsvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NsvARB)
#define SET_VertexAttrib4NsvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4NsvARB, fn)
#define CALL_VertexAttrib4NubARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)), _gloffset_VertexAttrib4NubARB, parameters)
#define GET_VertexAttrib4NubARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NubARB)
#define SET_VertexAttrib4NubARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4NubARB, fn)
#define CALL_VertexAttrib4NubvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttrib4NubvARB, parameters)
#define GET_VertexAttrib4NubvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NubvARB)
#define SET_VertexAttrib4NubvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4NubvARB, fn)
#define CALL_VertexAttrib4NuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttrib4NuivARB, parameters)
#define GET_VertexAttrib4NuivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NuivARB)
#define SET_VertexAttrib4NuivARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4NuivARB, fn)
#define CALL_VertexAttrib4NusvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), _gloffset_VertexAttrib4NusvARB, parameters)
#define GET_VertexAttrib4NusvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NusvARB)
#define SET_VertexAttrib4NusvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4NusvARB, fn)
#define CALL_VertexAttrib4bvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), _gloffset_VertexAttrib4bvARB, parameters)
#define GET_VertexAttrib4bvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4bvARB)
#define SET_VertexAttrib4bvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4bvARB, fn)
#define CALL_VertexAttrib4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib4dARB, parameters)
#define GET_VertexAttrib4dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dARB)
#define SET_VertexAttrib4dARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4dARB, fn)
#define CALL_VertexAttrib4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib4dvARB, parameters)
#define GET_VertexAttrib4dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dvARB)
#define SET_VertexAttrib4dvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4dvARB, fn)
#define CALL_VertexAttrib4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib4fARB, parameters)
#define GET_VertexAttrib4fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fARB)
#define SET_VertexAttrib4fARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4fARB, fn)
#define CALL_VertexAttrib4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib4fvARB, parameters)
#define GET_VertexAttrib4fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fvARB)
#define SET_VertexAttrib4fvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4fvARB, fn)
#define CALL_VertexAttrib4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttrib4ivARB, parameters)
#define GET_VertexAttrib4ivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ivARB)
#define SET_VertexAttrib4ivARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4ivARB, fn)
#define CALL_VertexAttrib4sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib4sARB, parameters)
#define GET_VertexAttrib4sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4sARB)
#define SET_VertexAttrib4sARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4sARB, fn)
#define CALL_VertexAttrib4svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib4svARB, parameters)
#define GET_VertexAttrib4svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4svARB)
#define SET_VertexAttrib4svARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4svARB, fn)
#define CALL_VertexAttrib4ubvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttrib4ubvARB, parameters)
#define GET_VertexAttrib4ubvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ubvARB)
#define SET_VertexAttrib4ubvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4ubvARB, fn)
#define CALL_VertexAttrib4uivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttrib4uivARB, parameters)
#define GET_VertexAttrib4uivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4uivARB)
#define SET_VertexAttrib4uivARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4uivARB, fn)
#define CALL_VertexAttrib4usvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), _gloffset_VertexAttrib4usvARB, parameters)
#define GET_VertexAttrib4usvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4usvARB)
#define SET_VertexAttrib4usvARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4usvARB, fn)
#define CALL_VertexAttribPointerARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *)), _gloffset_VertexAttribPointerARB, parameters)
#define GET_VertexAttribPointerARB(disp) GET_by_offset(disp, _gloffset_VertexAttribPointerARB)
#define SET_VertexAttribPointerARB(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribPointerARB, fn)
#define CALL_BindBufferARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindBufferARB, parameters)
#define GET_BindBufferARB(disp) GET_by_offset(disp, _gloffset_BindBufferARB)
#define SET_BindBufferARB(disp, fn) SET_by_offset(disp, _gloffset_BindBufferARB, fn)
#define CALL_BufferDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum)), _gloffset_BufferDataARB, parameters)
#define GET_BufferDataARB(disp) GET_by_offset(disp, _gloffset_BufferDataARB)
#define SET_BufferDataARB(disp, fn) SET_by_offset(disp, _gloffset_BufferDataARB, fn)
#define CALL_BufferSubDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *)), _gloffset_BufferSubDataARB, parameters)
#define GET_BufferSubDataARB(disp) GET_by_offset(disp, _gloffset_BufferSubDataARB)
#define SET_BufferSubDataARB(disp, fn) SET_by_offset(disp, _gloffset_BufferSubDataARB, fn)
#define CALL_DeleteBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteBuffersARB, parameters)
#define GET_DeleteBuffersARB(disp) GET_by_offset(disp, _gloffset_DeleteBuffersARB)
#define SET_DeleteBuffersARB(disp, fn) SET_by_offset(disp, _gloffset_DeleteBuffersARB, fn)
#define CALL_GenBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenBuffersARB, parameters)
#define GET_GenBuffersARB(disp) GET_by_offset(disp, _gloffset_GenBuffersARB)
#define SET_GenBuffersARB(disp, fn) SET_by_offset(disp, _gloffset_GenBuffersARB, fn)
#define CALL_GetBufferParameterivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetBufferParameterivARB, parameters)
#define GET_GetBufferParameterivARB(disp) GET_by_offset(disp, _gloffset_GetBufferParameterivARB)
#define SET_GetBufferParameterivARB(disp, fn) SET_by_offset(disp, _gloffset_GetBufferParameterivARB, fn)
#define CALL_GetBufferPointervARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid **)), _gloffset_GetBufferPointervARB, parameters)
#define GET_GetBufferPointervARB(disp) GET_by_offset(disp, _gloffset_GetBufferPointervARB)
#define SET_GetBufferPointervARB(disp, fn) SET_by_offset(disp, _gloffset_GetBufferPointervARB, fn)
#define CALL_GetBufferSubDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *)), _gloffset_GetBufferSubDataARB, parameters)
#define GET_GetBufferSubDataARB(disp) GET_by_offset(disp, _gloffset_GetBufferSubDataARB)
#define SET_GetBufferSubDataARB(disp, fn) SET_by_offset(disp, _gloffset_GetBufferSubDataARB, fn)
#define CALL_IsBufferARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsBufferARB, parameters)
#define GET_IsBufferARB(disp) GET_by_offset(disp, _gloffset_IsBufferARB)
#define SET_IsBufferARB(disp, fn) SET_by_offset(disp, _gloffset_IsBufferARB, fn)
#define CALL_MapBufferARB(disp, parameters) CALL_by_offset(disp, (GLvoid * (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_MapBufferARB, parameters)
#define GET_MapBufferARB(disp) GET_by_offset(disp, _gloffset_MapBufferARB)
#define SET_MapBufferARB(disp, fn) SET_by_offset(disp, _gloffset_MapBufferARB, fn)
#define CALL_UnmapBufferARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLenum)), _gloffset_UnmapBufferARB, parameters)
#define GET_UnmapBufferARB(disp) GET_by_offset(disp, _gloffset_UnmapBufferARB)
#define SET_UnmapBufferARB(disp, fn) SET_by_offset(disp, _gloffset_UnmapBufferARB, fn)
#define CALL_BeginQueryARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BeginQueryARB, parameters)
#define GET_BeginQueryARB(disp) GET_by_offset(disp, _gloffset_BeginQueryARB)
#define SET_BeginQueryARB(disp, fn) SET_by_offset(disp, _gloffset_BeginQueryARB, fn)
#define CALL_DeleteQueriesARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteQueriesARB, parameters)
#define GET_DeleteQueriesARB(disp) GET_by_offset(disp, _gloffset_DeleteQueriesARB)
#define SET_DeleteQueriesARB(disp, fn) SET_by_offset(disp, _gloffset_DeleteQueriesARB, fn)
#define CALL_EndQueryARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_EndQueryARB, parameters)
#define GET_EndQueryARB(disp) GET_by_offset(disp, _gloffset_EndQueryARB)
#define SET_EndQueryARB(disp, fn) SET_by_offset(disp, _gloffset_EndQueryARB, fn)
#define CALL_GenQueriesARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenQueriesARB, parameters)
#define GET_GenQueriesARB(disp) GET_by_offset(disp, _gloffset_GenQueriesARB)
#define SET_GenQueriesARB(disp, fn) SET_by_offset(disp, _gloffset_GenQueriesARB, fn)
#define CALL_GetQueryObjectivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetQueryObjectivARB, parameters)
#define GET_GetQueryObjectivARB(disp) GET_by_offset(disp, _gloffset_GetQueryObjectivARB)
#define SET_GetQueryObjectivARB(disp, fn) SET_by_offset(disp, _gloffset_GetQueryObjectivARB, fn)
#define CALL_GetQueryObjectuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint *)), _gloffset_GetQueryObjectuivARB, parameters)
#define GET_GetQueryObjectuivARB(disp) GET_by_offset(disp, _gloffset_GetQueryObjectuivARB)
#define SET_GetQueryObjectuivARB(disp, fn) SET_by_offset(disp, _gloffset_GetQueryObjectuivARB, fn)
#define CALL_GetQueryivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetQueryivARB, parameters)
#define GET_GetQueryivARB(disp) GET_by_offset(disp, _gloffset_GetQueryivARB)
#define SET_GetQueryivARB(disp, fn) SET_by_offset(disp, _gloffset_GetQueryivARB, fn)
#define CALL_IsQueryARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsQueryARB, parameters)
#define GET_IsQueryARB(disp) GET_by_offset(disp, _gloffset_IsQueryARB)
#define SET_IsQueryARB(disp, fn) SET_by_offset(disp, _gloffset_IsQueryARB, fn)
#define CALL_AttachObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLhandleARB)), _gloffset_AttachObjectARB, parameters)
#define GET_AttachObjectARB(disp) GET_by_offset(disp, _gloffset_AttachObjectARB)
#define SET_AttachObjectARB(disp, fn) SET_by_offset(disp, _gloffset_AttachObjectARB, fn)
#define CALL_CompileShaderARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_CompileShaderARB, parameters)
#define GET_CompileShaderARB(disp) GET_by_offset(disp, _gloffset_CompileShaderARB)
#define SET_CompileShaderARB(disp, fn) SET_by_offset(disp, _gloffset_CompileShaderARB, fn)
#define CALL_CreateProgramObjectARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(void)), _gloffset_CreateProgramObjectARB, parameters)
#define GET_CreateProgramObjectARB(disp) GET_by_offset(disp, _gloffset_CreateProgramObjectARB)
#define SET_CreateProgramObjectARB(disp, fn) SET_by_offset(disp, _gloffset_CreateProgramObjectARB, fn)
#define CALL_CreateShaderObjectARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(GLenum)), _gloffset_CreateShaderObjectARB, parameters)
#define GET_CreateShaderObjectARB(disp) GET_by_offset(disp, _gloffset_CreateShaderObjectARB)
#define SET_CreateShaderObjectARB(disp, fn) SET_by_offset(disp, _gloffset_CreateShaderObjectARB, fn)
#define CALL_DeleteObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_DeleteObjectARB, parameters)
#define GET_DeleteObjectARB(disp) GET_by_offset(disp, _gloffset_DeleteObjectARB)
#define SET_DeleteObjectARB(disp, fn) SET_by_offset(disp, _gloffset_DeleteObjectARB, fn)
#define CALL_DetachObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLhandleARB)), _gloffset_DetachObjectARB, parameters)
#define GET_DetachObjectARB(disp) GET_by_offset(disp, _gloffset_DetachObjectARB)
#define SET_DetachObjectARB(disp, fn) SET_by_offset(disp, _gloffset_DetachObjectARB, fn)
#define CALL_GetActiveUniformARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)), _gloffset_GetActiveUniformARB, parameters)
#define GET_GetActiveUniformARB(disp) GET_by_offset(disp, _gloffset_GetActiveUniformARB)
#define SET_GetActiveUniformARB(disp, fn) SET_by_offset(disp, _gloffset_GetActiveUniformARB, fn)
#define CALL_GetAttachedObjectsARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *)), _gloffset_GetAttachedObjectsARB, parameters)
#define GET_GetAttachedObjectsARB(disp) GET_by_offset(disp, _gloffset_GetAttachedObjectsARB)
#define SET_GetAttachedObjectsARB(disp, fn) SET_by_offset(disp, _gloffset_GetAttachedObjectsARB, fn)
#define CALL_GetHandleARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(GLenum)), _gloffset_GetHandleARB, parameters)
#define GET_GetHandleARB(disp) GET_by_offset(disp, _gloffset_GetHandleARB)
#define SET_GetHandleARB(disp, fn) SET_by_offset(disp, _gloffset_GetHandleARB, fn)
#define CALL_GetInfoLogARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)), _gloffset_GetInfoLogARB, parameters)
#define GET_GetInfoLogARB(disp) GET_by_offset(disp, _gloffset_GetInfoLogARB)
#define SET_GetInfoLogARB(disp, fn) SET_by_offset(disp, _gloffset_GetInfoLogARB, fn)
#define CALL_GetObjectParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLenum, GLfloat *)), _gloffset_GetObjectParameterfvARB, parameters)
#define GET_GetObjectParameterfvARB(disp) GET_by_offset(disp, _gloffset_GetObjectParameterfvARB)
#define SET_GetObjectParameterfvARB(disp, fn) SET_by_offset(disp, _gloffset_GetObjectParameterfvARB, fn)
#define CALL_GetObjectParameterivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLenum, GLint *)), _gloffset_GetObjectParameterivARB, parameters)
#define GET_GetObjectParameterivARB(disp) GET_by_offset(disp, _gloffset_GetObjectParameterivARB)
#define SET_GetObjectParameterivARB(disp, fn) SET_by_offset(disp, _gloffset_GetObjectParameterivARB, fn)
#define CALL_GetShaderSourceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)), _gloffset_GetShaderSourceARB, parameters)
#define GET_GetShaderSourceARB(disp) GET_by_offset(disp, _gloffset_GetShaderSourceARB)
#define SET_GetShaderSourceARB(disp, fn) SET_by_offset(disp, _gloffset_GetShaderSourceARB, fn)
#define CALL_GetUniformLocationARB(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLhandleARB, const GLcharARB *)), _gloffset_GetUniformLocationARB, parameters)
#define GET_GetUniformLocationARB(disp) GET_by_offset(disp, _gloffset_GetUniformLocationARB)
#define SET_GetUniformLocationARB(disp, fn) SET_by_offset(disp, _gloffset_GetUniformLocationARB, fn)
#define CALL_GetUniformfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLfloat *)), _gloffset_GetUniformfvARB, parameters)
#define GET_GetUniformfvARB(disp) GET_by_offset(disp, _gloffset_GetUniformfvARB)
#define SET_GetUniformfvARB(disp, fn) SET_by_offset(disp, _gloffset_GetUniformfvARB, fn)
#define CALL_GetUniformivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLint *)), _gloffset_GetUniformivARB, parameters)
#define GET_GetUniformivARB(disp) GET_by_offset(disp, _gloffset_GetUniformivARB)
#define SET_GetUniformivARB(disp, fn) SET_by_offset(disp, _gloffset_GetUniformivARB, fn)
#define CALL_LinkProgramARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_LinkProgramARB, parameters)
#define GET_LinkProgramARB(disp) GET_by_offset(disp, _gloffset_LinkProgramARB)
#define SET_LinkProgramARB(disp, fn) SET_by_offset(disp, _gloffset_LinkProgramARB, fn)
#define CALL_ShaderSourceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *)), _gloffset_ShaderSourceARB, parameters)
#define GET_ShaderSourceARB(disp) GET_by_offset(disp, _gloffset_ShaderSourceARB)
#define SET_ShaderSourceARB(disp, fn) SET_by_offset(disp, _gloffset_ShaderSourceARB, fn)
#define CALL_Uniform1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat)), _gloffset_Uniform1fARB, parameters)
#define GET_Uniform1fARB(disp) GET_by_offset(disp, _gloffset_Uniform1fARB)
#define SET_Uniform1fARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform1fARB, fn)
#define CALL_Uniform1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform1fvARB, parameters)
#define GET_Uniform1fvARB(disp) GET_by_offset(disp, _gloffset_Uniform1fvARB)
#define SET_Uniform1fvARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform1fvARB, fn)
#define CALL_Uniform1iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_Uniform1iARB, parameters)
#define GET_Uniform1iARB(disp) GET_by_offset(disp, _gloffset_Uniform1iARB)
#define SET_Uniform1iARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform1iARB, fn)
#define CALL_Uniform1ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform1ivARB, parameters)
#define GET_Uniform1ivARB(disp) GET_by_offset(disp, _gloffset_Uniform1ivARB)
#define SET_Uniform1ivARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform1ivARB, fn)
#define CALL_Uniform2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat)), _gloffset_Uniform2fARB, parameters)
#define GET_Uniform2fARB(disp) GET_by_offset(disp, _gloffset_Uniform2fARB)
#define SET_Uniform2fARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform2fARB, fn)
#define CALL_Uniform2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform2fvARB, parameters)
#define GET_Uniform2fvARB(disp) GET_by_offset(disp, _gloffset_Uniform2fvARB)
#define SET_Uniform2fvARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform2fvARB, fn)
#define CALL_Uniform2iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Uniform2iARB, parameters)
#define GET_Uniform2iARB(disp) GET_by_offset(disp, _gloffset_Uniform2iARB)
#define SET_Uniform2iARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform2iARB, fn)
#define CALL_Uniform2ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform2ivARB, parameters)
#define GET_Uniform2ivARB(disp) GET_by_offset(disp, _gloffset_Uniform2ivARB)
#define SET_Uniform2ivARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform2ivARB, fn)
#define CALL_Uniform3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLfloat)), _gloffset_Uniform3fARB, parameters)
#define GET_Uniform3fARB(disp) GET_by_offset(disp, _gloffset_Uniform3fARB)
#define SET_Uniform3fARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform3fARB, fn)
#define CALL_Uniform3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform3fvARB, parameters)
#define GET_Uniform3fvARB(disp) GET_by_offset(disp, _gloffset_Uniform3fvARB)
#define SET_Uniform3fvARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform3fvARB, fn)
#define CALL_Uniform3iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Uniform3iARB, parameters)
#define GET_Uniform3iARB(disp) GET_by_offset(disp, _gloffset_Uniform3iARB)
#define SET_Uniform3iARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform3iARB, fn)
#define CALL_Uniform3ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform3ivARB, parameters)
#define GET_Uniform3ivARB(disp) GET_by_offset(disp, _gloffset_Uniform3ivARB)
#define SET_Uniform3ivARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform3ivARB, fn)
#define CALL_Uniform4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Uniform4fARB, parameters)
#define GET_Uniform4fARB(disp) GET_by_offset(disp, _gloffset_Uniform4fARB)
#define SET_Uniform4fARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform4fARB, fn)
#define CALL_Uniform4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform4fvARB, parameters)
#define GET_Uniform4fvARB(disp) GET_by_offset(disp, _gloffset_Uniform4fvARB)
#define SET_Uniform4fvARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform4fvARB, fn)
#define CALL_Uniform4iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint, GLint)), _gloffset_Uniform4iARB, parameters)
#define GET_Uniform4iARB(disp) GET_by_offset(disp, _gloffset_Uniform4iARB)
#define SET_Uniform4iARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform4iARB, fn)
#define CALL_Uniform4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform4ivARB, parameters)
#define GET_Uniform4ivARB(disp) GET_by_offset(disp, _gloffset_Uniform4ivARB)
#define SET_Uniform4ivARB(disp, fn) SET_by_offset(disp, _gloffset_Uniform4ivARB, fn)
#define CALL_UniformMatrix2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix2fvARB, parameters)
#define GET_UniformMatrix2fvARB(disp) GET_by_offset(disp, _gloffset_UniformMatrix2fvARB)
#define SET_UniformMatrix2fvARB(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix2fvARB, fn)
#define CALL_UniformMatrix3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix3fvARB, parameters)
#define GET_UniformMatrix3fvARB(disp) GET_by_offset(disp, _gloffset_UniformMatrix3fvARB)
#define SET_UniformMatrix3fvARB(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix3fvARB, fn)
#define CALL_UniformMatrix4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix4fvARB, parameters)
#define GET_UniformMatrix4fvARB(disp) GET_by_offset(disp, _gloffset_UniformMatrix4fvARB)
#define SET_UniformMatrix4fvARB(disp, fn) SET_by_offset(disp, _gloffset_UniformMatrix4fvARB, fn)
#define CALL_UseProgramObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_UseProgramObjectARB, parameters)
#define GET_UseProgramObjectARB(disp) GET_by_offset(disp, _gloffset_UseProgramObjectARB)
#define SET_UseProgramObjectARB(disp, fn) SET_by_offset(disp, _gloffset_UseProgramObjectARB, fn)
#define CALL_ValidateProgramARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_ValidateProgramARB, parameters)
#define GET_ValidateProgramARB(disp) GET_by_offset(disp, _gloffset_ValidateProgramARB)
#define SET_ValidateProgramARB(disp, fn) SET_by_offset(disp, _gloffset_ValidateProgramARB, fn)
#define CALL_BindAttribLocationARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, const GLcharARB *)), _gloffset_BindAttribLocationARB, parameters)
#define GET_BindAttribLocationARB(disp) GET_by_offset(disp, _gloffset_BindAttribLocationARB)
#define SET_BindAttribLocationARB(disp, fn) SET_by_offset(disp, _gloffset_BindAttribLocationARB, fn)
#define CALL_GetActiveAttribARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)), _gloffset_GetActiveAttribARB, parameters)
#define GET_GetActiveAttribARB(disp) GET_by_offset(disp, _gloffset_GetActiveAttribARB)
#define SET_GetActiveAttribARB(disp, fn) SET_by_offset(disp, _gloffset_GetActiveAttribARB, fn)
#define CALL_GetAttribLocationARB(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLhandleARB, const GLcharARB *)), _gloffset_GetAttribLocationARB, parameters)
#define GET_GetAttribLocationARB(disp) GET_by_offset(disp, _gloffset_GetAttribLocationARB)
#define SET_GetAttribLocationARB(disp, fn) SET_by_offset(disp, _gloffset_GetAttribLocationARB, fn)
#define CALL_DrawBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLenum *)), _gloffset_DrawBuffersARB, parameters)
#define GET_DrawBuffersARB(disp) GET_by_offset(disp, _gloffset_DrawBuffersARB)
#define SET_DrawBuffersARB(disp, fn) SET_by_offset(disp, _gloffset_DrawBuffersARB, fn)
#define CALL_RenderbufferStorageMultisample(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, GLsizei, GLsizei)), _gloffset_RenderbufferStorageMultisample, parameters)
#define GET_RenderbufferStorageMultisample(disp) GET_by_offset(disp, _gloffset_RenderbufferStorageMultisample)
#define SET_RenderbufferStorageMultisample(disp, fn) SET_by_offset(disp, _gloffset_RenderbufferStorageMultisample, fn)
#define CALL_FramebufferTextureARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint, GLint)), _gloffset_FramebufferTextureARB, parameters)
#define GET_FramebufferTextureARB(disp) GET_by_offset(disp, _gloffset_FramebufferTextureARB)
#define SET_FramebufferTextureARB(disp, fn) SET_by_offset(disp, _gloffset_FramebufferTextureARB, fn)
#define CALL_FramebufferTextureFaceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint, GLint, GLenum)), _gloffset_FramebufferTextureFaceARB, parameters)
#define GET_FramebufferTextureFaceARB(disp) GET_by_offset(disp, _gloffset_FramebufferTextureFaceARB)
#define SET_FramebufferTextureFaceARB(disp, fn) SET_by_offset(disp, _gloffset_FramebufferTextureFaceARB, fn)
#define CALL_ProgramParameteriARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint)), _gloffset_ProgramParameteriARB, parameters)
#define GET_ProgramParameteriARB(disp) GET_by_offset(disp, _gloffset_ProgramParameteriARB)
#define SET_ProgramParameteriARB(disp, fn) SET_by_offset(disp, _gloffset_ProgramParameteriARB, fn)
#define CALL_FlushMappedBufferRange(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptr, GLsizeiptr)), _gloffset_FlushMappedBufferRange, parameters)
#define GET_FlushMappedBufferRange(disp) GET_by_offset(disp, _gloffset_FlushMappedBufferRange)
#define SET_FlushMappedBufferRange(disp, fn) SET_by_offset(disp, _gloffset_FlushMappedBufferRange, fn)
#define CALL_MapBufferRange(disp, parameters) CALL_by_offset(disp, (GLvoid * (GLAPIENTRYP)(GLenum, GLintptr, GLsizeiptr, GLbitfield)), _gloffset_MapBufferRange, parameters)
#define GET_MapBufferRange(disp) GET_by_offset(disp, _gloffset_MapBufferRange)
#define SET_MapBufferRange(disp, fn) SET_by_offset(disp, _gloffset_MapBufferRange, fn)
#define CALL_BindVertexArray(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_BindVertexArray, parameters)
#define GET_BindVertexArray(disp) GET_by_offset(disp, _gloffset_BindVertexArray)
#define SET_BindVertexArray(disp, fn) SET_by_offset(disp, _gloffset_BindVertexArray, fn)
#define CALL_GenVertexArrays(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenVertexArrays, parameters)
#define GET_GenVertexArrays(disp) GET_by_offset(disp, _gloffset_GenVertexArrays)
#define SET_GenVertexArrays(disp, fn) SET_by_offset(disp, _gloffset_GenVertexArrays, fn)
#define CALL_CopyBufferSubData(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr)), _gloffset_CopyBufferSubData, parameters)
#define GET_CopyBufferSubData(disp) GET_by_offset(disp, _gloffset_CopyBufferSubData)
#define SET_CopyBufferSubData(disp, fn) SET_by_offset(disp, _gloffset_CopyBufferSubData, fn)
#define CALL_ClientWaitSync(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLsync, GLbitfield, GLuint64)), _gloffset_ClientWaitSync, parameters)
#define GET_ClientWaitSync(disp) GET_by_offset(disp, _gloffset_ClientWaitSync)
#define SET_ClientWaitSync(disp, fn) SET_by_offset(disp, _gloffset_ClientWaitSync, fn)
#define CALL_DeleteSync(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsync)), _gloffset_DeleteSync, parameters)
#define GET_DeleteSync(disp) GET_by_offset(disp, _gloffset_DeleteSync)
#define SET_DeleteSync(disp, fn) SET_by_offset(disp, _gloffset_DeleteSync, fn)
#define CALL_FenceSync(disp, parameters) CALL_by_offset(disp, (GLsync (GLAPIENTRYP)(GLenum, GLbitfield)), _gloffset_FenceSync, parameters)
#define GET_FenceSync(disp) GET_by_offset(disp, _gloffset_FenceSync)
#define SET_FenceSync(disp, fn) SET_by_offset(disp, _gloffset_FenceSync, fn)
#define CALL_GetInteger64v(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint64 *)), _gloffset_GetInteger64v, parameters)
#define GET_GetInteger64v(disp) GET_by_offset(disp, _gloffset_GetInteger64v)
#define SET_GetInteger64v(disp, fn) SET_by_offset(disp, _gloffset_GetInteger64v, fn)
#define CALL_GetSynciv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsync, GLenum, GLsizei, GLsizei *, GLint *)), _gloffset_GetSynciv, parameters)
#define GET_GetSynciv(disp) GET_by_offset(disp, _gloffset_GetSynciv)
#define SET_GetSynciv(disp, fn) SET_by_offset(disp, _gloffset_GetSynciv, fn)
#define CALL_IsSync(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLsync)), _gloffset_IsSync, parameters)
#define GET_IsSync(disp) GET_by_offset(disp, _gloffset_IsSync)
#define SET_IsSync(disp, fn) SET_by_offset(disp, _gloffset_IsSync, fn)
#define CALL_WaitSync(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsync, GLbitfield, GLuint64)), _gloffset_WaitSync, parameters)
#define GET_WaitSync(disp) GET_by_offset(disp, _gloffset_WaitSync)
#define SET_WaitSync(disp, fn) SET_by_offset(disp, _gloffset_WaitSync, fn)
#define CALL_DrawElementsBaseVertex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, const GLvoid *, GLint)), _gloffset_DrawElementsBaseVertex, parameters)
#define GET_DrawElementsBaseVertex(disp) GET_by_offset(disp, _gloffset_DrawElementsBaseVertex)
#define SET_DrawElementsBaseVertex(disp, fn) SET_by_offset(disp, _gloffset_DrawElementsBaseVertex, fn)
#define CALL_DrawRangeElementsBaseVertex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *, GLint)), _gloffset_DrawRangeElementsBaseVertex, parameters)
#define GET_DrawRangeElementsBaseVertex(disp) GET_by_offset(disp, _gloffset_DrawRangeElementsBaseVertex)
#define SET_DrawRangeElementsBaseVertex(disp, fn) SET_by_offset(disp, _gloffset_DrawRangeElementsBaseVertex, fn)
#define CALL_MultiDrawElementsBaseVertex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei, const GLint *)), _gloffset_MultiDrawElementsBaseVertex, parameters)
#define GET_MultiDrawElementsBaseVertex(disp) GET_by_offset(disp, _gloffset_MultiDrawElementsBaseVertex)
#define SET_MultiDrawElementsBaseVertex(disp, fn) SET_by_offset(disp, _gloffset_MultiDrawElementsBaseVertex, fn)
#define CALL_BindTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindTransformFeedback, parameters)
#define GET_BindTransformFeedback(disp) GET_by_offset(disp, _gloffset_BindTransformFeedback)
#define SET_BindTransformFeedback(disp, fn) SET_by_offset(disp, _gloffset_BindTransformFeedback, fn)
#define CALL_DeleteTransformFeedbacks(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteTransformFeedbacks, parameters)
#define GET_DeleteTransformFeedbacks(disp) GET_by_offset(disp, _gloffset_DeleteTransformFeedbacks)
#define SET_DeleteTransformFeedbacks(disp, fn) SET_by_offset(disp, _gloffset_DeleteTransformFeedbacks, fn)
#define CALL_DrawTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_DrawTransformFeedback, parameters)
#define GET_DrawTransformFeedback(disp) GET_by_offset(disp, _gloffset_DrawTransformFeedback)
#define SET_DrawTransformFeedback(disp, fn) SET_by_offset(disp, _gloffset_DrawTransformFeedback, fn)
#define CALL_GenTransformFeedbacks(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenTransformFeedbacks, parameters)
#define GET_GenTransformFeedbacks(disp) GET_by_offset(disp, _gloffset_GenTransformFeedbacks)
#define SET_GenTransformFeedbacks(disp, fn) SET_by_offset(disp, _gloffset_GenTransformFeedbacks, fn)
#define CALL_IsTransformFeedback(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsTransformFeedback, parameters)
#define GET_IsTransformFeedback(disp) GET_by_offset(disp, _gloffset_IsTransformFeedback)
#define SET_IsTransformFeedback(disp, fn) SET_by_offset(disp, _gloffset_IsTransformFeedback, fn)
#define CALL_PauseTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PauseTransformFeedback, parameters)
#define GET_PauseTransformFeedback(disp) GET_by_offset(disp, _gloffset_PauseTransformFeedback)
#define SET_PauseTransformFeedback(disp, fn) SET_by_offset(disp, _gloffset_PauseTransformFeedback, fn)
#define CALL_ResumeTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_ResumeTransformFeedback, parameters)
#define GET_ResumeTransformFeedback(disp) GET_by_offset(disp, _gloffset_ResumeTransformFeedback)
#define SET_ResumeTransformFeedback(disp, fn) SET_by_offset(disp, _gloffset_ResumeTransformFeedback, fn)
#define CALL_PolygonOffsetEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_PolygonOffsetEXT, parameters)
#define GET_PolygonOffsetEXT(disp) GET_by_offset(disp, _gloffset_PolygonOffsetEXT)
#define SET_PolygonOffsetEXT(disp, fn) SET_by_offset(disp, _gloffset_PolygonOffsetEXT, fn)
#define CALL_GetPixelTexGenParameterfvSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetPixelTexGenParameterfvSGIS, parameters)
#define GET_GetPixelTexGenParameterfvSGIS(disp) GET_by_offset(disp, _gloffset_GetPixelTexGenParameterfvSGIS)
#define SET_GetPixelTexGenParameterfvSGIS(disp, fn) SET_by_offset(disp, _gloffset_GetPixelTexGenParameterfvSGIS, fn)
#define CALL_GetPixelTexGenParameterivSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *)), _gloffset_GetPixelTexGenParameterivSGIS, parameters)
#define GET_GetPixelTexGenParameterivSGIS(disp) GET_by_offset(disp, _gloffset_GetPixelTexGenParameterivSGIS)
#define SET_GetPixelTexGenParameterivSGIS(disp, fn) SET_by_offset(disp, _gloffset_GetPixelTexGenParameterivSGIS, fn)
#define CALL_PixelTexGenParameterfSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PixelTexGenParameterfSGIS, parameters)
#define GET_PixelTexGenParameterfSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameterfSGIS)
#define SET_PixelTexGenParameterfSGIS(disp, fn) SET_by_offset(disp, _gloffset_PixelTexGenParameterfSGIS, fn)
#define CALL_PixelTexGenParameterfvSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_PixelTexGenParameterfvSGIS, parameters)
#define GET_PixelTexGenParameterfvSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameterfvSGIS)
#define SET_PixelTexGenParameterfvSGIS(disp, fn) SET_by_offset(disp, _gloffset_PixelTexGenParameterfvSGIS, fn)
#define CALL_PixelTexGenParameteriSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PixelTexGenParameteriSGIS, parameters)
#define GET_PixelTexGenParameteriSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameteriSGIS)
#define SET_PixelTexGenParameteriSGIS(disp, fn) SET_by_offset(disp, _gloffset_PixelTexGenParameteriSGIS, fn)
#define CALL_PixelTexGenParameterivSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_PixelTexGenParameterivSGIS, parameters)
#define GET_PixelTexGenParameterivSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameterivSGIS)
#define SET_PixelTexGenParameterivSGIS(disp, fn) SET_by_offset(disp, _gloffset_PixelTexGenParameterivSGIS, fn)
#define CALL_SampleMaskSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLboolean)), _gloffset_SampleMaskSGIS, parameters)
#define GET_SampleMaskSGIS(disp) GET_by_offset(disp, _gloffset_SampleMaskSGIS)
#define SET_SampleMaskSGIS(disp, fn) SET_by_offset(disp, _gloffset_SampleMaskSGIS, fn)
#define CALL_SamplePatternSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_SamplePatternSGIS, parameters)
#define GET_SamplePatternSGIS(disp) GET_by_offset(disp, _gloffset_SamplePatternSGIS)
#define SET_SamplePatternSGIS(disp, fn) SET_by_offset(disp, _gloffset_SamplePatternSGIS, fn)
#define CALL_ColorPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_ColorPointerEXT, parameters)
#define GET_ColorPointerEXT(disp) GET_by_offset(disp, _gloffset_ColorPointerEXT)
#define SET_ColorPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_ColorPointerEXT, fn)
#define CALL_EdgeFlagPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLsizei, const GLboolean *)), _gloffset_EdgeFlagPointerEXT, parameters)
#define GET_EdgeFlagPointerEXT(disp) GET_by_offset(disp, _gloffset_EdgeFlagPointerEXT)
#define SET_EdgeFlagPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_EdgeFlagPointerEXT, fn)
#define CALL_IndexPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_IndexPointerEXT, parameters)
#define GET_IndexPointerEXT(disp) GET_by_offset(disp, _gloffset_IndexPointerEXT)
#define SET_IndexPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_IndexPointerEXT, fn)
#define CALL_NormalPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_NormalPointerEXT, parameters)
#define GET_NormalPointerEXT(disp) GET_by_offset(disp, _gloffset_NormalPointerEXT)
#define SET_NormalPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_NormalPointerEXT, fn)
#define CALL_TexCoordPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_TexCoordPointerEXT, parameters)
#define GET_TexCoordPointerEXT(disp) GET_by_offset(disp, _gloffset_TexCoordPointerEXT)
#define SET_TexCoordPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_TexCoordPointerEXT, fn)
#define CALL_VertexPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_VertexPointerEXT, parameters)
#define GET_VertexPointerEXT(disp) GET_by_offset(disp, _gloffset_VertexPointerEXT)
#define SET_VertexPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexPointerEXT, fn)
#define CALL_PointParameterfEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PointParameterfEXT, parameters)
#define GET_PointParameterfEXT(disp) GET_by_offset(disp, _gloffset_PointParameterfEXT)
#define SET_PointParameterfEXT(disp, fn) SET_by_offset(disp, _gloffset_PointParameterfEXT, fn)
#define CALL_PointParameterfvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_PointParameterfvEXT, parameters)
#define GET_PointParameterfvEXT(disp) GET_by_offset(disp, _gloffset_PointParameterfvEXT)
#define SET_PointParameterfvEXT(disp, fn) SET_by_offset(disp, _gloffset_PointParameterfvEXT, fn)
#define CALL_LockArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei)), _gloffset_LockArraysEXT, parameters)
#define GET_LockArraysEXT(disp) GET_by_offset(disp, _gloffset_LockArraysEXT)
#define SET_LockArraysEXT(disp, fn) SET_by_offset(disp, _gloffset_LockArraysEXT, fn)
#define CALL_UnlockArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_UnlockArraysEXT, parameters)
#define GET_UnlockArraysEXT(disp) GET_by_offset(disp, _gloffset_UnlockArraysEXT)
#define SET_UnlockArraysEXT(disp, fn) SET_by_offset(disp, _gloffset_UnlockArraysEXT, fn)
#define CALL_SecondaryColor3bEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte)), _gloffset_SecondaryColor3bEXT, parameters)
#define GET_SecondaryColor3bEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3bEXT)
#define SET_SecondaryColor3bEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3bEXT, fn)
#define CALL_SecondaryColor3bvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_SecondaryColor3bvEXT, parameters)
#define GET_SecondaryColor3bvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3bvEXT)
#define SET_SecondaryColor3bvEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3bvEXT, fn)
#define CALL_SecondaryColor3dEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_SecondaryColor3dEXT, parameters)
#define GET_SecondaryColor3dEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3dEXT)
#define SET_SecondaryColor3dEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3dEXT, fn)
#define CALL_SecondaryColor3dvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_SecondaryColor3dvEXT, parameters)
#define GET_SecondaryColor3dvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3dvEXT)
#define SET_SecondaryColor3dvEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3dvEXT, fn)
#define CALL_SecondaryColor3fEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_SecondaryColor3fEXT, parameters)
#define GET_SecondaryColor3fEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3fEXT)
#define SET_SecondaryColor3fEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3fEXT, fn)
#define CALL_SecondaryColor3fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_SecondaryColor3fvEXT, parameters)
#define GET_SecondaryColor3fvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3fvEXT)
#define SET_SecondaryColor3fvEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3fvEXT, fn)
#define CALL_SecondaryColor3iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_SecondaryColor3iEXT, parameters)
#define GET_SecondaryColor3iEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3iEXT)
#define SET_SecondaryColor3iEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3iEXT, fn)
#define CALL_SecondaryColor3ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_SecondaryColor3ivEXT, parameters)
#define GET_SecondaryColor3ivEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3ivEXT)
#define SET_SecondaryColor3ivEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3ivEXT, fn)
#define CALL_SecondaryColor3sEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_SecondaryColor3sEXT, parameters)
#define GET_SecondaryColor3sEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3sEXT)
#define SET_SecondaryColor3sEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3sEXT, fn)
#define CALL_SecondaryColor3svEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_SecondaryColor3svEXT, parameters)
#define GET_SecondaryColor3svEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3svEXT)
#define SET_SecondaryColor3svEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3svEXT, fn)
#define CALL_SecondaryColor3ubEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte, GLubyte, GLubyte)), _gloffset_SecondaryColor3ubEXT, parameters)
#define GET_SecondaryColor3ubEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3ubEXT)
#define SET_SecondaryColor3ubEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3ubEXT, fn)
#define CALL_SecondaryColor3ubvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_SecondaryColor3ubvEXT, parameters)
#define GET_SecondaryColor3ubvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3ubvEXT)
#define SET_SecondaryColor3ubvEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3ubvEXT, fn)
#define CALL_SecondaryColor3uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint)), _gloffset_SecondaryColor3uiEXT, parameters)
#define GET_SecondaryColor3uiEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3uiEXT)
#define SET_SecondaryColor3uiEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3uiEXT, fn)
#define CALL_SecondaryColor3uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLuint *)), _gloffset_SecondaryColor3uivEXT, parameters)
#define GET_SecondaryColor3uivEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3uivEXT)
#define SET_SecondaryColor3uivEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3uivEXT, fn)
#define CALL_SecondaryColor3usEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLushort, GLushort, GLushort)), _gloffset_SecondaryColor3usEXT, parameters)
#define GET_SecondaryColor3usEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3usEXT)
#define SET_SecondaryColor3usEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3usEXT, fn)
#define CALL_SecondaryColor3usvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLushort *)), _gloffset_SecondaryColor3usvEXT, parameters)
#define GET_SecondaryColor3usvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3usvEXT)
#define SET_SecondaryColor3usvEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColor3usvEXT, fn)
#define CALL_SecondaryColorPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_SecondaryColorPointerEXT, parameters)
#define GET_SecondaryColorPointerEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColorPointerEXT)
#define SET_SecondaryColorPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_SecondaryColorPointerEXT, fn)
#define CALL_MultiDrawArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *, const GLsizei *, GLsizei)), _gloffset_MultiDrawArraysEXT, parameters)
#define GET_MultiDrawArraysEXT(disp) GET_by_offset(disp, _gloffset_MultiDrawArraysEXT)
#define SET_MultiDrawArraysEXT(disp, fn) SET_by_offset(disp, _gloffset_MultiDrawArraysEXT, fn)
#define CALL_MultiDrawElementsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei)), _gloffset_MultiDrawElementsEXT, parameters)
#define GET_MultiDrawElementsEXT(disp) GET_by_offset(disp, _gloffset_MultiDrawElementsEXT)
#define SET_MultiDrawElementsEXT(disp, fn) SET_by_offset(disp, _gloffset_MultiDrawElementsEXT, fn)
#define CALL_FogCoordPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_FogCoordPointerEXT, parameters)
#define GET_FogCoordPointerEXT(disp) GET_by_offset(disp, _gloffset_FogCoordPointerEXT)
#define SET_FogCoordPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_FogCoordPointerEXT, fn)
#define CALL_FogCoorddEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_FogCoorddEXT, parameters)
#define GET_FogCoorddEXT(disp) GET_by_offset(disp, _gloffset_FogCoorddEXT)
#define SET_FogCoorddEXT(disp, fn) SET_by_offset(disp, _gloffset_FogCoorddEXT, fn)
#define CALL_FogCoorddvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_FogCoorddvEXT, parameters)
#define GET_FogCoorddvEXT(disp) GET_by_offset(disp, _gloffset_FogCoorddvEXT)
#define SET_FogCoorddvEXT(disp, fn) SET_by_offset(disp, _gloffset_FogCoorddvEXT, fn)
#define CALL_FogCoordfEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_FogCoordfEXT, parameters)
#define GET_FogCoordfEXT(disp) GET_by_offset(disp, _gloffset_FogCoordfEXT)
#define SET_FogCoordfEXT(disp, fn) SET_by_offset(disp, _gloffset_FogCoordfEXT, fn)
#define CALL_FogCoordfvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_FogCoordfvEXT, parameters)
#define GET_FogCoordfvEXT(disp) GET_by_offset(disp, _gloffset_FogCoordfvEXT)
#define SET_FogCoordfvEXT(disp, fn) SET_by_offset(disp, _gloffset_FogCoordfvEXT, fn)
#define CALL_PixelTexGenSGIX(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_PixelTexGenSGIX, parameters)
#define GET_PixelTexGenSGIX(disp) GET_by_offset(disp, _gloffset_PixelTexGenSGIX)
#define SET_PixelTexGenSGIX(disp, fn) SET_by_offset(disp, _gloffset_PixelTexGenSGIX, fn)
#define CALL_BlendFuncSeparateEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), _gloffset_BlendFuncSeparateEXT, parameters)
#define GET_BlendFuncSeparateEXT(disp) GET_by_offset(disp, _gloffset_BlendFuncSeparateEXT)
#define SET_BlendFuncSeparateEXT(disp, fn) SET_by_offset(disp, _gloffset_BlendFuncSeparateEXT, fn)
#define CALL_FlushVertexArrayRangeNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_FlushVertexArrayRangeNV, parameters)
#define GET_FlushVertexArrayRangeNV(disp) GET_by_offset(disp, _gloffset_FlushVertexArrayRangeNV)
#define SET_FlushVertexArrayRangeNV(disp, fn) SET_by_offset(disp, _gloffset_FlushVertexArrayRangeNV, fn)
#define CALL_VertexArrayRangeNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLvoid *)), _gloffset_VertexArrayRangeNV, parameters)
#define GET_VertexArrayRangeNV(disp) GET_by_offset(disp, _gloffset_VertexArrayRangeNV)
#define SET_VertexArrayRangeNV(disp, fn) SET_by_offset(disp, _gloffset_VertexArrayRangeNV, fn)
#define CALL_CombinerInputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum)), _gloffset_CombinerInputNV, parameters)
#define GET_CombinerInputNV(disp) GET_by_offset(disp, _gloffset_CombinerInputNV)
#define SET_CombinerInputNV(disp, fn) SET_by_offset(disp, _gloffset_CombinerInputNV, fn)
#define CALL_CombinerOutputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean)), _gloffset_CombinerOutputNV, parameters)
#define GET_CombinerOutputNV(disp) GET_by_offset(disp, _gloffset_CombinerOutputNV)
#define SET_CombinerOutputNV(disp, fn) SET_by_offset(disp, _gloffset_CombinerOutputNV, fn)
#define CALL_CombinerParameterfNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_CombinerParameterfNV, parameters)
#define GET_CombinerParameterfNV(disp) GET_by_offset(disp, _gloffset_CombinerParameterfNV)
#define SET_CombinerParameterfNV(disp, fn) SET_by_offset(disp, _gloffset_CombinerParameterfNV, fn)
#define CALL_CombinerParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_CombinerParameterfvNV, parameters)
#define GET_CombinerParameterfvNV(disp) GET_by_offset(disp, _gloffset_CombinerParameterfvNV)
#define SET_CombinerParameterfvNV(disp, fn) SET_by_offset(disp, _gloffset_CombinerParameterfvNV, fn)
#define CALL_CombinerParameteriNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_CombinerParameteriNV, parameters)
#define GET_CombinerParameteriNV(disp) GET_by_offset(disp, _gloffset_CombinerParameteriNV)
#define SET_CombinerParameteriNV(disp, fn) SET_by_offset(disp, _gloffset_CombinerParameteriNV, fn)
#define CALL_CombinerParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_CombinerParameterivNV, parameters)
#define GET_CombinerParameterivNV(disp) GET_by_offset(disp, _gloffset_CombinerParameterivNV)
#define SET_CombinerParameterivNV(disp, fn) SET_by_offset(disp, _gloffset_CombinerParameterivNV, fn)
#define CALL_FinalCombinerInputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), _gloffset_FinalCombinerInputNV, parameters)
#define GET_FinalCombinerInputNV(disp) GET_by_offset(disp, _gloffset_FinalCombinerInputNV)
#define SET_FinalCombinerInputNV(disp, fn) SET_by_offset(disp, _gloffset_FinalCombinerInputNV, fn)
#define CALL_GetCombinerInputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLfloat *)), _gloffset_GetCombinerInputParameterfvNV, parameters)
#define GET_GetCombinerInputParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetCombinerInputParameterfvNV)
#define SET_GetCombinerInputParameterfvNV(disp, fn) SET_by_offset(disp, _gloffset_GetCombinerInputParameterfvNV, fn)
#define CALL_GetCombinerInputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLint *)), _gloffset_GetCombinerInputParameterivNV, parameters)
#define GET_GetCombinerInputParameterivNV(disp) GET_by_offset(disp, _gloffset_GetCombinerInputParameterivNV)
#define SET_GetCombinerInputParameterivNV(disp, fn) SET_by_offset(disp, _gloffset_GetCombinerInputParameterivNV, fn)
#define CALL_GetCombinerOutputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLfloat *)), _gloffset_GetCombinerOutputParameterfvNV, parameters)
#define GET_GetCombinerOutputParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetCombinerOutputParameterfvNV)
#define SET_GetCombinerOutputParameterfvNV(disp, fn) SET_by_offset(disp, _gloffset_GetCombinerOutputParameterfvNV, fn)
#define CALL_GetCombinerOutputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLint *)), _gloffset_GetCombinerOutputParameterivNV, parameters)
#define GET_GetCombinerOutputParameterivNV(disp) GET_by_offset(disp, _gloffset_GetCombinerOutputParameterivNV)
#define SET_GetCombinerOutputParameterivNV(disp, fn) SET_by_offset(disp, _gloffset_GetCombinerOutputParameterivNV, fn)
#define CALL_GetFinalCombinerInputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetFinalCombinerInputParameterfvNV, parameters)
#define GET_GetFinalCombinerInputParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterfvNV)
#define SET_GetFinalCombinerInputParameterfvNV(disp, fn) SET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterfvNV, fn)
#define CALL_GetFinalCombinerInputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetFinalCombinerInputParameterivNV, parameters)
#define GET_GetFinalCombinerInputParameterivNV(disp) GET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterivNV)
#define SET_GetFinalCombinerInputParameterivNV(disp, fn) SET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterivNV, fn)
#define CALL_ResizeBuffersMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_ResizeBuffersMESA, parameters)
#define GET_ResizeBuffersMESA(disp) GET_by_offset(disp, _gloffset_ResizeBuffersMESA)
#define SET_ResizeBuffersMESA(disp, fn) SET_by_offset(disp, _gloffset_ResizeBuffersMESA, fn)
#define CALL_WindowPos2dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_WindowPos2dMESA, parameters)
#define GET_WindowPos2dMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2dMESA)
#define SET_WindowPos2dMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2dMESA, fn)
#define CALL_WindowPos2dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_WindowPos2dvMESA, parameters)
#define GET_WindowPos2dvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2dvMESA)
#define SET_WindowPos2dvMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2dvMESA, fn)
#define CALL_WindowPos2fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_WindowPos2fMESA, parameters)
#define GET_WindowPos2fMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2fMESA)
#define SET_WindowPos2fMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2fMESA, fn)
#define CALL_WindowPos2fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_WindowPos2fvMESA, parameters)
#define GET_WindowPos2fvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2fvMESA)
#define SET_WindowPos2fvMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2fvMESA, fn)
#define CALL_WindowPos2iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_WindowPos2iMESA, parameters)
#define GET_WindowPos2iMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2iMESA)
#define SET_WindowPos2iMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2iMESA, fn)
#define CALL_WindowPos2ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_WindowPos2ivMESA, parameters)
#define GET_WindowPos2ivMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2ivMESA)
#define SET_WindowPos2ivMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2ivMESA, fn)
#define CALL_WindowPos2sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_WindowPos2sMESA, parameters)
#define GET_WindowPos2sMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2sMESA)
#define SET_WindowPos2sMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2sMESA, fn)
#define CALL_WindowPos2svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_WindowPos2svMESA, parameters)
#define GET_WindowPos2svMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2svMESA)
#define SET_WindowPos2svMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos2svMESA, fn)
#define CALL_WindowPos3dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_WindowPos3dMESA, parameters)
#define GET_WindowPos3dMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3dMESA)
#define SET_WindowPos3dMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3dMESA, fn)
#define CALL_WindowPos3dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_WindowPos3dvMESA, parameters)
#define GET_WindowPos3dvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3dvMESA)
#define SET_WindowPos3dvMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3dvMESA, fn)
#define CALL_WindowPos3fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_WindowPos3fMESA, parameters)
#define GET_WindowPos3fMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3fMESA)
#define SET_WindowPos3fMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3fMESA, fn)
#define CALL_WindowPos3fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_WindowPos3fvMESA, parameters)
#define GET_WindowPos3fvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3fvMESA)
#define SET_WindowPos3fvMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3fvMESA, fn)
#define CALL_WindowPos3iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_WindowPos3iMESA, parameters)
#define GET_WindowPos3iMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3iMESA)
#define SET_WindowPos3iMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3iMESA, fn)
#define CALL_WindowPos3ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_WindowPos3ivMESA, parameters)
#define GET_WindowPos3ivMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3ivMESA)
#define SET_WindowPos3ivMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3ivMESA, fn)
#define CALL_WindowPos3sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_WindowPos3sMESA, parameters)
#define GET_WindowPos3sMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3sMESA)
#define SET_WindowPos3sMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3sMESA, fn)
#define CALL_WindowPos3svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_WindowPos3svMESA, parameters)
#define GET_WindowPos3svMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3svMESA)
#define SET_WindowPos3svMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos3svMESA, fn)
#define CALL_WindowPos4dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_WindowPos4dMESA, parameters)
#define GET_WindowPos4dMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4dMESA)
#define SET_WindowPos4dMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4dMESA, fn)
#define CALL_WindowPos4dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_WindowPos4dvMESA, parameters)
#define GET_WindowPos4dvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4dvMESA)
#define SET_WindowPos4dvMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4dvMESA, fn)
#define CALL_WindowPos4fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_WindowPos4fMESA, parameters)
#define GET_WindowPos4fMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4fMESA)
#define SET_WindowPos4fMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4fMESA, fn)
#define CALL_WindowPos4fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_WindowPos4fvMESA, parameters)
#define GET_WindowPos4fvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4fvMESA)
#define SET_WindowPos4fvMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4fvMESA, fn)
#define CALL_WindowPos4iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_WindowPos4iMESA, parameters)
#define GET_WindowPos4iMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4iMESA)
#define SET_WindowPos4iMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4iMESA, fn)
#define CALL_WindowPos4ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_WindowPos4ivMESA, parameters)
#define GET_WindowPos4ivMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4ivMESA)
#define SET_WindowPos4ivMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4ivMESA, fn)
#define CALL_WindowPos4sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_WindowPos4sMESA, parameters)
#define GET_WindowPos4sMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4sMESA)
#define SET_WindowPos4sMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4sMESA, fn)
#define CALL_WindowPos4svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_WindowPos4svMESA, parameters)
#define GET_WindowPos4svMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4svMESA)
#define SET_WindowPos4svMESA(disp, fn) SET_by_offset(disp, _gloffset_WindowPos4svMESA, fn)
#define CALL_MultiModeDrawArraysIBM(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLenum *, const GLint *, const GLsizei *, GLsizei, GLint)), _gloffset_MultiModeDrawArraysIBM, parameters)
#define GET_MultiModeDrawArraysIBM(disp) GET_by_offset(disp, _gloffset_MultiModeDrawArraysIBM)
#define SET_MultiModeDrawArraysIBM(disp, fn) SET_by_offset(disp, _gloffset_MultiModeDrawArraysIBM, fn)
#define CALL_MultiModeDrawElementsIBM(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLenum *, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, GLint)), _gloffset_MultiModeDrawElementsIBM, parameters)
#define GET_MultiModeDrawElementsIBM(disp) GET_by_offset(disp, _gloffset_MultiModeDrawElementsIBM)
#define SET_MultiModeDrawElementsIBM(disp, fn) SET_by_offset(disp, _gloffset_MultiModeDrawElementsIBM, fn)
#define CALL_DeleteFencesNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteFencesNV, parameters)
#define GET_DeleteFencesNV(disp) GET_by_offset(disp, _gloffset_DeleteFencesNV)
#define SET_DeleteFencesNV(disp, fn) SET_by_offset(disp, _gloffset_DeleteFencesNV, fn)
#define CALL_FinishFenceNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_FinishFenceNV, parameters)
#define GET_FinishFenceNV(disp) GET_by_offset(disp, _gloffset_FinishFenceNV)
#define SET_FinishFenceNV(disp, fn) SET_by_offset(disp, _gloffset_FinishFenceNV, fn)
#define CALL_GenFencesNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenFencesNV, parameters)
#define GET_GenFencesNV(disp) GET_by_offset(disp, _gloffset_GenFencesNV)
#define SET_GenFencesNV(disp, fn) SET_by_offset(disp, _gloffset_GenFencesNV, fn)
#define CALL_GetFenceivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetFenceivNV, parameters)
#define GET_GetFenceivNV(disp) GET_by_offset(disp, _gloffset_GetFenceivNV)
#define SET_GetFenceivNV(disp, fn) SET_by_offset(disp, _gloffset_GetFenceivNV, fn)
#define CALL_IsFenceNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsFenceNV, parameters)
#define GET_IsFenceNV(disp) GET_by_offset(disp, _gloffset_IsFenceNV)
#define SET_IsFenceNV(disp, fn) SET_by_offset(disp, _gloffset_IsFenceNV, fn)
#define CALL_SetFenceNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum)), _gloffset_SetFenceNV, parameters)
#define GET_SetFenceNV(disp) GET_by_offset(disp, _gloffset_SetFenceNV)
#define SET_SetFenceNV(disp, fn) SET_by_offset(disp, _gloffset_SetFenceNV, fn)
#define CALL_TestFenceNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_TestFenceNV, parameters)
#define GET_TestFenceNV(disp) GET_by_offset(disp, _gloffset_TestFenceNV)
#define SET_TestFenceNV(disp, fn) SET_by_offset(disp, _gloffset_TestFenceNV, fn)
#define CALL_AreProgramsResidentNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLsizei, const GLuint *, GLboolean *)), _gloffset_AreProgramsResidentNV, parameters)
#define GET_AreProgramsResidentNV(disp) GET_by_offset(disp, _gloffset_AreProgramsResidentNV)
#define SET_AreProgramsResidentNV(disp, fn) SET_by_offset(disp, _gloffset_AreProgramsResidentNV, fn)
#define CALL_BindProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindProgramNV, parameters)
#define GET_BindProgramNV(disp) GET_by_offset(disp, _gloffset_BindProgramNV)
#define SET_BindProgramNV(disp, fn) SET_by_offset(disp, _gloffset_BindProgramNV, fn)
#define CALL_DeleteProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteProgramsNV, parameters)
#define GET_DeleteProgramsNV(disp) GET_by_offset(disp, _gloffset_DeleteProgramsNV)
#define SET_DeleteProgramsNV(disp, fn) SET_by_offset(disp, _gloffset_DeleteProgramsNV, fn)
#define CALL_ExecuteProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), _gloffset_ExecuteProgramNV, parameters)
#define GET_ExecuteProgramNV(disp) GET_by_offset(disp, _gloffset_ExecuteProgramNV)
#define SET_ExecuteProgramNV(disp, fn) SET_by_offset(disp, _gloffset_ExecuteProgramNV, fn)
#define CALL_GenProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenProgramsNV, parameters)
#define GET_GenProgramsNV(disp) GET_by_offset(disp, _gloffset_GenProgramsNV)
#define SET_GenProgramsNV(disp, fn) SET_by_offset(disp, _gloffset_GenProgramsNV, fn)
#define CALL_GetProgramParameterdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLdouble *)), _gloffset_GetProgramParameterdvNV, parameters)
#define GET_GetProgramParameterdvNV(disp) GET_by_offset(disp, _gloffset_GetProgramParameterdvNV)
#define SET_GetProgramParameterdvNV(disp, fn) SET_by_offset(disp, _gloffset_GetProgramParameterdvNV, fn)
#define CALL_GetProgramParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLfloat *)), _gloffset_GetProgramParameterfvNV, parameters)
#define GET_GetProgramParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetProgramParameterfvNV)
#define SET_GetProgramParameterfvNV(disp, fn) SET_by_offset(disp, _gloffset_GetProgramParameterfvNV, fn)
#define CALL_GetProgramStringNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLubyte *)), _gloffset_GetProgramStringNV, parameters)
#define GET_GetProgramStringNV(disp) GET_by_offset(disp, _gloffset_GetProgramStringNV)
#define SET_GetProgramStringNV(disp, fn) SET_by_offset(disp, _gloffset_GetProgramStringNV, fn)
#define CALL_GetProgramivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetProgramivNV, parameters)
#define GET_GetProgramivNV(disp) GET_by_offset(disp, _gloffset_GetProgramivNV)
#define SET_GetProgramivNV(disp, fn) SET_by_offset(disp, _gloffset_GetProgramivNV, fn)
#define CALL_GetTrackMatrixivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLint *)), _gloffset_GetTrackMatrixivNV, parameters)
#define GET_GetTrackMatrixivNV(disp) GET_by_offset(disp, _gloffset_GetTrackMatrixivNV)
#define SET_GetTrackMatrixivNV(disp, fn) SET_by_offset(disp, _gloffset_GetTrackMatrixivNV, fn)
#define CALL_GetVertexAttribPointervNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLvoid **)), _gloffset_GetVertexAttribPointervNV, parameters)
#define GET_GetVertexAttribPointervNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribPointervNV)
#define SET_GetVertexAttribPointervNV(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribPointervNV, fn)
#define CALL_GetVertexAttribdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLdouble *)), _gloffset_GetVertexAttribdvNV, parameters)
#define GET_GetVertexAttribdvNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribdvNV)
#define SET_GetVertexAttribdvNV(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribdvNV, fn)
#define CALL_GetVertexAttribfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat *)), _gloffset_GetVertexAttribfvNV, parameters)
#define GET_GetVertexAttribfvNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribfvNV)
#define SET_GetVertexAttribfvNV(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribfvNV, fn)
#define CALL_GetVertexAttribivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetVertexAttribivNV, parameters)
#define GET_GetVertexAttribivNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribivNV)
#define SET_GetVertexAttribivNV(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribivNV, fn)
#define CALL_IsProgramNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsProgramNV, parameters)
#define GET_IsProgramNV(disp) GET_by_offset(disp, _gloffset_IsProgramNV)
#define SET_IsProgramNV(disp, fn) SET_by_offset(disp, _gloffset_IsProgramNV, fn)
#define CALL_LoadProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLubyte *)), _gloffset_LoadProgramNV, parameters)
#define GET_LoadProgramNV(disp) GET_by_offset(disp, _gloffset_LoadProgramNV)
#define SET_LoadProgramNV(disp, fn) SET_by_offset(disp, _gloffset_LoadProgramNV, fn)
#define CALL_ProgramParameters4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLdouble *)), _gloffset_ProgramParameters4dvNV, parameters)
#define GET_ProgramParameters4dvNV(disp) GET_by_offset(disp, _gloffset_ProgramParameters4dvNV)
#define SET_ProgramParameters4dvNV(disp, fn) SET_by_offset(disp, _gloffset_ProgramParameters4dvNV, fn)
#define CALL_ProgramParameters4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), _gloffset_ProgramParameters4fvNV, parameters)
#define GET_ProgramParameters4fvNV(disp) GET_by_offset(disp, _gloffset_ProgramParameters4fvNV)
#define SET_ProgramParameters4fvNV(disp, fn) SET_by_offset(disp, _gloffset_ProgramParameters4fvNV, fn)
#define CALL_RequestResidentProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_RequestResidentProgramsNV, parameters)
#define GET_RequestResidentProgramsNV(disp) GET_by_offset(disp, _gloffset_RequestResidentProgramsNV)
#define SET_RequestResidentProgramsNV(disp, fn) SET_by_offset(disp, _gloffset_RequestResidentProgramsNV, fn)
#define CALL_TrackMatrixNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLenum)), _gloffset_TrackMatrixNV, parameters)
#define GET_TrackMatrixNV(disp) GET_by_offset(disp, _gloffset_TrackMatrixNV)
#define SET_TrackMatrixNV(disp, fn) SET_by_offset(disp, _gloffset_TrackMatrixNV, fn)
#define CALL_VertexAttrib1dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble)), _gloffset_VertexAttrib1dNV, parameters)
#define GET_VertexAttrib1dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dNV)
#define SET_VertexAttrib1dNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1dNV, fn)
#define CALL_VertexAttrib1dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib1dvNV, parameters)
#define GET_VertexAttrib1dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dvNV)
#define SET_VertexAttrib1dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1dvNV, fn)
#define CALL_VertexAttrib1fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat)), _gloffset_VertexAttrib1fNV, parameters)
#define GET_VertexAttrib1fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fNV)
#define SET_VertexAttrib1fNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1fNV, fn)
#define CALL_VertexAttrib1fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib1fvNV, parameters)
#define GET_VertexAttrib1fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fvNV)
#define SET_VertexAttrib1fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1fvNV, fn)
#define CALL_VertexAttrib1sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort)), _gloffset_VertexAttrib1sNV, parameters)
#define GET_VertexAttrib1sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1sNV)
#define SET_VertexAttrib1sNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1sNV, fn)
#define CALL_VertexAttrib1svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib1svNV, parameters)
#define GET_VertexAttrib1svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1svNV)
#define SET_VertexAttrib1svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib1svNV, fn)
#define CALL_VertexAttrib2dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble)), _gloffset_VertexAttrib2dNV, parameters)
#define GET_VertexAttrib2dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dNV)
#define SET_VertexAttrib2dNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2dNV, fn)
#define CALL_VertexAttrib2dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib2dvNV, parameters)
#define GET_VertexAttrib2dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dvNV)
#define SET_VertexAttrib2dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2dvNV, fn)
#define CALL_VertexAttrib2fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat)), _gloffset_VertexAttrib2fNV, parameters)
#define GET_VertexAttrib2fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fNV)
#define SET_VertexAttrib2fNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2fNV, fn)
#define CALL_VertexAttrib2fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib2fvNV, parameters)
#define GET_VertexAttrib2fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fvNV)
#define SET_VertexAttrib2fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2fvNV, fn)
#define CALL_VertexAttrib2sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort)), _gloffset_VertexAttrib2sNV, parameters)
#define GET_VertexAttrib2sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2sNV)
#define SET_VertexAttrib2sNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2sNV, fn)
#define CALL_VertexAttrib2svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib2svNV, parameters)
#define GET_VertexAttrib2svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2svNV)
#define SET_VertexAttrib2svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib2svNV, fn)
#define CALL_VertexAttrib3dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib3dNV, parameters)
#define GET_VertexAttrib3dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dNV)
#define SET_VertexAttrib3dNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3dNV, fn)
#define CALL_VertexAttrib3dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib3dvNV, parameters)
#define GET_VertexAttrib3dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dvNV)
#define SET_VertexAttrib3dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3dvNV, fn)
#define CALL_VertexAttrib3fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib3fNV, parameters)
#define GET_VertexAttrib3fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fNV)
#define SET_VertexAttrib3fNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3fNV, fn)
#define CALL_VertexAttrib3fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib3fvNV, parameters)
#define GET_VertexAttrib3fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fvNV)
#define SET_VertexAttrib3fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3fvNV, fn)
#define CALL_VertexAttrib3sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib3sNV, parameters)
#define GET_VertexAttrib3sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3sNV)
#define SET_VertexAttrib3sNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3sNV, fn)
#define CALL_VertexAttrib3svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib3svNV, parameters)
#define GET_VertexAttrib3svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3svNV)
#define SET_VertexAttrib3svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib3svNV, fn)
#define CALL_VertexAttrib4dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib4dNV, parameters)
#define GET_VertexAttrib4dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dNV)
#define SET_VertexAttrib4dNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4dNV, fn)
#define CALL_VertexAttrib4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib4dvNV, parameters)
#define GET_VertexAttrib4dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dvNV)
#define SET_VertexAttrib4dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4dvNV, fn)
#define CALL_VertexAttrib4fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib4fNV, parameters)
#define GET_VertexAttrib4fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fNV)
#define SET_VertexAttrib4fNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4fNV, fn)
#define CALL_VertexAttrib4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib4fvNV, parameters)
#define GET_VertexAttrib4fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fvNV)
#define SET_VertexAttrib4fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4fvNV, fn)
#define CALL_VertexAttrib4sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib4sNV, parameters)
#define GET_VertexAttrib4sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4sNV)
#define SET_VertexAttrib4sNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4sNV, fn)
#define CALL_VertexAttrib4svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib4svNV, parameters)
#define GET_VertexAttrib4svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4svNV)
#define SET_VertexAttrib4svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4svNV, fn)
#define CALL_VertexAttrib4ubNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)), _gloffset_VertexAttrib4ubNV, parameters)
#define GET_VertexAttrib4ubNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ubNV)
#define SET_VertexAttrib4ubNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4ubNV, fn)
#define CALL_VertexAttrib4ubvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttrib4ubvNV, parameters)
#define GET_VertexAttrib4ubvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ubvNV)
#define SET_VertexAttrib4ubvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttrib4ubvNV, fn)
#define CALL_VertexAttribPointerNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_VertexAttribPointerNV, parameters)
#define GET_VertexAttribPointerNV(disp) GET_by_offset(disp, _gloffset_VertexAttribPointerNV)
#define SET_VertexAttribPointerNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribPointerNV, fn)
#define CALL_VertexAttribs1dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs1dvNV, parameters)
#define GET_VertexAttribs1dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs1dvNV)
#define SET_VertexAttribs1dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs1dvNV, fn)
#define CALL_VertexAttribs1fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs1fvNV, parameters)
#define GET_VertexAttribs1fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs1fvNV)
#define SET_VertexAttribs1fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs1fvNV, fn)
#define CALL_VertexAttribs1svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs1svNV, parameters)
#define GET_VertexAttribs1svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs1svNV)
#define SET_VertexAttribs1svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs1svNV, fn)
#define CALL_VertexAttribs2dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs2dvNV, parameters)
#define GET_VertexAttribs2dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs2dvNV)
#define SET_VertexAttribs2dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs2dvNV, fn)
#define CALL_VertexAttribs2fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs2fvNV, parameters)
#define GET_VertexAttribs2fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs2fvNV)
#define SET_VertexAttribs2fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs2fvNV, fn)
#define CALL_VertexAttribs2svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs2svNV, parameters)
#define GET_VertexAttribs2svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs2svNV)
#define SET_VertexAttribs2svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs2svNV, fn)
#define CALL_VertexAttribs3dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs3dvNV, parameters)
#define GET_VertexAttribs3dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs3dvNV)
#define SET_VertexAttribs3dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs3dvNV, fn)
#define CALL_VertexAttribs3fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs3fvNV, parameters)
#define GET_VertexAttribs3fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs3fvNV)
#define SET_VertexAttribs3fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs3fvNV, fn)
#define CALL_VertexAttribs3svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs3svNV, parameters)
#define GET_VertexAttribs3svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs3svNV)
#define SET_VertexAttribs3svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs3svNV, fn)
#define CALL_VertexAttribs4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs4dvNV, parameters)
#define GET_VertexAttribs4dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4dvNV)
#define SET_VertexAttribs4dvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs4dvNV, fn)
#define CALL_VertexAttribs4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs4fvNV, parameters)
#define GET_VertexAttribs4fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4fvNV)
#define SET_VertexAttribs4fvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs4fvNV, fn)
#define CALL_VertexAttribs4svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs4svNV, parameters)
#define GET_VertexAttribs4svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4svNV)
#define SET_VertexAttribs4svNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs4svNV, fn)
#define CALL_VertexAttribs4ubvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *)), _gloffset_VertexAttribs4ubvNV, parameters)
#define GET_VertexAttribs4ubvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4ubvNV)
#define SET_VertexAttribs4ubvNV(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribs4ubvNV, fn)
#define CALL_GetTexBumpParameterfvATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetTexBumpParameterfvATI, parameters)
#define GET_GetTexBumpParameterfvATI(disp) GET_by_offset(disp, _gloffset_GetTexBumpParameterfvATI)
#define SET_GetTexBumpParameterfvATI(disp, fn) SET_by_offset(disp, _gloffset_GetTexBumpParameterfvATI, fn)
#define CALL_GetTexBumpParameterivATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *)), _gloffset_GetTexBumpParameterivATI, parameters)
#define GET_GetTexBumpParameterivATI(disp) GET_by_offset(disp, _gloffset_GetTexBumpParameterivATI)
#define SET_GetTexBumpParameterivATI(disp, fn) SET_by_offset(disp, _gloffset_GetTexBumpParameterivATI, fn)
#define CALL_TexBumpParameterfvATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_TexBumpParameterfvATI, parameters)
#define GET_TexBumpParameterfvATI(disp) GET_by_offset(disp, _gloffset_TexBumpParameterfvATI)
#define SET_TexBumpParameterfvATI(disp, fn) SET_by_offset(disp, _gloffset_TexBumpParameterfvATI, fn)
#define CALL_TexBumpParameterivATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_TexBumpParameterivATI, parameters)
#define GET_TexBumpParameterivATI(disp) GET_by_offset(disp, _gloffset_TexBumpParameterivATI)
#define SET_TexBumpParameterivATI(disp, fn) SET_by_offset(disp, _gloffset_TexBumpParameterivATI, fn)
#define CALL_AlphaFragmentOp1ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_AlphaFragmentOp1ATI, parameters)
#define GET_AlphaFragmentOp1ATI(disp) GET_by_offset(disp, _gloffset_AlphaFragmentOp1ATI)
#define SET_AlphaFragmentOp1ATI(disp, fn) SET_by_offset(disp, _gloffset_AlphaFragmentOp1ATI, fn)
#define CALL_AlphaFragmentOp2ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_AlphaFragmentOp2ATI, parameters)
#define GET_AlphaFragmentOp2ATI(disp) GET_by_offset(disp, _gloffset_AlphaFragmentOp2ATI)
#define SET_AlphaFragmentOp2ATI(disp, fn) SET_by_offset(disp, _gloffset_AlphaFragmentOp2ATI, fn)
#define CALL_AlphaFragmentOp3ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_AlphaFragmentOp3ATI, parameters)
#define GET_AlphaFragmentOp3ATI(disp) GET_by_offset(disp, _gloffset_AlphaFragmentOp3ATI)
#define SET_AlphaFragmentOp3ATI(disp, fn) SET_by_offset(disp, _gloffset_AlphaFragmentOp3ATI, fn)
#define CALL_BeginFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_BeginFragmentShaderATI, parameters)
#define GET_BeginFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_BeginFragmentShaderATI)
#define SET_BeginFragmentShaderATI(disp, fn) SET_by_offset(disp, _gloffset_BeginFragmentShaderATI, fn)
#define CALL_BindFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_BindFragmentShaderATI, parameters)
#define GET_BindFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_BindFragmentShaderATI)
#define SET_BindFragmentShaderATI(disp, fn) SET_by_offset(disp, _gloffset_BindFragmentShaderATI, fn)
#define CALL_ColorFragmentOp1ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_ColorFragmentOp1ATI, parameters)
#define GET_ColorFragmentOp1ATI(disp) GET_by_offset(disp, _gloffset_ColorFragmentOp1ATI)
#define SET_ColorFragmentOp1ATI(disp, fn) SET_by_offset(disp, _gloffset_ColorFragmentOp1ATI, fn)
#define CALL_ColorFragmentOp2ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_ColorFragmentOp2ATI, parameters)
#define GET_ColorFragmentOp2ATI(disp) GET_by_offset(disp, _gloffset_ColorFragmentOp2ATI)
#define SET_ColorFragmentOp2ATI(disp, fn) SET_by_offset(disp, _gloffset_ColorFragmentOp2ATI, fn)
#define CALL_ColorFragmentOp3ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_ColorFragmentOp3ATI, parameters)
#define GET_ColorFragmentOp3ATI(disp) GET_by_offset(disp, _gloffset_ColorFragmentOp3ATI)
#define SET_ColorFragmentOp3ATI(disp, fn) SET_by_offset(disp, _gloffset_ColorFragmentOp3ATI, fn)
#define CALL_DeleteFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DeleteFragmentShaderATI, parameters)
#define GET_DeleteFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_DeleteFragmentShaderATI)
#define SET_DeleteFragmentShaderATI(disp, fn) SET_by_offset(disp, _gloffset_DeleteFragmentShaderATI, fn)
#define CALL_EndFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndFragmentShaderATI, parameters)
#define GET_EndFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_EndFragmentShaderATI)
#define SET_EndFragmentShaderATI(disp, fn) SET_by_offset(disp, _gloffset_EndFragmentShaderATI, fn)
#define CALL_GenFragmentShadersATI(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLuint)), _gloffset_GenFragmentShadersATI, parameters)
#define GET_GenFragmentShadersATI(disp) GET_by_offset(disp, _gloffset_GenFragmentShadersATI)
#define SET_GenFragmentShadersATI(disp, fn) SET_by_offset(disp, _gloffset_GenFragmentShadersATI, fn)
#define CALL_PassTexCoordATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLenum)), _gloffset_PassTexCoordATI, parameters)
#define GET_PassTexCoordATI(disp) GET_by_offset(disp, _gloffset_PassTexCoordATI)
#define SET_PassTexCoordATI(disp, fn) SET_by_offset(disp, _gloffset_PassTexCoordATI, fn)
#define CALL_SampleMapATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLenum)), _gloffset_SampleMapATI, parameters)
#define GET_SampleMapATI(disp) GET_by_offset(disp, _gloffset_SampleMapATI)
#define SET_SampleMapATI(disp, fn) SET_by_offset(disp, _gloffset_SampleMapATI, fn)
#define CALL_SetFragmentShaderConstantATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_SetFragmentShaderConstantATI, parameters)
#define GET_SetFragmentShaderConstantATI(disp) GET_by_offset(disp, _gloffset_SetFragmentShaderConstantATI)
#define SET_SetFragmentShaderConstantATI(disp, fn) SET_by_offset(disp, _gloffset_SetFragmentShaderConstantATI, fn)
#define CALL_PointParameteriNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PointParameteriNV, parameters)
#define GET_PointParameteriNV(disp) GET_by_offset(disp, _gloffset_PointParameteriNV)
#define SET_PointParameteriNV(disp, fn) SET_by_offset(disp, _gloffset_PointParameteriNV, fn)
#define CALL_PointParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_PointParameterivNV, parameters)
#define GET_PointParameterivNV(disp) GET_by_offset(disp, _gloffset_PointParameterivNV)
#define SET_PointParameterivNV(disp, fn) SET_by_offset(disp, _gloffset_PointParameterivNV, fn)
#define CALL_ActiveStencilFaceEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ActiveStencilFaceEXT, parameters)
#define GET_ActiveStencilFaceEXT(disp) GET_by_offset(disp, _gloffset_ActiveStencilFaceEXT)
#define SET_ActiveStencilFaceEXT(disp, fn) SET_by_offset(disp, _gloffset_ActiveStencilFaceEXT, fn)
#define CALL_BindVertexArrayAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_BindVertexArrayAPPLE, parameters)
#define GET_BindVertexArrayAPPLE(disp) GET_by_offset(disp, _gloffset_BindVertexArrayAPPLE)
#define SET_BindVertexArrayAPPLE(disp, fn) SET_by_offset(disp, _gloffset_BindVertexArrayAPPLE, fn)
#define CALL_DeleteVertexArraysAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteVertexArraysAPPLE, parameters)
#define GET_DeleteVertexArraysAPPLE(disp) GET_by_offset(disp, _gloffset_DeleteVertexArraysAPPLE)
#define SET_DeleteVertexArraysAPPLE(disp, fn) SET_by_offset(disp, _gloffset_DeleteVertexArraysAPPLE, fn)
#define CALL_GenVertexArraysAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenVertexArraysAPPLE, parameters)
#define GET_GenVertexArraysAPPLE(disp) GET_by_offset(disp, _gloffset_GenVertexArraysAPPLE)
#define SET_GenVertexArraysAPPLE(disp, fn) SET_by_offset(disp, _gloffset_GenVertexArraysAPPLE, fn)
#define CALL_IsVertexArrayAPPLE(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsVertexArrayAPPLE, parameters)
#define GET_IsVertexArrayAPPLE(disp) GET_by_offset(disp, _gloffset_IsVertexArrayAPPLE)
#define SET_IsVertexArrayAPPLE(disp, fn) SET_by_offset(disp, _gloffset_IsVertexArrayAPPLE, fn)
#define CALL_GetProgramNamedParameterdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLdouble *)), _gloffset_GetProgramNamedParameterdvNV, parameters)
#define GET_GetProgramNamedParameterdvNV(disp) GET_by_offset(disp, _gloffset_GetProgramNamedParameterdvNV)
#define SET_GetProgramNamedParameterdvNV(disp, fn) SET_by_offset(disp, _gloffset_GetProgramNamedParameterdvNV, fn)
#define CALL_GetProgramNamedParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLfloat *)), _gloffset_GetProgramNamedParameterfvNV, parameters)
#define GET_GetProgramNamedParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetProgramNamedParameterfvNV)
#define SET_GetProgramNamedParameterfvNV(disp, fn) SET_by_offset(disp, _gloffset_GetProgramNamedParameterfvNV, fn)
#define CALL_ProgramNamedParameter4dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_ProgramNamedParameter4dNV, parameters)
#define GET_ProgramNamedParameter4dNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4dNV)
#define SET_ProgramNamedParameter4dNV(disp, fn) SET_by_offset(disp, _gloffset_ProgramNamedParameter4dNV, fn)
#define CALL_ProgramNamedParameter4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, const GLdouble *)), _gloffset_ProgramNamedParameter4dvNV, parameters)
#define GET_ProgramNamedParameter4dvNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4dvNV)
#define SET_ProgramNamedParameter4dvNV(disp, fn) SET_by_offset(disp, _gloffset_ProgramNamedParameter4dvNV, fn)
#define CALL_ProgramNamedParameter4fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ProgramNamedParameter4fNV, parameters)
#define GET_ProgramNamedParameter4fNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4fNV)
#define SET_ProgramNamedParameter4fNV(disp, fn) SET_by_offset(disp, _gloffset_ProgramNamedParameter4fNV, fn)
#define CALL_ProgramNamedParameter4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, const GLfloat *)), _gloffset_ProgramNamedParameter4fvNV, parameters)
#define GET_ProgramNamedParameter4fvNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4fvNV)
#define SET_ProgramNamedParameter4fvNV(disp, fn) SET_by_offset(disp, _gloffset_ProgramNamedParameter4fvNV, fn)
#define CALL_PrimitiveRestartIndexNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_PrimitiveRestartIndexNV, parameters)
#define GET_PrimitiveRestartIndexNV(disp) GET_by_offset(disp, _gloffset_PrimitiveRestartIndexNV)
#define SET_PrimitiveRestartIndexNV(disp, fn) SET_by_offset(disp, _gloffset_PrimitiveRestartIndexNV, fn)
#define CALL_PrimitiveRestartNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PrimitiveRestartNV, parameters)
#define GET_PrimitiveRestartNV(disp) GET_by_offset(disp, _gloffset_PrimitiveRestartNV)
#define SET_PrimitiveRestartNV(disp, fn) SET_by_offset(disp, _gloffset_PrimitiveRestartNV, fn)
#define CALL_DepthBoundsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampd, GLclampd)), _gloffset_DepthBoundsEXT, parameters)
#define GET_DepthBoundsEXT(disp) GET_by_offset(disp, _gloffset_DepthBoundsEXT)
#define SET_DepthBoundsEXT(disp, fn) SET_by_offset(disp, _gloffset_DepthBoundsEXT, fn)
#define CALL_BlendEquationSeparateEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_BlendEquationSeparateEXT, parameters)
#define GET_BlendEquationSeparateEXT(disp) GET_by_offset(disp, _gloffset_BlendEquationSeparateEXT)
#define SET_BlendEquationSeparateEXT(disp, fn) SET_by_offset(disp, _gloffset_BlendEquationSeparateEXT, fn)
#define CALL_BindFramebufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindFramebufferEXT, parameters)
#define GET_BindFramebufferEXT(disp) GET_by_offset(disp, _gloffset_BindFramebufferEXT)
#define SET_BindFramebufferEXT(disp, fn) SET_by_offset(disp, _gloffset_BindFramebufferEXT, fn)
#define CALL_BindRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindRenderbufferEXT, parameters)
#define GET_BindRenderbufferEXT(disp) GET_by_offset(disp, _gloffset_BindRenderbufferEXT)
#define SET_BindRenderbufferEXT(disp, fn) SET_by_offset(disp, _gloffset_BindRenderbufferEXT, fn)
#define CALL_CheckFramebufferStatusEXT(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLenum)), _gloffset_CheckFramebufferStatusEXT, parameters)
#define GET_CheckFramebufferStatusEXT(disp) GET_by_offset(disp, _gloffset_CheckFramebufferStatusEXT)
#define SET_CheckFramebufferStatusEXT(disp, fn) SET_by_offset(disp, _gloffset_CheckFramebufferStatusEXT, fn)
#define CALL_DeleteFramebuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteFramebuffersEXT, parameters)
#define GET_DeleteFramebuffersEXT(disp) GET_by_offset(disp, _gloffset_DeleteFramebuffersEXT)
#define SET_DeleteFramebuffersEXT(disp, fn) SET_by_offset(disp, _gloffset_DeleteFramebuffersEXT, fn)
#define CALL_DeleteRenderbuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteRenderbuffersEXT, parameters)
#define GET_DeleteRenderbuffersEXT(disp) GET_by_offset(disp, _gloffset_DeleteRenderbuffersEXT)
#define SET_DeleteRenderbuffersEXT(disp, fn) SET_by_offset(disp, _gloffset_DeleteRenderbuffersEXT, fn)
#define CALL_FramebufferRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint)), _gloffset_FramebufferRenderbufferEXT, parameters)
#define GET_FramebufferRenderbufferEXT(disp) GET_by_offset(disp, _gloffset_FramebufferRenderbufferEXT)
#define SET_FramebufferRenderbufferEXT(disp, fn) SET_by_offset(disp, _gloffset_FramebufferRenderbufferEXT, fn)
#define CALL_FramebufferTexture1DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint)), _gloffset_FramebufferTexture1DEXT, parameters)
#define GET_FramebufferTexture1DEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTexture1DEXT)
#define SET_FramebufferTexture1DEXT(disp, fn) SET_by_offset(disp, _gloffset_FramebufferTexture1DEXT, fn)
#define CALL_FramebufferTexture2DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint)), _gloffset_FramebufferTexture2DEXT, parameters)
#define GET_FramebufferTexture2DEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTexture2DEXT)
#define SET_FramebufferTexture2DEXT(disp, fn) SET_by_offset(disp, _gloffset_FramebufferTexture2DEXT, fn)
#define CALL_FramebufferTexture3DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint, GLint)), _gloffset_FramebufferTexture3DEXT, parameters)
#define GET_FramebufferTexture3DEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTexture3DEXT)
#define SET_FramebufferTexture3DEXT(disp, fn) SET_by_offset(disp, _gloffset_FramebufferTexture3DEXT, fn)
#define CALL_GenFramebuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenFramebuffersEXT, parameters)
#define GET_GenFramebuffersEXT(disp) GET_by_offset(disp, _gloffset_GenFramebuffersEXT)
#define SET_GenFramebuffersEXT(disp, fn) SET_by_offset(disp, _gloffset_GenFramebuffersEXT, fn)
#define CALL_GenRenderbuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenRenderbuffersEXT, parameters)
#define GET_GenRenderbuffersEXT(disp) GET_by_offset(disp, _gloffset_GenRenderbuffersEXT)
#define SET_GenRenderbuffersEXT(disp, fn) SET_by_offset(disp, _gloffset_GenRenderbuffersEXT, fn)
#define CALL_GenerateMipmapEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_GenerateMipmapEXT, parameters)
#define GET_GenerateMipmapEXT(disp) GET_by_offset(disp, _gloffset_GenerateMipmapEXT)
#define SET_GenerateMipmapEXT(disp, fn) SET_by_offset(disp, _gloffset_GenerateMipmapEXT, fn)
#define CALL_GetFramebufferAttachmentParameterivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLint *)), _gloffset_GetFramebufferAttachmentParameterivEXT, parameters)
#define GET_GetFramebufferAttachmentParameterivEXT(disp) GET_by_offset(disp, _gloffset_GetFramebufferAttachmentParameterivEXT)
#define SET_GetFramebufferAttachmentParameterivEXT(disp, fn) SET_by_offset(disp, _gloffset_GetFramebufferAttachmentParameterivEXT, fn)
#define CALL_GetRenderbufferParameterivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetRenderbufferParameterivEXT, parameters)
#define GET_GetRenderbufferParameterivEXT(disp) GET_by_offset(disp, _gloffset_GetRenderbufferParameterivEXT)
#define SET_GetRenderbufferParameterivEXT(disp, fn) SET_by_offset(disp, _gloffset_GetRenderbufferParameterivEXT, fn)
#define CALL_IsFramebufferEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsFramebufferEXT, parameters)
#define GET_IsFramebufferEXT(disp) GET_by_offset(disp, _gloffset_IsFramebufferEXT)
#define SET_IsFramebufferEXT(disp, fn) SET_by_offset(disp, _gloffset_IsFramebufferEXT, fn)
#define CALL_IsRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsRenderbufferEXT, parameters)
#define GET_IsRenderbufferEXT(disp) GET_by_offset(disp, _gloffset_IsRenderbufferEXT)
#define SET_IsRenderbufferEXT(disp, fn) SET_by_offset(disp, _gloffset_IsRenderbufferEXT, fn)
#define CALL_RenderbufferStorageEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLsizei)), _gloffset_RenderbufferStorageEXT, parameters)
#define GET_RenderbufferStorageEXT(disp) GET_by_offset(disp, _gloffset_RenderbufferStorageEXT)
#define SET_RenderbufferStorageEXT(disp, fn) SET_by_offset(disp, _gloffset_RenderbufferStorageEXT, fn)
#define CALL_BlitFramebufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)), _gloffset_BlitFramebufferEXT, parameters)
#define GET_BlitFramebufferEXT(disp) GET_by_offset(disp, _gloffset_BlitFramebufferEXT)
#define SET_BlitFramebufferEXT(disp, fn) SET_by_offset(disp, _gloffset_BlitFramebufferEXT, fn)
#define CALL_BufferParameteriAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_BufferParameteriAPPLE, parameters)
#define GET_BufferParameteriAPPLE(disp) GET_by_offset(disp, _gloffset_BufferParameteriAPPLE)
#define SET_BufferParameteriAPPLE(disp, fn) SET_by_offset(disp, _gloffset_BufferParameteriAPPLE, fn)
#define CALL_FlushMappedBufferRangeAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptr, GLsizeiptr)), _gloffset_FlushMappedBufferRangeAPPLE, parameters)
#define GET_FlushMappedBufferRangeAPPLE(disp) GET_by_offset(disp, _gloffset_FlushMappedBufferRangeAPPLE)
#define SET_FlushMappedBufferRangeAPPLE(disp, fn) SET_by_offset(disp, _gloffset_FlushMappedBufferRangeAPPLE, fn)
#define CALL_BindFragDataLocationEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, const GLchar *)), _gloffset_BindFragDataLocationEXT, parameters)
#define GET_BindFragDataLocationEXT(disp) GET_by_offset(disp, _gloffset_BindFragDataLocationEXT)
#define SET_BindFragDataLocationEXT(disp, fn) SET_by_offset(disp, _gloffset_BindFragDataLocationEXT, fn)
#define CALL_GetFragDataLocationEXT(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLuint, const GLchar *)), _gloffset_GetFragDataLocationEXT, parameters)
#define GET_GetFragDataLocationEXT(disp) GET_by_offset(disp, _gloffset_GetFragDataLocationEXT)
#define SET_GetFragDataLocationEXT(disp, fn) SET_by_offset(disp, _gloffset_GetFragDataLocationEXT, fn)
#define CALL_GetUniformuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLuint *)), _gloffset_GetUniformuivEXT, parameters)
#define GET_GetUniformuivEXT(disp) GET_by_offset(disp, _gloffset_GetUniformuivEXT)
#define SET_GetUniformuivEXT(disp, fn) SET_by_offset(disp, _gloffset_GetUniformuivEXT, fn)
#define CALL_GetVertexAttribIivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetVertexAttribIivEXT, parameters)
#define GET_GetVertexAttribIivEXT(disp) GET_by_offset(disp, _gloffset_GetVertexAttribIivEXT)
#define SET_GetVertexAttribIivEXT(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribIivEXT, fn)
#define CALL_GetVertexAttribIuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint *)), _gloffset_GetVertexAttribIuivEXT, parameters)
#define GET_GetVertexAttribIuivEXT(disp) GET_by_offset(disp, _gloffset_GetVertexAttribIuivEXT)
#define SET_GetVertexAttribIuivEXT(disp, fn) SET_by_offset(disp, _gloffset_GetVertexAttribIuivEXT, fn)
#define CALL_Uniform1uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint)), _gloffset_Uniform1uiEXT, parameters)
#define GET_Uniform1uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform1uiEXT)
#define SET_Uniform1uiEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform1uiEXT, fn)
#define CALL_Uniform1uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform1uivEXT, parameters)
#define GET_Uniform1uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform1uivEXT)
#define SET_Uniform1uivEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform1uivEXT, fn)
#define CALL_Uniform2uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint, GLuint)), _gloffset_Uniform2uiEXT, parameters)
#define GET_Uniform2uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform2uiEXT)
#define SET_Uniform2uiEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform2uiEXT, fn)
#define CALL_Uniform2uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform2uivEXT, parameters)
#define GET_Uniform2uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform2uivEXT)
#define SET_Uniform2uivEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform2uivEXT, fn)
#define CALL_Uniform3uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint, GLuint, GLuint)), _gloffset_Uniform3uiEXT, parameters)
#define GET_Uniform3uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform3uiEXT)
#define SET_Uniform3uiEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform3uiEXT, fn)
#define CALL_Uniform3uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform3uivEXT, parameters)
#define GET_Uniform3uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform3uivEXT)
#define SET_Uniform3uivEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform3uivEXT, fn)
#define CALL_Uniform4uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint, GLuint, GLuint, GLuint)), _gloffset_Uniform4uiEXT, parameters)
#define GET_Uniform4uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform4uiEXT)
#define SET_Uniform4uiEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform4uiEXT, fn)
#define CALL_Uniform4uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform4uivEXT, parameters)
#define GET_Uniform4uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform4uivEXT)
#define SET_Uniform4uivEXT(disp, fn) SET_by_offset(disp, _gloffset_Uniform4uivEXT, fn)
#define CALL_VertexAttribI1iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint)), _gloffset_VertexAttribI1iEXT, parameters)
#define GET_VertexAttribI1iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1iEXT)
#define SET_VertexAttribI1iEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI1iEXT, fn)
#define CALL_VertexAttribI1ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI1ivEXT, parameters)
#define GET_VertexAttribI1ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1ivEXT)
#define SET_VertexAttribI1ivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI1ivEXT, fn)
#define CALL_VertexAttribI1uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_VertexAttribI1uiEXT, parameters)
#define GET_VertexAttribI1uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1uiEXT)
#define SET_VertexAttribI1uiEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI1uiEXT, fn)
#define CALL_VertexAttribI1uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI1uivEXT, parameters)
#define GET_VertexAttribI1uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1uivEXT)
#define SET_VertexAttribI1uivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI1uivEXT, fn)
#define CALL_VertexAttribI2iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLint)), _gloffset_VertexAttribI2iEXT, parameters)
#define GET_VertexAttribI2iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2iEXT)
#define SET_VertexAttribI2iEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI2iEXT, fn)
#define CALL_VertexAttribI2ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI2ivEXT, parameters)
#define GET_VertexAttribI2ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2ivEXT)
#define SET_VertexAttribI2ivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI2ivEXT, fn)
#define CALL_VertexAttribI2uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint)), _gloffset_VertexAttribI2uiEXT, parameters)
#define GET_VertexAttribI2uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2uiEXT)
#define SET_VertexAttribI2uiEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI2uiEXT, fn)
#define CALL_VertexAttribI2uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI2uivEXT, parameters)
#define GET_VertexAttribI2uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2uivEXT)
#define SET_VertexAttribI2uivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI2uivEXT, fn)
#define CALL_VertexAttribI3iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLint, GLint)), _gloffset_VertexAttribI3iEXT, parameters)
#define GET_VertexAttribI3iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3iEXT)
#define SET_VertexAttribI3iEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI3iEXT, fn)
#define CALL_VertexAttribI3ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI3ivEXT, parameters)
#define GET_VertexAttribI3ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3ivEXT)
#define SET_VertexAttribI3ivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI3ivEXT, fn)
#define CALL_VertexAttribI3uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint)), _gloffset_VertexAttribI3uiEXT, parameters)
#define GET_VertexAttribI3uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3uiEXT)
#define SET_VertexAttribI3uiEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI3uiEXT, fn)
#define CALL_VertexAttribI3uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI3uivEXT, parameters)
#define GET_VertexAttribI3uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3uivEXT)
#define SET_VertexAttribI3uivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI3uivEXT, fn)
#define CALL_VertexAttribI4bvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), _gloffset_VertexAttribI4bvEXT, parameters)
#define GET_VertexAttribI4bvEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4bvEXT)
#define SET_VertexAttribI4bvEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4bvEXT, fn)
#define CALL_VertexAttribI4iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLint, GLint, GLint)), _gloffset_VertexAttribI4iEXT, parameters)
#define GET_VertexAttribI4iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4iEXT)
#define SET_VertexAttribI4iEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4iEXT, fn)
#define CALL_VertexAttribI4ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI4ivEXT, parameters)
#define GET_VertexAttribI4ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4ivEXT)
#define SET_VertexAttribI4ivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4ivEXT, fn)
#define CALL_VertexAttribI4svEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttribI4svEXT, parameters)
#define GET_VertexAttribI4svEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4svEXT)
#define SET_VertexAttribI4svEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4svEXT, fn)
#define CALL_VertexAttribI4ubvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttribI4ubvEXT, parameters)
#define GET_VertexAttribI4ubvEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4ubvEXT)
#define SET_VertexAttribI4ubvEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4ubvEXT, fn)
#define CALL_VertexAttribI4uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_VertexAttribI4uiEXT, parameters)
#define GET_VertexAttribI4uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4uiEXT)
#define SET_VertexAttribI4uiEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4uiEXT, fn)
#define CALL_VertexAttribI4uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI4uivEXT, parameters)
#define GET_VertexAttribI4uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4uivEXT)
#define SET_VertexAttribI4uivEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4uivEXT, fn)
#define CALL_VertexAttribI4usvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), _gloffset_VertexAttribI4usvEXT, parameters)
#define GET_VertexAttribI4usvEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4usvEXT)
#define SET_VertexAttribI4usvEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribI4usvEXT, fn)
#define CALL_VertexAttribIPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_VertexAttribIPointerEXT, parameters)
#define GET_VertexAttribIPointerEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribIPointerEXT)
#define SET_VertexAttribIPointerEXT(disp, fn) SET_by_offset(disp, _gloffset_VertexAttribIPointerEXT, fn)
#define CALL_FramebufferTextureLayerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint, GLint, GLint)), _gloffset_FramebufferTextureLayerEXT, parameters)
#define GET_FramebufferTextureLayerEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTextureLayerEXT)
#define SET_FramebufferTextureLayerEXT(disp, fn) SET_by_offset(disp, _gloffset_FramebufferTextureLayerEXT, fn)
#define CALL_ColorMaskIndexedEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean)), _gloffset_ColorMaskIndexedEXT, parameters)
#define GET_ColorMaskIndexedEXT(disp) GET_by_offset(disp, _gloffset_ColorMaskIndexedEXT)
#define SET_ColorMaskIndexedEXT(disp, fn) SET_by_offset(disp, _gloffset_ColorMaskIndexedEXT, fn)
#define CALL_DisableIndexedEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_DisableIndexedEXT, parameters)
#define GET_DisableIndexedEXT(disp) GET_by_offset(disp, _gloffset_DisableIndexedEXT)
#define SET_DisableIndexedEXT(disp, fn) SET_by_offset(disp, _gloffset_DisableIndexedEXT, fn)
#define CALL_EnableIndexedEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_EnableIndexedEXT, parameters)
#define GET_EnableIndexedEXT(disp) GET_by_offset(disp, _gloffset_EnableIndexedEXT)
#define SET_EnableIndexedEXT(disp, fn) SET_by_offset(disp, _gloffset_EnableIndexedEXT, fn)
#define CALL_GetBooleanIndexedvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLboolean *)), _gloffset_GetBooleanIndexedvEXT, parameters)
#define GET_GetBooleanIndexedvEXT(disp) GET_by_offset(disp, _gloffset_GetBooleanIndexedvEXT)
#define SET_GetBooleanIndexedvEXT(disp, fn) SET_by_offset(disp, _gloffset_GetBooleanIndexedvEXT, fn)
#define CALL_GetIntegerIndexedvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLint *)), _gloffset_GetIntegerIndexedvEXT, parameters)
#define GET_GetIntegerIndexedvEXT(disp) GET_by_offset(disp, _gloffset_GetIntegerIndexedvEXT)
#define SET_GetIntegerIndexedvEXT(disp, fn) SET_by_offset(disp, _gloffset_GetIntegerIndexedvEXT, fn)
#define CALL_IsEnabledIndexedEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_IsEnabledIndexedEXT, parameters)
#define GET_IsEnabledIndexedEXT(disp) GET_by_offset(disp, _gloffset_IsEnabledIndexedEXT)
#define SET_IsEnabledIndexedEXT(disp, fn) SET_by_offset(disp, _gloffset_IsEnabledIndexedEXT, fn)
#define CALL_ClearColorIiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_ClearColorIiEXT, parameters)
#define GET_ClearColorIiEXT(disp) GET_by_offset(disp, _gloffset_ClearColorIiEXT)
#define SET_ClearColorIiEXT(disp, fn) SET_by_offset(disp, _gloffset_ClearColorIiEXT, fn)
#define CALL_ClearColorIuiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint)), _gloffset_ClearColorIuiEXT, parameters)
#define GET_ClearColorIuiEXT(disp) GET_by_offset(disp, _gloffset_ClearColorIuiEXT)
#define SET_ClearColorIuiEXT(disp, fn) SET_by_offset(disp, _gloffset_ClearColorIuiEXT, fn)
#define CALL_GetTexParameterIivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexParameterIivEXT, parameters)
#define GET_GetTexParameterIivEXT(disp) GET_by_offset(disp, _gloffset_GetTexParameterIivEXT)
#define SET_GetTexParameterIivEXT(disp, fn) SET_by_offset(disp, _gloffset_GetTexParameterIivEXT, fn)
#define CALL_GetTexParameterIuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint *)), _gloffset_GetTexParameterIuivEXT, parameters)
#define GET_GetTexParameterIuivEXT(disp) GET_by_offset(disp, _gloffset_GetTexParameterIuivEXT)
#define SET_GetTexParameterIuivEXT(disp, fn) SET_by_offset(disp, _gloffset_GetTexParameterIuivEXT, fn)
#define CALL_TexParameterIivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexParameterIivEXT, parameters)
#define GET_TexParameterIivEXT(disp) GET_by_offset(disp, _gloffset_TexParameterIivEXT)
#define SET_TexParameterIivEXT(disp, fn) SET_by_offset(disp, _gloffset_TexParameterIivEXT, fn)
#define CALL_TexParameterIuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLuint *)), _gloffset_TexParameterIuivEXT, parameters)
#define GET_TexParameterIuivEXT(disp) GET_by_offset(disp, _gloffset_TexParameterIuivEXT)
#define SET_TexParameterIuivEXT(disp, fn) SET_by_offset(disp, _gloffset_TexParameterIuivEXT, fn)
#define CALL_BeginConditionalRenderNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum)), _gloffset_BeginConditionalRenderNV, parameters)
#define GET_BeginConditionalRenderNV(disp) GET_by_offset(disp, _gloffset_BeginConditionalRenderNV)
#define SET_BeginConditionalRenderNV(disp, fn) SET_by_offset(disp, _gloffset_BeginConditionalRenderNV, fn)
#define CALL_EndConditionalRenderNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndConditionalRenderNV, parameters)
#define GET_EndConditionalRenderNV(disp) GET_by_offset(disp, _gloffset_EndConditionalRenderNV)
#define SET_EndConditionalRenderNV(disp, fn) SET_by_offset(disp, _gloffset_EndConditionalRenderNV, fn)
#define CALL_BeginTransformFeedbackEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_BeginTransformFeedbackEXT, parameters)
#define GET_BeginTransformFeedbackEXT(disp) GET_by_offset(disp, _gloffset_BeginTransformFeedbackEXT)
#define SET_BeginTransformFeedbackEXT(disp, fn) SET_by_offset(disp, _gloffset_BeginTransformFeedbackEXT, fn)
#define CALL_BindBufferBaseEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint)), _gloffset_BindBufferBaseEXT, parameters)
#define GET_BindBufferBaseEXT(disp) GET_by_offset(disp, _gloffset_BindBufferBaseEXT)
#define SET_BindBufferBaseEXT(disp, fn) SET_by_offset(disp, _gloffset_BindBufferBaseEXT, fn)
#define CALL_BindBufferOffsetEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLintptr)), _gloffset_BindBufferOffsetEXT, parameters)
#define GET_BindBufferOffsetEXT(disp) GET_by_offset(disp, _gloffset_BindBufferOffsetEXT)
#define SET_BindBufferOffsetEXT(disp, fn) SET_by_offset(disp, _gloffset_BindBufferOffsetEXT, fn)
#define CALL_BindBufferRangeEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr)), _gloffset_BindBufferRangeEXT, parameters)
#define GET_BindBufferRangeEXT(disp) GET_by_offset(disp, _gloffset_BindBufferRangeEXT)
#define SET_BindBufferRangeEXT(disp, fn) SET_by_offset(disp, _gloffset_BindBufferRangeEXT, fn)
#define CALL_EndTransformFeedbackEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndTransformFeedbackEXT, parameters)
#define GET_EndTransformFeedbackEXT(disp) GET_by_offset(disp, _gloffset_EndTransformFeedbackEXT)
#define SET_EndTransformFeedbackEXT(disp, fn) SET_by_offset(disp, _gloffset_EndTransformFeedbackEXT, fn)
#define CALL_GetTransformFeedbackVaryingEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *)), _gloffset_GetTransformFeedbackVaryingEXT, parameters)
#define GET_GetTransformFeedbackVaryingEXT(disp) GET_by_offset(disp, _gloffset_GetTransformFeedbackVaryingEXT)
#define SET_GetTransformFeedbackVaryingEXT(disp, fn) SET_by_offset(disp, _gloffset_GetTransformFeedbackVaryingEXT, fn)
#define CALL_TransformFeedbackVaryingsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const char **, GLenum)), _gloffset_TransformFeedbackVaryingsEXT, parameters)
#define GET_TransformFeedbackVaryingsEXT(disp) GET_by_offset(disp, _gloffset_TransformFeedbackVaryingsEXT)
#define SET_TransformFeedbackVaryingsEXT(disp, fn) SET_by_offset(disp, _gloffset_TransformFeedbackVaryingsEXT, fn)
#define CALL_ProvokingVertexEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ProvokingVertexEXT, parameters)
#define GET_ProvokingVertexEXT(disp) GET_by_offset(disp, _gloffset_ProvokingVertexEXT)
#define SET_ProvokingVertexEXT(disp, fn) SET_by_offset(disp, _gloffset_ProvokingVertexEXT, fn)
#define CALL_GetTexParameterPointervAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid **)), _gloffset_GetTexParameterPointervAPPLE, parameters)
#define GET_GetTexParameterPointervAPPLE(disp) GET_by_offset(disp, _gloffset_GetTexParameterPointervAPPLE)
#define SET_GetTexParameterPointervAPPLE(disp, fn) SET_by_offset(disp, _gloffset_GetTexParameterPointervAPPLE, fn)
#define CALL_TextureRangeAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLvoid *)), _gloffset_TextureRangeAPPLE, parameters)
#define GET_TextureRangeAPPLE(disp) GET_by_offset(disp, _gloffset_TextureRangeAPPLE)
#define SET_TextureRangeAPPLE(disp, fn) SET_by_offset(disp, _gloffset_TextureRangeAPPLE, fn)
#define CALL_GetObjectParameterivAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLint *)), _gloffset_GetObjectParameterivAPPLE, parameters)
#define GET_GetObjectParameterivAPPLE(disp) GET_by_offset(disp, _gloffset_GetObjectParameterivAPPLE)
#define SET_GetObjectParameterivAPPLE(disp, fn) SET_by_offset(disp, _gloffset_GetObjectParameterivAPPLE, fn)
#define CALL_ObjectPurgeableAPPLE(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLenum, GLuint, GLenum)), _gloffset_ObjectPurgeableAPPLE, parameters)
#define GET_ObjectPurgeableAPPLE(disp) GET_by_offset(disp, _gloffset_ObjectPurgeableAPPLE)
#define SET_ObjectPurgeableAPPLE(disp, fn) SET_by_offset(disp, _gloffset_ObjectPurgeableAPPLE, fn)
#define CALL_ObjectUnpurgeableAPPLE(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLenum, GLuint, GLenum)), _gloffset_ObjectUnpurgeableAPPLE, parameters)
#define GET_ObjectUnpurgeableAPPLE(disp) GET_by_offset(disp, _gloffset_ObjectUnpurgeableAPPLE)
#define SET_ObjectUnpurgeableAPPLE(disp, fn) SET_by_offset(disp, _gloffset_ObjectUnpurgeableAPPLE, fn)
#define CALL_ActiveProgramEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_ActiveProgramEXT, parameters)
#define GET_ActiveProgramEXT(disp) GET_by_offset(disp, _gloffset_ActiveProgramEXT)
#define SET_ActiveProgramEXT(disp, fn) SET_by_offset(disp, _gloffset_ActiveProgramEXT, fn)
#define CALL_CreateShaderProgramEXT(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLenum, const GLchar *)), _gloffset_CreateShaderProgramEXT, parameters)
#define GET_CreateShaderProgramEXT(disp) GET_by_offset(disp, _gloffset_CreateShaderProgramEXT)
#define SET_CreateShaderProgramEXT(disp, fn) SET_by_offset(disp, _gloffset_CreateShaderProgramEXT, fn)
#define CALL_UseShaderProgramEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_UseShaderProgramEXT, parameters)
#define GET_UseShaderProgramEXT(disp) GET_by_offset(disp, _gloffset_UseShaderProgramEXT)
#define SET_UseShaderProgramEXT(disp, fn) SET_by_offset(disp, _gloffset_UseShaderProgramEXT, fn)
#define CALL_StencilFuncSeparateATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLuint)), _gloffset_StencilFuncSeparateATI, parameters)
#define GET_StencilFuncSeparateATI(disp) GET_by_offset(disp, _gloffset_StencilFuncSeparateATI)
#define SET_StencilFuncSeparateATI(disp, fn) SET_by_offset(disp, _gloffset_StencilFuncSeparateATI, fn)
#define CALL_ProgramEnvParameters4fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), _gloffset_ProgramEnvParameters4fvEXT, parameters)
#define GET_ProgramEnvParameters4fvEXT(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameters4fvEXT)
#define SET_ProgramEnvParameters4fvEXT(disp, fn) SET_by_offset(disp, _gloffset_ProgramEnvParameters4fvEXT, fn)
#define CALL_ProgramLocalParameters4fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), _gloffset_ProgramLocalParameters4fvEXT, parameters)
#define GET_ProgramLocalParameters4fvEXT(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameters4fvEXT)
#define SET_ProgramLocalParameters4fvEXT(disp, fn) SET_by_offset(disp, _gloffset_ProgramLocalParameters4fvEXT, fn)
#define CALL_GetQueryObjecti64vEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint64EXT *)), _gloffset_GetQueryObjecti64vEXT, parameters)
#define GET_GetQueryObjecti64vEXT(disp) GET_by_offset(disp, _gloffset_GetQueryObjecti64vEXT)
#define SET_GetQueryObjecti64vEXT(disp, fn) SET_by_offset(disp, _gloffset_GetQueryObjecti64vEXT, fn)
#define CALL_GetQueryObjectui64vEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint64EXT *)), _gloffset_GetQueryObjectui64vEXT, parameters)
#define GET_GetQueryObjectui64vEXT(disp) GET_by_offset(disp, _gloffset_GetQueryObjectui64vEXT)
#define SET_GetQueryObjectui64vEXT(disp, fn) SET_by_offset(disp, _gloffset_GetQueryObjectui64vEXT, fn)
#define CALL_EGLImageTargetRenderbufferStorageOES(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLvoid *)), _gloffset_EGLImageTargetRenderbufferStorageOES, parameters)
#define GET_EGLImageTargetRenderbufferStorageOES(disp) GET_by_offset(disp, _gloffset_EGLImageTargetRenderbufferStorageOES)
#define SET_EGLImageTargetRenderbufferStorageOES(disp, fn) SET_by_offset(disp, _gloffset_EGLImageTargetRenderbufferStorageOES, fn)
#define CALL_EGLImageTargetTexture2DOES(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLvoid *)), _gloffset_EGLImageTargetTexture2DOES, parameters)
#define GET_EGLImageTargetTexture2DOES(disp) GET_by_offset(disp, _gloffset_EGLImageTargetTexture2DOES)
#define SET_EGLImageTargetTexture2DOES(disp, fn) SET_by_offset(disp, _gloffset_EGLImageTargetTexture2DOES, fn)

#endif /* !defined( _GLAPI_DISPATCH_H_ ) */
