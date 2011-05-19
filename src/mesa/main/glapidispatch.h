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
#define _gloffset_COUNT 928

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
#define _gloffset_ClampColor 430
#define _gloffset_ClearBufferfi 431
#define _gloffset_ClearBufferfv 432
#define _gloffset_ClearBufferiv 433
#define _gloffset_ClearBufferuiv 434
#define _gloffset_GetStringi 435
#define _gloffset_TexBuffer 436
#define _gloffset_FramebufferTexture 437
#define _gloffset_GetBufferParameteri64v 438
#define _gloffset_GetInteger64i_v 439
#define _gloffset_VertexAttribDivisor 440
#define _gloffset_LoadTransposeMatrixdARB 441
#define _gloffset_LoadTransposeMatrixfARB 442
#define _gloffset_MultTransposeMatrixdARB 443
#define _gloffset_MultTransposeMatrixfARB 444
#define _gloffset_SampleCoverageARB 445
#define _gloffset_CompressedTexImage1DARB 446
#define _gloffset_CompressedTexImage2DARB 447
#define _gloffset_CompressedTexImage3DARB 448
#define _gloffset_CompressedTexSubImage1DARB 449
#define _gloffset_CompressedTexSubImage2DARB 450
#define _gloffset_CompressedTexSubImage3DARB 451
#define _gloffset_GetCompressedTexImageARB 452
#define _gloffset_DisableVertexAttribArrayARB 453
#define _gloffset_EnableVertexAttribArrayARB 454
#define _gloffset_GetProgramEnvParameterdvARB 455
#define _gloffset_GetProgramEnvParameterfvARB 456
#define _gloffset_GetProgramLocalParameterdvARB 457
#define _gloffset_GetProgramLocalParameterfvARB 458
#define _gloffset_GetProgramStringARB 459
#define _gloffset_GetProgramivARB 460
#define _gloffset_GetVertexAttribdvARB 461
#define _gloffset_GetVertexAttribfvARB 462
#define _gloffset_GetVertexAttribivARB 463
#define _gloffset_ProgramEnvParameter4dARB 464
#define _gloffset_ProgramEnvParameter4dvARB 465
#define _gloffset_ProgramEnvParameter4fARB 466
#define _gloffset_ProgramEnvParameter4fvARB 467
#define _gloffset_ProgramLocalParameter4dARB 468
#define _gloffset_ProgramLocalParameter4dvARB 469
#define _gloffset_ProgramLocalParameter4fARB 470
#define _gloffset_ProgramLocalParameter4fvARB 471
#define _gloffset_ProgramStringARB 472
#define _gloffset_VertexAttrib1dARB 473
#define _gloffset_VertexAttrib1dvARB 474
#define _gloffset_VertexAttrib1fARB 475
#define _gloffset_VertexAttrib1fvARB 476
#define _gloffset_VertexAttrib1sARB 477
#define _gloffset_VertexAttrib1svARB 478
#define _gloffset_VertexAttrib2dARB 479
#define _gloffset_VertexAttrib2dvARB 480
#define _gloffset_VertexAttrib2fARB 481
#define _gloffset_VertexAttrib2fvARB 482
#define _gloffset_VertexAttrib2sARB 483
#define _gloffset_VertexAttrib2svARB 484
#define _gloffset_VertexAttrib3dARB 485
#define _gloffset_VertexAttrib3dvARB 486
#define _gloffset_VertexAttrib3fARB 487
#define _gloffset_VertexAttrib3fvARB 488
#define _gloffset_VertexAttrib3sARB 489
#define _gloffset_VertexAttrib3svARB 490
#define _gloffset_VertexAttrib4NbvARB 491
#define _gloffset_VertexAttrib4NivARB 492
#define _gloffset_VertexAttrib4NsvARB 493
#define _gloffset_VertexAttrib4NubARB 494
#define _gloffset_VertexAttrib4NubvARB 495
#define _gloffset_VertexAttrib4NuivARB 496
#define _gloffset_VertexAttrib4NusvARB 497
#define _gloffset_VertexAttrib4bvARB 498
#define _gloffset_VertexAttrib4dARB 499
#define _gloffset_VertexAttrib4dvARB 500
#define _gloffset_VertexAttrib4fARB 501
#define _gloffset_VertexAttrib4fvARB 502
#define _gloffset_VertexAttrib4ivARB 503
#define _gloffset_VertexAttrib4sARB 504
#define _gloffset_VertexAttrib4svARB 505
#define _gloffset_VertexAttrib4ubvARB 506
#define _gloffset_VertexAttrib4uivARB 507
#define _gloffset_VertexAttrib4usvARB 508
#define _gloffset_VertexAttribPointerARB 509
#define _gloffset_BindBufferARB 510
#define _gloffset_BufferDataARB 511
#define _gloffset_BufferSubDataARB 512
#define _gloffset_DeleteBuffersARB 513
#define _gloffset_GenBuffersARB 514
#define _gloffset_GetBufferParameterivARB 515
#define _gloffset_GetBufferPointervARB 516
#define _gloffset_GetBufferSubDataARB 517
#define _gloffset_IsBufferARB 518
#define _gloffset_MapBufferARB 519
#define _gloffset_UnmapBufferARB 520
#define _gloffset_BeginQueryARB 521
#define _gloffset_DeleteQueriesARB 522
#define _gloffset_EndQueryARB 523
#define _gloffset_GenQueriesARB 524
#define _gloffset_GetQueryObjectivARB 525
#define _gloffset_GetQueryObjectuivARB 526
#define _gloffset_GetQueryivARB 527
#define _gloffset_IsQueryARB 528
#define _gloffset_AttachObjectARB 529
#define _gloffset_CompileShaderARB 530
#define _gloffset_CreateProgramObjectARB 531
#define _gloffset_CreateShaderObjectARB 532
#define _gloffset_DeleteObjectARB 533
#define _gloffset_DetachObjectARB 534
#define _gloffset_GetActiveUniformARB 535
#define _gloffset_GetAttachedObjectsARB 536
#define _gloffset_GetHandleARB 537
#define _gloffset_GetInfoLogARB 538
#define _gloffset_GetObjectParameterfvARB 539
#define _gloffset_GetObjectParameterivARB 540
#define _gloffset_GetShaderSourceARB 541
#define _gloffset_GetUniformLocationARB 542
#define _gloffset_GetUniformfvARB 543
#define _gloffset_GetUniformivARB 544
#define _gloffset_LinkProgramARB 545
#define _gloffset_ShaderSourceARB 546
#define _gloffset_Uniform1fARB 547
#define _gloffset_Uniform1fvARB 548
#define _gloffset_Uniform1iARB 549
#define _gloffset_Uniform1ivARB 550
#define _gloffset_Uniform2fARB 551
#define _gloffset_Uniform2fvARB 552
#define _gloffset_Uniform2iARB 553
#define _gloffset_Uniform2ivARB 554
#define _gloffset_Uniform3fARB 555
#define _gloffset_Uniform3fvARB 556
#define _gloffset_Uniform3iARB 557
#define _gloffset_Uniform3ivARB 558
#define _gloffset_Uniform4fARB 559
#define _gloffset_Uniform4fvARB 560
#define _gloffset_Uniform4iARB 561
#define _gloffset_Uniform4ivARB 562
#define _gloffset_UniformMatrix2fvARB 563
#define _gloffset_UniformMatrix3fvARB 564
#define _gloffset_UniformMatrix4fvARB 565
#define _gloffset_UseProgramObjectARB 566
#define _gloffset_ValidateProgramARB 567
#define _gloffset_BindAttribLocationARB 568
#define _gloffset_GetActiveAttribARB 569
#define _gloffset_GetAttribLocationARB 570
#define _gloffset_DrawBuffersARB 571
#define _gloffset_ClampColorARB 572
#define _gloffset_DrawArraysInstancedARB 573
#define _gloffset_DrawElementsInstancedARB 574
#define _gloffset_RenderbufferStorageMultisample 575
#define _gloffset_FramebufferTextureARB 576
#define _gloffset_FramebufferTextureFaceARB 577
#define _gloffset_ProgramParameteriARB 578
#define _gloffset_VertexAttribDivisorARB 579
#define _gloffset_FlushMappedBufferRange 580
#define _gloffset_MapBufferRange 581
#define _gloffset_TexBufferARB 582
#define _gloffset_BindVertexArray 583
#define _gloffset_GenVertexArrays 584
#define _gloffset_CopyBufferSubData 585
#define _gloffset_ClientWaitSync 586
#define _gloffset_DeleteSync 587
#define _gloffset_FenceSync 588
#define _gloffset_GetInteger64v 589
#define _gloffset_GetSynciv 590
#define _gloffset_IsSync 591
#define _gloffset_WaitSync 592
#define _gloffset_DrawElementsBaseVertex 593
#define _gloffset_DrawRangeElementsBaseVertex 594
#define _gloffset_MultiDrawElementsBaseVertex 595
#define _gloffset_BlendEquationSeparateiARB 596
#define _gloffset_BlendEquationiARB 597
#define _gloffset_BlendFuncSeparateiARB 598
#define _gloffset_BlendFunciARB 599
#define _gloffset_BindSampler 600
#define _gloffset_DeleteSamplers 601
#define _gloffset_GenSamplers 602
#define _gloffset_GetSamplerParameterIiv 603
#define _gloffset_GetSamplerParameterIuiv 604
#define _gloffset_GetSamplerParameterfv 605
#define _gloffset_GetSamplerParameteriv 606
#define _gloffset_IsSampler 607
#define _gloffset_SamplerParameterIiv 608
#define _gloffset_SamplerParameterIuiv 609
#define _gloffset_SamplerParameterf 610
#define _gloffset_SamplerParameterfv 611
#define _gloffset_SamplerParameteri 612
#define _gloffset_SamplerParameteriv 613
#define _gloffset_BindTransformFeedback 614
#define _gloffset_DeleteTransformFeedbacks 615
#define _gloffset_DrawTransformFeedback 616
#define _gloffset_GenTransformFeedbacks 617
#define _gloffset_IsTransformFeedback 618
#define _gloffset_PauseTransformFeedback 619
#define _gloffset_ResumeTransformFeedback 620
#define _gloffset_ClearDepthf 621
#define _gloffset_DepthRangef 622
#define _gloffset_GetShaderPrecisionFormat 623
#define _gloffset_ReleaseShaderCompiler 624
#define _gloffset_ShaderBinary 625
#define _gloffset_GetGraphicsResetStatusARB 626
#define _gloffset_GetnColorTableARB 627
#define _gloffset_GetnCompressedTexImageARB 628
#define _gloffset_GetnConvolutionFilterARB 629
#define _gloffset_GetnHistogramARB 630
#define _gloffset_GetnMapdvARB 631
#define _gloffset_GetnMapfvARB 632
#define _gloffset_GetnMapivARB 633
#define _gloffset_GetnMinmaxARB 634
#define _gloffset_GetnPixelMapfvARB 635
#define _gloffset_GetnPixelMapuivARB 636
#define _gloffset_GetnPixelMapusvARB 637
#define _gloffset_GetnPolygonStippleARB 638
#define _gloffset_GetnSeparableFilterARB 639
#define _gloffset_GetnTexImageARB 640
#define _gloffset_GetnUniformdvARB 641
#define _gloffset_GetnUniformfvARB 642
#define _gloffset_GetnUniformivARB 643
#define _gloffset_GetnUniformuivARB 644
#define _gloffset_ReadnPixelsARB 645
#define _gloffset_PolygonOffsetEXT 646
#define _gloffset_GetPixelTexGenParameterfvSGIS 647
#define _gloffset_GetPixelTexGenParameterivSGIS 648
#define _gloffset_PixelTexGenParameterfSGIS 649
#define _gloffset_PixelTexGenParameterfvSGIS 650
#define _gloffset_PixelTexGenParameteriSGIS 651
#define _gloffset_PixelTexGenParameterivSGIS 652
#define _gloffset_SampleMaskSGIS 653
#define _gloffset_SamplePatternSGIS 654
#define _gloffset_ColorPointerEXT 655
#define _gloffset_EdgeFlagPointerEXT 656
#define _gloffset_IndexPointerEXT 657
#define _gloffset_NormalPointerEXT 658
#define _gloffset_TexCoordPointerEXT 659
#define _gloffset_VertexPointerEXT 660
#define _gloffset_PointParameterfEXT 661
#define _gloffset_PointParameterfvEXT 662
#define _gloffset_LockArraysEXT 663
#define _gloffset_UnlockArraysEXT 664
#define _gloffset_SecondaryColor3bEXT 665
#define _gloffset_SecondaryColor3bvEXT 666
#define _gloffset_SecondaryColor3dEXT 667
#define _gloffset_SecondaryColor3dvEXT 668
#define _gloffset_SecondaryColor3fEXT 669
#define _gloffset_SecondaryColor3fvEXT 670
#define _gloffset_SecondaryColor3iEXT 671
#define _gloffset_SecondaryColor3ivEXT 672
#define _gloffset_SecondaryColor3sEXT 673
#define _gloffset_SecondaryColor3svEXT 674
#define _gloffset_SecondaryColor3ubEXT 675
#define _gloffset_SecondaryColor3ubvEXT 676
#define _gloffset_SecondaryColor3uiEXT 677
#define _gloffset_SecondaryColor3uivEXT 678
#define _gloffset_SecondaryColor3usEXT 679
#define _gloffset_SecondaryColor3usvEXT 680
#define _gloffset_SecondaryColorPointerEXT 681
#define _gloffset_MultiDrawArraysEXT 682
#define _gloffset_MultiDrawElementsEXT 683
#define _gloffset_FogCoordPointerEXT 684
#define _gloffset_FogCoorddEXT 685
#define _gloffset_FogCoorddvEXT 686
#define _gloffset_FogCoordfEXT 687
#define _gloffset_FogCoordfvEXT 688
#define _gloffset_PixelTexGenSGIX 689
#define _gloffset_BlendFuncSeparateEXT 690
#define _gloffset_FlushVertexArrayRangeNV 691
#define _gloffset_VertexArrayRangeNV 692
#define _gloffset_CombinerInputNV 693
#define _gloffset_CombinerOutputNV 694
#define _gloffset_CombinerParameterfNV 695
#define _gloffset_CombinerParameterfvNV 696
#define _gloffset_CombinerParameteriNV 697
#define _gloffset_CombinerParameterivNV 698
#define _gloffset_FinalCombinerInputNV 699
#define _gloffset_GetCombinerInputParameterfvNV 700
#define _gloffset_GetCombinerInputParameterivNV 701
#define _gloffset_GetCombinerOutputParameterfvNV 702
#define _gloffset_GetCombinerOutputParameterivNV 703
#define _gloffset_GetFinalCombinerInputParameterfvNV 704
#define _gloffset_GetFinalCombinerInputParameterivNV 705
#define _gloffset_ResizeBuffersMESA 706
#define _gloffset_WindowPos2dMESA 707
#define _gloffset_WindowPos2dvMESA 708
#define _gloffset_WindowPos2fMESA 709
#define _gloffset_WindowPos2fvMESA 710
#define _gloffset_WindowPos2iMESA 711
#define _gloffset_WindowPos2ivMESA 712
#define _gloffset_WindowPos2sMESA 713
#define _gloffset_WindowPos2svMESA 714
#define _gloffset_WindowPos3dMESA 715
#define _gloffset_WindowPos3dvMESA 716
#define _gloffset_WindowPos3fMESA 717
#define _gloffset_WindowPos3fvMESA 718
#define _gloffset_WindowPos3iMESA 719
#define _gloffset_WindowPos3ivMESA 720
#define _gloffset_WindowPos3sMESA 721
#define _gloffset_WindowPos3svMESA 722
#define _gloffset_WindowPos4dMESA 723
#define _gloffset_WindowPos4dvMESA 724
#define _gloffset_WindowPos4fMESA 725
#define _gloffset_WindowPos4fvMESA 726
#define _gloffset_WindowPos4iMESA 727
#define _gloffset_WindowPos4ivMESA 728
#define _gloffset_WindowPos4sMESA 729
#define _gloffset_WindowPos4svMESA 730
#define _gloffset_MultiModeDrawArraysIBM 731
#define _gloffset_MultiModeDrawElementsIBM 732
#define _gloffset_DeleteFencesNV 733
#define _gloffset_FinishFenceNV 734
#define _gloffset_GenFencesNV 735
#define _gloffset_GetFenceivNV 736
#define _gloffset_IsFenceNV 737
#define _gloffset_SetFenceNV 738
#define _gloffset_TestFenceNV 739
#define _gloffset_AreProgramsResidentNV 740
#define _gloffset_BindProgramNV 741
#define _gloffset_DeleteProgramsNV 742
#define _gloffset_ExecuteProgramNV 743
#define _gloffset_GenProgramsNV 744
#define _gloffset_GetProgramParameterdvNV 745
#define _gloffset_GetProgramParameterfvNV 746
#define _gloffset_GetProgramStringNV 747
#define _gloffset_GetProgramivNV 748
#define _gloffset_GetTrackMatrixivNV 749
#define _gloffset_GetVertexAttribPointervNV 750
#define _gloffset_GetVertexAttribdvNV 751
#define _gloffset_GetVertexAttribfvNV 752
#define _gloffset_GetVertexAttribivNV 753
#define _gloffset_IsProgramNV 754
#define _gloffset_LoadProgramNV 755
#define _gloffset_ProgramParameters4dvNV 756
#define _gloffset_ProgramParameters4fvNV 757
#define _gloffset_RequestResidentProgramsNV 758
#define _gloffset_TrackMatrixNV 759
#define _gloffset_VertexAttrib1dNV 760
#define _gloffset_VertexAttrib1dvNV 761
#define _gloffset_VertexAttrib1fNV 762
#define _gloffset_VertexAttrib1fvNV 763
#define _gloffset_VertexAttrib1sNV 764
#define _gloffset_VertexAttrib1svNV 765
#define _gloffset_VertexAttrib2dNV 766
#define _gloffset_VertexAttrib2dvNV 767
#define _gloffset_VertexAttrib2fNV 768
#define _gloffset_VertexAttrib2fvNV 769
#define _gloffset_VertexAttrib2sNV 770
#define _gloffset_VertexAttrib2svNV 771
#define _gloffset_VertexAttrib3dNV 772
#define _gloffset_VertexAttrib3dvNV 773
#define _gloffset_VertexAttrib3fNV 774
#define _gloffset_VertexAttrib3fvNV 775
#define _gloffset_VertexAttrib3sNV 776
#define _gloffset_VertexAttrib3svNV 777
#define _gloffset_VertexAttrib4dNV 778
#define _gloffset_VertexAttrib4dvNV 779
#define _gloffset_VertexAttrib4fNV 780
#define _gloffset_VertexAttrib4fvNV 781
#define _gloffset_VertexAttrib4sNV 782
#define _gloffset_VertexAttrib4svNV 783
#define _gloffset_VertexAttrib4ubNV 784
#define _gloffset_VertexAttrib4ubvNV 785
#define _gloffset_VertexAttribPointerNV 786
#define _gloffset_VertexAttribs1dvNV 787
#define _gloffset_VertexAttribs1fvNV 788
#define _gloffset_VertexAttribs1svNV 789
#define _gloffset_VertexAttribs2dvNV 790
#define _gloffset_VertexAttribs2fvNV 791
#define _gloffset_VertexAttribs2svNV 792
#define _gloffset_VertexAttribs3dvNV 793
#define _gloffset_VertexAttribs3fvNV 794
#define _gloffset_VertexAttribs3svNV 795
#define _gloffset_VertexAttribs4dvNV 796
#define _gloffset_VertexAttribs4fvNV 797
#define _gloffset_VertexAttribs4svNV 798
#define _gloffset_VertexAttribs4ubvNV 799
#define _gloffset_GetTexBumpParameterfvATI 800
#define _gloffset_GetTexBumpParameterivATI 801
#define _gloffset_TexBumpParameterfvATI 802
#define _gloffset_TexBumpParameterivATI 803
#define _gloffset_AlphaFragmentOp1ATI 804
#define _gloffset_AlphaFragmentOp2ATI 805
#define _gloffset_AlphaFragmentOp3ATI 806
#define _gloffset_BeginFragmentShaderATI 807
#define _gloffset_BindFragmentShaderATI 808
#define _gloffset_ColorFragmentOp1ATI 809
#define _gloffset_ColorFragmentOp2ATI 810
#define _gloffset_ColorFragmentOp3ATI 811
#define _gloffset_DeleteFragmentShaderATI 812
#define _gloffset_EndFragmentShaderATI 813
#define _gloffset_GenFragmentShadersATI 814
#define _gloffset_PassTexCoordATI 815
#define _gloffset_SampleMapATI 816
#define _gloffset_SetFragmentShaderConstantATI 817
#define _gloffset_PointParameteriNV 818
#define _gloffset_PointParameterivNV 819
#define _gloffset_ActiveStencilFaceEXT 820
#define _gloffset_BindVertexArrayAPPLE 821
#define _gloffset_DeleteVertexArraysAPPLE 822
#define _gloffset_GenVertexArraysAPPLE 823
#define _gloffset_IsVertexArrayAPPLE 824
#define _gloffset_GetProgramNamedParameterdvNV 825
#define _gloffset_GetProgramNamedParameterfvNV 826
#define _gloffset_ProgramNamedParameter4dNV 827
#define _gloffset_ProgramNamedParameter4dvNV 828
#define _gloffset_ProgramNamedParameter4fNV 829
#define _gloffset_ProgramNamedParameter4fvNV 830
#define _gloffset_PrimitiveRestartIndexNV 831
#define _gloffset_PrimitiveRestartNV 832
#define _gloffset_DepthBoundsEXT 833
#define _gloffset_BlendEquationSeparateEXT 834
#define _gloffset_BindFramebufferEXT 835
#define _gloffset_BindRenderbufferEXT 836
#define _gloffset_CheckFramebufferStatusEXT 837
#define _gloffset_DeleteFramebuffersEXT 838
#define _gloffset_DeleteRenderbuffersEXT 839
#define _gloffset_FramebufferRenderbufferEXT 840
#define _gloffset_FramebufferTexture1DEXT 841
#define _gloffset_FramebufferTexture2DEXT 842
#define _gloffset_FramebufferTexture3DEXT 843
#define _gloffset_GenFramebuffersEXT 844
#define _gloffset_GenRenderbuffersEXT 845
#define _gloffset_GenerateMipmapEXT 846
#define _gloffset_GetFramebufferAttachmentParameterivEXT 847
#define _gloffset_GetRenderbufferParameterivEXT 848
#define _gloffset_IsFramebufferEXT 849
#define _gloffset_IsRenderbufferEXT 850
#define _gloffset_RenderbufferStorageEXT 851
#define _gloffset_BlitFramebufferEXT 852
#define _gloffset_BufferParameteriAPPLE 853
#define _gloffset_FlushMappedBufferRangeAPPLE 854
#define _gloffset_BindFragDataLocationEXT 855
#define _gloffset_GetFragDataLocationEXT 856
#define _gloffset_GetUniformuivEXT 857
#define _gloffset_GetVertexAttribIivEXT 858
#define _gloffset_GetVertexAttribIuivEXT 859
#define _gloffset_Uniform1uiEXT 860
#define _gloffset_Uniform1uivEXT 861
#define _gloffset_Uniform2uiEXT 862
#define _gloffset_Uniform2uivEXT 863
#define _gloffset_Uniform3uiEXT 864
#define _gloffset_Uniform3uivEXT 865
#define _gloffset_Uniform4uiEXT 866
#define _gloffset_Uniform4uivEXT 867
#define _gloffset_VertexAttribI1iEXT 868
#define _gloffset_VertexAttribI1ivEXT 869
#define _gloffset_VertexAttribI1uiEXT 870
#define _gloffset_VertexAttribI1uivEXT 871
#define _gloffset_VertexAttribI2iEXT 872
#define _gloffset_VertexAttribI2ivEXT 873
#define _gloffset_VertexAttribI2uiEXT 874
#define _gloffset_VertexAttribI2uivEXT 875
#define _gloffset_VertexAttribI3iEXT 876
#define _gloffset_VertexAttribI3ivEXT 877
#define _gloffset_VertexAttribI3uiEXT 878
#define _gloffset_VertexAttribI3uivEXT 879
#define _gloffset_VertexAttribI4bvEXT 880
#define _gloffset_VertexAttribI4iEXT 881
#define _gloffset_VertexAttribI4ivEXT 882
#define _gloffset_VertexAttribI4svEXT 883
#define _gloffset_VertexAttribI4ubvEXT 884
#define _gloffset_VertexAttribI4uiEXT 885
#define _gloffset_VertexAttribI4uivEXT 886
#define _gloffset_VertexAttribI4usvEXT 887
#define _gloffset_VertexAttribIPointerEXT 888
#define _gloffset_FramebufferTextureLayerEXT 889
#define _gloffset_ColorMaskIndexedEXT 890
#define _gloffset_DisableIndexedEXT 891
#define _gloffset_EnableIndexedEXT 892
#define _gloffset_GetBooleanIndexedvEXT 893
#define _gloffset_GetIntegerIndexedvEXT 894
#define _gloffset_IsEnabledIndexedEXT 895
#define _gloffset_ClearColorIiEXT 896
#define _gloffset_ClearColorIuiEXT 897
#define _gloffset_GetTexParameterIivEXT 898
#define _gloffset_GetTexParameterIuivEXT 899
#define _gloffset_TexParameterIivEXT 900
#define _gloffset_TexParameterIuivEXT 901
#define _gloffset_BeginConditionalRenderNV 902
#define _gloffset_EndConditionalRenderNV 903
#define _gloffset_BeginTransformFeedbackEXT 904
#define _gloffset_BindBufferBaseEXT 905
#define _gloffset_BindBufferOffsetEXT 906
#define _gloffset_BindBufferRangeEXT 907
#define _gloffset_EndTransformFeedbackEXT 908
#define _gloffset_GetTransformFeedbackVaryingEXT 909
#define _gloffset_TransformFeedbackVaryingsEXT 910
#define _gloffset_ProvokingVertexEXT 911
#define _gloffset_GetTexParameterPointervAPPLE 912
#define _gloffset_TextureRangeAPPLE 913
#define _gloffset_GetObjectParameterivAPPLE 914
#define _gloffset_ObjectPurgeableAPPLE 915
#define _gloffset_ObjectUnpurgeableAPPLE 916
#define _gloffset_ActiveProgramEXT 917
#define _gloffset_CreateShaderProgramEXT 918
#define _gloffset_UseShaderProgramEXT 919
#define _gloffset_TextureBarrierNV 920
#define _gloffset_StencilFuncSeparateATI 921
#define _gloffset_ProgramEnvParameters4fvEXT 922
#define _gloffset_ProgramLocalParameters4fvEXT 923
#define _gloffset_GetQueryObjecti64vEXT 924
#define _gloffset_GetQueryObjectui64vEXT 925
#define _gloffset_EGLImageTargetRenderbufferStorageOES 926
#define _gloffset_EGLImageTargetTexture2DOES 927

#else /* !_GLAPI_USE_REMAP_TABLE */

#define driDispatchRemapTable_size 520
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
#define ClampColor_remap_index 22
#define ClearBufferfi_remap_index 23
#define ClearBufferfv_remap_index 24
#define ClearBufferiv_remap_index 25
#define ClearBufferuiv_remap_index 26
#define GetStringi_remap_index 27
#define TexBuffer_remap_index 28
#define FramebufferTexture_remap_index 29
#define GetBufferParameteri64v_remap_index 30
#define GetInteger64i_v_remap_index 31
#define VertexAttribDivisor_remap_index 32
#define LoadTransposeMatrixdARB_remap_index 33
#define LoadTransposeMatrixfARB_remap_index 34
#define MultTransposeMatrixdARB_remap_index 35
#define MultTransposeMatrixfARB_remap_index 36
#define SampleCoverageARB_remap_index 37
#define CompressedTexImage1DARB_remap_index 38
#define CompressedTexImage2DARB_remap_index 39
#define CompressedTexImage3DARB_remap_index 40
#define CompressedTexSubImage1DARB_remap_index 41
#define CompressedTexSubImage2DARB_remap_index 42
#define CompressedTexSubImage3DARB_remap_index 43
#define GetCompressedTexImageARB_remap_index 44
#define DisableVertexAttribArrayARB_remap_index 45
#define EnableVertexAttribArrayARB_remap_index 46
#define GetProgramEnvParameterdvARB_remap_index 47
#define GetProgramEnvParameterfvARB_remap_index 48
#define GetProgramLocalParameterdvARB_remap_index 49
#define GetProgramLocalParameterfvARB_remap_index 50
#define GetProgramStringARB_remap_index 51
#define GetProgramivARB_remap_index 52
#define GetVertexAttribdvARB_remap_index 53
#define GetVertexAttribfvARB_remap_index 54
#define GetVertexAttribivARB_remap_index 55
#define ProgramEnvParameter4dARB_remap_index 56
#define ProgramEnvParameter4dvARB_remap_index 57
#define ProgramEnvParameter4fARB_remap_index 58
#define ProgramEnvParameter4fvARB_remap_index 59
#define ProgramLocalParameter4dARB_remap_index 60
#define ProgramLocalParameter4dvARB_remap_index 61
#define ProgramLocalParameter4fARB_remap_index 62
#define ProgramLocalParameter4fvARB_remap_index 63
#define ProgramStringARB_remap_index 64
#define VertexAttrib1dARB_remap_index 65
#define VertexAttrib1dvARB_remap_index 66
#define VertexAttrib1fARB_remap_index 67
#define VertexAttrib1fvARB_remap_index 68
#define VertexAttrib1sARB_remap_index 69
#define VertexAttrib1svARB_remap_index 70
#define VertexAttrib2dARB_remap_index 71
#define VertexAttrib2dvARB_remap_index 72
#define VertexAttrib2fARB_remap_index 73
#define VertexAttrib2fvARB_remap_index 74
#define VertexAttrib2sARB_remap_index 75
#define VertexAttrib2svARB_remap_index 76
#define VertexAttrib3dARB_remap_index 77
#define VertexAttrib3dvARB_remap_index 78
#define VertexAttrib3fARB_remap_index 79
#define VertexAttrib3fvARB_remap_index 80
#define VertexAttrib3sARB_remap_index 81
#define VertexAttrib3svARB_remap_index 82
#define VertexAttrib4NbvARB_remap_index 83
#define VertexAttrib4NivARB_remap_index 84
#define VertexAttrib4NsvARB_remap_index 85
#define VertexAttrib4NubARB_remap_index 86
#define VertexAttrib4NubvARB_remap_index 87
#define VertexAttrib4NuivARB_remap_index 88
#define VertexAttrib4NusvARB_remap_index 89
#define VertexAttrib4bvARB_remap_index 90
#define VertexAttrib4dARB_remap_index 91
#define VertexAttrib4dvARB_remap_index 92
#define VertexAttrib4fARB_remap_index 93
#define VertexAttrib4fvARB_remap_index 94
#define VertexAttrib4ivARB_remap_index 95
#define VertexAttrib4sARB_remap_index 96
#define VertexAttrib4svARB_remap_index 97
#define VertexAttrib4ubvARB_remap_index 98
#define VertexAttrib4uivARB_remap_index 99
#define VertexAttrib4usvARB_remap_index 100
#define VertexAttribPointerARB_remap_index 101
#define BindBufferARB_remap_index 102
#define BufferDataARB_remap_index 103
#define BufferSubDataARB_remap_index 104
#define DeleteBuffersARB_remap_index 105
#define GenBuffersARB_remap_index 106
#define GetBufferParameterivARB_remap_index 107
#define GetBufferPointervARB_remap_index 108
#define GetBufferSubDataARB_remap_index 109
#define IsBufferARB_remap_index 110
#define MapBufferARB_remap_index 111
#define UnmapBufferARB_remap_index 112
#define BeginQueryARB_remap_index 113
#define DeleteQueriesARB_remap_index 114
#define EndQueryARB_remap_index 115
#define GenQueriesARB_remap_index 116
#define GetQueryObjectivARB_remap_index 117
#define GetQueryObjectuivARB_remap_index 118
#define GetQueryivARB_remap_index 119
#define IsQueryARB_remap_index 120
#define AttachObjectARB_remap_index 121
#define CompileShaderARB_remap_index 122
#define CreateProgramObjectARB_remap_index 123
#define CreateShaderObjectARB_remap_index 124
#define DeleteObjectARB_remap_index 125
#define DetachObjectARB_remap_index 126
#define GetActiveUniformARB_remap_index 127
#define GetAttachedObjectsARB_remap_index 128
#define GetHandleARB_remap_index 129
#define GetInfoLogARB_remap_index 130
#define GetObjectParameterfvARB_remap_index 131
#define GetObjectParameterivARB_remap_index 132
#define GetShaderSourceARB_remap_index 133
#define GetUniformLocationARB_remap_index 134
#define GetUniformfvARB_remap_index 135
#define GetUniformivARB_remap_index 136
#define LinkProgramARB_remap_index 137
#define ShaderSourceARB_remap_index 138
#define Uniform1fARB_remap_index 139
#define Uniform1fvARB_remap_index 140
#define Uniform1iARB_remap_index 141
#define Uniform1ivARB_remap_index 142
#define Uniform2fARB_remap_index 143
#define Uniform2fvARB_remap_index 144
#define Uniform2iARB_remap_index 145
#define Uniform2ivARB_remap_index 146
#define Uniform3fARB_remap_index 147
#define Uniform3fvARB_remap_index 148
#define Uniform3iARB_remap_index 149
#define Uniform3ivARB_remap_index 150
#define Uniform4fARB_remap_index 151
#define Uniform4fvARB_remap_index 152
#define Uniform4iARB_remap_index 153
#define Uniform4ivARB_remap_index 154
#define UniformMatrix2fvARB_remap_index 155
#define UniformMatrix3fvARB_remap_index 156
#define UniformMatrix4fvARB_remap_index 157
#define UseProgramObjectARB_remap_index 158
#define ValidateProgramARB_remap_index 159
#define BindAttribLocationARB_remap_index 160
#define GetActiveAttribARB_remap_index 161
#define GetAttribLocationARB_remap_index 162
#define DrawBuffersARB_remap_index 163
#define ClampColorARB_remap_index 164
#define DrawArraysInstancedARB_remap_index 165
#define DrawElementsInstancedARB_remap_index 166
#define RenderbufferStorageMultisample_remap_index 167
#define FramebufferTextureARB_remap_index 168
#define FramebufferTextureFaceARB_remap_index 169
#define ProgramParameteriARB_remap_index 170
#define VertexAttribDivisorARB_remap_index 171
#define FlushMappedBufferRange_remap_index 172
#define MapBufferRange_remap_index 173
#define TexBufferARB_remap_index 174
#define BindVertexArray_remap_index 175
#define GenVertexArrays_remap_index 176
#define CopyBufferSubData_remap_index 177
#define ClientWaitSync_remap_index 178
#define DeleteSync_remap_index 179
#define FenceSync_remap_index 180
#define GetInteger64v_remap_index 181
#define GetSynciv_remap_index 182
#define IsSync_remap_index 183
#define WaitSync_remap_index 184
#define DrawElementsBaseVertex_remap_index 185
#define DrawRangeElementsBaseVertex_remap_index 186
#define MultiDrawElementsBaseVertex_remap_index 187
#define BlendEquationSeparateiARB_remap_index 188
#define BlendEquationiARB_remap_index 189
#define BlendFuncSeparateiARB_remap_index 190
#define BlendFunciARB_remap_index 191
#define BindSampler_remap_index 192
#define DeleteSamplers_remap_index 193
#define GenSamplers_remap_index 194
#define GetSamplerParameterIiv_remap_index 195
#define GetSamplerParameterIuiv_remap_index 196
#define GetSamplerParameterfv_remap_index 197
#define GetSamplerParameteriv_remap_index 198
#define IsSampler_remap_index 199
#define SamplerParameterIiv_remap_index 200
#define SamplerParameterIuiv_remap_index 201
#define SamplerParameterf_remap_index 202
#define SamplerParameterfv_remap_index 203
#define SamplerParameteri_remap_index 204
#define SamplerParameteriv_remap_index 205
#define BindTransformFeedback_remap_index 206
#define DeleteTransformFeedbacks_remap_index 207
#define DrawTransformFeedback_remap_index 208
#define GenTransformFeedbacks_remap_index 209
#define IsTransformFeedback_remap_index 210
#define PauseTransformFeedback_remap_index 211
#define ResumeTransformFeedback_remap_index 212
#define ClearDepthf_remap_index 213
#define DepthRangef_remap_index 214
#define GetShaderPrecisionFormat_remap_index 215
#define ReleaseShaderCompiler_remap_index 216
#define ShaderBinary_remap_index 217
#define GetGraphicsResetStatusARB_remap_index 218
#define GetnColorTableARB_remap_index 219
#define GetnCompressedTexImageARB_remap_index 220
#define GetnConvolutionFilterARB_remap_index 221
#define GetnHistogramARB_remap_index 222
#define GetnMapdvARB_remap_index 223
#define GetnMapfvARB_remap_index 224
#define GetnMapivARB_remap_index 225
#define GetnMinmaxARB_remap_index 226
#define GetnPixelMapfvARB_remap_index 227
#define GetnPixelMapuivARB_remap_index 228
#define GetnPixelMapusvARB_remap_index 229
#define GetnPolygonStippleARB_remap_index 230
#define GetnSeparableFilterARB_remap_index 231
#define GetnTexImageARB_remap_index 232
#define GetnUniformdvARB_remap_index 233
#define GetnUniformfvARB_remap_index 234
#define GetnUniformivARB_remap_index 235
#define GetnUniformuivARB_remap_index 236
#define ReadnPixelsARB_remap_index 237
#define PolygonOffsetEXT_remap_index 238
#define GetPixelTexGenParameterfvSGIS_remap_index 239
#define GetPixelTexGenParameterivSGIS_remap_index 240
#define PixelTexGenParameterfSGIS_remap_index 241
#define PixelTexGenParameterfvSGIS_remap_index 242
#define PixelTexGenParameteriSGIS_remap_index 243
#define PixelTexGenParameterivSGIS_remap_index 244
#define SampleMaskSGIS_remap_index 245
#define SamplePatternSGIS_remap_index 246
#define ColorPointerEXT_remap_index 247
#define EdgeFlagPointerEXT_remap_index 248
#define IndexPointerEXT_remap_index 249
#define NormalPointerEXT_remap_index 250
#define TexCoordPointerEXT_remap_index 251
#define VertexPointerEXT_remap_index 252
#define PointParameterfEXT_remap_index 253
#define PointParameterfvEXT_remap_index 254
#define LockArraysEXT_remap_index 255
#define UnlockArraysEXT_remap_index 256
#define SecondaryColor3bEXT_remap_index 257
#define SecondaryColor3bvEXT_remap_index 258
#define SecondaryColor3dEXT_remap_index 259
#define SecondaryColor3dvEXT_remap_index 260
#define SecondaryColor3fEXT_remap_index 261
#define SecondaryColor3fvEXT_remap_index 262
#define SecondaryColor3iEXT_remap_index 263
#define SecondaryColor3ivEXT_remap_index 264
#define SecondaryColor3sEXT_remap_index 265
#define SecondaryColor3svEXT_remap_index 266
#define SecondaryColor3ubEXT_remap_index 267
#define SecondaryColor3ubvEXT_remap_index 268
#define SecondaryColor3uiEXT_remap_index 269
#define SecondaryColor3uivEXT_remap_index 270
#define SecondaryColor3usEXT_remap_index 271
#define SecondaryColor3usvEXT_remap_index 272
#define SecondaryColorPointerEXT_remap_index 273
#define MultiDrawArraysEXT_remap_index 274
#define MultiDrawElementsEXT_remap_index 275
#define FogCoordPointerEXT_remap_index 276
#define FogCoorddEXT_remap_index 277
#define FogCoorddvEXT_remap_index 278
#define FogCoordfEXT_remap_index 279
#define FogCoordfvEXT_remap_index 280
#define PixelTexGenSGIX_remap_index 281
#define BlendFuncSeparateEXT_remap_index 282
#define FlushVertexArrayRangeNV_remap_index 283
#define VertexArrayRangeNV_remap_index 284
#define CombinerInputNV_remap_index 285
#define CombinerOutputNV_remap_index 286
#define CombinerParameterfNV_remap_index 287
#define CombinerParameterfvNV_remap_index 288
#define CombinerParameteriNV_remap_index 289
#define CombinerParameterivNV_remap_index 290
#define FinalCombinerInputNV_remap_index 291
#define GetCombinerInputParameterfvNV_remap_index 292
#define GetCombinerInputParameterivNV_remap_index 293
#define GetCombinerOutputParameterfvNV_remap_index 294
#define GetCombinerOutputParameterivNV_remap_index 295
#define GetFinalCombinerInputParameterfvNV_remap_index 296
#define GetFinalCombinerInputParameterivNV_remap_index 297
#define ResizeBuffersMESA_remap_index 298
#define WindowPos2dMESA_remap_index 299
#define WindowPos2dvMESA_remap_index 300
#define WindowPos2fMESA_remap_index 301
#define WindowPos2fvMESA_remap_index 302
#define WindowPos2iMESA_remap_index 303
#define WindowPos2ivMESA_remap_index 304
#define WindowPos2sMESA_remap_index 305
#define WindowPos2svMESA_remap_index 306
#define WindowPos3dMESA_remap_index 307
#define WindowPos3dvMESA_remap_index 308
#define WindowPos3fMESA_remap_index 309
#define WindowPos3fvMESA_remap_index 310
#define WindowPos3iMESA_remap_index 311
#define WindowPos3ivMESA_remap_index 312
#define WindowPos3sMESA_remap_index 313
#define WindowPos3svMESA_remap_index 314
#define WindowPos4dMESA_remap_index 315
#define WindowPos4dvMESA_remap_index 316
#define WindowPos4fMESA_remap_index 317
#define WindowPos4fvMESA_remap_index 318
#define WindowPos4iMESA_remap_index 319
#define WindowPos4ivMESA_remap_index 320
#define WindowPos4sMESA_remap_index 321
#define WindowPos4svMESA_remap_index 322
#define MultiModeDrawArraysIBM_remap_index 323
#define MultiModeDrawElementsIBM_remap_index 324
#define DeleteFencesNV_remap_index 325
#define FinishFenceNV_remap_index 326
#define GenFencesNV_remap_index 327
#define GetFenceivNV_remap_index 328
#define IsFenceNV_remap_index 329
#define SetFenceNV_remap_index 330
#define TestFenceNV_remap_index 331
#define AreProgramsResidentNV_remap_index 332
#define BindProgramNV_remap_index 333
#define DeleteProgramsNV_remap_index 334
#define ExecuteProgramNV_remap_index 335
#define GenProgramsNV_remap_index 336
#define GetProgramParameterdvNV_remap_index 337
#define GetProgramParameterfvNV_remap_index 338
#define GetProgramStringNV_remap_index 339
#define GetProgramivNV_remap_index 340
#define GetTrackMatrixivNV_remap_index 341
#define GetVertexAttribPointervNV_remap_index 342
#define GetVertexAttribdvNV_remap_index 343
#define GetVertexAttribfvNV_remap_index 344
#define GetVertexAttribivNV_remap_index 345
#define IsProgramNV_remap_index 346
#define LoadProgramNV_remap_index 347
#define ProgramParameters4dvNV_remap_index 348
#define ProgramParameters4fvNV_remap_index 349
#define RequestResidentProgramsNV_remap_index 350
#define TrackMatrixNV_remap_index 351
#define VertexAttrib1dNV_remap_index 352
#define VertexAttrib1dvNV_remap_index 353
#define VertexAttrib1fNV_remap_index 354
#define VertexAttrib1fvNV_remap_index 355
#define VertexAttrib1sNV_remap_index 356
#define VertexAttrib1svNV_remap_index 357
#define VertexAttrib2dNV_remap_index 358
#define VertexAttrib2dvNV_remap_index 359
#define VertexAttrib2fNV_remap_index 360
#define VertexAttrib2fvNV_remap_index 361
#define VertexAttrib2sNV_remap_index 362
#define VertexAttrib2svNV_remap_index 363
#define VertexAttrib3dNV_remap_index 364
#define VertexAttrib3dvNV_remap_index 365
#define VertexAttrib3fNV_remap_index 366
#define VertexAttrib3fvNV_remap_index 367
#define VertexAttrib3sNV_remap_index 368
#define VertexAttrib3svNV_remap_index 369
#define VertexAttrib4dNV_remap_index 370
#define VertexAttrib4dvNV_remap_index 371
#define VertexAttrib4fNV_remap_index 372
#define VertexAttrib4fvNV_remap_index 373
#define VertexAttrib4sNV_remap_index 374
#define VertexAttrib4svNV_remap_index 375
#define VertexAttrib4ubNV_remap_index 376
#define VertexAttrib4ubvNV_remap_index 377
#define VertexAttribPointerNV_remap_index 378
#define VertexAttribs1dvNV_remap_index 379
#define VertexAttribs1fvNV_remap_index 380
#define VertexAttribs1svNV_remap_index 381
#define VertexAttribs2dvNV_remap_index 382
#define VertexAttribs2fvNV_remap_index 383
#define VertexAttribs2svNV_remap_index 384
#define VertexAttribs3dvNV_remap_index 385
#define VertexAttribs3fvNV_remap_index 386
#define VertexAttribs3svNV_remap_index 387
#define VertexAttribs4dvNV_remap_index 388
#define VertexAttribs4fvNV_remap_index 389
#define VertexAttribs4svNV_remap_index 390
#define VertexAttribs4ubvNV_remap_index 391
#define GetTexBumpParameterfvATI_remap_index 392
#define GetTexBumpParameterivATI_remap_index 393
#define TexBumpParameterfvATI_remap_index 394
#define TexBumpParameterivATI_remap_index 395
#define AlphaFragmentOp1ATI_remap_index 396
#define AlphaFragmentOp2ATI_remap_index 397
#define AlphaFragmentOp3ATI_remap_index 398
#define BeginFragmentShaderATI_remap_index 399
#define BindFragmentShaderATI_remap_index 400
#define ColorFragmentOp1ATI_remap_index 401
#define ColorFragmentOp2ATI_remap_index 402
#define ColorFragmentOp3ATI_remap_index 403
#define DeleteFragmentShaderATI_remap_index 404
#define EndFragmentShaderATI_remap_index 405
#define GenFragmentShadersATI_remap_index 406
#define PassTexCoordATI_remap_index 407
#define SampleMapATI_remap_index 408
#define SetFragmentShaderConstantATI_remap_index 409
#define PointParameteriNV_remap_index 410
#define PointParameterivNV_remap_index 411
#define ActiveStencilFaceEXT_remap_index 412
#define BindVertexArrayAPPLE_remap_index 413
#define DeleteVertexArraysAPPLE_remap_index 414
#define GenVertexArraysAPPLE_remap_index 415
#define IsVertexArrayAPPLE_remap_index 416
#define GetProgramNamedParameterdvNV_remap_index 417
#define GetProgramNamedParameterfvNV_remap_index 418
#define ProgramNamedParameter4dNV_remap_index 419
#define ProgramNamedParameter4dvNV_remap_index 420
#define ProgramNamedParameter4fNV_remap_index 421
#define ProgramNamedParameter4fvNV_remap_index 422
#define PrimitiveRestartIndexNV_remap_index 423
#define PrimitiveRestartNV_remap_index 424
#define DepthBoundsEXT_remap_index 425
#define BlendEquationSeparateEXT_remap_index 426
#define BindFramebufferEXT_remap_index 427
#define BindRenderbufferEXT_remap_index 428
#define CheckFramebufferStatusEXT_remap_index 429
#define DeleteFramebuffersEXT_remap_index 430
#define DeleteRenderbuffersEXT_remap_index 431
#define FramebufferRenderbufferEXT_remap_index 432
#define FramebufferTexture1DEXT_remap_index 433
#define FramebufferTexture2DEXT_remap_index 434
#define FramebufferTexture3DEXT_remap_index 435
#define GenFramebuffersEXT_remap_index 436
#define GenRenderbuffersEXT_remap_index 437
#define GenerateMipmapEXT_remap_index 438
#define GetFramebufferAttachmentParameterivEXT_remap_index 439
#define GetRenderbufferParameterivEXT_remap_index 440
#define IsFramebufferEXT_remap_index 441
#define IsRenderbufferEXT_remap_index 442
#define RenderbufferStorageEXT_remap_index 443
#define BlitFramebufferEXT_remap_index 444
#define BufferParameteriAPPLE_remap_index 445
#define FlushMappedBufferRangeAPPLE_remap_index 446
#define BindFragDataLocationEXT_remap_index 447
#define GetFragDataLocationEXT_remap_index 448
#define GetUniformuivEXT_remap_index 449
#define GetVertexAttribIivEXT_remap_index 450
#define GetVertexAttribIuivEXT_remap_index 451
#define Uniform1uiEXT_remap_index 452
#define Uniform1uivEXT_remap_index 453
#define Uniform2uiEXT_remap_index 454
#define Uniform2uivEXT_remap_index 455
#define Uniform3uiEXT_remap_index 456
#define Uniform3uivEXT_remap_index 457
#define Uniform4uiEXT_remap_index 458
#define Uniform4uivEXT_remap_index 459
#define VertexAttribI1iEXT_remap_index 460
#define VertexAttribI1ivEXT_remap_index 461
#define VertexAttribI1uiEXT_remap_index 462
#define VertexAttribI1uivEXT_remap_index 463
#define VertexAttribI2iEXT_remap_index 464
#define VertexAttribI2ivEXT_remap_index 465
#define VertexAttribI2uiEXT_remap_index 466
#define VertexAttribI2uivEXT_remap_index 467
#define VertexAttribI3iEXT_remap_index 468
#define VertexAttribI3ivEXT_remap_index 469
#define VertexAttribI3uiEXT_remap_index 470
#define VertexAttribI3uivEXT_remap_index 471
#define VertexAttribI4bvEXT_remap_index 472
#define VertexAttribI4iEXT_remap_index 473
#define VertexAttribI4ivEXT_remap_index 474
#define VertexAttribI4svEXT_remap_index 475
#define VertexAttribI4ubvEXT_remap_index 476
#define VertexAttribI4uiEXT_remap_index 477
#define VertexAttribI4uivEXT_remap_index 478
#define VertexAttribI4usvEXT_remap_index 479
#define VertexAttribIPointerEXT_remap_index 480
#define FramebufferTextureLayerEXT_remap_index 481
#define ColorMaskIndexedEXT_remap_index 482
#define DisableIndexedEXT_remap_index 483
#define EnableIndexedEXT_remap_index 484
#define GetBooleanIndexedvEXT_remap_index 485
#define GetIntegerIndexedvEXT_remap_index 486
#define IsEnabledIndexedEXT_remap_index 487
#define ClearColorIiEXT_remap_index 488
#define ClearColorIuiEXT_remap_index 489
#define GetTexParameterIivEXT_remap_index 490
#define GetTexParameterIuivEXT_remap_index 491
#define TexParameterIivEXT_remap_index 492
#define TexParameterIuivEXT_remap_index 493
#define BeginConditionalRenderNV_remap_index 494
#define EndConditionalRenderNV_remap_index 495
#define BeginTransformFeedbackEXT_remap_index 496
#define BindBufferBaseEXT_remap_index 497
#define BindBufferOffsetEXT_remap_index 498
#define BindBufferRangeEXT_remap_index 499
#define EndTransformFeedbackEXT_remap_index 500
#define GetTransformFeedbackVaryingEXT_remap_index 501
#define TransformFeedbackVaryingsEXT_remap_index 502
#define ProvokingVertexEXT_remap_index 503
#define GetTexParameterPointervAPPLE_remap_index 504
#define TextureRangeAPPLE_remap_index 505
#define GetObjectParameterivAPPLE_remap_index 506
#define ObjectPurgeableAPPLE_remap_index 507
#define ObjectUnpurgeableAPPLE_remap_index 508
#define ActiveProgramEXT_remap_index 509
#define CreateShaderProgramEXT_remap_index 510
#define UseShaderProgramEXT_remap_index 511
#define TextureBarrierNV_remap_index 512
#define StencilFuncSeparateATI_remap_index 513
#define ProgramEnvParameters4fvEXT_remap_index 514
#define ProgramLocalParameters4fvEXT_remap_index 515
#define GetQueryObjecti64vEXT_remap_index 516
#define GetQueryObjectui64vEXT_remap_index 517
#define EGLImageTargetRenderbufferStorageOES_remap_index 518
#define EGLImageTargetTexture2DOES_remap_index 519

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
#define _gloffset_ClampColor driDispatchRemapTable[ClampColor_remap_index]
#define _gloffset_ClearBufferfi driDispatchRemapTable[ClearBufferfi_remap_index]
#define _gloffset_ClearBufferfv driDispatchRemapTable[ClearBufferfv_remap_index]
#define _gloffset_ClearBufferiv driDispatchRemapTable[ClearBufferiv_remap_index]
#define _gloffset_ClearBufferuiv driDispatchRemapTable[ClearBufferuiv_remap_index]
#define _gloffset_GetStringi driDispatchRemapTable[GetStringi_remap_index]
#define _gloffset_TexBuffer driDispatchRemapTable[TexBuffer_remap_index]
#define _gloffset_FramebufferTexture driDispatchRemapTable[FramebufferTexture_remap_index]
#define _gloffset_GetBufferParameteri64v driDispatchRemapTable[GetBufferParameteri64v_remap_index]
#define _gloffset_GetInteger64i_v driDispatchRemapTable[GetInteger64i_v_remap_index]
#define _gloffset_VertexAttribDivisor driDispatchRemapTable[VertexAttribDivisor_remap_index]
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
#define _gloffset_ClampColorARB driDispatchRemapTable[ClampColorARB_remap_index]
#define _gloffset_DrawArraysInstancedARB driDispatchRemapTable[DrawArraysInstancedARB_remap_index]
#define _gloffset_DrawElementsInstancedARB driDispatchRemapTable[DrawElementsInstancedARB_remap_index]
#define _gloffset_RenderbufferStorageMultisample driDispatchRemapTable[RenderbufferStorageMultisample_remap_index]
#define _gloffset_FramebufferTextureARB driDispatchRemapTable[FramebufferTextureARB_remap_index]
#define _gloffset_FramebufferTextureFaceARB driDispatchRemapTable[FramebufferTextureFaceARB_remap_index]
#define _gloffset_ProgramParameteriARB driDispatchRemapTable[ProgramParameteriARB_remap_index]
#define _gloffset_VertexAttribDivisorARB driDispatchRemapTable[VertexAttribDivisorARB_remap_index]
#define _gloffset_FlushMappedBufferRange driDispatchRemapTable[FlushMappedBufferRange_remap_index]
#define _gloffset_MapBufferRange driDispatchRemapTable[MapBufferRange_remap_index]
#define _gloffset_TexBufferARB driDispatchRemapTable[TexBufferARB_remap_index]
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
#define _gloffset_BlendEquationSeparateiARB driDispatchRemapTable[BlendEquationSeparateiARB_remap_index]
#define _gloffset_BlendEquationiARB driDispatchRemapTable[BlendEquationiARB_remap_index]
#define _gloffset_BlendFuncSeparateiARB driDispatchRemapTable[BlendFuncSeparateiARB_remap_index]
#define _gloffset_BlendFunciARB driDispatchRemapTable[BlendFunciARB_remap_index]
#define _gloffset_BindSampler driDispatchRemapTable[BindSampler_remap_index]
#define _gloffset_DeleteSamplers driDispatchRemapTable[DeleteSamplers_remap_index]
#define _gloffset_GenSamplers driDispatchRemapTable[GenSamplers_remap_index]
#define _gloffset_GetSamplerParameterIiv driDispatchRemapTable[GetSamplerParameterIiv_remap_index]
#define _gloffset_GetSamplerParameterIuiv driDispatchRemapTable[GetSamplerParameterIuiv_remap_index]
#define _gloffset_GetSamplerParameterfv driDispatchRemapTable[GetSamplerParameterfv_remap_index]
#define _gloffset_GetSamplerParameteriv driDispatchRemapTable[GetSamplerParameteriv_remap_index]
#define _gloffset_IsSampler driDispatchRemapTable[IsSampler_remap_index]
#define _gloffset_SamplerParameterIiv driDispatchRemapTable[SamplerParameterIiv_remap_index]
#define _gloffset_SamplerParameterIuiv driDispatchRemapTable[SamplerParameterIuiv_remap_index]
#define _gloffset_SamplerParameterf driDispatchRemapTable[SamplerParameterf_remap_index]
#define _gloffset_SamplerParameterfv driDispatchRemapTable[SamplerParameterfv_remap_index]
#define _gloffset_SamplerParameteri driDispatchRemapTable[SamplerParameteri_remap_index]
#define _gloffset_SamplerParameteriv driDispatchRemapTable[SamplerParameteriv_remap_index]
#define _gloffset_BindTransformFeedback driDispatchRemapTable[BindTransformFeedback_remap_index]
#define _gloffset_DeleteTransformFeedbacks driDispatchRemapTable[DeleteTransformFeedbacks_remap_index]
#define _gloffset_DrawTransformFeedback driDispatchRemapTable[DrawTransformFeedback_remap_index]
#define _gloffset_GenTransformFeedbacks driDispatchRemapTable[GenTransformFeedbacks_remap_index]
#define _gloffset_IsTransformFeedback driDispatchRemapTable[IsTransformFeedback_remap_index]
#define _gloffset_PauseTransformFeedback driDispatchRemapTable[PauseTransformFeedback_remap_index]
#define _gloffset_ResumeTransformFeedback driDispatchRemapTable[ResumeTransformFeedback_remap_index]
#define _gloffset_ClearDepthf driDispatchRemapTable[ClearDepthf_remap_index]
#define _gloffset_DepthRangef driDispatchRemapTable[DepthRangef_remap_index]
#define _gloffset_GetShaderPrecisionFormat driDispatchRemapTable[GetShaderPrecisionFormat_remap_index]
#define _gloffset_ReleaseShaderCompiler driDispatchRemapTable[ReleaseShaderCompiler_remap_index]
#define _gloffset_ShaderBinary driDispatchRemapTable[ShaderBinary_remap_index]
#define _gloffset_GetGraphicsResetStatusARB driDispatchRemapTable[GetGraphicsResetStatusARB_remap_index]
#define _gloffset_GetnColorTableARB driDispatchRemapTable[GetnColorTableARB_remap_index]
#define _gloffset_GetnCompressedTexImageARB driDispatchRemapTable[GetnCompressedTexImageARB_remap_index]
#define _gloffset_GetnConvolutionFilterARB driDispatchRemapTable[GetnConvolutionFilterARB_remap_index]
#define _gloffset_GetnHistogramARB driDispatchRemapTable[GetnHistogramARB_remap_index]
#define _gloffset_GetnMapdvARB driDispatchRemapTable[GetnMapdvARB_remap_index]
#define _gloffset_GetnMapfvARB driDispatchRemapTable[GetnMapfvARB_remap_index]
#define _gloffset_GetnMapivARB driDispatchRemapTable[GetnMapivARB_remap_index]
#define _gloffset_GetnMinmaxARB driDispatchRemapTable[GetnMinmaxARB_remap_index]
#define _gloffset_GetnPixelMapfvARB driDispatchRemapTable[GetnPixelMapfvARB_remap_index]
#define _gloffset_GetnPixelMapuivARB driDispatchRemapTable[GetnPixelMapuivARB_remap_index]
#define _gloffset_GetnPixelMapusvARB driDispatchRemapTable[GetnPixelMapusvARB_remap_index]
#define _gloffset_GetnPolygonStippleARB driDispatchRemapTable[GetnPolygonStippleARB_remap_index]
#define _gloffset_GetnSeparableFilterARB driDispatchRemapTable[GetnSeparableFilterARB_remap_index]
#define _gloffset_GetnTexImageARB driDispatchRemapTable[GetnTexImageARB_remap_index]
#define _gloffset_GetnUniformdvARB driDispatchRemapTable[GetnUniformdvARB_remap_index]
#define _gloffset_GetnUniformfvARB driDispatchRemapTable[GetnUniformfvARB_remap_index]
#define _gloffset_GetnUniformivARB driDispatchRemapTable[GetnUniformivARB_remap_index]
#define _gloffset_GetnUniformuivARB driDispatchRemapTable[GetnUniformuivARB_remap_index]
#define _gloffset_ReadnPixelsARB driDispatchRemapTable[ReadnPixelsARB_remap_index]
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
#define _gloffset_TextureBarrierNV driDispatchRemapTable[TextureBarrierNV_remap_index]
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
static void INLINE SET_NewList(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_NewList, fn);
}

#define CALL_EndList(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndList, parameters)
#define GET_EndList(disp) GET_by_offset(disp, _gloffset_EndList)
static void INLINE SET_EndList(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndList, fn);
}

#define CALL_CallList(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_CallList, parameters)
#define GET_CallList(disp) GET_by_offset(disp, _gloffset_CallList)
static void INLINE SET_CallList(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_CallList, fn);
}

#define CALL_CallLists(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLenum, const GLvoid *)), _gloffset_CallLists, parameters)
#define GET_CallLists(disp) GET_by_offset(disp, _gloffset_CallLists)
static void INLINE SET_CallLists(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CallLists, fn);
}

#define CALL_DeleteLists(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei)), _gloffset_DeleteLists, parameters)
#define GET_DeleteLists(disp) GET_by_offset(disp, _gloffset_DeleteLists)
static void INLINE SET_DeleteLists(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei)) {
   SET_by_offset(disp, _gloffset_DeleteLists, fn);
}

#define CALL_GenLists(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLsizei)), _gloffset_GenLists, parameters)
#define GET_GenLists(disp) GET_by_offset(disp, _gloffset_GenLists)
static void INLINE SET_GenLists(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLsizei)) {
   SET_by_offset(disp, _gloffset_GenLists, fn);
}

#define CALL_ListBase(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_ListBase, parameters)
#define GET_ListBase(disp) GET_by_offset(disp, _gloffset_ListBase)
static void INLINE SET_ListBase(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_ListBase, fn);
}

#define CALL_Begin(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_Begin, parameters)
#define GET_Begin(disp) GET_by_offset(disp, _gloffset_Begin)
static void INLINE SET_Begin(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_Begin, fn);
}

#define CALL_Bitmap(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *)), _gloffset_Bitmap, parameters)
#define GET_Bitmap(disp) GET_by_offset(disp, _gloffset_Bitmap)
static void INLINE SET_Bitmap(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Bitmap, fn);
}

#define CALL_Color3b(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte)), _gloffset_Color3b, parameters)
#define GET_Color3b(disp) GET_by_offset(disp, _gloffset_Color3b)
static void INLINE SET_Color3b(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_Color3b, fn);
}

#define CALL_Color3bv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_Color3bv, parameters)
#define GET_Color3bv(disp) GET_by_offset(disp, _gloffset_Color3bv)
static void INLINE SET_Color3bv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_Color3bv, fn);
}

#define CALL_Color3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Color3d, parameters)
#define GET_Color3d(disp) GET_by_offset(disp, _gloffset_Color3d)
static void INLINE SET_Color3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Color3d, fn);
}

#define CALL_Color3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Color3dv, parameters)
#define GET_Color3dv(disp) GET_by_offset(disp, _gloffset_Color3dv)
static void INLINE SET_Color3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Color3dv, fn);
}

#define CALL_Color3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Color3f, parameters)
#define GET_Color3f(disp) GET_by_offset(disp, _gloffset_Color3f)
static void INLINE SET_Color3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Color3f, fn);
}

#define CALL_Color3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Color3fv, parameters)
#define GET_Color3fv(disp) GET_by_offset(disp, _gloffset_Color3fv)
static void INLINE SET_Color3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Color3fv, fn);
}

#define CALL_Color3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Color3i, parameters)
#define GET_Color3i(disp) GET_by_offset(disp, _gloffset_Color3i)
static void INLINE SET_Color3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Color3i, fn);
}

#define CALL_Color3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Color3iv, parameters)
#define GET_Color3iv(disp) GET_by_offset(disp, _gloffset_Color3iv)
static void INLINE SET_Color3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Color3iv, fn);
}

#define CALL_Color3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_Color3s, parameters)
#define GET_Color3s(disp) GET_by_offset(disp, _gloffset_Color3s)
static void INLINE SET_Color3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Color3s, fn);
}

#define CALL_Color3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Color3sv, parameters)
#define GET_Color3sv(disp) GET_by_offset(disp, _gloffset_Color3sv)
static void INLINE SET_Color3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Color3sv, fn);
}

#define CALL_Color3ub(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte, GLubyte, GLubyte)), _gloffset_Color3ub, parameters)
#define GET_Color3ub(disp) GET_by_offset(disp, _gloffset_Color3ub)
static void INLINE SET_Color3ub(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_Color3ub, fn);
}

#define CALL_Color3ubv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_Color3ubv, parameters)
#define GET_Color3ubv(disp) GET_by_offset(disp, _gloffset_Color3ubv)
static void INLINE SET_Color3ubv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Color3ubv, fn);
}

#define CALL_Color3ui(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint)), _gloffset_Color3ui, parameters)
#define GET_Color3ui(disp) GET_by_offset(disp, _gloffset_Color3ui)
static void INLINE SET_Color3ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Color3ui, fn);
}

#define CALL_Color3uiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLuint *)), _gloffset_Color3uiv, parameters)
#define GET_Color3uiv(disp) GET_by_offset(disp, _gloffset_Color3uiv)
static void INLINE SET_Color3uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLuint *)) {
   SET_by_offset(disp, _gloffset_Color3uiv, fn);
}

#define CALL_Color3us(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLushort, GLushort, GLushort)), _gloffset_Color3us, parameters)
#define GET_Color3us(disp) GET_by_offset(disp, _gloffset_Color3us)
static void INLINE SET_Color3us(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLushort, GLushort, GLushort)) {
   SET_by_offset(disp, _gloffset_Color3us, fn);
}

#define CALL_Color3usv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLushort *)), _gloffset_Color3usv, parameters)
#define GET_Color3usv(disp) GET_by_offset(disp, _gloffset_Color3usv)
static void INLINE SET_Color3usv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLushort *)) {
   SET_by_offset(disp, _gloffset_Color3usv, fn);
}

#define CALL_Color4b(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte, GLbyte)), _gloffset_Color4b, parameters)
#define GET_Color4b(disp) GET_by_offset(disp, _gloffset_Color4b)
static void INLINE SET_Color4b(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_Color4b, fn);
}

#define CALL_Color4bv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_Color4bv, parameters)
#define GET_Color4bv(disp) GET_by_offset(disp, _gloffset_Color4bv)
static void INLINE SET_Color4bv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_Color4bv, fn);
}

#define CALL_Color4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Color4d, parameters)
#define GET_Color4d(disp) GET_by_offset(disp, _gloffset_Color4d)
static void INLINE SET_Color4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Color4d, fn);
}

#define CALL_Color4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Color4dv, parameters)
#define GET_Color4dv(disp) GET_by_offset(disp, _gloffset_Color4dv)
static void INLINE SET_Color4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Color4dv, fn);
}

#define CALL_Color4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Color4f, parameters)
#define GET_Color4f(disp) GET_by_offset(disp, _gloffset_Color4f)
static void INLINE SET_Color4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Color4f, fn);
}

#define CALL_Color4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Color4fv, parameters)
#define GET_Color4fv(disp) GET_by_offset(disp, _gloffset_Color4fv)
static void INLINE SET_Color4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Color4fv, fn);
}

#define CALL_Color4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Color4i, parameters)
#define GET_Color4i(disp) GET_by_offset(disp, _gloffset_Color4i)
static void INLINE SET_Color4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Color4i, fn);
}

#define CALL_Color4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Color4iv, parameters)
#define GET_Color4iv(disp) GET_by_offset(disp, _gloffset_Color4iv)
static void INLINE SET_Color4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Color4iv, fn);
}

#define CALL_Color4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_Color4s, parameters)
#define GET_Color4s(disp) GET_by_offset(disp, _gloffset_Color4s)
static void INLINE SET_Color4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Color4s, fn);
}

#define CALL_Color4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Color4sv, parameters)
#define GET_Color4sv(disp) GET_by_offset(disp, _gloffset_Color4sv)
static void INLINE SET_Color4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Color4sv, fn);
}

#define CALL_Color4ub(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte, GLubyte, GLubyte, GLubyte)), _gloffset_Color4ub, parameters)
#define GET_Color4ub(disp) GET_by_offset(disp, _gloffset_Color4ub)
static void INLINE SET_Color4ub(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte, GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_Color4ub, fn);
}

#define CALL_Color4ubv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_Color4ubv, parameters)
#define GET_Color4ubv(disp) GET_by_offset(disp, _gloffset_Color4ubv)
static void INLINE SET_Color4ubv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Color4ubv, fn);
}

#define CALL_Color4ui(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint)), _gloffset_Color4ui, parameters)
#define GET_Color4ui(disp) GET_by_offset(disp, _gloffset_Color4ui)
static void INLINE SET_Color4ui(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Color4ui, fn);
}

#define CALL_Color4uiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLuint *)), _gloffset_Color4uiv, parameters)
#define GET_Color4uiv(disp) GET_by_offset(disp, _gloffset_Color4uiv)
static void INLINE SET_Color4uiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLuint *)) {
   SET_by_offset(disp, _gloffset_Color4uiv, fn);
}

#define CALL_Color4us(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLushort, GLushort, GLushort, GLushort)), _gloffset_Color4us, parameters)
#define GET_Color4us(disp) GET_by_offset(disp, _gloffset_Color4us)
static void INLINE SET_Color4us(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLushort, GLushort, GLushort, GLushort)) {
   SET_by_offset(disp, _gloffset_Color4us, fn);
}

#define CALL_Color4usv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLushort *)), _gloffset_Color4usv, parameters)
#define GET_Color4usv(disp) GET_by_offset(disp, _gloffset_Color4usv)
static void INLINE SET_Color4usv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLushort *)) {
   SET_by_offset(disp, _gloffset_Color4usv, fn);
}

#define CALL_EdgeFlag(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLboolean)), _gloffset_EdgeFlag, parameters)
#define GET_EdgeFlag(disp) GET_by_offset(disp, _gloffset_EdgeFlag)
static void INLINE SET_EdgeFlag(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLboolean)) {
   SET_by_offset(disp, _gloffset_EdgeFlag, fn);
}

#define CALL_EdgeFlagv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLboolean *)), _gloffset_EdgeFlagv, parameters)
#define GET_EdgeFlagv(disp) GET_by_offset(disp, _gloffset_EdgeFlagv)
static void INLINE SET_EdgeFlagv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLboolean *)) {
   SET_by_offset(disp, _gloffset_EdgeFlagv, fn);
}

#define CALL_End(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_End, parameters)
#define GET_End(disp) GET_by_offset(disp, _gloffset_End)
static void INLINE SET_End(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_End, fn);
}

#define CALL_Indexd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_Indexd, parameters)
#define GET_Indexd(disp) GET_by_offset(disp, _gloffset_Indexd)
static void INLINE SET_Indexd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_Indexd, fn);
}

#define CALL_Indexdv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Indexdv, parameters)
#define GET_Indexdv(disp) GET_by_offset(disp, _gloffset_Indexdv)
static void INLINE SET_Indexdv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Indexdv, fn);
}

#define CALL_Indexf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_Indexf, parameters)
#define GET_Indexf(disp) GET_by_offset(disp, _gloffset_Indexf)
static void INLINE SET_Indexf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_Indexf, fn);
}

#define CALL_Indexfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Indexfv, parameters)
#define GET_Indexfv(disp) GET_by_offset(disp, _gloffset_Indexfv)
static void INLINE SET_Indexfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Indexfv, fn);
}

#define CALL_Indexi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_Indexi, parameters)
#define GET_Indexi(disp) GET_by_offset(disp, _gloffset_Indexi)
static void INLINE SET_Indexi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_Indexi, fn);
}

#define CALL_Indexiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Indexiv, parameters)
#define GET_Indexiv(disp) GET_by_offset(disp, _gloffset_Indexiv)
static void INLINE SET_Indexiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Indexiv, fn);
}

#define CALL_Indexs(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort)), _gloffset_Indexs, parameters)
#define GET_Indexs(disp) GET_by_offset(disp, _gloffset_Indexs)
static void INLINE SET_Indexs(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort)) {
   SET_by_offset(disp, _gloffset_Indexs, fn);
}

#define CALL_Indexsv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Indexsv, parameters)
#define GET_Indexsv(disp) GET_by_offset(disp, _gloffset_Indexsv)
static void INLINE SET_Indexsv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Indexsv, fn);
}

#define CALL_Normal3b(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte)), _gloffset_Normal3b, parameters)
#define GET_Normal3b(disp) GET_by_offset(disp, _gloffset_Normal3b)
static void INLINE SET_Normal3b(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_Normal3b, fn);
}

#define CALL_Normal3bv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_Normal3bv, parameters)
#define GET_Normal3bv(disp) GET_by_offset(disp, _gloffset_Normal3bv)
static void INLINE SET_Normal3bv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_Normal3bv, fn);
}

#define CALL_Normal3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Normal3d, parameters)
#define GET_Normal3d(disp) GET_by_offset(disp, _gloffset_Normal3d)
static void INLINE SET_Normal3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Normal3d, fn);
}

#define CALL_Normal3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Normal3dv, parameters)
#define GET_Normal3dv(disp) GET_by_offset(disp, _gloffset_Normal3dv)
static void INLINE SET_Normal3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Normal3dv, fn);
}

#define CALL_Normal3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Normal3f, parameters)
#define GET_Normal3f(disp) GET_by_offset(disp, _gloffset_Normal3f)
static void INLINE SET_Normal3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Normal3f, fn);
}

#define CALL_Normal3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Normal3fv, parameters)
#define GET_Normal3fv(disp) GET_by_offset(disp, _gloffset_Normal3fv)
static void INLINE SET_Normal3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Normal3fv, fn);
}

#define CALL_Normal3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Normal3i, parameters)
#define GET_Normal3i(disp) GET_by_offset(disp, _gloffset_Normal3i)
static void INLINE SET_Normal3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Normal3i, fn);
}

#define CALL_Normal3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Normal3iv, parameters)
#define GET_Normal3iv(disp) GET_by_offset(disp, _gloffset_Normal3iv)
static void INLINE SET_Normal3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Normal3iv, fn);
}

#define CALL_Normal3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_Normal3s, parameters)
#define GET_Normal3s(disp) GET_by_offset(disp, _gloffset_Normal3s)
static void INLINE SET_Normal3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Normal3s, fn);
}

#define CALL_Normal3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Normal3sv, parameters)
#define GET_Normal3sv(disp) GET_by_offset(disp, _gloffset_Normal3sv)
static void INLINE SET_Normal3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Normal3sv, fn);
}

#define CALL_RasterPos2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_RasterPos2d, parameters)
#define GET_RasterPos2d(disp) GET_by_offset(disp, _gloffset_RasterPos2d)
static void INLINE SET_RasterPos2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_RasterPos2d, fn);
}

#define CALL_RasterPos2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_RasterPos2dv, parameters)
#define GET_RasterPos2dv(disp) GET_by_offset(disp, _gloffset_RasterPos2dv)
static void INLINE SET_RasterPos2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_RasterPos2dv, fn);
}

#define CALL_RasterPos2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_RasterPos2f, parameters)
#define GET_RasterPos2f(disp) GET_by_offset(disp, _gloffset_RasterPos2f)
static void INLINE SET_RasterPos2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_RasterPos2f, fn);
}

#define CALL_RasterPos2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_RasterPos2fv, parameters)
#define GET_RasterPos2fv(disp) GET_by_offset(disp, _gloffset_RasterPos2fv)
static void INLINE SET_RasterPos2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_RasterPos2fv, fn);
}

#define CALL_RasterPos2i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_RasterPos2i, parameters)
#define GET_RasterPos2i(disp) GET_by_offset(disp, _gloffset_RasterPos2i)
static void INLINE SET_RasterPos2i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_RasterPos2i, fn);
}

#define CALL_RasterPos2iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_RasterPos2iv, parameters)
#define GET_RasterPos2iv(disp) GET_by_offset(disp, _gloffset_RasterPos2iv)
static void INLINE SET_RasterPos2iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_RasterPos2iv, fn);
}

#define CALL_RasterPos2s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_RasterPos2s, parameters)
#define GET_RasterPos2s(disp) GET_by_offset(disp, _gloffset_RasterPos2s)
static void INLINE SET_RasterPos2s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_RasterPos2s, fn);
}

#define CALL_RasterPos2sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_RasterPos2sv, parameters)
#define GET_RasterPos2sv(disp) GET_by_offset(disp, _gloffset_RasterPos2sv)
static void INLINE SET_RasterPos2sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_RasterPos2sv, fn);
}

#define CALL_RasterPos3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_RasterPos3d, parameters)
#define GET_RasterPos3d(disp) GET_by_offset(disp, _gloffset_RasterPos3d)
static void INLINE SET_RasterPos3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_RasterPos3d, fn);
}

#define CALL_RasterPos3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_RasterPos3dv, parameters)
#define GET_RasterPos3dv(disp) GET_by_offset(disp, _gloffset_RasterPos3dv)
static void INLINE SET_RasterPos3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_RasterPos3dv, fn);
}

#define CALL_RasterPos3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_RasterPos3f, parameters)
#define GET_RasterPos3f(disp) GET_by_offset(disp, _gloffset_RasterPos3f)
static void INLINE SET_RasterPos3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_RasterPos3f, fn);
}

#define CALL_RasterPos3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_RasterPos3fv, parameters)
#define GET_RasterPos3fv(disp) GET_by_offset(disp, _gloffset_RasterPos3fv)
static void INLINE SET_RasterPos3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_RasterPos3fv, fn);
}

#define CALL_RasterPos3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_RasterPos3i, parameters)
#define GET_RasterPos3i(disp) GET_by_offset(disp, _gloffset_RasterPos3i)
static void INLINE SET_RasterPos3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_RasterPos3i, fn);
}

#define CALL_RasterPos3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_RasterPos3iv, parameters)
#define GET_RasterPos3iv(disp) GET_by_offset(disp, _gloffset_RasterPos3iv)
static void INLINE SET_RasterPos3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_RasterPos3iv, fn);
}

#define CALL_RasterPos3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_RasterPos3s, parameters)
#define GET_RasterPos3s(disp) GET_by_offset(disp, _gloffset_RasterPos3s)
static void INLINE SET_RasterPos3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_RasterPos3s, fn);
}

#define CALL_RasterPos3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_RasterPos3sv, parameters)
#define GET_RasterPos3sv(disp) GET_by_offset(disp, _gloffset_RasterPos3sv)
static void INLINE SET_RasterPos3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_RasterPos3sv, fn);
}

#define CALL_RasterPos4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_RasterPos4d, parameters)
#define GET_RasterPos4d(disp) GET_by_offset(disp, _gloffset_RasterPos4d)
static void INLINE SET_RasterPos4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_RasterPos4d, fn);
}

#define CALL_RasterPos4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_RasterPos4dv, parameters)
#define GET_RasterPos4dv(disp) GET_by_offset(disp, _gloffset_RasterPos4dv)
static void INLINE SET_RasterPos4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_RasterPos4dv, fn);
}

#define CALL_RasterPos4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_RasterPos4f, parameters)
#define GET_RasterPos4f(disp) GET_by_offset(disp, _gloffset_RasterPos4f)
static void INLINE SET_RasterPos4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_RasterPos4f, fn);
}

#define CALL_RasterPos4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_RasterPos4fv, parameters)
#define GET_RasterPos4fv(disp) GET_by_offset(disp, _gloffset_RasterPos4fv)
static void INLINE SET_RasterPos4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_RasterPos4fv, fn);
}

#define CALL_RasterPos4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_RasterPos4i, parameters)
#define GET_RasterPos4i(disp) GET_by_offset(disp, _gloffset_RasterPos4i)
static void INLINE SET_RasterPos4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_RasterPos4i, fn);
}

#define CALL_RasterPos4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_RasterPos4iv, parameters)
#define GET_RasterPos4iv(disp) GET_by_offset(disp, _gloffset_RasterPos4iv)
static void INLINE SET_RasterPos4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_RasterPos4iv, fn);
}

#define CALL_RasterPos4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_RasterPos4s, parameters)
#define GET_RasterPos4s(disp) GET_by_offset(disp, _gloffset_RasterPos4s)
static void INLINE SET_RasterPos4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_RasterPos4s, fn);
}

#define CALL_RasterPos4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_RasterPos4sv, parameters)
#define GET_RasterPos4sv(disp) GET_by_offset(disp, _gloffset_RasterPos4sv)
static void INLINE SET_RasterPos4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_RasterPos4sv, fn);
}

#define CALL_Rectd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Rectd, parameters)
#define GET_Rectd(disp) GET_by_offset(disp, _gloffset_Rectd)
static void INLINE SET_Rectd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Rectd, fn);
}

#define CALL_Rectdv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *, const GLdouble *)), _gloffset_Rectdv, parameters)
#define GET_Rectdv(disp) GET_by_offset(disp, _gloffset_Rectdv)
static void INLINE SET_Rectdv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Rectdv, fn);
}

#define CALL_Rectf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Rectf, parameters)
#define GET_Rectf(disp) GET_by_offset(disp, _gloffset_Rectf)
static void INLINE SET_Rectf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Rectf, fn);
}

#define CALL_Rectfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *, const GLfloat *)), _gloffset_Rectfv, parameters)
#define GET_Rectfv(disp) GET_by_offset(disp, _gloffset_Rectfv)
static void INLINE SET_Rectfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Rectfv, fn);
}

#define CALL_Recti(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Recti, parameters)
#define GET_Recti(disp) GET_by_offset(disp, _gloffset_Recti)
static void INLINE SET_Recti(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Recti, fn);
}

#define CALL_Rectiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *, const GLint *)), _gloffset_Rectiv, parameters)
#define GET_Rectiv(disp) GET_by_offset(disp, _gloffset_Rectiv)
static void INLINE SET_Rectiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *, const GLint *)) {
   SET_by_offset(disp, _gloffset_Rectiv, fn);
}

#define CALL_Rects(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_Rects, parameters)
#define GET_Rects(disp) GET_by_offset(disp, _gloffset_Rects)
static void INLINE SET_Rects(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Rects, fn);
}

#define CALL_Rectsv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *, const GLshort *)), _gloffset_Rectsv, parameters)
#define GET_Rectsv(disp) GET_by_offset(disp, _gloffset_Rectsv)
static void INLINE SET_Rectsv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *, const GLshort *)) {
   SET_by_offset(disp, _gloffset_Rectsv, fn);
}

#define CALL_TexCoord1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_TexCoord1d, parameters)
#define GET_TexCoord1d(disp) GET_by_offset(disp, _gloffset_TexCoord1d)
static void INLINE SET_TexCoord1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord1d, fn);
}

#define CALL_TexCoord1dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord1dv, parameters)
#define GET_TexCoord1dv(disp) GET_by_offset(disp, _gloffset_TexCoord1dv)
static void INLINE SET_TexCoord1dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord1dv, fn);
}

#define CALL_TexCoord1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_TexCoord1f, parameters)
#define GET_TexCoord1f(disp) GET_by_offset(disp, _gloffset_TexCoord1f)
static void INLINE SET_TexCoord1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord1f, fn);
}

#define CALL_TexCoord1fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord1fv, parameters)
#define GET_TexCoord1fv(disp) GET_by_offset(disp, _gloffset_TexCoord1fv)
static void INLINE SET_TexCoord1fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord1fv, fn);
}

#define CALL_TexCoord1i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_TexCoord1i, parameters)
#define GET_TexCoord1i(disp) GET_by_offset(disp, _gloffset_TexCoord1i)
static void INLINE SET_TexCoord1i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord1i, fn);
}

#define CALL_TexCoord1iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord1iv, parameters)
#define GET_TexCoord1iv(disp) GET_by_offset(disp, _gloffset_TexCoord1iv)
static void INLINE SET_TexCoord1iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord1iv, fn);
}

#define CALL_TexCoord1s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort)), _gloffset_TexCoord1s, parameters)
#define GET_TexCoord1s(disp) GET_by_offset(disp, _gloffset_TexCoord1s)
static void INLINE SET_TexCoord1s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord1s, fn);
}

#define CALL_TexCoord1sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord1sv, parameters)
#define GET_TexCoord1sv(disp) GET_by_offset(disp, _gloffset_TexCoord1sv)
static void INLINE SET_TexCoord1sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord1sv, fn);
}

#define CALL_TexCoord2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_TexCoord2d, parameters)
#define GET_TexCoord2d(disp) GET_by_offset(disp, _gloffset_TexCoord2d)
static void INLINE SET_TexCoord2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord2d, fn);
}

#define CALL_TexCoord2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord2dv, parameters)
#define GET_TexCoord2dv(disp) GET_by_offset(disp, _gloffset_TexCoord2dv)
static void INLINE SET_TexCoord2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord2dv, fn);
}

#define CALL_TexCoord2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_TexCoord2f, parameters)
#define GET_TexCoord2f(disp) GET_by_offset(disp, _gloffset_TexCoord2f)
static void INLINE SET_TexCoord2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord2f, fn);
}

#define CALL_TexCoord2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord2fv, parameters)
#define GET_TexCoord2fv(disp) GET_by_offset(disp, _gloffset_TexCoord2fv)
static void INLINE SET_TexCoord2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord2fv, fn);
}

#define CALL_TexCoord2i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_TexCoord2i, parameters)
#define GET_TexCoord2i(disp) GET_by_offset(disp, _gloffset_TexCoord2i)
static void INLINE SET_TexCoord2i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord2i, fn);
}

#define CALL_TexCoord2iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord2iv, parameters)
#define GET_TexCoord2iv(disp) GET_by_offset(disp, _gloffset_TexCoord2iv)
static void INLINE SET_TexCoord2iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord2iv, fn);
}

#define CALL_TexCoord2s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_TexCoord2s, parameters)
#define GET_TexCoord2s(disp) GET_by_offset(disp, _gloffset_TexCoord2s)
static void INLINE SET_TexCoord2s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord2s, fn);
}

#define CALL_TexCoord2sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord2sv, parameters)
#define GET_TexCoord2sv(disp) GET_by_offset(disp, _gloffset_TexCoord2sv)
static void INLINE SET_TexCoord2sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord2sv, fn);
}

#define CALL_TexCoord3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_TexCoord3d, parameters)
#define GET_TexCoord3d(disp) GET_by_offset(disp, _gloffset_TexCoord3d)
static void INLINE SET_TexCoord3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord3d, fn);
}

#define CALL_TexCoord3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord3dv, parameters)
#define GET_TexCoord3dv(disp) GET_by_offset(disp, _gloffset_TexCoord3dv)
static void INLINE SET_TexCoord3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord3dv, fn);
}

#define CALL_TexCoord3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_TexCoord3f, parameters)
#define GET_TexCoord3f(disp) GET_by_offset(disp, _gloffset_TexCoord3f)
static void INLINE SET_TexCoord3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord3f, fn);
}

#define CALL_TexCoord3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord3fv, parameters)
#define GET_TexCoord3fv(disp) GET_by_offset(disp, _gloffset_TexCoord3fv)
static void INLINE SET_TexCoord3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord3fv, fn);
}

#define CALL_TexCoord3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_TexCoord3i, parameters)
#define GET_TexCoord3i(disp) GET_by_offset(disp, _gloffset_TexCoord3i)
static void INLINE SET_TexCoord3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord3i, fn);
}

#define CALL_TexCoord3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord3iv, parameters)
#define GET_TexCoord3iv(disp) GET_by_offset(disp, _gloffset_TexCoord3iv)
static void INLINE SET_TexCoord3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord3iv, fn);
}

#define CALL_TexCoord3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_TexCoord3s, parameters)
#define GET_TexCoord3s(disp) GET_by_offset(disp, _gloffset_TexCoord3s)
static void INLINE SET_TexCoord3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord3s, fn);
}

#define CALL_TexCoord3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord3sv, parameters)
#define GET_TexCoord3sv(disp) GET_by_offset(disp, _gloffset_TexCoord3sv)
static void INLINE SET_TexCoord3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord3sv, fn);
}

#define CALL_TexCoord4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_TexCoord4d, parameters)
#define GET_TexCoord4d(disp) GET_by_offset(disp, _gloffset_TexCoord4d)
static void INLINE SET_TexCoord4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexCoord4d, fn);
}

#define CALL_TexCoord4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_TexCoord4dv, parameters)
#define GET_TexCoord4dv(disp) GET_by_offset(disp, _gloffset_TexCoord4dv)
static void INLINE SET_TexCoord4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexCoord4dv, fn);
}

#define CALL_TexCoord4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_TexCoord4f, parameters)
#define GET_TexCoord4f(disp) GET_by_offset(disp, _gloffset_TexCoord4f)
static void INLINE SET_TexCoord4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexCoord4f, fn);
}

#define CALL_TexCoord4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_TexCoord4fv, parameters)
#define GET_TexCoord4fv(disp) GET_by_offset(disp, _gloffset_TexCoord4fv)
static void INLINE SET_TexCoord4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexCoord4fv, fn);
}

#define CALL_TexCoord4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_TexCoord4i, parameters)
#define GET_TexCoord4i(disp) GET_by_offset(disp, _gloffset_TexCoord4i)
static void INLINE SET_TexCoord4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_TexCoord4i, fn);
}

#define CALL_TexCoord4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_TexCoord4iv, parameters)
#define GET_TexCoord4iv(disp) GET_by_offset(disp, _gloffset_TexCoord4iv)
static void INLINE SET_TexCoord4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_TexCoord4iv, fn);
}

#define CALL_TexCoord4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_TexCoord4s, parameters)
#define GET_TexCoord4s(disp) GET_by_offset(disp, _gloffset_TexCoord4s)
static void INLINE SET_TexCoord4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_TexCoord4s, fn);
}

#define CALL_TexCoord4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_TexCoord4sv, parameters)
#define GET_TexCoord4sv(disp) GET_by_offset(disp, _gloffset_TexCoord4sv)
static void INLINE SET_TexCoord4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_TexCoord4sv, fn);
}

#define CALL_Vertex2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_Vertex2d, parameters)
#define GET_Vertex2d(disp) GET_by_offset(disp, _gloffset_Vertex2d)
static void INLINE SET_Vertex2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Vertex2d, fn);
}

#define CALL_Vertex2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Vertex2dv, parameters)
#define GET_Vertex2dv(disp) GET_by_offset(disp, _gloffset_Vertex2dv)
static void INLINE SET_Vertex2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Vertex2dv, fn);
}

#define CALL_Vertex2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_Vertex2f, parameters)
#define GET_Vertex2f(disp) GET_by_offset(disp, _gloffset_Vertex2f)
static void INLINE SET_Vertex2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Vertex2f, fn);
}

#define CALL_Vertex2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Vertex2fv, parameters)
#define GET_Vertex2fv(disp) GET_by_offset(disp, _gloffset_Vertex2fv)
static void INLINE SET_Vertex2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Vertex2fv, fn);
}

#define CALL_Vertex2i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_Vertex2i, parameters)
#define GET_Vertex2i(disp) GET_by_offset(disp, _gloffset_Vertex2i)
static void INLINE SET_Vertex2i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Vertex2i, fn);
}

#define CALL_Vertex2iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Vertex2iv, parameters)
#define GET_Vertex2iv(disp) GET_by_offset(disp, _gloffset_Vertex2iv)
static void INLINE SET_Vertex2iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Vertex2iv, fn);
}

#define CALL_Vertex2s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_Vertex2s, parameters)
#define GET_Vertex2s(disp) GET_by_offset(disp, _gloffset_Vertex2s)
static void INLINE SET_Vertex2s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Vertex2s, fn);
}

#define CALL_Vertex2sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Vertex2sv, parameters)
#define GET_Vertex2sv(disp) GET_by_offset(disp, _gloffset_Vertex2sv)
static void INLINE SET_Vertex2sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Vertex2sv, fn);
}

#define CALL_Vertex3d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Vertex3d, parameters)
#define GET_Vertex3d(disp) GET_by_offset(disp, _gloffset_Vertex3d)
static void INLINE SET_Vertex3d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Vertex3d, fn);
}

#define CALL_Vertex3dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Vertex3dv, parameters)
#define GET_Vertex3dv(disp) GET_by_offset(disp, _gloffset_Vertex3dv)
static void INLINE SET_Vertex3dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Vertex3dv, fn);
}

#define CALL_Vertex3f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Vertex3f, parameters)
#define GET_Vertex3f(disp) GET_by_offset(disp, _gloffset_Vertex3f)
static void INLINE SET_Vertex3f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Vertex3f, fn);
}

#define CALL_Vertex3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Vertex3fv, parameters)
#define GET_Vertex3fv(disp) GET_by_offset(disp, _gloffset_Vertex3fv)
static void INLINE SET_Vertex3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Vertex3fv, fn);
}

#define CALL_Vertex3i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Vertex3i, parameters)
#define GET_Vertex3i(disp) GET_by_offset(disp, _gloffset_Vertex3i)
static void INLINE SET_Vertex3i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Vertex3i, fn);
}

#define CALL_Vertex3iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Vertex3iv, parameters)
#define GET_Vertex3iv(disp) GET_by_offset(disp, _gloffset_Vertex3iv)
static void INLINE SET_Vertex3iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Vertex3iv, fn);
}

#define CALL_Vertex3s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_Vertex3s, parameters)
#define GET_Vertex3s(disp) GET_by_offset(disp, _gloffset_Vertex3s)
static void INLINE SET_Vertex3s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Vertex3s, fn);
}

#define CALL_Vertex3sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Vertex3sv, parameters)
#define GET_Vertex3sv(disp) GET_by_offset(disp, _gloffset_Vertex3sv)
static void INLINE SET_Vertex3sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Vertex3sv, fn);
}

#define CALL_Vertex4d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Vertex4d, parameters)
#define GET_Vertex4d(disp) GET_by_offset(disp, _gloffset_Vertex4d)
static void INLINE SET_Vertex4d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Vertex4d, fn);
}

#define CALL_Vertex4dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_Vertex4dv, parameters)
#define GET_Vertex4dv(disp) GET_by_offset(disp, _gloffset_Vertex4dv)
static void INLINE SET_Vertex4dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Vertex4dv, fn);
}

#define CALL_Vertex4f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Vertex4f, parameters)
#define GET_Vertex4f(disp) GET_by_offset(disp, _gloffset_Vertex4f)
static void INLINE SET_Vertex4f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Vertex4f, fn);
}

#define CALL_Vertex4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_Vertex4fv, parameters)
#define GET_Vertex4fv(disp) GET_by_offset(disp, _gloffset_Vertex4fv)
static void INLINE SET_Vertex4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Vertex4fv, fn);
}

#define CALL_Vertex4i(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Vertex4i, parameters)
#define GET_Vertex4i(disp) GET_by_offset(disp, _gloffset_Vertex4i)
static void INLINE SET_Vertex4i(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Vertex4i, fn);
}

#define CALL_Vertex4iv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_Vertex4iv, parameters)
#define GET_Vertex4iv(disp) GET_by_offset(disp, _gloffset_Vertex4iv)
static void INLINE SET_Vertex4iv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_Vertex4iv, fn);
}

#define CALL_Vertex4s(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_Vertex4s, parameters)
#define GET_Vertex4s(disp) GET_by_offset(disp, _gloffset_Vertex4s)
static void INLINE SET_Vertex4s(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_Vertex4s, fn);
}

#define CALL_Vertex4sv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_Vertex4sv, parameters)
#define GET_Vertex4sv(disp) GET_by_offset(disp, _gloffset_Vertex4sv)
static void INLINE SET_Vertex4sv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_Vertex4sv, fn);
}

#define CALL_ClipPlane(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_ClipPlane, parameters)
#define GET_ClipPlane(disp) GET_by_offset(disp, _gloffset_ClipPlane)
static void INLINE SET_ClipPlane(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ClipPlane, fn);
}

#define CALL_ColorMaterial(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_ColorMaterial, parameters)
#define GET_ColorMaterial(disp) GET_by_offset(disp, _gloffset_ColorMaterial)
static void INLINE SET_ColorMaterial(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_ColorMaterial, fn);
}

#define CALL_CullFace(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_CullFace, parameters)
#define GET_CullFace(disp) GET_by_offset(disp, _gloffset_CullFace)
static void INLINE SET_CullFace(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CullFace, fn);
}

#define CALL_Fogf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_Fogf, parameters)
#define GET_Fogf(disp) GET_by_offset(disp, _gloffset_Fogf)
static void INLINE SET_Fogf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Fogf, fn);
}

#define CALL_Fogfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_Fogfv, parameters)
#define GET_Fogfv(disp) GET_by_offset(disp, _gloffset_Fogfv)
static void INLINE SET_Fogfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Fogfv, fn);
}

#define CALL_Fogi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_Fogi, parameters)
#define GET_Fogi(disp) GET_by_offset(disp, _gloffset_Fogi)
static void INLINE SET_Fogi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_Fogi, fn);
}

#define CALL_Fogiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_Fogiv, parameters)
#define GET_Fogiv(disp) GET_by_offset(disp, _gloffset_Fogiv)
static void INLINE SET_Fogiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_Fogiv, fn);
}

#define CALL_FrontFace(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_FrontFace, parameters)
#define GET_FrontFace(disp) GET_by_offset(disp, _gloffset_FrontFace)
static void INLINE SET_FrontFace(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_FrontFace, fn);
}

#define CALL_Hint(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_Hint, parameters)
#define GET_Hint(disp) GET_by_offset(disp, _gloffset_Hint)
static void INLINE SET_Hint(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_Hint, fn);
}

#define CALL_Lightf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_Lightf, parameters)
#define GET_Lightf(disp) GET_by_offset(disp, _gloffset_Lightf)
static void INLINE SET_Lightf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Lightf, fn);
}

#define CALL_Lightfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_Lightfv, parameters)
#define GET_Lightfv(disp) GET_by_offset(disp, _gloffset_Lightfv)
static void INLINE SET_Lightfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Lightfv, fn);
}

#define CALL_Lighti(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_Lighti, parameters)
#define GET_Lighti(disp) GET_by_offset(disp, _gloffset_Lighti)
static void INLINE SET_Lighti(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_Lighti, fn);
}

#define CALL_Lightiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_Lightiv, parameters)
#define GET_Lightiv(disp) GET_by_offset(disp, _gloffset_Lightiv)
static void INLINE SET_Lightiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_Lightiv, fn);
}

#define CALL_LightModelf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_LightModelf, parameters)
#define GET_LightModelf(disp) GET_by_offset(disp, _gloffset_LightModelf)
static void INLINE SET_LightModelf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_LightModelf, fn);
}

#define CALL_LightModelfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_LightModelfv, parameters)
#define GET_LightModelfv(disp) GET_by_offset(disp, _gloffset_LightModelfv)
static void INLINE SET_LightModelfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_LightModelfv, fn);
}

#define CALL_LightModeli(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_LightModeli, parameters)
#define GET_LightModeli(disp) GET_by_offset(disp, _gloffset_LightModeli)
static void INLINE SET_LightModeli(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_LightModeli, fn);
}

#define CALL_LightModeliv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_LightModeliv, parameters)
#define GET_LightModeliv(disp) GET_by_offset(disp, _gloffset_LightModeliv)
static void INLINE SET_LightModeliv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_LightModeliv, fn);
}

#define CALL_LineStipple(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLushort)), _gloffset_LineStipple, parameters)
#define GET_LineStipple(disp) GET_by_offset(disp, _gloffset_LineStipple)
static void INLINE SET_LineStipple(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLushort)) {
   SET_by_offset(disp, _gloffset_LineStipple, fn);
}

#define CALL_LineWidth(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_LineWidth, parameters)
#define GET_LineWidth(disp) GET_by_offset(disp, _gloffset_LineWidth)
static void INLINE SET_LineWidth(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_LineWidth, fn);
}

#define CALL_Materialf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_Materialf, parameters)
#define GET_Materialf(disp) GET_by_offset(disp, _gloffset_Materialf)
static void INLINE SET_Materialf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Materialf, fn);
}

#define CALL_Materialfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_Materialfv, parameters)
#define GET_Materialfv(disp) GET_by_offset(disp, _gloffset_Materialfv)
static void INLINE SET_Materialfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Materialfv, fn);
}

#define CALL_Materiali(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_Materiali, parameters)
#define GET_Materiali(disp) GET_by_offset(disp, _gloffset_Materiali)
static void INLINE SET_Materiali(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_Materiali, fn);
}

#define CALL_Materialiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_Materialiv, parameters)
#define GET_Materialiv(disp) GET_by_offset(disp, _gloffset_Materialiv)
static void INLINE SET_Materialiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_Materialiv, fn);
}

#define CALL_PointSize(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_PointSize, parameters)
#define GET_PointSize(disp) GET_by_offset(disp, _gloffset_PointSize)
static void INLINE SET_PointSize(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_PointSize, fn);
}

#define CALL_PolygonMode(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_PolygonMode, parameters)
#define GET_PolygonMode(disp) GET_by_offset(disp, _gloffset_PolygonMode)
static void INLINE SET_PolygonMode(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_PolygonMode, fn);
}

#define CALL_PolygonStipple(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_PolygonStipple, parameters)
#define GET_PolygonStipple(disp) GET_by_offset(disp, _gloffset_PolygonStipple)
static void INLINE SET_PolygonStipple(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_PolygonStipple, fn);
}

#define CALL_Scissor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei)), _gloffset_Scissor, parameters)
#define GET_Scissor(disp) GET_by_offset(disp, _gloffset_Scissor)
static void INLINE SET_Scissor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_Scissor, fn);
}

#define CALL_ShadeModel(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ShadeModel, parameters)
#define GET_ShadeModel(disp) GET_by_offset(disp, _gloffset_ShadeModel)
static void INLINE SET_ShadeModel(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ShadeModel, fn);
}

#define CALL_TexParameterf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_TexParameterf, parameters)
#define GET_TexParameterf(disp) GET_by_offset(disp, _gloffset_TexParameterf)
static void INLINE SET_TexParameterf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexParameterf, fn);
}

#define CALL_TexParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_TexParameterfv, parameters)
#define GET_TexParameterfv(disp) GET_by_offset(disp, _gloffset_TexParameterfv)
static void INLINE SET_TexParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexParameterfv, fn);
}

#define CALL_TexParameteri(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_TexParameteri, parameters)
#define GET_TexParameteri(disp) GET_by_offset(disp, _gloffset_TexParameteri)
static void INLINE SET_TexParameteri(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_TexParameteri, fn);
}

#define CALL_TexParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexParameteriv, parameters)
#define GET_TexParameteriv(disp) GET_by_offset(disp, _gloffset_TexParameteriv)
static void INLINE SET_TexParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexParameteriv, fn);
}

#define CALL_TexImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *)), _gloffset_TexImage1D, parameters)
#define GET_TexImage1D(disp) GET_by_offset(disp, _gloffset_TexImage1D)
static void INLINE SET_TexImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexImage1D, fn);
}

#define CALL_TexImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)), _gloffset_TexImage2D, parameters)
#define GET_TexImage2D(disp) GET_by_offset(disp, _gloffset_TexImage2D)
static void INLINE SET_TexImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexImage2D, fn);
}

#define CALL_TexEnvf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_TexEnvf, parameters)
#define GET_TexEnvf(disp) GET_by_offset(disp, _gloffset_TexEnvf)
static void INLINE SET_TexEnvf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexEnvf, fn);
}

#define CALL_TexEnvfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_TexEnvfv, parameters)
#define GET_TexEnvfv(disp) GET_by_offset(disp, _gloffset_TexEnvfv)
static void INLINE SET_TexEnvfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexEnvfv, fn);
}

#define CALL_TexEnvi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_TexEnvi, parameters)
#define GET_TexEnvi(disp) GET_by_offset(disp, _gloffset_TexEnvi)
static void INLINE SET_TexEnvi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_TexEnvi, fn);
}

#define CALL_TexEnviv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexEnviv, parameters)
#define GET_TexEnviv(disp) GET_by_offset(disp, _gloffset_TexEnviv)
static void INLINE SET_TexEnviv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexEnviv, fn);
}

#define CALL_TexGend(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLdouble)), _gloffset_TexGend, parameters)
#define GET_TexGend(disp) GET_by_offset(disp, _gloffset_TexGend)
static void INLINE SET_TexGend(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLdouble)) {
   SET_by_offset(disp, _gloffset_TexGend, fn);
}

#define CALL_TexGendv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLdouble *)), _gloffset_TexGendv, parameters)
#define GET_TexGendv(disp) GET_by_offset(disp, _gloffset_TexGendv)
static void INLINE SET_TexGendv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_TexGendv, fn);
}

#define CALL_TexGenf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_TexGenf, parameters)
#define GET_TexGenf(disp) GET_by_offset(disp, _gloffset_TexGenf)
static void INLINE SET_TexGenf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_TexGenf, fn);
}

#define CALL_TexGenfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_TexGenfv, parameters)
#define GET_TexGenfv(disp) GET_by_offset(disp, _gloffset_TexGenfv)
static void INLINE SET_TexGenfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexGenfv, fn);
}

#define CALL_TexGeni(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_TexGeni, parameters)
#define GET_TexGeni(disp) GET_by_offset(disp, _gloffset_TexGeni)
static void INLINE SET_TexGeni(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_TexGeni, fn);
}

#define CALL_TexGeniv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexGeniv, parameters)
#define GET_TexGeniv(disp) GET_by_offset(disp, _gloffset_TexGeniv)
static void INLINE SET_TexGeniv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexGeniv, fn);
}

#define CALL_FeedbackBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLenum, GLfloat *)), _gloffset_FeedbackBuffer, parameters)
#define GET_FeedbackBuffer(disp) GET_by_offset(disp, _gloffset_FeedbackBuffer)
static void INLINE SET_FeedbackBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_FeedbackBuffer, fn);
}

#define CALL_SelectBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_SelectBuffer, parameters)
#define GET_SelectBuffer(disp) GET_by_offset(disp, _gloffset_SelectBuffer)
static void INLINE SET_SelectBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_SelectBuffer, fn);
}

#define CALL_RenderMode(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLenum)), _gloffset_RenderMode, parameters)
#define GET_RenderMode(disp) GET_by_offset(disp, _gloffset_RenderMode)
static void INLINE SET_RenderMode(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_RenderMode, fn);
}

#define CALL_InitNames(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_InitNames, parameters)
#define GET_InitNames(disp) GET_by_offset(disp, _gloffset_InitNames)
static void INLINE SET_InitNames(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_InitNames, fn);
}

#define CALL_LoadName(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_LoadName, parameters)
#define GET_LoadName(disp) GET_by_offset(disp, _gloffset_LoadName)
static void INLINE SET_LoadName(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_LoadName, fn);
}

#define CALL_PassThrough(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_PassThrough, parameters)
#define GET_PassThrough(disp) GET_by_offset(disp, _gloffset_PassThrough)
static void INLINE SET_PassThrough(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_PassThrough, fn);
}

#define CALL_PopName(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopName, parameters)
#define GET_PopName(disp) GET_by_offset(disp, _gloffset_PopName)
static void INLINE SET_PopName(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopName, fn);
}

#define CALL_PushName(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_PushName, parameters)
#define GET_PushName(disp) GET_by_offset(disp, _gloffset_PushName)
static void INLINE SET_PushName(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_PushName, fn);
}

#define CALL_DrawBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_DrawBuffer, parameters)
#define GET_DrawBuffer(disp) GET_by_offset(disp, _gloffset_DrawBuffer)
static void INLINE SET_DrawBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_DrawBuffer, fn);
}

#define CALL_Clear(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbitfield)), _gloffset_Clear, parameters)
#define GET_Clear(disp) GET_by_offset(disp, _gloffset_Clear)
static void INLINE SET_Clear(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbitfield)) {
   SET_by_offset(disp, _gloffset_Clear, fn);
}

#define CALL_ClearAccum(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ClearAccum, parameters)
#define GET_ClearAccum(disp) GET_by_offset(disp, _gloffset_ClearAccum)
static void INLINE SET_ClearAccum(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ClearAccum, fn);
}

#define CALL_ClearIndex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_ClearIndex, parameters)
#define GET_ClearIndex(disp) GET_by_offset(disp, _gloffset_ClearIndex)
static void INLINE SET_ClearIndex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_ClearIndex, fn);
}

#define CALL_ClearColor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLclampf, GLclampf, GLclampf)), _gloffset_ClearColor, parameters)
#define GET_ClearColor(disp) GET_by_offset(disp, _gloffset_ClearColor)
static void INLINE SET_ClearColor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLclampf, GLclampf, GLclampf)) {
   SET_by_offset(disp, _gloffset_ClearColor, fn);
}

#define CALL_ClearStencil(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_ClearStencil, parameters)
#define GET_ClearStencil(disp) GET_by_offset(disp, _gloffset_ClearStencil)
static void INLINE SET_ClearStencil(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_ClearStencil, fn);
}

#define CALL_ClearDepth(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampd)), _gloffset_ClearDepth, parameters)
#define GET_ClearDepth(disp) GET_by_offset(disp, _gloffset_ClearDepth)
static void INLINE SET_ClearDepth(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampd)) {
   SET_by_offset(disp, _gloffset_ClearDepth, fn);
}

#define CALL_StencilMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_StencilMask, parameters)
#define GET_StencilMask(disp) GET_by_offset(disp, _gloffset_StencilMask)
static void INLINE SET_StencilMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_StencilMask, fn);
}

#define CALL_ColorMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLboolean, GLboolean, GLboolean, GLboolean)), _gloffset_ColorMask, parameters)
#define GET_ColorMask(disp) GET_by_offset(disp, _gloffset_ColorMask)
static void INLINE SET_ColorMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLboolean, GLboolean, GLboolean, GLboolean)) {
   SET_by_offset(disp, _gloffset_ColorMask, fn);
}

#define CALL_DepthMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLboolean)), _gloffset_DepthMask, parameters)
#define GET_DepthMask(disp) GET_by_offset(disp, _gloffset_DepthMask)
static void INLINE SET_DepthMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLboolean)) {
   SET_by_offset(disp, _gloffset_DepthMask, fn);
}

#define CALL_IndexMask(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_IndexMask, parameters)
#define GET_IndexMask(disp) GET_by_offset(disp, _gloffset_IndexMask)
static void INLINE SET_IndexMask(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IndexMask, fn);
}

#define CALL_Accum(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_Accum, parameters)
#define GET_Accum(disp) GET_by_offset(disp, _gloffset_Accum)
static void INLINE SET_Accum(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_Accum, fn);
}

#define CALL_Disable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_Disable, parameters)
#define GET_Disable(disp) GET_by_offset(disp, _gloffset_Disable)
static void INLINE SET_Disable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_Disable, fn);
}

#define CALL_Enable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_Enable, parameters)
#define GET_Enable(disp) GET_by_offset(disp, _gloffset_Enable)
static void INLINE SET_Enable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_Enable, fn);
}

#define CALL_Finish(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_Finish, parameters)
#define GET_Finish(disp) GET_by_offset(disp, _gloffset_Finish)
static void INLINE SET_Finish(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_Finish, fn);
}

#define CALL_Flush(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_Flush, parameters)
#define GET_Flush(disp) GET_by_offset(disp, _gloffset_Flush)
static void INLINE SET_Flush(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_Flush, fn);
}

#define CALL_PopAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopAttrib, parameters)
#define GET_PopAttrib(disp) GET_by_offset(disp, _gloffset_PopAttrib)
static void INLINE SET_PopAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopAttrib, fn);
}

#define CALL_PushAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbitfield)), _gloffset_PushAttrib, parameters)
#define GET_PushAttrib(disp) GET_by_offset(disp, _gloffset_PushAttrib)
static void INLINE SET_PushAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbitfield)) {
   SET_by_offset(disp, _gloffset_PushAttrib, fn);
}

#define CALL_Map1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *)), _gloffset_Map1d, parameters)
#define GET_Map1d(disp) GET_by_offset(disp, _gloffset_Map1d)
static void INLINE SET_Map1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Map1d, fn);
}

#define CALL_Map1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *)), _gloffset_Map1f, parameters)
#define GET_Map1f(disp) GET_by_offset(disp, _gloffset_Map1f)
static void INLINE SET_Map1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Map1f, fn);
}

#define CALL_Map2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *)), _gloffset_Map2d, parameters)
#define GET_Map2d(disp) GET_by_offset(disp, _gloffset_Map2d)
static void INLINE SET_Map2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_Map2d, fn);
}

#define CALL_Map2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *)), _gloffset_Map2f, parameters)
#define GET_Map2f(disp) GET_by_offset(disp, _gloffset_Map2f)
static void INLINE SET_Map2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Map2f, fn);
}

#define CALL_MapGrid1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLdouble, GLdouble)), _gloffset_MapGrid1d, parameters)
#define GET_MapGrid1d(disp) GET_by_offset(disp, _gloffset_MapGrid1d)
static void INLINE SET_MapGrid1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MapGrid1d, fn);
}

#define CALL_MapGrid1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat)), _gloffset_MapGrid1f, parameters)
#define GET_MapGrid1f(disp) GET_by_offset(disp, _gloffset_MapGrid1f)
static void INLINE SET_MapGrid1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MapGrid1f, fn);
}

#define CALL_MapGrid2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble)), _gloffset_MapGrid2d, parameters)
#define GET_MapGrid2d(disp) GET_by_offset(disp, _gloffset_MapGrid2d)
static void INLINE SET_MapGrid2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MapGrid2d, fn);
}

#define CALL_MapGrid2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat)), _gloffset_MapGrid2f, parameters)
#define GET_MapGrid2f(disp) GET_by_offset(disp, _gloffset_MapGrid2f)
static void INLINE SET_MapGrid2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MapGrid2f, fn);
}

#define CALL_EvalCoord1d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_EvalCoord1d, parameters)
#define GET_EvalCoord1d(disp) GET_by_offset(disp, _gloffset_EvalCoord1d)
static void INLINE SET_EvalCoord1d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_EvalCoord1d, fn);
}

#define CALL_EvalCoord1dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_EvalCoord1dv, parameters)
#define GET_EvalCoord1dv(disp) GET_by_offset(disp, _gloffset_EvalCoord1dv)
static void INLINE SET_EvalCoord1dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_EvalCoord1dv, fn);
}

#define CALL_EvalCoord1f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_EvalCoord1f, parameters)
#define GET_EvalCoord1f(disp) GET_by_offset(disp, _gloffset_EvalCoord1f)
static void INLINE SET_EvalCoord1f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_EvalCoord1f, fn);
}

#define CALL_EvalCoord1fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_EvalCoord1fv, parameters)
#define GET_EvalCoord1fv(disp) GET_by_offset(disp, _gloffset_EvalCoord1fv)
static void INLINE SET_EvalCoord1fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_EvalCoord1fv, fn);
}

#define CALL_EvalCoord2d(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_EvalCoord2d, parameters)
#define GET_EvalCoord2d(disp) GET_by_offset(disp, _gloffset_EvalCoord2d)
static void INLINE SET_EvalCoord2d(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_EvalCoord2d, fn);
}

#define CALL_EvalCoord2dv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_EvalCoord2dv, parameters)
#define GET_EvalCoord2dv(disp) GET_by_offset(disp, _gloffset_EvalCoord2dv)
static void INLINE SET_EvalCoord2dv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_EvalCoord2dv, fn);
}

#define CALL_EvalCoord2f(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_EvalCoord2f, parameters)
#define GET_EvalCoord2f(disp) GET_by_offset(disp, _gloffset_EvalCoord2f)
static void INLINE SET_EvalCoord2f(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_EvalCoord2f, fn);
}

#define CALL_EvalCoord2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_EvalCoord2fv, parameters)
#define GET_EvalCoord2fv(disp) GET_by_offset(disp, _gloffset_EvalCoord2fv)
static void INLINE SET_EvalCoord2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_EvalCoord2fv, fn);
}

#define CALL_EvalMesh1(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint)), _gloffset_EvalMesh1, parameters)
#define GET_EvalMesh1(disp) GET_by_offset(disp, _gloffset_EvalMesh1)
static void INLINE SET_EvalMesh1(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_EvalMesh1, fn);
}

#define CALL_EvalPoint1(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_EvalPoint1, parameters)
#define GET_EvalPoint1(disp) GET_by_offset(disp, _gloffset_EvalPoint1)
static void INLINE SET_EvalPoint1(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_EvalPoint1, fn);
}

#define CALL_EvalMesh2(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint)), _gloffset_EvalMesh2, parameters)
#define GET_EvalMesh2(disp) GET_by_offset(disp, _gloffset_EvalMesh2)
static void INLINE SET_EvalMesh2(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_EvalMesh2, fn);
}

#define CALL_EvalPoint2(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_EvalPoint2, parameters)
#define GET_EvalPoint2(disp) GET_by_offset(disp, _gloffset_EvalPoint2)
static void INLINE SET_EvalPoint2(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_EvalPoint2, fn);
}

#define CALL_AlphaFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLclampf)), _gloffset_AlphaFunc, parameters)
#define GET_AlphaFunc(disp) GET_by_offset(disp, _gloffset_AlphaFunc)
static void INLINE SET_AlphaFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLclampf)) {
   SET_by_offset(disp, _gloffset_AlphaFunc, fn);
}

#define CALL_BlendFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_BlendFunc, parameters)
#define GET_BlendFunc(disp) GET_by_offset(disp, _gloffset_BlendFunc)
static void INLINE SET_BlendFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFunc, fn);
}

#define CALL_LogicOp(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_LogicOp, parameters)
#define GET_LogicOp(disp) GET_by_offset(disp, _gloffset_LogicOp)
static void INLINE SET_LogicOp(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_LogicOp, fn);
}

#define CALL_StencilFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLuint)), _gloffset_StencilFunc, parameters)
#define GET_StencilFunc(disp) GET_by_offset(disp, _gloffset_StencilFunc)
static void INLINE SET_StencilFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilFunc, fn);
}

#define CALL_StencilOp(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum)), _gloffset_StencilOp, parameters)
#define GET_StencilOp(disp) GET_by_offset(disp, _gloffset_StencilOp)
static void INLINE SET_StencilOp(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_StencilOp, fn);
}

#define CALL_DepthFunc(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_DepthFunc, parameters)
#define GET_DepthFunc(disp) GET_by_offset(disp, _gloffset_DepthFunc)
static void INLINE SET_DepthFunc(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_DepthFunc, fn);
}

#define CALL_PixelZoom(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_PixelZoom, parameters)
#define GET_PixelZoom(disp) GET_by_offset(disp, _gloffset_PixelZoom)
static void INLINE SET_PixelZoom(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelZoom, fn);
}

#define CALL_PixelTransferf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PixelTransferf, parameters)
#define GET_PixelTransferf(disp) GET_by_offset(disp, _gloffset_PixelTransferf)
static void INLINE SET_PixelTransferf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelTransferf, fn);
}

#define CALL_PixelTransferi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PixelTransferi, parameters)
#define GET_PixelTransferi(disp) GET_by_offset(disp, _gloffset_PixelTransferi)
static void INLINE SET_PixelTransferi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PixelTransferi, fn);
}

#define CALL_PixelStoref(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PixelStoref, parameters)
#define GET_PixelStoref(disp) GET_by_offset(disp, _gloffset_PixelStoref)
static void INLINE SET_PixelStoref(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelStoref, fn);
}

#define CALL_PixelStorei(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PixelStorei, parameters)
#define GET_PixelStorei(disp) GET_by_offset(disp, _gloffset_PixelStorei)
static void INLINE SET_PixelStorei(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PixelStorei, fn);
}

#define CALL_PixelMapfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLfloat *)), _gloffset_PixelMapfv, parameters)
#define GET_PixelMapfv(disp) GET_by_offset(disp, _gloffset_PixelMapfv)
static void INLINE SET_PixelMapfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_PixelMapfv, fn);
}

#define CALL_PixelMapuiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLuint *)), _gloffset_PixelMapuiv, parameters)
#define GET_PixelMapuiv(disp) GET_by_offset(disp, _gloffset_PixelMapuiv)
static void INLINE SET_PixelMapuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_PixelMapuiv, fn);
}

#define CALL_PixelMapusv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLushort *)), _gloffset_PixelMapusv, parameters)
#define GET_PixelMapusv(disp) GET_by_offset(disp, _gloffset_PixelMapusv)
static void INLINE SET_PixelMapusv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLushort *)) {
   SET_by_offset(disp, _gloffset_PixelMapusv, fn);
}

#define CALL_ReadBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ReadBuffer, parameters)
#define GET_ReadBuffer(disp) GET_by_offset(disp, _gloffset_ReadBuffer)
static void INLINE SET_ReadBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ReadBuffer, fn);
}

#define CALL_CopyPixels(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei, GLenum)), _gloffset_CopyPixels, parameters)
#define GET_CopyPixels(disp) GET_by_offset(disp, _gloffset_CopyPixels)
static void INLINE SET_CopyPixels(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei, GLenum)) {
   SET_by_offset(disp, _gloffset_CopyPixels, fn);
}

#define CALL_ReadPixels(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *)), _gloffset_ReadPixels, parameters)
#define GET_ReadPixels(disp) GET_by_offset(disp, _gloffset_ReadPixels)
static void INLINE SET_ReadPixels(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_ReadPixels, fn);
}

#define CALL_DrawPixels(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_DrawPixels, parameters)
#define GET_DrawPixels(disp) GET_by_offset(disp, _gloffset_DrawPixels)
static void INLINE SET_DrawPixels(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_DrawPixels, fn);
}

#define CALL_GetBooleanv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean *)), _gloffset_GetBooleanv, parameters)
#define GET_GetBooleanv(disp) GET_by_offset(disp, _gloffset_GetBooleanv)
static void INLINE SET_GetBooleanv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean *)) {
   SET_by_offset(disp, _gloffset_GetBooleanv, fn);
}

#define CALL_GetClipPlane(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble *)), _gloffset_GetClipPlane, parameters)
#define GET_GetClipPlane(disp) GET_by_offset(disp, _gloffset_GetClipPlane)
static void INLINE SET_GetClipPlane(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetClipPlane, fn);
}

#define CALL_GetDoublev(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble *)), _gloffset_GetDoublev, parameters)
#define GET_GetDoublev(disp) GET_by_offset(disp, _gloffset_GetDoublev)
static void INLINE SET_GetDoublev(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetDoublev, fn);
}

#define CALL_GetError(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(void)), _gloffset_GetError, parameters)
#define GET_GetError(disp) GET_by_offset(disp, _gloffset_GetError)
static void INLINE SET_GetError(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_GetError, fn);
}

#define CALL_GetFloatv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetFloatv, parameters)
#define GET_GetFloatv(disp) GET_by_offset(disp, _gloffset_GetFloatv)
static void INLINE SET_GetFloatv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetFloatv, fn);
}

#define CALL_GetIntegerv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *)), _gloffset_GetIntegerv, parameters)
#define GET_GetIntegerv(disp) GET_by_offset(disp, _gloffset_GetIntegerv)
static void INLINE SET_GetIntegerv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetIntegerv, fn);
}

#define CALL_GetLightfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetLightfv, parameters)
#define GET_GetLightfv(disp) GET_by_offset(disp, _gloffset_GetLightfv)
static void INLINE SET_GetLightfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetLightfv, fn);
}

#define CALL_GetLightiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetLightiv, parameters)
#define GET_GetLightiv(disp) GET_by_offset(disp, _gloffset_GetLightiv)
static void INLINE SET_GetLightiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetLightiv, fn);
}

#define CALL_GetMapdv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLdouble *)), _gloffset_GetMapdv, parameters)
#define GET_GetMapdv(disp) GET_by_offset(disp, _gloffset_GetMapdv)
static void INLINE SET_GetMapdv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetMapdv, fn);
}

#define CALL_GetMapfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetMapfv, parameters)
#define GET_GetMapfv(disp) GET_by_offset(disp, _gloffset_GetMapfv)
static void INLINE SET_GetMapfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetMapfv, fn);
}

#define CALL_GetMapiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetMapiv, parameters)
#define GET_GetMapiv(disp) GET_by_offset(disp, _gloffset_GetMapiv)
static void INLINE SET_GetMapiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetMapiv, fn);
}

#define CALL_GetMaterialfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetMaterialfv, parameters)
#define GET_GetMaterialfv(disp) GET_by_offset(disp, _gloffset_GetMaterialfv)
static void INLINE SET_GetMaterialfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetMaterialfv, fn);
}

#define CALL_GetMaterialiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetMaterialiv, parameters)
#define GET_GetMaterialiv(disp) GET_by_offset(disp, _gloffset_GetMaterialiv)
static void INLINE SET_GetMaterialiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetMaterialiv, fn);
}

#define CALL_GetPixelMapfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetPixelMapfv, parameters)
#define GET_GetPixelMapfv(disp) GET_by_offset(disp, _gloffset_GetPixelMapfv)
static void INLINE SET_GetPixelMapfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetPixelMapfv, fn);
}

#define CALL_GetPixelMapuiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint *)), _gloffset_GetPixelMapuiv, parameters)
#define GET_GetPixelMapuiv(disp) GET_by_offset(disp, _gloffset_GetPixelMapuiv)
static void INLINE SET_GetPixelMapuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetPixelMapuiv, fn);
}

#define CALL_GetPixelMapusv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLushort *)), _gloffset_GetPixelMapusv, parameters)
#define GET_GetPixelMapusv(disp) GET_by_offset(disp, _gloffset_GetPixelMapusv)
static void INLINE SET_GetPixelMapusv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLushort *)) {
   SET_by_offset(disp, _gloffset_GetPixelMapusv, fn);
}

#define CALL_GetPolygonStipple(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte *)), _gloffset_GetPolygonStipple, parameters)
#define GET_GetPolygonStipple(disp) GET_by_offset(disp, _gloffset_GetPolygonStipple)
static void INLINE SET_GetPolygonStipple(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte *)) {
   SET_by_offset(disp, _gloffset_GetPolygonStipple, fn);
}

#define CALL_GetString(disp, parameters) CALL_by_offset(disp, (const GLubyte * (GLAPIENTRYP)(GLenum)), _gloffset_GetString, parameters)
#define GET_GetString(disp) GET_by_offset(disp, _gloffset_GetString)
static void INLINE SET_GetString(struct _glapi_table *disp, const GLubyte * (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_GetString, fn);
}

#define CALL_GetTexEnvfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetTexEnvfv, parameters)
#define GET_GetTexEnvfv(disp) GET_by_offset(disp, _gloffset_GetTexEnvfv)
static void INLINE SET_GetTexEnvfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexEnvfv, fn);
}

#define CALL_GetTexEnviv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexEnviv, parameters)
#define GET_GetTexEnviv(disp) GET_by_offset(disp, _gloffset_GetTexEnviv)
static void INLINE SET_GetTexEnviv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexEnviv, fn);
}

#define CALL_GetTexGendv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLdouble *)), _gloffset_GetTexGendv, parameters)
#define GET_GetTexGendv(disp) GET_by_offset(disp, _gloffset_GetTexGendv)
static void INLINE SET_GetTexGendv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetTexGendv, fn);
}

#define CALL_GetTexGenfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetTexGenfv, parameters)
#define GET_GetTexGenfv(disp) GET_by_offset(disp, _gloffset_GetTexGenfv)
static void INLINE SET_GetTexGenfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexGenfv, fn);
}

#define CALL_GetTexGeniv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexGeniv, parameters)
#define GET_GetTexGeniv(disp) GET_by_offset(disp, _gloffset_GetTexGeniv)
static void INLINE SET_GetTexGeniv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexGeniv, fn);
}

#define CALL_GetTexImage(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLenum, GLvoid *)), _gloffset_GetTexImage, parameters)
#define GET_GetTexImage(disp) GET_by_offset(disp, _gloffset_GetTexImage)
static void INLINE SET_GetTexImage(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetTexImage, fn);
}

#define CALL_GetTexParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetTexParameterfv, parameters)
#define GET_GetTexParameterfv(disp) GET_by_offset(disp, _gloffset_GetTexParameterfv)
static void INLINE SET_GetTexParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexParameterfv, fn);
}

#define CALL_GetTexParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexParameteriv, parameters)
#define GET_GetTexParameteriv(disp) GET_by_offset(disp, _gloffset_GetTexParameteriv)
static void INLINE SET_GetTexParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexParameteriv, fn);
}

#define CALL_GetTexLevelParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLfloat *)), _gloffset_GetTexLevelParameterfv, parameters)
#define GET_GetTexLevelParameterfv(disp) GET_by_offset(disp, _gloffset_GetTexLevelParameterfv)
static void INLINE SET_GetTexLevelParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexLevelParameterfv, fn);
}

#define CALL_GetTexLevelParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLint *)), _gloffset_GetTexLevelParameteriv, parameters)
#define GET_GetTexLevelParameteriv(disp) GET_by_offset(disp, _gloffset_GetTexLevelParameteriv)
static void INLINE SET_GetTexLevelParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexLevelParameteriv, fn);
}

#define CALL_IsEnabled(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLenum)), _gloffset_IsEnabled, parameters)
#define GET_IsEnabled(disp) GET_by_offset(disp, _gloffset_IsEnabled)
static void INLINE SET_IsEnabled(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_IsEnabled, fn);
}

#define CALL_IsList(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsList, parameters)
#define GET_IsList(disp) GET_by_offset(disp, _gloffset_IsList)
static void INLINE SET_IsList(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsList, fn);
}

#define CALL_DepthRange(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampd, GLclampd)), _gloffset_DepthRange, parameters)
#define GET_DepthRange(disp) GET_by_offset(disp, _gloffset_DepthRange)
static void INLINE SET_DepthRange(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampd, GLclampd)) {
   SET_by_offset(disp, _gloffset_DepthRange, fn);
}

#define CALL_Frustum(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Frustum, parameters)
#define GET_Frustum(disp) GET_by_offset(disp, _gloffset_Frustum)
static void INLINE SET_Frustum(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Frustum, fn);
}

#define CALL_LoadIdentity(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_LoadIdentity, parameters)
#define GET_LoadIdentity(disp) GET_by_offset(disp, _gloffset_LoadIdentity)
static void INLINE SET_LoadIdentity(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_LoadIdentity, fn);
}

#define CALL_LoadMatrixf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_LoadMatrixf, parameters)
#define GET_LoadMatrixf(disp) GET_by_offset(disp, _gloffset_LoadMatrixf)
static void INLINE SET_LoadMatrixf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_LoadMatrixf, fn);
}

#define CALL_LoadMatrixd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_LoadMatrixd, parameters)
#define GET_LoadMatrixd(disp) GET_by_offset(disp, _gloffset_LoadMatrixd)
static void INLINE SET_LoadMatrixd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_LoadMatrixd, fn);
}

#define CALL_MatrixMode(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_MatrixMode, parameters)
#define GET_MatrixMode(disp) GET_by_offset(disp, _gloffset_MatrixMode)
static void INLINE SET_MatrixMode(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_MatrixMode, fn);
}

#define CALL_MultMatrixf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_MultMatrixf, parameters)
#define GET_MultMatrixf(disp) GET_by_offset(disp, _gloffset_MultMatrixf)
static void INLINE SET_MultMatrixf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultMatrixf, fn);
}

#define CALL_MultMatrixd(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_MultMatrixd, parameters)
#define GET_MultMatrixd(disp) GET_by_offset(disp, _gloffset_MultMatrixd)
static void INLINE SET_MultMatrixd(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultMatrixd, fn);
}

#define CALL_Ortho(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Ortho, parameters)
#define GET_Ortho(disp) GET_by_offset(disp, _gloffset_Ortho)
static void INLINE SET_Ortho(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Ortho, fn);
}

#define CALL_PopMatrix(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopMatrix, parameters)
#define GET_PopMatrix(disp) GET_by_offset(disp, _gloffset_PopMatrix)
static void INLINE SET_PopMatrix(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopMatrix, fn);
}

#define CALL_PushMatrix(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PushMatrix, parameters)
#define GET_PushMatrix(disp) GET_by_offset(disp, _gloffset_PushMatrix)
static void INLINE SET_PushMatrix(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PushMatrix, fn);
}

#define CALL_Rotated(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_Rotated, parameters)
#define GET_Rotated(disp) GET_by_offset(disp, _gloffset_Rotated)
static void INLINE SET_Rotated(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Rotated, fn);
}

#define CALL_Rotatef(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Rotatef, parameters)
#define GET_Rotatef(disp) GET_by_offset(disp, _gloffset_Rotatef)
static void INLINE SET_Rotatef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Rotatef, fn);
}

#define CALL_Scaled(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Scaled, parameters)
#define GET_Scaled(disp) GET_by_offset(disp, _gloffset_Scaled)
static void INLINE SET_Scaled(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Scaled, fn);
}

#define CALL_Scalef(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Scalef, parameters)
#define GET_Scalef(disp) GET_by_offset(disp, _gloffset_Scalef)
static void INLINE SET_Scalef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Scalef, fn);
}

#define CALL_Translated(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_Translated, parameters)
#define GET_Translated(disp) GET_by_offset(disp, _gloffset_Translated)
static void INLINE SET_Translated(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_Translated, fn);
}

#define CALL_Translatef(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_Translatef, parameters)
#define GET_Translatef(disp) GET_by_offset(disp, _gloffset_Translatef)
static void INLINE SET_Translatef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Translatef, fn);
}

#define CALL_Viewport(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei)), _gloffset_Viewport, parameters)
#define GET_Viewport(disp) GET_by_offset(disp, _gloffset_Viewport)
static void INLINE SET_Viewport(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_Viewport, fn);
}

#define CALL_ArrayElement(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint)), _gloffset_ArrayElement, parameters)
#define GET_ArrayElement(disp) GET_by_offset(disp, _gloffset_ArrayElement)
static void INLINE SET_ArrayElement(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint)) {
   SET_by_offset(disp, _gloffset_ArrayElement, fn);
}

#define CALL_BindTexture(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindTexture, parameters)
#define GET_BindTexture(disp) GET_by_offset(disp, _gloffset_BindTexture)
static void INLINE SET_BindTexture(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindTexture, fn);
}

#define CALL_ColorPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_ColorPointer, parameters)
#define GET_ColorPointer(disp) GET_by_offset(disp, _gloffset_ColorPointer)
static void INLINE SET_ColorPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorPointer, fn);
}

#define CALL_DisableClientState(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_DisableClientState, parameters)
#define GET_DisableClientState(disp) GET_by_offset(disp, _gloffset_DisableClientState)
static void INLINE SET_DisableClientState(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_DisableClientState, fn);
}

#define CALL_DrawArrays(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLsizei)), _gloffset_DrawArrays, parameters)
#define GET_DrawArrays(disp) GET_by_offset(disp, _gloffset_DrawArrays)
static void INLINE SET_DrawArrays(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_DrawArrays, fn);
}

#define CALL_DrawElements(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, const GLvoid *)), _gloffset_DrawElements, parameters)
#define GET_DrawElements(disp) GET_by_offset(disp, _gloffset_DrawElements)
static void INLINE SET_DrawElements(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_DrawElements, fn);
}

#define CALL_EdgeFlagPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLvoid *)), _gloffset_EdgeFlagPointer, parameters)
#define GET_EdgeFlagPointer(disp) GET_by_offset(disp, _gloffset_EdgeFlagPointer)
static void INLINE SET_EdgeFlagPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_EdgeFlagPointer, fn);
}

#define CALL_EnableClientState(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_EnableClientState, parameters)
#define GET_EnableClientState(disp) GET_by_offset(disp, _gloffset_EnableClientState)
static void INLINE SET_EnableClientState(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_EnableClientState, fn);
}

#define CALL_IndexPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_IndexPointer, parameters)
#define GET_IndexPointer(disp) GET_by_offset(disp, _gloffset_IndexPointer)
static void INLINE SET_IndexPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_IndexPointer, fn);
}

#define CALL_Indexub(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte)), _gloffset_Indexub, parameters)
#define GET_Indexub(disp) GET_by_offset(disp, _gloffset_Indexub)
static void INLINE SET_Indexub(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte)) {
   SET_by_offset(disp, _gloffset_Indexub, fn);
}

#define CALL_Indexubv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_Indexubv, parameters)
#define GET_Indexubv(disp) GET_by_offset(disp, _gloffset_Indexubv)
static void INLINE SET_Indexubv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_Indexubv, fn);
}

#define CALL_InterleavedArrays(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_InterleavedArrays, parameters)
#define GET_InterleavedArrays(disp) GET_by_offset(disp, _gloffset_InterleavedArrays)
static void INLINE SET_InterleavedArrays(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_InterleavedArrays, fn);
}

#define CALL_NormalPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_NormalPointer, parameters)
#define GET_NormalPointer(disp) GET_by_offset(disp, _gloffset_NormalPointer)
static void INLINE SET_NormalPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_NormalPointer, fn);
}

#define CALL_PolygonOffset(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_PolygonOffset, parameters)
#define GET_PolygonOffset(disp) GET_by_offset(disp, _gloffset_PolygonOffset)
static void INLINE SET_PolygonOffset(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_PolygonOffset, fn);
}

#define CALL_TexCoordPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_TexCoordPointer, parameters)
#define GET_TexCoordPointer(disp) GET_by_offset(disp, _gloffset_TexCoordPointer)
static void INLINE SET_TexCoordPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexCoordPointer, fn);
}

#define CALL_VertexPointer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_VertexPointer, parameters)
#define GET_VertexPointer(disp) GET_by_offset(disp, _gloffset_VertexPointer)
static void INLINE SET_VertexPointer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexPointer, fn);
}

#define CALL_AreTexturesResident(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLsizei, const GLuint *, GLboolean *)), _gloffset_AreTexturesResident, parameters)
#define GET_AreTexturesResident(disp) GET_by_offset(disp, _gloffset_AreTexturesResident)
static void INLINE SET_AreTexturesResident(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLsizei, const GLuint *, GLboolean *)) {
   SET_by_offset(disp, _gloffset_AreTexturesResident, fn);
}

#define CALL_CopyTexImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint)), _gloffset_CopyTexImage1D, parameters)
#define GET_CopyTexImage1D(disp) GET_by_offset(disp, _gloffset_CopyTexImage1D)
static void INLINE SET_CopyTexImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_CopyTexImage1D, fn);
}

#define CALL_CopyTexImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)), _gloffset_CopyTexImage2D, parameters)
#define GET_CopyTexImage2D(disp) GET_by_offset(disp, _gloffset_CopyTexImage2D)
static void INLINE SET_CopyTexImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_CopyTexImage2D, fn);
}

#define CALL_CopyTexSubImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei)), _gloffset_CopyTexSubImage1D, parameters)
#define GET_CopyTexSubImage1D(disp) GET_by_offset(disp, _gloffset_CopyTexSubImage1D)
static void INLINE SET_CopyTexSubImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyTexSubImage1D, fn);
}

#define CALL_CopyTexSubImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)), _gloffset_CopyTexSubImage2D, parameters)
#define GET_CopyTexSubImage2D(disp) GET_by_offset(disp, _gloffset_CopyTexSubImage2D)
static void INLINE SET_CopyTexSubImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyTexSubImage2D, fn);
}

#define CALL_DeleteTextures(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteTextures, parameters)
#define GET_DeleteTextures(disp) GET_by_offset(disp, _gloffset_DeleteTextures)
static void INLINE SET_DeleteTextures(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteTextures, fn);
}

#define CALL_GenTextures(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenTextures, parameters)
#define GET_GenTextures(disp) GET_by_offset(disp, _gloffset_GenTextures)
static void INLINE SET_GenTextures(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenTextures, fn);
}

#define CALL_GetPointerv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLvoid **)), _gloffset_GetPointerv, parameters)
#define GET_GetPointerv(disp) GET_by_offset(disp, _gloffset_GetPointerv)
static void INLINE SET_GetPointerv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetPointerv, fn);
}

#define CALL_IsTexture(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsTexture, parameters)
#define GET_IsTexture(disp) GET_by_offset(disp, _gloffset_IsTexture)
static void INLINE SET_IsTexture(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsTexture, fn);
}

#define CALL_PrioritizeTextures(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *, const GLclampf *)), _gloffset_PrioritizeTextures, parameters)
#define GET_PrioritizeTextures(disp) GET_by_offset(disp, _gloffset_PrioritizeTextures)
static void INLINE SET_PrioritizeTextures(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *, const GLclampf *)) {
   SET_by_offset(disp, _gloffset_PrioritizeTextures, fn);
}

#define CALL_TexSubImage1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_TexSubImage1D, parameters)
#define GET_TexSubImage1D(disp) GET_by_offset(disp, _gloffset_TexSubImage1D)
static void INLINE SET_TexSubImage1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexSubImage1D, fn);
}

#define CALL_TexSubImage2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_TexSubImage2D, parameters)
#define GET_TexSubImage2D(disp) GET_by_offset(disp, _gloffset_TexSubImage2D)
static void INLINE SET_TexSubImage2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexSubImage2D, fn);
}

#define CALL_PopClientAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PopClientAttrib, parameters)
#define GET_PopClientAttrib(disp) GET_by_offset(disp, _gloffset_PopClientAttrib)
static void INLINE SET_PopClientAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PopClientAttrib, fn);
}

#define CALL_PushClientAttrib(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbitfield)), _gloffset_PushClientAttrib, parameters)
#define GET_PushClientAttrib(disp) GET_by_offset(disp, _gloffset_PushClientAttrib)
static void INLINE SET_PushClientAttrib(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbitfield)) {
   SET_by_offset(disp, _gloffset_PushClientAttrib, fn);
}

#define CALL_BlendColor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLclampf, GLclampf, GLclampf)), _gloffset_BlendColor, parameters)
#define GET_BlendColor(disp) GET_by_offset(disp, _gloffset_BlendColor)
static void INLINE SET_BlendColor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLclampf, GLclampf, GLclampf)) {
   SET_by_offset(disp, _gloffset_BlendColor, fn);
}

#define CALL_BlendEquation(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_BlendEquation, parameters)
#define GET_BlendEquation(disp) GET_by_offset(disp, _gloffset_BlendEquation)
static void INLINE SET_BlendEquation(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquation, fn);
}

#define CALL_DrawRangeElements(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *)), _gloffset_DrawRangeElements, parameters)
#define GET_DrawRangeElements(disp) GET_by_offset(disp, _gloffset_DrawRangeElements)
static void INLINE SET_DrawRangeElements(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_DrawRangeElements, fn);
}

#define CALL_ColorTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ColorTable, parameters)
#define GET_ColorTable(disp) GET_by_offset(disp, _gloffset_ColorTable)
static void INLINE SET_ColorTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorTable, fn);
}

#define CALL_ColorTableParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_ColorTableParameterfv, parameters)
#define GET_ColorTableParameterfv(disp) GET_by_offset(disp, _gloffset_ColorTableParameterfv)
static void INLINE SET_ColorTableParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ColorTableParameterfv, fn);
}

#define CALL_ColorTableParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_ColorTableParameteriv, parameters)
#define GET_ColorTableParameteriv(disp) GET_by_offset(disp, _gloffset_ColorTableParameteriv)
static void INLINE SET_ColorTableParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_ColorTableParameteriv, fn);
}

#define CALL_CopyColorTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLint, GLsizei)), _gloffset_CopyColorTable, parameters)
#define GET_CopyColorTable(disp) GET_by_offset(disp, _gloffset_CopyColorTable)
static void INLINE SET_CopyColorTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyColorTable, fn);
}

#define CALL_GetColorTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLvoid *)), _gloffset_GetColorTable, parameters)
#define GET_GetColorTable(disp) GET_by_offset(disp, _gloffset_GetColorTable)
static void INLINE SET_GetColorTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetColorTable, fn);
}

#define CALL_GetColorTableParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetColorTableParameterfv, parameters)
#define GET_GetColorTableParameterfv(disp) GET_by_offset(disp, _gloffset_GetColorTableParameterfv)
static void INLINE SET_GetColorTableParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetColorTableParameterfv, fn);
}

#define CALL_GetColorTableParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetColorTableParameteriv, parameters)
#define GET_GetColorTableParameteriv(disp) GET_by_offset(disp, _gloffset_GetColorTableParameteriv)
static void INLINE SET_GetColorTableParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetColorTableParameteriv, fn);
}

#define CALL_ColorSubTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ColorSubTable, parameters)
#define GET_ColorSubTable(disp) GET_by_offset(disp, _gloffset_ColorSubTable)
static void INLINE SET_ColorSubTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorSubTable, fn);
}

#define CALL_CopyColorSubTable(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLint, GLint, GLsizei)), _gloffset_CopyColorSubTable, parameters)
#define GET_CopyColorSubTable(disp) GET_by_offset(disp, _gloffset_CopyColorSubTable)
static void INLINE SET_CopyColorSubTable(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyColorSubTable, fn);
}

#define CALL_ConvolutionFilter1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ConvolutionFilter1D, parameters)
#define GET_ConvolutionFilter1D(disp) GET_by_offset(disp, _gloffset_ConvolutionFilter1D)
static void INLINE SET_ConvolutionFilter1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ConvolutionFilter1D, fn);
}

#define CALL_ConvolutionFilter2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_ConvolutionFilter2D, parameters)
#define GET_ConvolutionFilter2D(disp) GET_by_offset(disp, _gloffset_ConvolutionFilter2D)
static void INLINE SET_ConvolutionFilter2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ConvolutionFilter2D, fn);
}

#define CALL_ConvolutionParameterf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat)), _gloffset_ConvolutionParameterf, parameters)
#define GET_ConvolutionParameterf(disp) GET_by_offset(disp, _gloffset_ConvolutionParameterf)
static void INLINE SET_ConvolutionParameterf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameterf, fn);
}

#define CALL_ConvolutionParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLfloat *)), _gloffset_ConvolutionParameterfv, parameters)
#define GET_ConvolutionParameterfv(disp) GET_by_offset(disp, _gloffset_ConvolutionParameterfv)
static void INLINE SET_ConvolutionParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameterfv, fn);
}

#define CALL_ConvolutionParameteri(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_ConvolutionParameteri, parameters)
#define GET_ConvolutionParameteri(disp) GET_by_offset(disp, _gloffset_ConvolutionParameteri)
static void INLINE SET_ConvolutionParameteri(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameteri, fn);
}

#define CALL_ConvolutionParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_ConvolutionParameteriv, parameters)
#define GET_ConvolutionParameteriv(disp) GET_by_offset(disp, _gloffset_ConvolutionParameteriv)
static void INLINE SET_ConvolutionParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_ConvolutionParameteriv, fn);
}

#define CALL_CopyConvolutionFilter1D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLint, GLsizei)), _gloffset_CopyConvolutionFilter1D, parameters)
#define GET_CopyConvolutionFilter1D(disp) GET_by_offset(disp, _gloffset_CopyConvolutionFilter1D)
static void INLINE SET_CopyConvolutionFilter1D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyConvolutionFilter1D, fn);
}

#define CALL_CopyConvolutionFilter2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei)), _gloffset_CopyConvolutionFilter2D, parameters)
#define GET_CopyConvolutionFilter2D(disp) GET_by_offset(disp, _gloffset_CopyConvolutionFilter2D)
static void INLINE SET_CopyConvolutionFilter2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyConvolutionFilter2D, fn);
}

#define CALL_GetConvolutionFilter(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLvoid *)), _gloffset_GetConvolutionFilter, parameters)
#define GET_GetConvolutionFilter(disp) GET_by_offset(disp, _gloffset_GetConvolutionFilter)
static void INLINE SET_GetConvolutionFilter(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetConvolutionFilter, fn);
}

#define CALL_GetConvolutionParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetConvolutionParameterfv, parameters)
#define GET_GetConvolutionParameterfv(disp) GET_by_offset(disp, _gloffset_GetConvolutionParameterfv)
static void INLINE SET_GetConvolutionParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetConvolutionParameterfv, fn);
}

#define CALL_GetConvolutionParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetConvolutionParameteriv, parameters)
#define GET_GetConvolutionParameteriv(disp) GET_by_offset(disp, _gloffset_GetConvolutionParameteriv)
static void INLINE SET_GetConvolutionParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetConvolutionParameteriv, fn);
}

#define CALL_GetSeparableFilter(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *)), _gloffset_GetSeparableFilter, parameters)
#define GET_GetSeparableFilter(disp) GET_by_offset(disp, _gloffset_GetSeparableFilter)
static void INLINE SET_GetSeparableFilter(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetSeparableFilter, fn);
}

#define CALL_SeparableFilter2D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *)), _gloffset_SeparableFilter2D, parameters)
#define GET_SeparableFilter2D(disp) GET_by_offset(disp, _gloffset_SeparableFilter2D)
static void INLINE SET_SeparableFilter2D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_SeparableFilter2D, fn);
}

#define CALL_GetHistogram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)), _gloffset_GetHistogram, parameters)
#define GET_GetHistogram(disp) GET_by_offset(disp, _gloffset_GetHistogram)
static void INLINE SET_GetHistogram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetHistogram, fn);
}

#define CALL_GetHistogramParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetHistogramParameterfv, parameters)
#define GET_GetHistogramParameterfv(disp) GET_by_offset(disp, _gloffset_GetHistogramParameterfv)
static void INLINE SET_GetHistogramParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetHistogramParameterfv, fn);
}

#define CALL_GetHistogramParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetHistogramParameteriv, parameters)
#define GET_GetHistogramParameteriv(disp) GET_by_offset(disp, _gloffset_GetHistogramParameteriv)
static void INLINE SET_GetHistogramParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetHistogramParameteriv, fn);
}

#define CALL_GetMinmax(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)), _gloffset_GetMinmax, parameters)
#define GET_GetMinmax(disp) GET_by_offset(disp, _gloffset_GetMinmax)
static void INLINE SET_GetMinmax(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetMinmax, fn);
}

#define CALL_GetMinmaxParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetMinmaxParameterfv, parameters)
#define GET_GetMinmaxParameterfv(disp) GET_by_offset(disp, _gloffset_GetMinmaxParameterfv)
static void INLINE SET_GetMinmaxParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetMinmaxParameterfv, fn);
}

#define CALL_GetMinmaxParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetMinmaxParameteriv, parameters)
#define GET_GetMinmaxParameteriv(disp) GET_by_offset(disp, _gloffset_GetMinmaxParameteriv)
static void INLINE SET_GetMinmaxParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetMinmaxParameteriv, fn);
}

#define CALL_Histogram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, GLboolean)), _gloffset_Histogram, parameters)
#define GET_Histogram(disp) GET_by_offset(disp, _gloffset_Histogram)
static void INLINE SET_Histogram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, GLboolean)) {
   SET_by_offset(disp, _gloffset_Histogram, fn);
}

#define CALL_Minmax(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLboolean)), _gloffset_Minmax, parameters)
#define GET_Minmax(disp) GET_by_offset(disp, _gloffset_Minmax)
static void INLINE SET_Minmax(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLboolean)) {
   SET_by_offset(disp, _gloffset_Minmax, fn);
}

#define CALL_ResetHistogram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ResetHistogram, parameters)
#define GET_ResetHistogram(disp) GET_by_offset(disp, _gloffset_ResetHistogram)
static void INLINE SET_ResetHistogram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ResetHistogram, fn);
}

#define CALL_ResetMinmax(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ResetMinmax, parameters)
#define GET_ResetMinmax(disp) GET_by_offset(disp, _gloffset_ResetMinmax)
static void INLINE SET_ResetMinmax(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ResetMinmax, fn);
}

#define CALL_TexImage3D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)), _gloffset_TexImage3D, parameters)
#define GET_TexImage3D(disp) GET_by_offset(disp, _gloffset_TexImage3D)
static void INLINE SET_TexImage3D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexImage3D, fn);
}

#define CALL_TexSubImage3D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)), _gloffset_TexSubImage3D, parameters)
#define GET_TexSubImage3D(disp) GET_by_offset(disp, _gloffset_TexSubImage3D)
static void INLINE SET_TexSubImage3D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexSubImage3D, fn);
}

#define CALL_CopyTexSubImage3D(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)), _gloffset_CopyTexSubImage3D, parameters)
#define GET_CopyTexSubImage3D(disp) GET_by_offset(disp, _gloffset_CopyTexSubImage3D)
static void INLINE SET_CopyTexSubImage3D(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_CopyTexSubImage3D, fn);
}

#define CALL_ActiveTextureARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ActiveTextureARB, parameters)
#define GET_ActiveTextureARB(disp) GET_by_offset(disp, _gloffset_ActiveTextureARB)
static void INLINE SET_ActiveTextureARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ActiveTextureARB, fn);
}

#define CALL_ClientActiveTextureARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ClientActiveTextureARB, parameters)
#define GET_ClientActiveTextureARB(disp) GET_by_offset(disp, _gloffset_ClientActiveTextureARB)
static void INLINE SET_ClientActiveTextureARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ClientActiveTextureARB, fn);
}

#define CALL_MultiTexCoord1dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble)), _gloffset_MultiTexCoord1dARB, parameters)
#define GET_MultiTexCoord1dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1dARB)
static void INLINE SET_MultiTexCoord1dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1dARB, fn);
}

#define CALL_MultiTexCoord1dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord1dvARB, parameters)
#define GET_MultiTexCoord1dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1dvARB)
static void INLINE SET_MultiTexCoord1dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1dvARB, fn);
}

#define CALL_MultiTexCoord1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_MultiTexCoord1fARB, parameters)
#define GET_MultiTexCoord1fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1fARB)
static void INLINE SET_MultiTexCoord1fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1fARB, fn);
}

#define CALL_MultiTexCoord1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord1fvARB, parameters)
#define GET_MultiTexCoord1fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1fvARB)
static void INLINE SET_MultiTexCoord1fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1fvARB, fn);
}

#define CALL_MultiTexCoord1iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_MultiTexCoord1iARB, parameters)
#define GET_MultiTexCoord1iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1iARB)
static void INLINE SET_MultiTexCoord1iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1iARB, fn);
}

#define CALL_MultiTexCoord1ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord1ivARB, parameters)
#define GET_MultiTexCoord1ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1ivARB)
static void INLINE SET_MultiTexCoord1ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1ivARB, fn);
}

#define CALL_MultiTexCoord1sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort)), _gloffset_MultiTexCoord1sARB, parameters)
#define GET_MultiTexCoord1sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1sARB)
static void INLINE SET_MultiTexCoord1sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1sARB, fn);
}

#define CALL_MultiTexCoord1svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord1svARB, parameters)
#define GET_MultiTexCoord1svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord1svARB)
static void INLINE SET_MultiTexCoord1svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord1svARB, fn);
}

#define CALL_MultiTexCoord2dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble)), _gloffset_MultiTexCoord2dARB, parameters)
#define GET_MultiTexCoord2dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2dARB)
static void INLINE SET_MultiTexCoord2dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2dARB, fn);
}

#define CALL_MultiTexCoord2dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord2dvARB, parameters)
#define GET_MultiTexCoord2dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2dvARB)
static void INLINE SET_MultiTexCoord2dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2dvARB, fn);
}

#define CALL_MultiTexCoord2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat)), _gloffset_MultiTexCoord2fARB, parameters)
#define GET_MultiTexCoord2fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2fARB)
static void INLINE SET_MultiTexCoord2fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2fARB, fn);
}

#define CALL_MultiTexCoord2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord2fvARB, parameters)
#define GET_MultiTexCoord2fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2fvARB)
static void INLINE SET_MultiTexCoord2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2fvARB, fn);
}

#define CALL_MultiTexCoord2iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint)), _gloffset_MultiTexCoord2iARB, parameters)
#define GET_MultiTexCoord2iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2iARB)
static void INLINE SET_MultiTexCoord2iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2iARB, fn);
}

#define CALL_MultiTexCoord2ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord2ivARB, parameters)
#define GET_MultiTexCoord2ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2ivARB)
static void INLINE SET_MultiTexCoord2ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2ivARB, fn);
}

#define CALL_MultiTexCoord2sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort, GLshort)), _gloffset_MultiTexCoord2sARB, parameters)
#define GET_MultiTexCoord2sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2sARB)
static void INLINE SET_MultiTexCoord2sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2sARB, fn);
}

#define CALL_MultiTexCoord2svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord2svARB, parameters)
#define GET_MultiTexCoord2svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord2svARB)
static void INLINE SET_MultiTexCoord2svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord2svARB, fn);
}

#define CALL_MultiTexCoord3dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLdouble)), _gloffset_MultiTexCoord3dARB, parameters)
#define GET_MultiTexCoord3dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3dARB)
static void INLINE SET_MultiTexCoord3dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3dARB, fn);
}

#define CALL_MultiTexCoord3dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord3dvARB, parameters)
#define GET_MultiTexCoord3dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3dvARB)
static void INLINE SET_MultiTexCoord3dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3dvARB, fn);
}

#define CALL_MultiTexCoord3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLfloat)), _gloffset_MultiTexCoord3fARB, parameters)
#define GET_MultiTexCoord3fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3fARB)
static void INLINE SET_MultiTexCoord3fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3fARB, fn);
}

#define CALL_MultiTexCoord3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord3fvARB, parameters)
#define GET_MultiTexCoord3fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3fvARB)
static void INLINE SET_MultiTexCoord3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3fvARB, fn);
}

#define CALL_MultiTexCoord3iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint)), _gloffset_MultiTexCoord3iARB, parameters)
#define GET_MultiTexCoord3iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3iARB)
static void INLINE SET_MultiTexCoord3iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3iARB, fn);
}

#define CALL_MultiTexCoord3ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord3ivARB, parameters)
#define GET_MultiTexCoord3ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3ivARB)
static void INLINE SET_MultiTexCoord3ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3ivARB, fn);
}

#define CALL_MultiTexCoord3sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort, GLshort, GLshort)), _gloffset_MultiTexCoord3sARB, parameters)
#define GET_MultiTexCoord3sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3sARB)
static void INLINE SET_MultiTexCoord3sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3sARB, fn);
}

#define CALL_MultiTexCoord3svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord3svARB, parameters)
#define GET_MultiTexCoord3svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord3svARB)
static void INLINE SET_MultiTexCoord3svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord3svARB, fn);
}

#define CALL_MultiTexCoord4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_MultiTexCoord4dARB, parameters)
#define GET_MultiTexCoord4dARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4dARB)
static void INLINE SET_MultiTexCoord4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4dARB, fn);
}

#define CALL_MultiTexCoord4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLdouble *)), _gloffset_MultiTexCoord4dvARB, parameters)
#define GET_MultiTexCoord4dvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4dvARB)
static void INLINE SET_MultiTexCoord4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4dvARB, fn);
}

#define CALL_MultiTexCoord4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_MultiTexCoord4fARB, parameters)
#define GET_MultiTexCoord4fARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4fARB)
static void INLINE SET_MultiTexCoord4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4fARB, fn);
}

#define CALL_MultiTexCoord4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_MultiTexCoord4fvARB, parameters)
#define GET_MultiTexCoord4fvARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4fvARB)
static void INLINE SET_MultiTexCoord4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4fvARB, fn);
}

#define CALL_MultiTexCoord4iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint)), _gloffset_MultiTexCoord4iARB, parameters)
#define GET_MultiTexCoord4iARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4iARB)
static void INLINE SET_MultiTexCoord4iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4iARB, fn);
}

#define CALL_MultiTexCoord4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_MultiTexCoord4ivARB, parameters)
#define GET_MultiTexCoord4ivARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4ivARB)
static void INLINE SET_MultiTexCoord4ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4ivARB, fn);
}

#define CALL_MultiTexCoord4sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLshort, GLshort, GLshort, GLshort)), _gloffset_MultiTexCoord4sARB, parameters)
#define GET_MultiTexCoord4sARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4sARB)
static void INLINE SET_MultiTexCoord4sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4sARB, fn);
}

#define CALL_MultiTexCoord4svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLshort *)), _gloffset_MultiTexCoord4svARB, parameters)
#define GET_MultiTexCoord4svARB(disp) GET_by_offset(disp, _gloffset_MultiTexCoord4svARB)
static void INLINE SET_MultiTexCoord4svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLshort *)) {
   SET_by_offset(disp, _gloffset_MultiTexCoord4svARB, fn);
}

#define CALL_AttachShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_AttachShader, parameters)
#define GET_AttachShader(disp) GET_by_offset(disp, _gloffset_AttachShader)
static void INLINE SET_AttachShader(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AttachShader, fn);
}

#define CALL_CreateProgram(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(void)), _gloffset_CreateProgram, parameters)
#define GET_CreateProgram(disp) GET_by_offset(disp, _gloffset_CreateProgram)
static void INLINE SET_CreateProgram(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_CreateProgram, fn);
}

#define CALL_CreateShader(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLenum)), _gloffset_CreateShader, parameters)
#define GET_CreateShader(disp) GET_by_offset(disp, _gloffset_CreateShader)
static void INLINE SET_CreateShader(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CreateShader, fn);
}

#define CALL_DeleteProgram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DeleteProgram, parameters)
#define GET_DeleteProgram(disp) GET_by_offset(disp, _gloffset_DeleteProgram)
static void INLINE SET_DeleteProgram(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DeleteProgram, fn);
}

#define CALL_DeleteShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DeleteShader, parameters)
#define GET_DeleteShader(disp) GET_by_offset(disp, _gloffset_DeleteShader)
static void INLINE SET_DeleteShader(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DeleteShader, fn);
}

#define CALL_DetachShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_DetachShader, parameters)
#define GET_DetachShader(disp) GET_by_offset(disp, _gloffset_DetachShader)
static void INLINE SET_DetachShader(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_DetachShader, fn);
}

#define CALL_GetAttachedShaders(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLuint *)), _gloffset_GetAttachedShaders, parameters)
#define GET_GetAttachedShaders(disp) GET_by_offset(disp, _gloffset_GetAttachedShaders)
static void INLINE SET_GetAttachedShaders(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, GLsizei *, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetAttachedShaders, fn);
}

#define CALL_GetProgramInfoLog(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLchar *)), _gloffset_GetProgramInfoLog, parameters)
#define GET_GetProgramInfoLog(disp) GET_by_offset(disp, _gloffset_GetProgramInfoLog)
static void INLINE SET_GetProgramInfoLog(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, GLsizei *, GLchar *)) {
   SET_by_offset(disp, _gloffset_GetProgramInfoLog, fn);
}

#define CALL_GetProgramiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetProgramiv, parameters)
#define GET_GetProgramiv(disp) GET_by_offset(disp, _gloffset_GetProgramiv)
static void INLINE SET_GetProgramiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetProgramiv, fn);
}

#define CALL_GetShaderInfoLog(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLchar *)), _gloffset_GetShaderInfoLog, parameters)
#define GET_GetShaderInfoLog(disp) GET_by_offset(disp, _gloffset_GetShaderInfoLog)
static void INLINE SET_GetShaderInfoLog(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, GLsizei *, GLchar *)) {
   SET_by_offset(disp, _gloffset_GetShaderInfoLog, fn);
}

#define CALL_GetShaderiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetShaderiv, parameters)
#define GET_GetShaderiv(disp) GET_by_offset(disp, _gloffset_GetShaderiv)
static void INLINE SET_GetShaderiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetShaderiv, fn);
}

#define CALL_IsProgram(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsProgram, parameters)
#define GET_IsProgram(disp) GET_by_offset(disp, _gloffset_IsProgram)
static void INLINE SET_IsProgram(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsProgram, fn);
}

#define CALL_IsShader(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsShader, parameters)
#define GET_IsShader(disp) GET_by_offset(disp, _gloffset_IsShader)
static void INLINE SET_IsShader(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsShader, fn);
}

#define CALL_StencilFuncSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLuint)), _gloffset_StencilFuncSeparate, parameters)
#define GET_StencilFuncSeparate(disp) GET_by_offset(disp, _gloffset_StencilFuncSeparate)
static void INLINE SET_StencilFuncSeparate(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilFuncSeparate, fn);
}

#define CALL_StencilMaskSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_StencilMaskSeparate, parameters)
#define GET_StencilMaskSeparate(disp) GET_by_offset(disp, _gloffset_StencilMaskSeparate)
static void INLINE SET_StencilMaskSeparate(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilMaskSeparate, fn);
}

#define CALL_StencilOpSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), _gloffset_StencilOpSeparate, parameters)
#define GET_StencilOpSeparate(disp) GET_by_offset(disp, _gloffset_StencilOpSeparate)
static void INLINE SET_StencilOpSeparate(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_StencilOpSeparate, fn);
}

#define CALL_UniformMatrix2x3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix2x3fv, parameters)
#define GET_UniformMatrix2x3fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix2x3fv)
static void INLINE SET_UniformMatrix2x3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix2x3fv, fn);
}

#define CALL_UniformMatrix2x4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix2x4fv, parameters)
#define GET_UniformMatrix2x4fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix2x4fv)
static void INLINE SET_UniformMatrix2x4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix2x4fv, fn);
}

#define CALL_UniformMatrix3x2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix3x2fv, parameters)
#define GET_UniformMatrix3x2fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix3x2fv)
static void INLINE SET_UniformMatrix3x2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix3x2fv, fn);
}

#define CALL_UniformMatrix3x4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix3x4fv, parameters)
#define GET_UniformMatrix3x4fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix3x4fv)
static void INLINE SET_UniformMatrix3x4fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix3x4fv, fn);
}

#define CALL_UniformMatrix4x2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix4x2fv, parameters)
#define GET_UniformMatrix4x2fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix4x2fv)
static void INLINE SET_UniformMatrix4x2fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix4x2fv, fn);
}

#define CALL_UniformMatrix4x3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix4x3fv, parameters)
#define GET_UniformMatrix4x3fv(disp) GET_by_offset(disp, _gloffset_UniformMatrix4x3fv)
static void INLINE SET_UniformMatrix4x3fv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix4x3fv, fn);
}

#define CALL_ClampColor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_ClampColor, parameters)
#define GET_ClampColor(disp) GET_by_offset(disp, _gloffset_ClampColor)
static void INLINE SET_ClampColor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_ClampColor, fn);
}

#define CALL_ClearBufferfi(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLfloat, GLint)), _gloffset_ClearBufferfi, parameters)
#define GET_ClearBufferfi(disp) GET_by_offset(disp, _gloffset_ClearBufferfi)
static void INLINE SET_ClearBufferfi(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLfloat, GLint)) {
   SET_by_offset(disp, _gloffset_ClearBufferfi, fn);
}

#define CALL_ClearBufferfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, const GLfloat *)), _gloffset_ClearBufferfv, parameters)
#define GET_ClearBufferfv(disp) GET_by_offset(disp, _gloffset_ClearBufferfv)
static void INLINE SET_ClearBufferfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ClearBufferfv, fn);
}

#define CALL_ClearBufferiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, const GLint *)), _gloffset_ClearBufferiv, parameters)
#define GET_ClearBufferiv(disp) GET_by_offset(disp, _gloffset_ClearBufferiv)
static void INLINE SET_ClearBufferiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, const GLint *)) {
   SET_by_offset(disp, _gloffset_ClearBufferiv, fn);
}

#define CALL_ClearBufferuiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, const GLuint *)), _gloffset_ClearBufferuiv, parameters)
#define GET_ClearBufferuiv(disp) GET_by_offset(disp, _gloffset_ClearBufferuiv)
static void INLINE SET_ClearBufferuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_ClearBufferuiv, fn);
}

#define CALL_GetStringi(disp, parameters) CALL_by_offset(disp, (const GLubyte * (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_GetStringi, parameters)
#define GET_GetStringi(disp) GET_by_offset(disp, _gloffset_GetStringi)
static void INLINE SET_GetStringi(struct _glapi_table *disp, const GLubyte * (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_GetStringi, fn);
}

#define CALL_TexBuffer(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint)), _gloffset_TexBuffer, parameters)
#define GET_TexBuffer(disp) GET_by_offset(disp, _gloffset_TexBuffer)
static void INLINE SET_TexBuffer(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexBuffer, fn);
}

#define CALL_FramebufferTexture(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint, GLint)), _gloffset_FramebufferTexture, parameters)
#define GET_FramebufferTexture(disp) GET_by_offset(disp, _gloffset_FramebufferTexture)
static void INLINE SET_FramebufferTexture(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture, fn);
}

#define CALL_GetBufferParameteri64v(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint64 *)), _gloffset_GetBufferParameteri64v, parameters)
#define GET_GetBufferParameteri64v(disp) GET_by_offset(disp, _gloffset_GetBufferParameteri64v)
static void INLINE SET_GetBufferParameteri64v(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint64 *)) {
   SET_by_offset(disp, _gloffset_GetBufferParameteri64v, fn);
}

#define CALL_GetInteger64i_v(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLint64 *)), _gloffset_GetInteger64i_v, parameters)
#define GET_GetInteger64i_v(disp) GET_by_offset(disp, _gloffset_GetInteger64i_v)
static void INLINE SET_GetInteger64i_v(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLint64 *)) {
   SET_by_offset(disp, _gloffset_GetInteger64i_v, fn);
}

#define CALL_VertexAttribDivisor(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_VertexAttribDivisor, parameters)
#define GET_VertexAttribDivisor(disp) GET_by_offset(disp, _gloffset_VertexAttribDivisor)
static void INLINE SET_VertexAttribDivisor(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribDivisor, fn);
}

#define CALL_LoadTransposeMatrixdARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_LoadTransposeMatrixdARB, parameters)
#define GET_LoadTransposeMatrixdARB(disp) GET_by_offset(disp, _gloffset_LoadTransposeMatrixdARB)
static void INLINE SET_LoadTransposeMatrixdARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_LoadTransposeMatrixdARB, fn);
}

#define CALL_LoadTransposeMatrixfARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_LoadTransposeMatrixfARB, parameters)
#define GET_LoadTransposeMatrixfARB(disp) GET_by_offset(disp, _gloffset_LoadTransposeMatrixfARB)
static void INLINE SET_LoadTransposeMatrixfARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_LoadTransposeMatrixfARB, fn);
}

#define CALL_MultTransposeMatrixdARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_MultTransposeMatrixdARB, parameters)
#define GET_MultTransposeMatrixdARB(disp) GET_by_offset(disp, _gloffset_MultTransposeMatrixdARB)
static void INLINE SET_MultTransposeMatrixdARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_MultTransposeMatrixdARB, fn);
}

#define CALL_MultTransposeMatrixfARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_MultTransposeMatrixfARB, parameters)
#define GET_MultTransposeMatrixfARB(disp) GET_by_offset(disp, _gloffset_MultTransposeMatrixfARB)
static void INLINE SET_MultTransposeMatrixfARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_MultTransposeMatrixfARB, fn);
}

#define CALL_SampleCoverageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLboolean)), _gloffset_SampleCoverageARB, parameters)
#define GET_SampleCoverageARB(disp) GET_by_offset(disp, _gloffset_SampleCoverageARB)
static void INLINE SET_SampleCoverageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLboolean)) {
   SET_by_offset(disp, _gloffset_SampleCoverageARB, fn);
}

#define CALL_CompressedTexImage1DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *)), _gloffset_CompressedTexImage1DARB, parameters)
#define GET_CompressedTexImage1DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexImage1DARB)
static void INLINE SET_CompressedTexImage1DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexImage1DARB, fn);
}

#define CALL_CompressedTexImage2DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)), _gloffset_CompressedTexImage2DARB, parameters)
#define GET_CompressedTexImage2DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexImage2DARB)
static void INLINE SET_CompressedTexImage2DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexImage2DARB, fn);
}

#define CALL_CompressedTexImage3DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)), _gloffset_CompressedTexImage3DARB, parameters)
#define GET_CompressedTexImage3DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexImage3DARB)
static void INLINE SET_CompressedTexImage3DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexImage3DARB, fn);
}

#define CALL_CompressedTexSubImage1DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *)), _gloffset_CompressedTexSubImage1DARB, parameters)
#define GET_CompressedTexSubImage1DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexSubImage1DARB)
static void INLINE SET_CompressedTexSubImage1DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexSubImage1DARB, fn);
}

#define CALL_CompressedTexSubImage2DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)), _gloffset_CompressedTexSubImage2DARB, parameters)
#define GET_CompressedTexSubImage2DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexSubImage2DARB)
static void INLINE SET_CompressedTexSubImage2DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexSubImage2DARB, fn);
}

#define CALL_CompressedTexSubImage3DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)), _gloffset_CompressedTexSubImage3DARB, parameters)
#define GET_CompressedTexSubImage3DARB(disp) GET_by_offset(disp, _gloffset_CompressedTexSubImage3DARB)
static void INLINE SET_CompressedTexSubImage3DARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_CompressedTexSubImage3DARB, fn);
}

#define CALL_GetCompressedTexImageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLvoid *)), _gloffset_GetCompressedTexImageARB, parameters)
#define GET_GetCompressedTexImageARB(disp) GET_by_offset(disp, _gloffset_GetCompressedTexImageARB)
static void INLINE SET_GetCompressedTexImageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetCompressedTexImageARB, fn);
}

#define CALL_DisableVertexAttribArrayARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DisableVertexAttribArrayARB, parameters)
#define GET_DisableVertexAttribArrayARB(disp) GET_by_offset(disp, _gloffset_DisableVertexAttribArrayARB)
static void INLINE SET_DisableVertexAttribArrayARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DisableVertexAttribArrayARB, fn);
}

#define CALL_EnableVertexAttribArrayARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_EnableVertexAttribArrayARB, parameters)
#define GET_EnableVertexAttribArrayARB(disp) GET_by_offset(disp, _gloffset_EnableVertexAttribArrayARB)
static void INLINE SET_EnableVertexAttribArrayARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_EnableVertexAttribArrayARB, fn);
}

#define CALL_GetProgramEnvParameterdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble *)), _gloffset_GetProgramEnvParameterdvARB, parameters)
#define GET_GetProgramEnvParameterdvARB(disp) GET_by_offset(disp, _gloffset_GetProgramEnvParameterdvARB)
static void INLINE SET_GetProgramEnvParameterdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramEnvParameterdvARB, fn);
}

#define CALL_GetProgramEnvParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat *)), _gloffset_GetProgramEnvParameterfvARB, parameters)
#define GET_GetProgramEnvParameterfvARB(disp) GET_by_offset(disp, _gloffset_GetProgramEnvParameterfvARB)
static void INLINE SET_GetProgramEnvParameterfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramEnvParameterfvARB, fn);
}

#define CALL_GetProgramLocalParameterdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble *)), _gloffset_GetProgramLocalParameterdvARB, parameters)
#define GET_GetProgramLocalParameterdvARB(disp) GET_by_offset(disp, _gloffset_GetProgramLocalParameterdvARB)
static void INLINE SET_GetProgramLocalParameterdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramLocalParameterdvARB, fn);
}

#define CALL_GetProgramLocalParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat *)), _gloffset_GetProgramLocalParameterfvARB, parameters)
#define GET_GetProgramLocalParameterfvARB(disp) GET_by_offset(disp, _gloffset_GetProgramLocalParameterfvARB)
static void INLINE SET_GetProgramLocalParameterfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramLocalParameterfvARB, fn);
}

#define CALL_GetProgramStringARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid *)), _gloffset_GetProgramStringARB, parameters)
#define GET_GetProgramStringARB(disp) GET_by_offset(disp, _gloffset_GetProgramStringARB)
static void INLINE SET_GetProgramStringARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetProgramStringARB, fn);
}

#define CALL_GetProgramivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetProgramivARB, parameters)
#define GET_GetProgramivARB(disp) GET_by_offset(disp, _gloffset_GetProgramivARB)
static void INLINE SET_GetProgramivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetProgramivARB, fn);
}

#define CALL_GetVertexAttribdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLdouble *)), _gloffset_GetVertexAttribdvARB, parameters)
#define GET_GetVertexAttribdvARB(disp) GET_by_offset(disp, _gloffset_GetVertexAttribdvARB)
static void INLINE SET_GetVertexAttribdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribdvARB, fn);
}

#define CALL_GetVertexAttribfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat *)), _gloffset_GetVertexAttribfvARB, parameters)
#define GET_GetVertexAttribfvARB(disp) GET_by_offset(disp, _gloffset_GetVertexAttribfvARB)
static void INLINE SET_GetVertexAttribfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribfvARB, fn);
}

#define CALL_GetVertexAttribivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetVertexAttribivARB, parameters)
#define GET_GetVertexAttribivARB(disp) GET_by_offset(disp, _gloffset_GetVertexAttribivARB)
static void INLINE SET_GetVertexAttribivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribivARB, fn);
}

#define CALL_ProgramEnvParameter4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_ProgramEnvParameter4dARB, parameters)
#define GET_ProgramEnvParameter4dARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4dARB)
static void INLINE SET_ProgramEnvParameter4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4dARB, fn);
}

#define CALL_ProgramEnvParameter4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLdouble *)), _gloffset_ProgramEnvParameter4dvARB, parameters)
#define GET_ProgramEnvParameter4dvARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4dvARB)
static void INLINE SET_ProgramEnvParameter4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4dvARB, fn);
}

#define CALL_ProgramEnvParameter4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ProgramEnvParameter4fARB, parameters)
#define GET_ProgramEnvParameter4fARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4fARB)
static void INLINE SET_ProgramEnvParameter4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4fARB, fn);
}

#define CALL_ProgramEnvParameter4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), _gloffset_ProgramEnvParameter4fvARB, parameters)
#define GET_ProgramEnvParameter4fvARB(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameter4fvARB)
static void INLINE SET_ProgramEnvParameter4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameter4fvARB, fn);
}

#define CALL_ProgramLocalParameter4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_ProgramLocalParameter4dARB, parameters)
#define GET_ProgramLocalParameter4dARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4dARB)
static void INLINE SET_ProgramLocalParameter4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4dARB, fn);
}

#define CALL_ProgramLocalParameter4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLdouble *)), _gloffset_ProgramLocalParameter4dvARB, parameters)
#define GET_ProgramLocalParameter4dvARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4dvARB)
static void INLINE SET_ProgramLocalParameter4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4dvARB, fn);
}

#define CALL_ProgramLocalParameter4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ProgramLocalParameter4fARB, parameters)
#define GET_ProgramLocalParameter4fARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4fARB)
static void INLINE SET_ProgramLocalParameter4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4fARB, fn);
}

#define CALL_ProgramLocalParameter4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), _gloffset_ProgramLocalParameter4fvARB, parameters)
#define GET_ProgramLocalParameter4fvARB(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameter4fvARB)
static void INLINE SET_ProgramLocalParameter4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameter4fvARB, fn);
}

#define CALL_ProgramStringARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, const GLvoid *)), _gloffset_ProgramStringARB, parameters)
#define GET_ProgramStringARB(disp) GET_by_offset(disp, _gloffset_ProgramStringARB)
static void INLINE SET_ProgramStringARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ProgramStringARB, fn);
}

#define CALL_VertexAttrib1dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble)), _gloffset_VertexAttrib1dARB, parameters)
#define GET_VertexAttrib1dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dARB)
static void INLINE SET_VertexAttrib1dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dARB, fn);
}

#define CALL_VertexAttrib1dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib1dvARB, parameters)
#define GET_VertexAttrib1dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dvARB)
static void INLINE SET_VertexAttrib1dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dvARB, fn);
}

#define CALL_VertexAttrib1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat)), _gloffset_VertexAttrib1fARB, parameters)
#define GET_VertexAttrib1fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fARB)
static void INLINE SET_VertexAttrib1fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fARB, fn);
}

#define CALL_VertexAttrib1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib1fvARB, parameters)
#define GET_VertexAttrib1fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fvARB)
static void INLINE SET_VertexAttrib1fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fvARB, fn);
}

#define CALL_VertexAttrib1sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort)), _gloffset_VertexAttrib1sARB, parameters)
#define GET_VertexAttrib1sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1sARB)
static void INLINE SET_VertexAttrib1sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1sARB, fn);
}

#define CALL_VertexAttrib1svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib1svARB, parameters)
#define GET_VertexAttrib1svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib1svARB)
static void INLINE SET_VertexAttrib1svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1svARB, fn);
}

#define CALL_VertexAttrib2dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble)), _gloffset_VertexAttrib2dARB, parameters)
#define GET_VertexAttrib2dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dARB)
static void INLINE SET_VertexAttrib2dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dARB, fn);
}

#define CALL_VertexAttrib2dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib2dvARB, parameters)
#define GET_VertexAttrib2dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dvARB)
static void INLINE SET_VertexAttrib2dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dvARB, fn);
}

#define CALL_VertexAttrib2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat)), _gloffset_VertexAttrib2fARB, parameters)
#define GET_VertexAttrib2fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fARB)
static void INLINE SET_VertexAttrib2fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fARB, fn);
}

#define CALL_VertexAttrib2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib2fvARB, parameters)
#define GET_VertexAttrib2fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fvARB)
static void INLINE SET_VertexAttrib2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fvARB, fn);
}

#define CALL_VertexAttrib2sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort)), _gloffset_VertexAttrib2sARB, parameters)
#define GET_VertexAttrib2sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2sARB)
static void INLINE SET_VertexAttrib2sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2sARB, fn);
}

#define CALL_VertexAttrib2svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib2svARB, parameters)
#define GET_VertexAttrib2svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib2svARB)
static void INLINE SET_VertexAttrib2svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2svARB, fn);
}

#define CALL_VertexAttrib3dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib3dARB, parameters)
#define GET_VertexAttrib3dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dARB)
static void INLINE SET_VertexAttrib3dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dARB, fn);
}

#define CALL_VertexAttrib3dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib3dvARB, parameters)
#define GET_VertexAttrib3dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dvARB)
static void INLINE SET_VertexAttrib3dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dvARB, fn);
}

#define CALL_VertexAttrib3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib3fARB, parameters)
#define GET_VertexAttrib3fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fARB)
static void INLINE SET_VertexAttrib3fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fARB, fn);
}

#define CALL_VertexAttrib3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib3fvARB, parameters)
#define GET_VertexAttrib3fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fvARB)
static void INLINE SET_VertexAttrib3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fvARB, fn);
}

#define CALL_VertexAttrib3sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib3sARB, parameters)
#define GET_VertexAttrib3sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3sARB)
static void INLINE SET_VertexAttrib3sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3sARB, fn);
}

#define CALL_VertexAttrib3svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib3svARB, parameters)
#define GET_VertexAttrib3svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib3svARB)
static void INLINE SET_VertexAttrib3svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3svARB, fn);
}

#define CALL_VertexAttrib4NbvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), _gloffset_VertexAttrib4NbvARB, parameters)
#define GET_VertexAttrib4NbvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NbvARB)
static void INLINE SET_VertexAttrib4NbvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLbyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NbvARB, fn);
}

#define CALL_VertexAttrib4NivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttrib4NivARB, parameters)
#define GET_VertexAttrib4NivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NivARB)
static void INLINE SET_VertexAttrib4NivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NivARB, fn);
}

#define CALL_VertexAttrib4NsvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib4NsvARB, parameters)
#define GET_VertexAttrib4NsvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NsvARB)
static void INLINE SET_VertexAttrib4NsvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NsvARB, fn);
}

#define CALL_VertexAttrib4NubARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)), _gloffset_VertexAttrib4NubARB, parameters)
#define GET_VertexAttrib4NubARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NubARB)
static void INLINE SET_VertexAttrib4NubARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NubARB, fn);
}

#define CALL_VertexAttrib4NubvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttrib4NubvARB, parameters)
#define GET_VertexAttrib4NubvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NubvARB)
static void INLINE SET_VertexAttrib4NubvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NubvARB, fn);
}

#define CALL_VertexAttrib4NuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttrib4NuivARB, parameters)
#define GET_VertexAttrib4NuivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NuivARB)
static void INLINE SET_VertexAttrib4NuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NuivARB, fn);
}

#define CALL_VertexAttrib4NusvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), _gloffset_VertexAttrib4NusvARB, parameters)
#define GET_VertexAttrib4NusvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4NusvARB)
static void INLINE SET_VertexAttrib4NusvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLushort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4NusvARB, fn);
}

#define CALL_VertexAttrib4bvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), _gloffset_VertexAttrib4bvARB, parameters)
#define GET_VertexAttrib4bvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4bvARB)
static void INLINE SET_VertexAttrib4bvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLbyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4bvARB, fn);
}

#define CALL_VertexAttrib4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib4dARB, parameters)
#define GET_VertexAttrib4dARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dARB)
static void INLINE SET_VertexAttrib4dARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dARB, fn);
}

#define CALL_VertexAttrib4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib4dvARB, parameters)
#define GET_VertexAttrib4dvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dvARB)
static void INLINE SET_VertexAttrib4dvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dvARB, fn);
}

#define CALL_VertexAttrib4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib4fARB, parameters)
#define GET_VertexAttrib4fARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fARB)
static void INLINE SET_VertexAttrib4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fARB, fn);
}

#define CALL_VertexAttrib4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib4fvARB, parameters)
#define GET_VertexAttrib4fvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fvARB)
static void INLINE SET_VertexAttrib4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fvARB, fn);
}

#define CALL_VertexAttrib4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttrib4ivARB, parameters)
#define GET_VertexAttrib4ivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ivARB)
static void INLINE SET_VertexAttrib4ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ivARB, fn);
}

#define CALL_VertexAttrib4sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib4sARB, parameters)
#define GET_VertexAttrib4sARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4sARB)
static void INLINE SET_VertexAttrib4sARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4sARB, fn);
}

#define CALL_VertexAttrib4svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib4svARB, parameters)
#define GET_VertexAttrib4svARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4svARB)
static void INLINE SET_VertexAttrib4svARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4svARB, fn);
}

#define CALL_VertexAttrib4ubvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttrib4ubvARB, parameters)
#define GET_VertexAttrib4ubvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ubvARB)
static void INLINE SET_VertexAttrib4ubvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ubvARB, fn);
}

#define CALL_VertexAttrib4uivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttrib4uivARB, parameters)
#define GET_VertexAttrib4uivARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4uivARB)
static void INLINE SET_VertexAttrib4uivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4uivARB, fn);
}

#define CALL_VertexAttrib4usvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), _gloffset_VertexAttrib4usvARB, parameters)
#define GET_VertexAttrib4usvARB(disp) GET_by_offset(disp, _gloffset_VertexAttrib4usvARB)
static void INLINE SET_VertexAttrib4usvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLushort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4usvARB, fn);
}

#define CALL_VertexAttribPointerARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *)), _gloffset_VertexAttribPointerARB, parameters)
#define GET_VertexAttribPointerARB(disp) GET_by_offset(disp, _gloffset_VertexAttribPointerARB)
static void INLINE SET_VertexAttribPointerARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexAttribPointerARB, fn);
}

#define CALL_BindBufferARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindBufferARB, parameters)
#define GET_BindBufferARB(disp) GET_by_offset(disp, _gloffset_BindBufferARB)
static void INLINE SET_BindBufferARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindBufferARB, fn);
}

#define CALL_BufferDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum)), _gloffset_BufferDataARB, parameters)
#define GET_BufferDataARB(disp) GET_by_offset(disp, _gloffset_BufferDataARB)
static void INLINE SET_BufferDataARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum)) {
   SET_by_offset(disp, _gloffset_BufferDataARB, fn);
}

#define CALL_BufferSubDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *)), _gloffset_BufferSubDataARB, parameters)
#define GET_BufferSubDataARB(disp) GET_by_offset(disp, _gloffset_BufferSubDataARB)
static void INLINE SET_BufferSubDataARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_BufferSubDataARB, fn);
}

#define CALL_DeleteBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteBuffersARB, parameters)
#define GET_DeleteBuffersARB(disp) GET_by_offset(disp, _gloffset_DeleteBuffersARB)
static void INLINE SET_DeleteBuffersARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteBuffersARB, fn);
}

#define CALL_GenBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenBuffersARB, parameters)
#define GET_GenBuffersARB(disp) GET_by_offset(disp, _gloffset_GenBuffersARB)
static void INLINE SET_GenBuffersARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenBuffersARB, fn);
}

#define CALL_GetBufferParameterivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetBufferParameterivARB, parameters)
#define GET_GetBufferParameterivARB(disp) GET_by_offset(disp, _gloffset_GetBufferParameterivARB)
static void INLINE SET_GetBufferParameterivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetBufferParameterivARB, fn);
}

#define CALL_GetBufferPointervARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid **)), _gloffset_GetBufferPointervARB, parameters)
#define GET_GetBufferPointervARB(disp) GET_by_offset(disp, _gloffset_GetBufferPointervARB)
static void INLINE SET_GetBufferPointervARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetBufferPointervARB, fn);
}

#define CALL_GetBufferSubDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *)), _gloffset_GetBufferSubDataARB, parameters)
#define GET_GetBufferSubDataARB(disp) GET_by_offset(disp, _gloffset_GetBufferSubDataARB)
static void INLINE SET_GetBufferSubDataARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetBufferSubDataARB, fn);
}

#define CALL_IsBufferARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsBufferARB, parameters)
#define GET_IsBufferARB(disp) GET_by_offset(disp, _gloffset_IsBufferARB)
static void INLINE SET_IsBufferARB(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsBufferARB, fn);
}

#define CALL_MapBufferARB(disp, parameters) CALL_by_offset(disp, (GLvoid * (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_MapBufferARB, parameters)
#define GET_MapBufferARB(disp) GET_by_offset(disp, _gloffset_MapBufferARB)
static void INLINE SET_MapBufferARB(struct _glapi_table *disp, GLvoid * (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_MapBufferARB, fn);
}

#define CALL_UnmapBufferARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLenum)), _gloffset_UnmapBufferARB, parameters)
#define GET_UnmapBufferARB(disp) GET_by_offset(disp, _gloffset_UnmapBufferARB)
static void INLINE SET_UnmapBufferARB(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_UnmapBufferARB, fn);
}

#define CALL_BeginQueryARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BeginQueryARB, parameters)
#define GET_BeginQueryARB(disp) GET_by_offset(disp, _gloffset_BeginQueryARB)
static void INLINE SET_BeginQueryARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BeginQueryARB, fn);
}

#define CALL_DeleteQueriesARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteQueriesARB, parameters)
#define GET_DeleteQueriesARB(disp) GET_by_offset(disp, _gloffset_DeleteQueriesARB)
static void INLINE SET_DeleteQueriesARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteQueriesARB, fn);
}

#define CALL_EndQueryARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_EndQueryARB, parameters)
#define GET_EndQueryARB(disp) GET_by_offset(disp, _gloffset_EndQueryARB)
static void INLINE SET_EndQueryARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_EndQueryARB, fn);
}

#define CALL_GenQueriesARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenQueriesARB, parameters)
#define GET_GenQueriesARB(disp) GET_by_offset(disp, _gloffset_GenQueriesARB)
static void INLINE SET_GenQueriesARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenQueriesARB, fn);
}

#define CALL_GetQueryObjectivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetQueryObjectivARB, parameters)
#define GET_GetQueryObjectivARB(disp) GET_by_offset(disp, _gloffset_GetQueryObjectivARB)
static void INLINE SET_GetQueryObjectivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjectivARB, fn);
}

#define CALL_GetQueryObjectuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint *)), _gloffset_GetQueryObjectuivARB, parameters)
#define GET_GetQueryObjectuivARB(disp) GET_by_offset(disp, _gloffset_GetQueryObjectuivARB)
static void INLINE SET_GetQueryObjectuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjectuivARB, fn);
}

#define CALL_GetQueryivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetQueryivARB, parameters)
#define GET_GetQueryivARB(disp) GET_by_offset(disp, _gloffset_GetQueryivARB)
static void INLINE SET_GetQueryivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetQueryivARB, fn);
}

#define CALL_IsQueryARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsQueryARB, parameters)
#define GET_IsQueryARB(disp) GET_by_offset(disp, _gloffset_IsQueryARB)
static void INLINE SET_IsQueryARB(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsQueryARB, fn);
}

#define CALL_AttachObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLhandleARB)), _gloffset_AttachObjectARB, parameters)
#define GET_AttachObjectARB(disp) GET_by_offset(disp, _gloffset_AttachObjectARB)
static void INLINE SET_AttachObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLhandleARB)) {
   SET_by_offset(disp, _gloffset_AttachObjectARB, fn);
}

#define CALL_CompileShaderARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_CompileShaderARB, parameters)
#define GET_CompileShaderARB(disp) GET_by_offset(disp, _gloffset_CompileShaderARB)
static void INLINE SET_CompileShaderARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_CompileShaderARB, fn);
}

#define CALL_CreateProgramObjectARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(void)), _gloffset_CreateProgramObjectARB, parameters)
#define GET_CreateProgramObjectARB(disp) GET_by_offset(disp, _gloffset_CreateProgramObjectARB)
static void INLINE SET_CreateProgramObjectARB(struct _glapi_table *disp, GLhandleARB (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_CreateProgramObjectARB, fn);
}

#define CALL_CreateShaderObjectARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(GLenum)), _gloffset_CreateShaderObjectARB, parameters)
#define GET_CreateShaderObjectARB(disp) GET_by_offset(disp, _gloffset_CreateShaderObjectARB)
static void INLINE SET_CreateShaderObjectARB(struct _glapi_table *disp, GLhandleARB (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CreateShaderObjectARB, fn);
}

#define CALL_DeleteObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_DeleteObjectARB, parameters)
#define GET_DeleteObjectARB(disp) GET_by_offset(disp, _gloffset_DeleteObjectARB)
static void INLINE SET_DeleteObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_DeleteObjectARB, fn);
}

#define CALL_DetachObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLhandleARB)), _gloffset_DetachObjectARB, parameters)
#define GET_DetachObjectARB(disp) GET_by_offset(disp, _gloffset_DetachObjectARB)
static void INLINE SET_DetachObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLhandleARB)) {
   SET_by_offset(disp, _gloffset_DetachObjectARB, fn);
}

#define CALL_GetActiveUniformARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)), _gloffset_GetActiveUniformARB, parameters)
#define GET_GetActiveUniformARB(disp) GET_by_offset(disp, _gloffset_GetActiveUniformARB)
static void INLINE SET_GetActiveUniformARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetActiveUniformARB, fn);
}

#define CALL_GetAttachedObjectsARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *)), _gloffset_GetAttachedObjectsARB, parameters)
#define GET_GetAttachedObjectsARB(disp) GET_by_offset(disp, _gloffset_GetAttachedObjectsARB)
static void INLINE SET_GetAttachedObjectsARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *)) {
   SET_by_offset(disp, _gloffset_GetAttachedObjectsARB, fn);
}

#define CALL_GetHandleARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(GLenum)), _gloffset_GetHandleARB, parameters)
#define GET_GetHandleARB(disp) GET_by_offset(disp, _gloffset_GetHandleARB)
static void INLINE SET_GetHandleARB(struct _glapi_table *disp, GLhandleARB (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_GetHandleARB, fn);
}

#define CALL_GetInfoLogARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)), _gloffset_GetInfoLogARB, parameters)
#define GET_GetInfoLogARB(disp) GET_by_offset(disp, _gloffset_GetInfoLogARB)
static void INLINE SET_GetInfoLogARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetInfoLogARB, fn);
}

#define CALL_GetObjectParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLenum, GLfloat *)), _gloffset_GetObjectParameterfvARB, parameters)
#define GET_GetObjectParameterfvARB(disp) GET_by_offset(disp, _gloffset_GetObjectParameterfvARB)
static void INLINE SET_GetObjectParameterfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetObjectParameterfvARB, fn);
}

#define CALL_GetObjectParameterivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLenum, GLint *)), _gloffset_GetObjectParameterivARB, parameters)
#define GET_GetObjectParameterivARB(disp) GET_by_offset(disp, _gloffset_GetObjectParameterivARB)
static void INLINE SET_GetObjectParameterivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetObjectParameterivARB, fn);
}

#define CALL_GetShaderSourceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)), _gloffset_GetShaderSourceARB, parameters)
#define GET_GetShaderSourceARB(disp) GET_by_offset(disp, _gloffset_GetShaderSourceARB)
static void INLINE SET_GetShaderSourceARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetShaderSourceARB, fn);
}

#define CALL_GetUniformLocationARB(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLhandleARB, const GLcharARB *)), _gloffset_GetUniformLocationARB, parameters)
#define GET_GetUniformLocationARB(disp) GET_by_offset(disp, _gloffset_GetUniformLocationARB)
static void INLINE SET_GetUniformLocationARB(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLhandleARB, const GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetUniformLocationARB, fn);
}

#define CALL_GetUniformfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLfloat *)), _gloffset_GetUniformfvARB, parameters)
#define GET_GetUniformfvARB(disp) GET_by_offset(disp, _gloffset_GetUniformfvARB)
static void INLINE SET_GetUniformfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetUniformfvARB, fn);
}

#define CALL_GetUniformivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLint *)), _gloffset_GetUniformivARB, parameters)
#define GET_GetUniformivARB(disp) GET_by_offset(disp, _gloffset_GetUniformivARB)
static void INLINE SET_GetUniformivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLint *)) {
   SET_by_offset(disp, _gloffset_GetUniformivARB, fn);
}

#define CALL_LinkProgramARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_LinkProgramARB, parameters)
#define GET_LinkProgramARB(disp) GET_by_offset(disp, _gloffset_LinkProgramARB)
static void INLINE SET_LinkProgramARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_LinkProgramARB, fn);
}

#define CALL_ShaderSourceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *)), _gloffset_ShaderSourceARB, parameters)
#define GET_ShaderSourceARB(disp) GET_by_offset(disp, _gloffset_ShaderSourceARB)
static void INLINE SET_ShaderSourceARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *)) {
   SET_by_offset(disp, _gloffset_ShaderSourceARB, fn);
}

#define CALL_Uniform1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat)), _gloffset_Uniform1fARB, parameters)
#define GET_Uniform1fARB(disp) GET_by_offset(disp, _gloffset_Uniform1fARB)
static void INLINE SET_Uniform1fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform1fARB, fn);
}

#define CALL_Uniform1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform1fvARB, parameters)
#define GET_Uniform1fvARB(disp) GET_by_offset(disp, _gloffset_Uniform1fvARB)
static void INLINE SET_Uniform1fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform1fvARB, fn);
}

#define CALL_Uniform1iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_Uniform1iARB, parameters)
#define GET_Uniform1iARB(disp) GET_by_offset(disp, _gloffset_Uniform1iARB)
static void INLINE SET_Uniform1iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform1iARB, fn);
}

#define CALL_Uniform1ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform1ivARB, parameters)
#define GET_Uniform1ivARB(disp) GET_by_offset(disp, _gloffset_Uniform1ivARB)
static void INLINE SET_Uniform1ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform1ivARB, fn);
}

#define CALL_Uniform2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat)), _gloffset_Uniform2fARB, parameters)
#define GET_Uniform2fARB(disp) GET_by_offset(disp, _gloffset_Uniform2fARB)
static void INLINE SET_Uniform2fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform2fARB, fn);
}

#define CALL_Uniform2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform2fvARB, parameters)
#define GET_Uniform2fvARB(disp) GET_by_offset(disp, _gloffset_Uniform2fvARB)
static void INLINE SET_Uniform2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform2fvARB, fn);
}

#define CALL_Uniform2iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_Uniform2iARB, parameters)
#define GET_Uniform2iARB(disp) GET_by_offset(disp, _gloffset_Uniform2iARB)
static void INLINE SET_Uniform2iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform2iARB, fn);
}

#define CALL_Uniform2ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform2ivARB, parameters)
#define GET_Uniform2ivARB(disp) GET_by_offset(disp, _gloffset_Uniform2ivARB)
static void INLINE SET_Uniform2ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform2ivARB, fn);
}

#define CALL_Uniform3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLfloat)), _gloffset_Uniform3fARB, parameters)
#define GET_Uniform3fARB(disp) GET_by_offset(disp, _gloffset_Uniform3fARB)
static void INLINE SET_Uniform3fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform3fARB, fn);
}

#define CALL_Uniform3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform3fvARB, parameters)
#define GET_Uniform3fvARB(disp) GET_by_offset(disp, _gloffset_Uniform3fvARB)
static void INLINE SET_Uniform3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform3fvARB, fn);
}

#define CALL_Uniform3iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_Uniform3iARB, parameters)
#define GET_Uniform3iARB(disp) GET_by_offset(disp, _gloffset_Uniform3iARB)
static void INLINE SET_Uniform3iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform3iARB, fn);
}

#define CALL_Uniform3ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform3ivARB, parameters)
#define GET_Uniform3ivARB(disp) GET_by_offset(disp, _gloffset_Uniform3ivARB)
static void INLINE SET_Uniform3ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform3ivARB, fn);
}

#define CALL_Uniform4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_Uniform4fARB, parameters)
#define GET_Uniform4fARB(disp) GET_by_offset(disp, _gloffset_Uniform4fARB)
static void INLINE SET_Uniform4fARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_Uniform4fARB, fn);
}

#define CALL_Uniform4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), _gloffset_Uniform4fvARB, parameters)
#define GET_Uniform4fvARB(disp) GET_by_offset(disp, _gloffset_Uniform4fvARB)
static void INLINE SET_Uniform4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_Uniform4fvARB, fn);
}

#define CALL_Uniform4iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint, GLint)), _gloffset_Uniform4iARB, parameters)
#define GET_Uniform4iARB(disp) GET_by_offset(disp, _gloffset_Uniform4iARB)
static void INLINE SET_Uniform4iARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_Uniform4iARB, fn);
}

#define CALL_Uniform4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), _gloffset_Uniform4ivARB, parameters)
#define GET_Uniform4ivARB(disp) GET_by_offset(disp, _gloffset_Uniform4ivARB)
static void INLINE SET_Uniform4ivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_Uniform4ivARB, fn);
}

#define CALL_UniformMatrix2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix2fvARB, parameters)
#define GET_UniformMatrix2fvARB(disp) GET_by_offset(disp, _gloffset_UniformMatrix2fvARB)
static void INLINE SET_UniformMatrix2fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix2fvARB, fn);
}

#define CALL_UniformMatrix3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix3fvARB, parameters)
#define GET_UniformMatrix3fvARB(disp) GET_by_offset(disp, _gloffset_UniformMatrix3fvARB)
static void INLINE SET_UniformMatrix3fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix3fvARB, fn);
}

#define CALL_UniformMatrix4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), _gloffset_UniformMatrix4fvARB, parameters)
#define GET_UniformMatrix4fvARB(disp) GET_by_offset(disp, _gloffset_UniformMatrix4fvARB)
static void INLINE SET_UniformMatrix4fvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, GLboolean, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_UniformMatrix4fvARB, fn);
}

#define CALL_UseProgramObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_UseProgramObjectARB, parameters)
#define GET_UseProgramObjectARB(disp) GET_by_offset(disp, _gloffset_UseProgramObjectARB)
static void INLINE SET_UseProgramObjectARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_UseProgramObjectARB, fn);
}

#define CALL_ValidateProgramARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), _gloffset_ValidateProgramARB, parameters)
#define GET_ValidateProgramARB(disp) GET_by_offset(disp, _gloffset_ValidateProgramARB)
static void INLINE SET_ValidateProgramARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB)) {
   SET_by_offset(disp, _gloffset_ValidateProgramARB, fn);
}

#define CALL_BindAttribLocationARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, const GLcharARB *)), _gloffset_BindAttribLocationARB, parameters)
#define GET_BindAttribLocationARB(disp) GET_by_offset(disp, _gloffset_BindAttribLocationARB)
static void INLINE SET_BindAttribLocationARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLuint, const GLcharARB *)) {
   SET_by_offset(disp, _gloffset_BindAttribLocationARB, fn);
}

#define CALL_GetActiveAttribARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)), _gloffset_GetActiveAttribARB, parameters)
#define GET_GetActiveAttribARB(disp) GET_by_offset(disp, _gloffset_GetActiveAttribARB)
static void INLINE SET_GetActiveAttribARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetActiveAttribARB, fn);
}

#define CALL_GetAttribLocationARB(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLhandleARB, const GLcharARB *)), _gloffset_GetAttribLocationARB, parameters)
#define GET_GetAttribLocationARB(disp) GET_by_offset(disp, _gloffset_GetAttribLocationARB)
static void INLINE SET_GetAttribLocationARB(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLhandleARB, const GLcharARB *)) {
   SET_by_offset(disp, _gloffset_GetAttribLocationARB, fn);
}

#define CALL_DrawBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLenum *)), _gloffset_DrawBuffersARB, parameters)
#define GET_DrawBuffersARB(disp) GET_by_offset(disp, _gloffset_DrawBuffersARB)
static void INLINE SET_DrawBuffersARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLenum *)) {
   SET_by_offset(disp, _gloffset_DrawBuffersARB, fn);
}

#define CALL_ClampColorARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_ClampColorARB, parameters)
#define GET_ClampColorARB(disp) GET_by_offset(disp, _gloffset_ClampColorARB)
static void INLINE SET_ClampColorARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_ClampColorARB, fn);
}

#define CALL_DrawArraysInstancedARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLsizei, GLsizei)), _gloffset_DrawArraysInstancedARB, parameters)
#define GET_DrawArraysInstancedARB(disp) GET_by_offset(disp, _gloffset_DrawArraysInstancedARB)
static void INLINE SET_DrawArraysInstancedARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_DrawArraysInstancedARB, fn);
}

#define CALL_DrawElementsInstancedARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei)), _gloffset_DrawElementsInstancedARB, parameters)
#define GET_DrawElementsInstancedARB(disp) GET_by_offset(disp, _gloffset_DrawElementsInstancedARB)
static void INLINE SET_DrawElementsInstancedARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, const GLvoid *, GLsizei)) {
   SET_by_offset(disp, _gloffset_DrawElementsInstancedARB, fn);
}

#define CALL_RenderbufferStorageMultisample(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, GLsizei, GLsizei)), _gloffset_RenderbufferStorageMultisample, parameters)
#define GET_RenderbufferStorageMultisample(disp) GET_by_offset(disp, _gloffset_RenderbufferStorageMultisample)
static void INLINE SET_RenderbufferStorageMultisample(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_RenderbufferStorageMultisample, fn);
}

#define CALL_FramebufferTextureARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint, GLint)), _gloffset_FramebufferTextureARB, parameters)
#define GET_FramebufferTextureARB(disp) GET_by_offset(disp, _gloffset_FramebufferTextureARB)
static void INLINE SET_FramebufferTextureARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTextureARB, fn);
}

#define CALL_FramebufferTextureFaceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint, GLint, GLenum)), _gloffset_FramebufferTextureFaceARB, parameters)
#define GET_FramebufferTextureFaceARB(disp) GET_by_offset(disp, _gloffset_FramebufferTextureFaceARB)
static void INLINE SET_FramebufferTextureFaceARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint, GLenum)) {
   SET_by_offset(disp, _gloffset_FramebufferTextureFaceARB, fn);
}

#define CALL_ProgramParameteriARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint)), _gloffset_ProgramParameteriARB, parameters)
#define GET_ProgramParameteriARB(disp) GET_by_offset(disp, _gloffset_ProgramParameteriARB)
static void INLINE SET_ProgramParameteriARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_ProgramParameteriARB, fn);
}

#define CALL_VertexAttribDivisorARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_VertexAttribDivisorARB, parameters)
#define GET_VertexAttribDivisorARB(disp) GET_by_offset(disp, _gloffset_VertexAttribDivisorARB)
static void INLINE SET_VertexAttribDivisorARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribDivisorARB, fn);
}

#define CALL_FlushMappedBufferRange(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptr, GLsizeiptr)), _gloffset_FlushMappedBufferRange, parameters)
#define GET_FlushMappedBufferRange(disp) GET_by_offset(disp, _gloffset_FlushMappedBufferRange)
static void INLINE SET_FlushMappedBufferRange(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_FlushMappedBufferRange, fn);
}

#define CALL_MapBufferRange(disp, parameters) CALL_by_offset(disp, (GLvoid * (GLAPIENTRYP)(GLenum, GLintptr, GLsizeiptr, GLbitfield)), _gloffset_MapBufferRange, parameters)
#define GET_MapBufferRange(disp) GET_by_offset(disp, _gloffset_MapBufferRange)
static void INLINE SET_MapBufferRange(struct _glapi_table *disp, GLvoid * (GLAPIENTRYP fn)(GLenum, GLintptr, GLsizeiptr, GLbitfield)) {
   SET_by_offset(disp, _gloffset_MapBufferRange, fn);
}

#define CALL_TexBufferARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint)), _gloffset_TexBufferARB, parameters)
#define GET_TexBufferARB(disp) GET_by_offset(disp, _gloffset_TexBufferARB)
static void INLINE SET_TexBufferARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_TexBufferARB, fn);
}

#define CALL_BindVertexArray(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_BindVertexArray, parameters)
#define GET_BindVertexArray(disp) GET_by_offset(disp, _gloffset_BindVertexArray)
static void INLINE SET_BindVertexArray(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_BindVertexArray, fn);
}

#define CALL_GenVertexArrays(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenVertexArrays, parameters)
#define GET_GenVertexArrays(disp) GET_by_offset(disp, _gloffset_GenVertexArrays)
static void INLINE SET_GenVertexArrays(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenVertexArrays, fn);
}

#define CALL_CopyBufferSubData(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr)), _gloffset_CopyBufferSubData, parameters)
#define GET_CopyBufferSubData(disp) GET_by_offset(disp, _gloffset_CopyBufferSubData)
static void INLINE SET_CopyBufferSubData(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLintptr, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_CopyBufferSubData, fn);
}

#define CALL_ClientWaitSync(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLsync, GLbitfield, GLuint64)), _gloffset_ClientWaitSync, parameters)
#define GET_ClientWaitSync(disp) GET_by_offset(disp, _gloffset_ClientWaitSync)
static void INLINE SET_ClientWaitSync(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLsync, GLbitfield, GLuint64)) {
   SET_by_offset(disp, _gloffset_ClientWaitSync, fn);
}

#define CALL_DeleteSync(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsync)), _gloffset_DeleteSync, parameters)
#define GET_DeleteSync(disp) GET_by_offset(disp, _gloffset_DeleteSync)
static void INLINE SET_DeleteSync(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsync)) {
   SET_by_offset(disp, _gloffset_DeleteSync, fn);
}

#define CALL_FenceSync(disp, parameters) CALL_by_offset(disp, (GLsync (GLAPIENTRYP)(GLenum, GLbitfield)), _gloffset_FenceSync, parameters)
#define GET_FenceSync(disp) GET_by_offset(disp, _gloffset_FenceSync)
static void INLINE SET_FenceSync(struct _glapi_table *disp, GLsync (GLAPIENTRYP fn)(GLenum, GLbitfield)) {
   SET_by_offset(disp, _gloffset_FenceSync, fn);
}

#define CALL_GetInteger64v(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint64 *)), _gloffset_GetInteger64v, parameters)
#define GET_GetInteger64v(disp) GET_by_offset(disp, _gloffset_GetInteger64v)
static void INLINE SET_GetInteger64v(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint64 *)) {
   SET_by_offset(disp, _gloffset_GetInteger64v, fn);
}

#define CALL_GetSynciv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsync, GLenum, GLsizei, GLsizei *, GLint *)), _gloffset_GetSynciv, parameters)
#define GET_GetSynciv(disp) GET_by_offset(disp, _gloffset_GetSynciv)
static void INLINE SET_GetSynciv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsync, GLenum, GLsizei, GLsizei *, GLint *)) {
   SET_by_offset(disp, _gloffset_GetSynciv, fn);
}

#define CALL_IsSync(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLsync)), _gloffset_IsSync, parameters)
#define GET_IsSync(disp) GET_by_offset(disp, _gloffset_IsSync)
static void INLINE SET_IsSync(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLsync)) {
   SET_by_offset(disp, _gloffset_IsSync, fn);
}

#define CALL_WaitSync(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsync, GLbitfield, GLuint64)), _gloffset_WaitSync, parameters)
#define GET_WaitSync(disp) GET_by_offset(disp, _gloffset_WaitSync)
static void INLINE SET_WaitSync(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsync, GLbitfield, GLuint64)) {
   SET_by_offset(disp, _gloffset_WaitSync, fn);
}

#define CALL_DrawElementsBaseVertex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLenum, const GLvoid *, GLint)), _gloffset_DrawElementsBaseVertex, parameters)
#define GET_DrawElementsBaseVertex(disp) GET_by_offset(disp, _gloffset_DrawElementsBaseVertex)
static void INLINE SET_DrawElementsBaseVertex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLenum, const GLvoid *, GLint)) {
   SET_by_offset(disp, _gloffset_DrawElementsBaseVertex, fn);
}

#define CALL_DrawRangeElementsBaseVertex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *, GLint)), _gloffset_DrawRangeElementsBaseVertex, parameters)
#define GET_DrawRangeElementsBaseVertex(disp) GET_by_offset(disp, _gloffset_DrawRangeElementsBaseVertex)
static void INLINE SET_DrawRangeElementsBaseVertex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *, GLint)) {
   SET_by_offset(disp, _gloffset_DrawRangeElementsBaseVertex, fn);
}

#define CALL_MultiDrawElementsBaseVertex(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei, const GLint *)), _gloffset_MultiDrawElementsBaseVertex, parameters)
#define GET_MultiDrawElementsBaseVertex(disp) GET_by_offset(disp, _gloffset_MultiDrawElementsBaseVertex)
static void INLINE SET_MultiDrawElementsBaseVertex(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei, const GLint *)) {
   SET_by_offset(disp, _gloffset_MultiDrawElementsBaseVertex, fn);
}

#define CALL_BlendEquationSeparateiARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLenum)), _gloffset_BlendEquationSeparateiARB, parameters)
#define GET_BlendEquationSeparateiARB(disp) GET_by_offset(disp, _gloffset_BlendEquationSeparateiARB)
static void INLINE SET_BlendEquationSeparateiARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquationSeparateiARB, fn);
}

#define CALL_BlendEquationiARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum)), _gloffset_BlendEquationiARB, parameters)
#define GET_BlendEquationiARB(disp) GET_by_offset(disp, _gloffset_BlendEquationiARB)
static void INLINE SET_BlendEquationiARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquationiARB, fn);
}

#define CALL_BlendFuncSeparateiARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLenum, GLenum, GLenum)), _gloffset_BlendFuncSeparateiARB, parameters)
#define GET_BlendFuncSeparateiARB(disp) GET_by_offset(disp, _gloffset_BlendFuncSeparateiARB)
static void INLINE SET_BlendFuncSeparateiARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFuncSeparateiARB, fn);
}

#define CALL_BlendFunciARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLenum)), _gloffset_BlendFunciARB, parameters)
#define GET_BlendFunciARB(disp) GET_by_offset(disp, _gloffset_BlendFunciARB)
static void INLINE SET_BlendFunciARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFunciARB, fn);
}

#define CALL_BindSampler(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_BindSampler, parameters)
#define GET_BindSampler(disp) GET_by_offset(disp, _gloffset_BindSampler)
static void INLINE SET_BindSampler(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_BindSampler, fn);
}

#define CALL_DeleteSamplers(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteSamplers, parameters)
#define GET_DeleteSamplers(disp) GET_by_offset(disp, _gloffset_DeleteSamplers)
static void INLINE SET_DeleteSamplers(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteSamplers, fn);
}

#define CALL_GenSamplers(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenSamplers, parameters)
#define GET_GenSamplers(disp) GET_by_offset(disp, _gloffset_GenSamplers)
static void INLINE SET_GenSamplers(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenSamplers, fn);
}

#define CALL_GetSamplerParameterIiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetSamplerParameterIiv, parameters)
#define GET_GetSamplerParameterIiv(disp) GET_by_offset(disp, _gloffset_GetSamplerParameterIiv)
static void INLINE SET_GetSamplerParameterIiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameterIiv, fn);
}

#define CALL_GetSamplerParameterIuiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint *)), _gloffset_GetSamplerParameterIuiv, parameters)
#define GET_GetSamplerParameterIuiv(disp) GET_by_offset(disp, _gloffset_GetSamplerParameterIuiv)
static void INLINE SET_GetSamplerParameterIuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameterIuiv, fn);
}

#define CALL_GetSamplerParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat *)), _gloffset_GetSamplerParameterfv, parameters)
#define GET_GetSamplerParameterfv(disp) GET_by_offset(disp, _gloffset_GetSamplerParameterfv)
static void INLINE SET_GetSamplerParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameterfv, fn);
}

#define CALL_GetSamplerParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetSamplerParameteriv, parameters)
#define GET_GetSamplerParameteriv(disp) GET_by_offset(disp, _gloffset_GetSamplerParameteriv)
static void INLINE SET_GetSamplerParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetSamplerParameteriv, fn);
}

#define CALL_IsSampler(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsSampler, parameters)
#define GET_IsSampler(disp) GET_by_offset(disp, _gloffset_IsSampler)
static void INLINE SET_IsSampler(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsSampler, fn);
}

#define CALL_SamplerParameterIiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, const GLint *)), _gloffset_SamplerParameterIiv, parameters)
#define GET_SamplerParameterIiv(disp) GET_by_offset(disp, _gloffset_SamplerParameterIiv)
static void INLINE SET_SamplerParameterIiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_SamplerParameterIiv, fn);
}

#define CALL_SamplerParameterIuiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, const GLuint *)), _gloffset_SamplerParameterIuiv, parameters)
#define GET_SamplerParameterIuiv(disp) GET_by_offset(disp, _gloffset_SamplerParameterIuiv)
static void INLINE SET_SamplerParameterIuiv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_SamplerParameterIuiv, fn);
}

#define CALL_SamplerParameterf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat)), _gloffset_SamplerParameterf, parameters)
#define GET_SamplerParameterf(disp) GET_by_offset(disp, _gloffset_SamplerParameterf)
static void INLINE SET_SamplerParameterf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_SamplerParameterf, fn);
}

#define CALL_SamplerParameterfv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, const GLfloat *)), _gloffset_SamplerParameterfv, parameters)
#define GET_SamplerParameterfv(disp) GET_by_offset(disp, _gloffset_SamplerParameterfv)
static void INLINE SET_SamplerParameterfv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_SamplerParameterfv, fn);
}

#define CALL_SamplerParameteri(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint)), _gloffset_SamplerParameteri, parameters)
#define GET_SamplerParameteri(disp) GET_by_offset(disp, _gloffset_SamplerParameteri)
static void INLINE SET_SamplerParameteri(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_SamplerParameteri, fn);
}

#define CALL_SamplerParameteriv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, const GLint *)), _gloffset_SamplerParameteriv, parameters)
#define GET_SamplerParameteriv(disp) GET_by_offset(disp, _gloffset_SamplerParameteriv)
static void INLINE SET_SamplerParameteriv(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_SamplerParameteriv, fn);
}

#define CALL_BindTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindTransformFeedback, parameters)
#define GET_BindTransformFeedback(disp) GET_by_offset(disp, _gloffset_BindTransformFeedback)
static void INLINE SET_BindTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindTransformFeedback, fn);
}

#define CALL_DeleteTransformFeedbacks(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteTransformFeedbacks, parameters)
#define GET_DeleteTransformFeedbacks(disp) GET_by_offset(disp, _gloffset_DeleteTransformFeedbacks)
static void INLINE SET_DeleteTransformFeedbacks(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteTransformFeedbacks, fn);
}

#define CALL_DrawTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_DrawTransformFeedback, parameters)
#define GET_DrawTransformFeedback(disp) GET_by_offset(disp, _gloffset_DrawTransformFeedback)
static void INLINE SET_DrawTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_DrawTransformFeedback, fn);
}

#define CALL_GenTransformFeedbacks(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenTransformFeedbacks, parameters)
#define GET_GenTransformFeedbacks(disp) GET_by_offset(disp, _gloffset_GenTransformFeedbacks)
static void INLINE SET_GenTransformFeedbacks(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenTransformFeedbacks, fn);
}

#define CALL_IsTransformFeedback(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsTransformFeedback, parameters)
#define GET_IsTransformFeedback(disp) GET_by_offset(disp, _gloffset_IsTransformFeedback)
static void INLINE SET_IsTransformFeedback(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsTransformFeedback, fn);
}

#define CALL_PauseTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PauseTransformFeedback, parameters)
#define GET_PauseTransformFeedback(disp) GET_by_offset(disp, _gloffset_PauseTransformFeedback)
static void INLINE SET_PauseTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PauseTransformFeedback, fn);
}

#define CALL_ResumeTransformFeedback(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_ResumeTransformFeedback, parameters)
#define GET_ResumeTransformFeedback(disp) GET_by_offset(disp, _gloffset_ResumeTransformFeedback)
static void INLINE SET_ResumeTransformFeedback(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_ResumeTransformFeedback, fn);
}

#define CALL_ClearDepthf(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf)), _gloffset_ClearDepthf, parameters)
#define GET_ClearDepthf(disp) GET_by_offset(disp, _gloffset_ClearDepthf)
static void INLINE SET_ClearDepthf(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf)) {
   SET_by_offset(disp, _gloffset_ClearDepthf, fn);
}

#define CALL_DepthRangef(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLclampf)), _gloffset_DepthRangef, parameters)
#define GET_DepthRangef(disp) GET_by_offset(disp, _gloffset_DepthRangef)
static void INLINE SET_DepthRangef(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLclampf)) {
   SET_by_offset(disp, _gloffset_DepthRangef, fn);
}

#define CALL_GetShaderPrecisionFormat(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *, GLint *)), _gloffset_GetShaderPrecisionFormat, parameters)
#define GET_GetShaderPrecisionFormat(disp) GET_by_offset(disp, _gloffset_GetShaderPrecisionFormat)
static void INLINE SET_GetShaderPrecisionFormat(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *, GLint *)) {
   SET_by_offset(disp, _gloffset_GetShaderPrecisionFormat, fn);
}

#define CALL_ReleaseShaderCompiler(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_ReleaseShaderCompiler, parameters)
#define GET_ReleaseShaderCompiler(disp) GET_by_offset(disp, _gloffset_ReleaseShaderCompiler)
static void INLINE SET_ReleaseShaderCompiler(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_ReleaseShaderCompiler, fn);
}

#define CALL_ShaderBinary(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *, GLenum, const GLvoid *, GLsizei)), _gloffset_ShaderBinary, parameters)
#define GET_ShaderBinary(disp) GET_by_offset(disp, _gloffset_ShaderBinary)
static void INLINE SET_ShaderBinary(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *, GLenum, const GLvoid *, GLsizei)) {
   SET_by_offset(disp, _gloffset_ShaderBinary, fn);
}

#define CALL_GetGraphicsResetStatusARB(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(void)), _gloffset_GetGraphicsResetStatusARB, parameters)
#define GET_GetGraphicsResetStatusARB(disp) GET_by_offset(disp, _gloffset_GetGraphicsResetStatusARB)
static void INLINE SET_GetGraphicsResetStatusARB(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_GetGraphicsResetStatusARB, fn);
}

#define CALL_GetnColorTableARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLsizei, GLvoid *)), _gloffset_GetnColorTableARB, parameters)
#define GET_GetnColorTableARB(disp) GET_by_offset(disp, _gloffset_GetnColorTableARB)
static void INLINE SET_GetnColorTableARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnColorTableARB, fn);
}

#define CALL_GetnCompressedTexImageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLsizei, GLvoid *)), _gloffset_GetnCompressedTexImageARB, parameters)
#define GET_GetnCompressedTexImageARB(disp) GET_by_offset(disp, _gloffset_GetnCompressedTexImageARB)
static void INLINE SET_GetnCompressedTexImageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnCompressedTexImageARB, fn);
}

#define CALL_GetnConvolutionFilterARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLsizei, GLvoid *)), _gloffset_GetnConvolutionFilterARB, parameters)
#define GET_GetnConvolutionFilterARB(disp) GET_by_offset(disp, _gloffset_GetnConvolutionFilterARB)
static void INLINE SET_GetnConvolutionFilterARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnConvolutionFilterARB, fn);
}

#define CALL_GetnHistogramARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *)), _gloffset_GetnHistogramARB, parameters)
#define GET_GetnHistogramARB(disp) GET_by_offset(disp, _gloffset_GetnHistogramARB)
static void INLINE SET_GetnHistogramARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnHistogramARB, fn);
}

#define CALL_GetnMapdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLdouble *)), _gloffset_GetnMapdvARB, parameters)
#define GET_GetnMapdvARB(disp) GET_by_offset(disp, _gloffset_GetnMapdvARB)
static void INLINE SET_GetnMapdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetnMapdvARB, fn);
}

#define CALL_GetnMapfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLfloat *)), _gloffset_GetnMapfvARB, parameters)
#define GET_GetnMapfvARB(disp) GET_by_offset(disp, _gloffset_GetnMapfvARB)
static void INLINE SET_GetnMapfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetnMapfvARB, fn);
}

#define CALL_GetnMapivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLint *)), _gloffset_GetnMapivARB, parameters)
#define GET_GetnMapivARB(disp) GET_by_offset(disp, _gloffset_GetnMapivARB)
static void INLINE SET_GetnMapivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLint *)) {
   SET_by_offset(disp, _gloffset_GetnMapivARB, fn);
}

#define CALL_GetnMinmaxARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *)), _gloffset_GetnMinmaxARB, parameters)
#define GET_GetnMinmaxARB(disp) GET_by_offset(disp, _gloffset_GetnMinmaxARB)
static void INLINE SET_GetnMinmaxARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLboolean, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnMinmaxARB, fn);
}

#define CALL_GetnPixelMapfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLfloat *)), _gloffset_GetnPixelMapfvARB, parameters)
#define GET_GetnPixelMapfvARB(disp) GET_by_offset(disp, _gloffset_GetnPixelMapfvARB)
static void INLINE SET_GetnPixelMapfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetnPixelMapfvARB, fn);
}

#define CALL_GetnPixelMapuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLuint *)), _gloffset_GetnPixelMapuivARB, parameters)
#define GET_GetnPixelMapuivARB(disp) GET_by_offset(disp, _gloffset_GetnPixelMapuivARB)
static void INLINE SET_GetnPixelMapuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetnPixelMapuivARB, fn);
}

#define CALL_GetnPixelMapusvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLushort *)), _gloffset_GetnPixelMapusvARB, parameters)
#define GET_GetnPixelMapusvARB(disp) GET_by_offset(disp, _gloffset_GetnPixelMapusvARB)
static void INLINE SET_GetnPixelMapusvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLushort *)) {
   SET_by_offset(disp, _gloffset_GetnPixelMapusvARB, fn);
}

#define CALL_GetnPolygonStippleARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLubyte *)), _gloffset_GetnPolygonStippleARB, parameters)
#define GET_GetnPolygonStippleARB(disp) GET_by_offset(disp, _gloffset_GetnPolygonStippleARB)
static void INLINE SET_GetnPolygonStippleARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLubyte *)) {
   SET_by_offset(disp, _gloffset_GetnPolygonStippleARB, fn);
}

#define CALL_GetnSeparableFilterARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLsizei, GLvoid *, GLsizei, GLvoid *, GLvoid *)), _gloffset_GetnSeparableFilterARB, parameters)
#define GET_GetnSeparableFilterARB(disp) GET_by_offset(disp, _gloffset_GetnSeparableFilterARB)
static void INLINE SET_GetnSeparableFilterARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLsizei, GLvoid *, GLsizei, GLvoid *, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnSeparableFilterARB, fn);
}

#define CALL_GetnTexImageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLenum, GLsizei, GLvoid *)), _gloffset_GetnTexImageARB, parameters)
#define GET_GetnTexImageARB(disp) GET_by_offset(disp, _gloffset_GetnTexImageARB)
static void INLINE SET_GetnTexImageARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_GetnTexImageARB, fn);
}

#define CALL_GetnUniformdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLsizei, GLdouble *)), _gloffset_GetnUniformdvARB, parameters)
#define GET_GetnUniformdvARB(disp) GET_by_offset(disp, _gloffset_GetnUniformdvARB)
static void INLINE SET_GetnUniformdvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetnUniformdvARB, fn);
}

#define CALL_GetnUniformfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLsizei, GLfloat *)), _gloffset_GetnUniformfvARB, parameters)
#define GET_GetnUniformfvARB(disp) GET_by_offset(disp, _gloffset_GetnUniformfvARB)
static void INLINE SET_GetnUniformfvARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetnUniformfvARB, fn);
}

#define CALL_GetnUniformivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLsizei, GLint *)), _gloffset_GetnUniformivARB, parameters)
#define GET_GetnUniformivARB(disp) GET_by_offset(disp, _gloffset_GetnUniformivARB)
static void INLINE SET_GetnUniformivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLint *)) {
   SET_by_offset(disp, _gloffset_GetnUniformivARB, fn);
}

#define CALL_GetnUniformuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLsizei, GLuint *)), _gloffset_GetnUniformuivARB, parameters)
#define GET_GetnUniformuivARB(disp) GET_by_offset(disp, _gloffset_GetnUniformuivARB)
static void INLINE SET_GetnUniformuivARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLhandleARB, GLint, GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetnUniformuivARB, fn);
}

#define CALL_ReadnPixelsARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, GLvoid *)), _gloffset_ReadnPixelsARB, parameters)
#define GET_ReadnPixelsARB(disp) GET_by_offset(disp, _gloffset_ReadnPixelsARB)
static void INLINE SET_ReadnPixelsARB(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_ReadnPixelsARB, fn);
}

#define CALL_PolygonOffsetEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_PolygonOffsetEXT, parameters)
#define GET_PolygonOffsetEXT(disp) GET_by_offset(disp, _gloffset_PolygonOffsetEXT)
static void INLINE SET_PolygonOffsetEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_PolygonOffsetEXT, fn);
}

#define CALL_GetPixelTexGenParameterfvSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetPixelTexGenParameterfvSGIS, parameters)
#define GET_GetPixelTexGenParameterfvSGIS(disp) GET_by_offset(disp, _gloffset_GetPixelTexGenParameterfvSGIS)
static void INLINE SET_GetPixelTexGenParameterfvSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetPixelTexGenParameterfvSGIS, fn);
}

#define CALL_GetPixelTexGenParameterivSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *)), _gloffset_GetPixelTexGenParameterivSGIS, parameters)
#define GET_GetPixelTexGenParameterivSGIS(disp) GET_by_offset(disp, _gloffset_GetPixelTexGenParameterivSGIS)
static void INLINE SET_GetPixelTexGenParameterivSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetPixelTexGenParameterivSGIS, fn);
}

#define CALL_PixelTexGenParameterfSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PixelTexGenParameterfSGIS, parameters)
#define GET_PixelTexGenParameterfSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameterfSGIS)
static void INLINE SET_PixelTexGenParameterfSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameterfSGIS, fn);
}

#define CALL_PixelTexGenParameterfvSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_PixelTexGenParameterfvSGIS, parameters)
#define GET_PixelTexGenParameterfvSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameterfvSGIS)
static void INLINE SET_PixelTexGenParameterfvSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameterfvSGIS, fn);
}

#define CALL_PixelTexGenParameteriSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PixelTexGenParameteriSGIS, parameters)
#define GET_PixelTexGenParameteriSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameteriSGIS)
static void INLINE SET_PixelTexGenParameteriSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameteriSGIS, fn);
}

#define CALL_PixelTexGenParameterivSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_PixelTexGenParameterivSGIS, parameters)
#define GET_PixelTexGenParameterivSGIS(disp) GET_by_offset(disp, _gloffset_PixelTexGenParameterivSGIS)
static void INLINE SET_PixelTexGenParameterivSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_PixelTexGenParameterivSGIS, fn);
}

#define CALL_SampleMaskSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLboolean)), _gloffset_SampleMaskSGIS, parameters)
#define GET_SampleMaskSGIS(disp) GET_by_offset(disp, _gloffset_SampleMaskSGIS)
static void INLINE SET_SampleMaskSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampf, GLboolean)) {
   SET_by_offset(disp, _gloffset_SampleMaskSGIS, fn);
}

#define CALL_SamplePatternSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_SamplePatternSGIS, parameters)
#define GET_SamplePatternSGIS(disp) GET_by_offset(disp, _gloffset_SamplePatternSGIS)
static void INLINE SET_SamplePatternSGIS(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_SamplePatternSGIS, fn);
}

#define CALL_ColorPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_ColorPointerEXT, parameters)
#define GET_ColorPointerEXT(disp) GET_by_offset(disp, _gloffset_ColorPointerEXT)
static void INLINE SET_ColorPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_ColorPointerEXT, fn);
}

#define CALL_EdgeFlagPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLsizei, const GLboolean *)), _gloffset_EdgeFlagPointerEXT, parameters)
#define GET_EdgeFlagPointerEXT(disp) GET_by_offset(disp, _gloffset_EdgeFlagPointerEXT)
static void INLINE SET_EdgeFlagPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLsizei, const GLboolean *)) {
   SET_by_offset(disp, _gloffset_EdgeFlagPointerEXT, fn);
}

#define CALL_IndexPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_IndexPointerEXT, parameters)
#define GET_IndexPointerEXT(disp) GET_by_offset(disp, _gloffset_IndexPointerEXT)
static void INLINE SET_IndexPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_IndexPointerEXT, fn);
}

#define CALL_NormalPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_NormalPointerEXT, parameters)
#define GET_NormalPointerEXT(disp) GET_by_offset(disp, _gloffset_NormalPointerEXT)
static void INLINE SET_NormalPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_NormalPointerEXT, fn);
}

#define CALL_TexCoordPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_TexCoordPointerEXT, parameters)
#define GET_TexCoordPointerEXT(disp) GET_by_offset(disp, _gloffset_TexCoordPointerEXT)
static void INLINE SET_TexCoordPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_TexCoordPointerEXT, fn);
}

#define CALL_VertexPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), _gloffset_VertexPointerEXT, parameters)
#define GET_VertexPointerEXT(disp) GET_by_offset(disp, _gloffset_VertexPointerEXT)
static void INLINE SET_VertexPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexPointerEXT, fn);
}

#define CALL_PointParameterfEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_PointParameterfEXT, parameters)
#define GET_PointParameterfEXT(disp) GET_by_offset(disp, _gloffset_PointParameterfEXT)
static void INLINE SET_PointParameterfEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_PointParameterfEXT, fn);
}

#define CALL_PointParameterfvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_PointParameterfvEXT, parameters)
#define GET_PointParameterfvEXT(disp) GET_by_offset(disp, _gloffset_PointParameterfvEXT)
static void INLINE SET_PointParameterfvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_PointParameterfvEXT, fn);
}

#define CALL_LockArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei)), _gloffset_LockArraysEXT, parameters)
#define GET_LockArraysEXT(disp) GET_by_offset(disp, _gloffset_LockArraysEXT)
static void INLINE SET_LockArraysEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei)) {
   SET_by_offset(disp, _gloffset_LockArraysEXT, fn);
}

#define CALL_UnlockArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_UnlockArraysEXT, parameters)
#define GET_UnlockArraysEXT(disp) GET_by_offset(disp, _gloffset_UnlockArraysEXT)
static void INLINE SET_UnlockArraysEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_UnlockArraysEXT, fn);
}

#define CALL_SecondaryColor3bEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte)), _gloffset_SecondaryColor3bEXT, parameters)
#define GET_SecondaryColor3bEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3bEXT)
static void INLINE SET_SecondaryColor3bEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLbyte, GLbyte, GLbyte)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3bEXT, fn);
}

#define CALL_SecondaryColor3bvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), _gloffset_SecondaryColor3bvEXT, parameters)
#define GET_SecondaryColor3bvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3bvEXT)
static void INLINE SET_SecondaryColor3bvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLbyte *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3bvEXT, fn);
}

#define CALL_SecondaryColor3dEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_SecondaryColor3dEXT, parameters)
#define GET_SecondaryColor3dEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3dEXT)
static void INLINE SET_SecondaryColor3dEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3dEXT, fn);
}

#define CALL_SecondaryColor3dvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_SecondaryColor3dvEXT, parameters)
#define GET_SecondaryColor3dvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3dvEXT)
static void INLINE SET_SecondaryColor3dvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3dvEXT, fn);
}

#define CALL_SecondaryColor3fEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_SecondaryColor3fEXT, parameters)
#define GET_SecondaryColor3fEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3fEXT)
static void INLINE SET_SecondaryColor3fEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3fEXT, fn);
}

#define CALL_SecondaryColor3fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_SecondaryColor3fvEXT, parameters)
#define GET_SecondaryColor3fvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3fvEXT)
static void INLINE SET_SecondaryColor3fvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3fvEXT, fn);
}

#define CALL_SecondaryColor3iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_SecondaryColor3iEXT, parameters)
#define GET_SecondaryColor3iEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3iEXT)
static void INLINE SET_SecondaryColor3iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3iEXT, fn);
}

#define CALL_SecondaryColor3ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_SecondaryColor3ivEXT, parameters)
#define GET_SecondaryColor3ivEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3ivEXT)
static void INLINE SET_SecondaryColor3ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3ivEXT, fn);
}

#define CALL_SecondaryColor3sEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_SecondaryColor3sEXT, parameters)
#define GET_SecondaryColor3sEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3sEXT)
static void INLINE SET_SecondaryColor3sEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3sEXT, fn);
}

#define CALL_SecondaryColor3svEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_SecondaryColor3svEXT, parameters)
#define GET_SecondaryColor3svEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3svEXT)
static void INLINE SET_SecondaryColor3svEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3svEXT, fn);
}

#define CALL_SecondaryColor3ubEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte, GLubyte, GLubyte)), _gloffset_SecondaryColor3ubEXT, parameters)
#define GET_SecondaryColor3ubEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3ubEXT)
static void INLINE SET_SecondaryColor3ubEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3ubEXT, fn);
}

#define CALL_SecondaryColor3ubvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), _gloffset_SecondaryColor3ubvEXT, parameters)
#define GET_SecondaryColor3ubvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3ubvEXT)
static void INLINE SET_SecondaryColor3ubvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLubyte *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3ubvEXT, fn);
}

#define CALL_SecondaryColor3uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint)), _gloffset_SecondaryColor3uiEXT, parameters)
#define GET_SecondaryColor3uiEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3uiEXT)
static void INLINE SET_SecondaryColor3uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3uiEXT, fn);
}

#define CALL_SecondaryColor3uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLuint *)), _gloffset_SecondaryColor3uivEXT, parameters)
#define GET_SecondaryColor3uivEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3uivEXT)
static void INLINE SET_SecondaryColor3uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLuint *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3uivEXT, fn);
}

#define CALL_SecondaryColor3usEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLushort, GLushort, GLushort)), _gloffset_SecondaryColor3usEXT, parameters)
#define GET_SecondaryColor3usEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3usEXT)
static void INLINE SET_SecondaryColor3usEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLushort, GLushort, GLushort)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3usEXT, fn);
}

#define CALL_SecondaryColor3usvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLushort *)), _gloffset_SecondaryColor3usvEXT, parameters)
#define GET_SecondaryColor3usvEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColor3usvEXT)
static void INLINE SET_SecondaryColor3usvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLushort *)) {
   SET_by_offset(disp, _gloffset_SecondaryColor3usvEXT, fn);
}

#define CALL_SecondaryColorPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_SecondaryColorPointerEXT, parameters)
#define GET_SecondaryColorPointerEXT(disp) GET_by_offset(disp, _gloffset_SecondaryColorPointerEXT)
static void INLINE SET_SecondaryColorPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_SecondaryColorPointerEXT, fn);
}

#define CALL_MultiDrawArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *, const GLsizei *, GLsizei)), _gloffset_MultiDrawArraysEXT, parameters)
#define GET_MultiDrawArraysEXT(disp) GET_by_offset(disp, _gloffset_MultiDrawArraysEXT)
static void INLINE SET_MultiDrawArraysEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *, const GLsizei *, GLsizei)) {
   SET_by_offset(disp, _gloffset_MultiDrawArraysEXT, fn);
}

#define CALL_MultiDrawElementsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei)), _gloffset_MultiDrawElementsEXT, parameters)
#define GET_MultiDrawElementsEXT(disp) GET_by_offset(disp, _gloffset_MultiDrawElementsEXT)
static void INLINE SET_MultiDrawElementsEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei)) {
   SET_by_offset(disp, _gloffset_MultiDrawElementsEXT, fn);
}

#define CALL_FogCoordPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), _gloffset_FogCoordPointerEXT, parameters)
#define GET_FogCoordPointerEXT(disp) GET_by_offset(disp, _gloffset_FogCoordPointerEXT)
static void INLINE SET_FogCoordPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_FogCoordPointerEXT, fn);
}

#define CALL_FogCoorddEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), _gloffset_FogCoorddEXT, parameters)
#define GET_FogCoorddEXT(disp) GET_by_offset(disp, _gloffset_FogCoorddEXT)
static void INLINE SET_FogCoorddEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble)) {
   SET_by_offset(disp, _gloffset_FogCoorddEXT, fn);
}

#define CALL_FogCoorddvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_FogCoorddvEXT, parameters)
#define GET_FogCoorddvEXT(disp) GET_by_offset(disp, _gloffset_FogCoorddvEXT)
static void INLINE SET_FogCoorddvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_FogCoorddvEXT, fn);
}

#define CALL_FogCoordfEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), _gloffset_FogCoordfEXT, parameters)
#define GET_FogCoordfEXT(disp) GET_by_offset(disp, _gloffset_FogCoordfEXT)
static void INLINE SET_FogCoordfEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat)) {
   SET_by_offset(disp, _gloffset_FogCoordfEXT, fn);
}

#define CALL_FogCoordfvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_FogCoordfvEXT, parameters)
#define GET_FogCoordfvEXT(disp) GET_by_offset(disp, _gloffset_FogCoordfvEXT)
static void INLINE SET_FogCoordfvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_FogCoordfvEXT, fn);
}

#define CALL_PixelTexGenSGIX(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_PixelTexGenSGIX, parameters)
#define GET_PixelTexGenSGIX(disp) GET_by_offset(disp, _gloffset_PixelTexGenSGIX)
static void INLINE SET_PixelTexGenSGIX(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_PixelTexGenSGIX, fn);
}

#define CALL_BlendFuncSeparateEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), _gloffset_BlendFuncSeparateEXT, parameters)
#define GET_BlendFuncSeparateEXT(disp) GET_by_offset(disp, _gloffset_BlendFuncSeparateEXT)
static void INLINE SET_BlendFuncSeparateEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendFuncSeparateEXT, fn);
}

#define CALL_FlushVertexArrayRangeNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_FlushVertexArrayRangeNV, parameters)
#define GET_FlushVertexArrayRangeNV(disp) GET_by_offset(disp, _gloffset_FlushVertexArrayRangeNV)
static void INLINE SET_FlushVertexArrayRangeNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_FlushVertexArrayRangeNV, fn);
}

#define CALL_VertexArrayRangeNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLvoid *)), _gloffset_VertexArrayRangeNV, parameters)
#define GET_VertexArrayRangeNV(disp) GET_by_offset(disp, _gloffset_VertexArrayRangeNV)
static void INLINE SET_VertexArrayRangeNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexArrayRangeNV, fn);
}

#define CALL_CombinerInputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum)), _gloffset_CombinerInputNV, parameters)
#define GET_CombinerInputNV(disp) GET_by_offset(disp, _gloffset_CombinerInputNV)
static void INLINE SET_CombinerInputNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_CombinerInputNV, fn);
}

#define CALL_CombinerOutputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean)), _gloffset_CombinerOutputNV, parameters)
#define GET_CombinerOutputNV(disp) GET_by_offset(disp, _gloffset_CombinerOutputNV)
static void INLINE SET_CombinerOutputNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean)) {
   SET_by_offset(disp, _gloffset_CombinerOutputNV, fn);
}

#define CALL_CombinerParameterfNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), _gloffset_CombinerParameterfNV, parameters)
#define GET_CombinerParameterfNV(disp) GET_by_offset(disp, _gloffset_CombinerParameterfNV)
static void INLINE SET_CombinerParameterfNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat)) {
   SET_by_offset(disp, _gloffset_CombinerParameterfNV, fn);
}

#define CALL_CombinerParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_CombinerParameterfvNV, parameters)
#define GET_CombinerParameterfvNV(disp) GET_by_offset(disp, _gloffset_CombinerParameterfvNV)
static void INLINE SET_CombinerParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_CombinerParameterfvNV, fn);
}

#define CALL_CombinerParameteriNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_CombinerParameteriNV, parameters)
#define GET_CombinerParameteriNV(disp) GET_by_offset(disp, _gloffset_CombinerParameteriNV)
static void INLINE SET_CombinerParameteriNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_CombinerParameteriNV, fn);
}

#define CALL_CombinerParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_CombinerParameterivNV, parameters)
#define GET_CombinerParameterivNV(disp) GET_by_offset(disp, _gloffset_CombinerParameterivNV)
static void INLINE SET_CombinerParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_CombinerParameterivNV, fn);
}

#define CALL_FinalCombinerInputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), _gloffset_FinalCombinerInputNV, parameters)
#define GET_FinalCombinerInputNV(disp) GET_by_offset(disp, _gloffset_FinalCombinerInputNV)
static void INLINE SET_FinalCombinerInputNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_FinalCombinerInputNV, fn);
}

#define CALL_GetCombinerInputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLfloat *)), _gloffset_GetCombinerInputParameterfvNV, parameters)
#define GET_GetCombinerInputParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetCombinerInputParameterfvNV)
static void INLINE SET_GetCombinerInputParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetCombinerInputParameterfvNV, fn);
}

#define CALL_GetCombinerInputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLint *)), _gloffset_GetCombinerInputParameterivNV, parameters)
#define GET_GetCombinerInputParameterivNV(disp) GET_by_offset(disp, _gloffset_GetCombinerInputParameterivNV)
static void INLINE SET_GetCombinerInputParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetCombinerInputParameterivNV, fn);
}

#define CALL_GetCombinerOutputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLfloat *)), _gloffset_GetCombinerOutputParameterfvNV, parameters)
#define GET_GetCombinerOutputParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetCombinerOutputParameterfvNV)
static void INLINE SET_GetCombinerOutputParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetCombinerOutputParameterfvNV, fn);
}

#define CALL_GetCombinerOutputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLint *)), _gloffset_GetCombinerOutputParameterivNV, parameters)
#define GET_GetCombinerOutputParameterivNV(disp) GET_by_offset(disp, _gloffset_GetCombinerOutputParameterivNV)
static void INLINE SET_GetCombinerOutputParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetCombinerOutputParameterivNV, fn);
}

#define CALL_GetFinalCombinerInputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), _gloffset_GetFinalCombinerInputParameterfvNV, parameters)
#define GET_GetFinalCombinerInputParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterfvNV)
static void INLINE SET_GetFinalCombinerInputParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterfvNV, fn);
}

#define CALL_GetFinalCombinerInputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetFinalCombinerInputParameterivNV, parameters)
#define GET_GetFinalCombinerInputParameterivNV(disp) GET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterivNV)
static void INLINE SET_GetFinalCombinerInputParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetFinalCombinerInputParameterivNV, fn);
}

#define CALL_ResizeBuffersMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_ResizeBuffersMESA, parameters)
#define GET_ResizeBuffersMESA(disp) GET_by_offset(disp, _gloffset_ResizeBuffersMESA)
static void INLINE SET_ResizeBuffersMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_ResizeBuffersMESA, fn);
}

#define CALL_WindowPos2dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), _gloffset_WindowPos2dMESA, parameters)
#define GET_WindowPos2dMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2dMESA)
static void INLINE SET_WindowPos2dMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_WindowPos2dMESA, fn);
}

#define CALL_WindowPos2dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_WindowPos2dvMESA, parameters)
#define GET_WindowPos2dvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2dvMESA)
static void INLINE SET_WindowPos2dvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_WindowPos2dvMESA, fn);
}

#define CALL_WindowPos2fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), _gloffset_WindowPos2fMESA, parameters)
#define GET_WindowPos2fMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2fMESA)
static void INLINE SET_WindowPos2fMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_WindowPos2fMESA, fn);
}

#define CALL_WindowPos2fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_WindowPos2fvMESA, parameters)
#define GET_WindowPos2fvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2fvMESA)
static void INLINE SET_WindowPos2fvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_WindowPos2fvMESA, fn);
}

#define CALL_WindowPos2iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), _gloffset_WindowPos2iMESA, parameters)
#define GET_WindowPos2iMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2iMESA)
static void INLINE SET_WindowPos2iMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint)) {
   SET_by_offset(disp, _gloffset_WindowPos2iMESA, fn);
}

#define CALL_WindowPos2ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_WindowPos2ivMESA, parameters)
#define GET_WindowPos2ivMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2ivMESA)
static void INLINE SET_WindowPos2ivMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_WindowPos2ivMESA, fn);
}

#define CALL_WindowPos2sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), _gloffset_WindowPos2sMESA, parameters)
#define GET_WindowPos2sMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2sMESA)
static void INLINE SET_WindowPos2sMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_WindowPos2sMESA, fn);
}

#define CALL_WindowPos2svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_WindowPos2svMESA, parameters)
#define GET_WindowPos2svMESA(disp) GET_by_offset(disp, _gloffset_WindowPos2svMESA)
static void INLINE SET_WindowPos2svMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_WindowPos2svMESA, fn);
}

#define CALL_WindowPos3dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), _gloffset_WindowPos3dMESA, parameters)
#define GET_WindowPos3dMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3dMESA)
static void INLINE SET_WindowPos3dMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_WindowPos3dMESA, fn);
}

#define CALL_WindowPos3dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_WindowPos3dvMESA, parameters)
#define GET_WindowPos3dvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3dvMESA)
static void INLINE SET_WindowPos3dvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_WindowPos3dvMESA, fn);
}

#define CALL_WindowPos3fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), _gloffset_WindowPos3fMESA, parameters)
#define GET_WindowPos3fMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3fMESA)
static void INLINE SET_WindowPos3fMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_WindowPos3fMESA, fn);
}

#define CALL_WindowPos3fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_WindowPos3fvMESA, parameters)
#define GET_WindowPos3fvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3fvMESA)
static void INLINE SET_WindowPos3fvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_WindowPos3fvMESA, fn);
}

#define CALL_WindowPos3iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), _gloffset_WindowPos3iMESA, parameters)
#define GET_WindowPos3iMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3iMESA)
static void INLINE SET_WindowPos3iMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_WindowPos3iMESA, fn);
}

#define CALL_WindowPos3ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_WindowPos3ivMESA, parameters)
#define GET_WindowPos3ivMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3ivMESA)
static void INLINE SET_WindowPos3ivMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_WindowPos3ivMESA, fn);
}

#define CALL_WindowPos3sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), _gloffset_WindowPos3sMESA, parameters)
#define GET_WindowPos3sMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3sMESA)
static void INLINE SET_WindowPos3sMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_WindowPos3sMESA, fn);
}

#define CALL_WindowPos3svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_WindowPos3svMESA, parameters)
#define GET_WindowPos3svMESA(disp) GET_by_offset(disp, _gloffset_WindowPos3svMESA)
static void INLINE SET_WindowPos3svMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_WindowPos3svMESA, fn);
}

#define CALL_WindowPos4dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_WindowPos4dMESA, parameters)
#define GET_WindowPos4dMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4dMESA)
static void INLINE SET_WindowPos4dMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_WindowPos4dMESA, fn);
}

#define CALL_WindowPos4dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), _gloffset_WindowPos4dvMESA, parameters)
#define GET_WindowPos4dvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4dvMESA)
static void INLINE SET_WindowPos4dvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLdouble *)) {
   SET_by_offset(disp, _gloffset_WindowPos4dvMESA, fn);
}

#define CALL_WindowPos4fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_WindowPos4fMESA, parameters)
#define GET_WindowPos4fMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4fMESA)
static void INLINE SET_WindowPos4fMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_WindowPos4fMESA, fn);
}

#define CALL_WindowPos4fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), _gloffset_WindowPos4fvMESA, parameters)
#define GET_WindowPos4fvMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4fvMESA)
static void INLINE SET_WindowPos4fvMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLfloat *)) {
   SET_by_offset(disp, _gloffset_WindowPos4fvMESA, fn);
}

#define CALL_WindowPos4iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_WindowPos4iMESA, parameters)
#define GET_WindowPos4iMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4iMESA)
static void INLINE SET_WindowPos4iMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_WindowPos4iMESA, fn);
}

#define CALL_WindowPos4ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), _gloffset_WindowPos4ivMESA, parameters)
#define GET_WindowPos4ivMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4ivMESA)
static void INLINE SET_WindowPos4ivMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLint *)) {
   SET_by_offset(disp, _gloffset_WindowPos4ivMESA, fn);
}

#define CALL_WindowPos4sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), _gloffset_WindowPos4sMESA, parameters)
#define GET_WindowPos4sMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4sMESA)
static void INLINE SET_WindowPos4sMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_WindowPos4sMESA, fn);
}

#define CALL_WindowPos4svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), _gloffset_WindowPos4svMESA, parameters)
#define GET_WindowPos4svMESA(disp) GET_by_offset(disp, _gloffset_WindowPos4svMESA)
static void INLINE SET_WindowPos4svMESA(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLshort *)) {
   SET_by_offset(disp, _gloffset_WindowPos4svMESA, fn);
}

#define CALL_MultiModeDrawArraysIBM(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLenum *, const GLint *, const GLsizei *, GLsizei, GLint)), _gloffset_MultiModeDrawArraysIBM, parameters)
#define GET_MultiModeDrawArraysIBM(disp) GET_by_offset(disp, _gloffset_MultiModeDrawArraysIBM)
static void INLINE SET_MultiModeDrawArraysIBM(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLenum *, const GLint *, const GLsizei *, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_MultiModeDrawArraysIBM, fn);
}

#define CALL_MultiModeDrawElementsIBM(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLenum *, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, GLint)), _gloffset_MultiModeDrawElementsIBM, parameters)
#define GET_MultiModeDrawElementsIBM(disp) GET_by_offset(disp, _gloffset_MultiModeDrawElementsIBM)
static void INLINE SET_MultiModeDrawElementsIBM(struct _glapi_table *disp, void (GLAPIENTRYP fn)(const GLenum *, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, GLint)) {
   SET_by_offset(disp, _gloffset_MultiModeDrawElementsIBM, fn);
}

#define CALL_DeleteFencesNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteFencesNV, parameters)
#define GET_DeleteFencesNV(disp) GET_by_offset(disp, _gloffset_DeleteFencesNV)
static void INLINE SET_DeleteFencesNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteFencesNV, fn);
}

#define CALL_FinishFenceNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_FinishFenceNV, parameters)
#define GET_FinishFenceNV(disp) GET_by_offset(disp, _gloffset_FinishFenceNV)
static void INLINE SET_FinishFenceNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_FinishFenceNV, fn);
}

#define CALL_GenFencesNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenFencesNV, parameters)
#define GET_GenFencesNV(disp) GET_by_offset(disp, _gloffset_GenFencesNV)
static void INLINE SET_GenFencesNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenFencesNV, fn);
}

#define CALL_GetFenceivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetFenceivNV, parameters)
#define GET_GetFenceivNV(disp) GET_by_offset(disp, _gloffset_GetFenceivNV)
static void INLINE SET_GetFenceivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetFenceivNV, fn);
}

#define CALL_IsFenceNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsFenceNV, parameters)
#define GET_IsFenceNV(disp) GET_by_offset(disp, _gloffset_IsFenceNV)
static void INLINE SET_IsFenceNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsFenceNV, fn);
}

#define CALL_SetFenceNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum)), _gloffset_SetFenceNV, parameters)
#define GET_SetFenceNV(disp) GET_by_offset(disp, _gloffset_SetFenceNV)
static void INLINE SET_SetFenceNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_SetFenceNV, fn);
}

#define CALL_TestFenceNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_TestFenceNV, parameters)
#define GET_TestFenceNV(disp) GET_by_offset(disp, _gloffset_TestFenceNV)
static void INLINE SET_TestFenceNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_TestFenceNV, fn);
}

#define CALL_AreProgramsResidentNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLsizei, const GLuint *, GLboolean *)), _gloffset_AreProgramsResidentNV, parameters)
#define GET_AreProgramsResidentNV(disp) GET_by_offset(disp, _gloffset_AreProgramsResidentNV)
static void INLINE SET_AreProgramsResidentNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLsizei, const GLuint *, GLboolean *)) {
   SET_by_offset(disp, _gloffset_AreProgramsResidentNV, fn);
}

#define CALL_BindProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindProgramNV, parameters)
#define GET_BindProgramNV(disp) GET_by_offset(disp, _gloffset_BindProgramNV)
static void INLINE SET_BindProgramNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindProgramNV, fn);
}

#define CALL_DeleteProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteProgramsNV, parameters)
#define GET_DeleteProgramsNV(disp) GET_by_offset(disp, _gloffset_DeleteProgramsNV)
static void INLINE SET_DeleteProgramsNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteProgramsNV, fn);
}

#define CALL_ExecuteProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), _gloffset_ExecuteProgramNV, parameters)
#define GET_ExecuteProgramNV(disp) GET_by_offset(disp, _gloffset_ExecuteProgramNV)
static void INLINE SET_ExecuteProgramNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ExecuteProgramNV, fn);
}

#define CALL_GenProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenProgramsNV, parameters)
#define GET_GenProgramsNV(disp) GET_by_offset(disp, _gloffset_GenProgramsNV)
static void INLINE SET_GenProgramsNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenProgramsNV, fn);
}

#define CALL_GetProgramParameterdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLdouble *)), _gloffset_GetProgramParameterdvNV, parameters)
#define GET_GetProgramParameterdvNV(disp) GET_by_offset(disp, _gloffset_GetProgramParameterdvNV)
static void INLINE SET_GetProgramParameterdvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramParameterdvNV, fn);
}

#define CALL_GetProgramParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLfloat *)), _gloffset_GetProgramParameterfvNV, parameters)
#define GET_GetProgramParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetProgramParameterfvNV)
static void INLINE SET_GetProgramParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramParameterfvNV, fn);
}

#define CALL_GetProgramStringNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLubyte *)), _gloffset_GetProgramStringNV, parameters)
#define GET_GetProgramStringNV(disp) GET_by_offset(disp, _gloffset_GetProgramStringNV)
static void INLINE SET_GetProgramStringNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLubyte *)) {
   SET_by_offset(disp, _gloffset_GetProgramStringNV, fn);
}

#define CALL_GetProgramivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetProgramivNV, parameters)
#define GET_GetProgramivNV(disp) GET_by_offset(disp, _gloffset_GetProgramivNV)
static void INLINE SET_GetProgramivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetProgramivNV, fn);
}

#define CALL_GetTrackMatrixivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLint *)), _gloffset_GetTrackMatrixivNV, parameters)
#define GET_GetTrackMatrixivNV(disp) GET_by_offset(disp, _gloffset_GetTrackMatrixivNV)
static void INLINE SET_GetTrackMatrixivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTrackMatrixivNV, fn);
}

#define CALL_GetVertexAttribPointervNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLvoid **)), _gloffset_GetVertexAttribPointervNV, parameters)
#define GET_GetVertexAttribPointervNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribPointervNV)
static void INLINE SET_GetVertexAttribPointervNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribPointervNV, fn);
}

#define CALL_GetVertexAttribdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLdouble *)), _gloffset_GetVertexAttribdvNV, parameters)
#define GET_GetVertexAttribdvNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribdvNV)
static void INLINE SET_GetVertexAttribdvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribdvNV, fn);
}

#define CALL_GetVertexAttribfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat *)), _gloffset_GetVertexAttribfvNV, parameters)
#define GET_GetVertexAttribfvNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribfvNV)
static void INLINE SET_GetVertexAttribfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribfvNV, fn);
}

#define CALL_GetVertexAttribivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetVertexAttribivNV, parameters)
#define GET_GetVertexAttribivNV(disp) GET_by_offset(disp, _gloffset_GetVertexAttribivNV)
static void INLINE SET_GetVertexAttribivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribivNV, fn);
}

#define CALL_IsProgramNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsProgramNV, parameters)
#define GET_IsProgramNV(disp) GET_by_offset(disp, _gloffset_IsProgramNV)
static void INLINE SET_IsProgramNV(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsProgramNV, fn);
}

#define CALL_LoadProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLubyte *)), _gloffset_LoadProgramNV, parameters)
#define GET_LoadProgramNV(disp) GET_by_offset(disp, _gloffset_LoadProgramNV)
static void INLINE SET_LoadProgramNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_LoadProgramNV, fn);
}

#define CALL_ProgramParameters4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLdouble *)), _gloffset_ProgramParameters4dvNV, parameters)
#define GET_ProgramParameters4dvNV(disp) GET_by_offset(disp, _gloffset_ProgramParameters4dvNV)
static void INLINE SET_ProgramParameters4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramParameters4dvNV, fn);
}

#define CALL_ProgramParameters4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), _gloffset_ProgramParameters4fvNV, parameters)
#define GET_ProgramParameters4fvNV(disp) GET_by_offset(disp, _gloffset_ProgramParameters4fvNV)
static void INLINE SET_ProgramParameters4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramParameters4fvNV, fn);
}

#define CALL_RequestResidentProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_RequestResidentProgramsNV, parameters)
#define GET_RequestResidentProgramsNV(disp) GET_by_offset(disp, _gloffset_RequestResidentProgramsNV)
static void INLINE SET_RequestResidentProgramsNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_RequestResidentProgramsNV, fn);
}

#define CALL_TrackMatrixNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLenum)), _gloffset_TrackMatrixNV, parameters)
#define GET_TrackMatrixNV(disp) GET_by_offset(disp, _gloffset_TrackMatrixNV)
static void INLINE SET_TrackMatrixNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_TrackMatrixNV, fn);
}

#define CALL_VertexAttrib1dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble)), _gloffset_VertexAttrib1dNV, parameters)
#define GET_VertexAttrib1dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dNV)
static void INLINE SET_VertexAttrib1dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dNV, fn);
}

#define CALL_VertexAttrib1dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib1dvNV, parameters)
#define GET_VertexAttrib1dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1dvNV)
static void INLINE SET_VertexAttrib1dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1dvNV, fn);
}

#define CALL_VertexAttrib1fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat)), _gloffset_VertexAttrib1fNV, parameters)
#define GET_VertexAttrib1fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fNV)
static void INLINE SET_VertexAttrib1fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fNV, fn);
}

#define CALL_VertexAttrib1fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib1fvNV, parameters)
#define GET_VertexAttrib1fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1fvNV)
static void INLINE SET_VertexAttrib1fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1fvNV, fn);
}

#define CALL_VertexAttrib1sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort)), _gloffset_VertexAttrib1sNV, parameters)
#define GET_VertexAttrib1sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1sNV)
static void INLINE SET_VertexAttrib1sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1sNV, fn);
}

#define CALL_VertexAttrib1svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib1svNV, parameters)
#define GET_VertexAttrib1svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib1svNV)
static void INLINE SET_VertexAttrib1svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib1svNV, fn);
}

#define CALL_VertexAttrib2dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble)), _gloffset_VertexAttrib2dNV, parameters)
#define GET_VertexAttrib2dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dNV)
static void INLINE SET_VertexAttrib2dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dNV, fn);
}

#define CALL_VertexAttrib2dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib2dvNV, parameters)
#define GET_VertexAttrib2dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2dvNV)
static void INLINE SET_VertexAttrib2dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2dvNV, fn);
}

#define CALL_VertexAttrib2fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat)), _gloffset_VertexAttrib2fNV, parameters)
#define GET_VertexAttrib2fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fNV)
static void INLINE SET_VertexAttrib2fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fNV, fn);
}

#define CALL_VertexAttrib2fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib2fvNV, parameters)
#define GET_VertexAttrib2fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2fvNV)
static void INLINE SET_VertexAttrib2fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2fvNV, fn);
}

#define CALL_VertexAttrib2sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort)), _gloffset_VertexAttrib2sNV, parameters)
#define GET_VertexAttrib2sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2sNV)
static void INLINE SET_VertexAttrib2sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2sNV, fn);
}

#define CALL_VertexAttrib2svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib2svNV, parameters)
#define GET_VertexAttrib2svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib2svNV)
static void INLINE SET_VertexAttrib2svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib2svNV, fn);
}

#define CALL_VertexAttrib3dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib3dNV, parameters)
#define GET_VertexAttrib3dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dNV)
static void INLINE SET_VertexAttrib3dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dNV, fn);
}

#define CALL_VertexAttrib3dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib3dvNV, parameters)
#define GET_VertexAttrib3dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3dvNV)
static void INLINE SET_VertexAttrib3dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3dvNV, fn);
}

#define CALL_VertexAttrib3fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib3fNV, parameters)
#define GET_VertexAttrib3fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fNV)
static void INLINE SET_VertexAttrib3fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fNV, fn);
}

#define CALL_VertexAttrib3fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib3fvNV, parameters)
#define GET_VertexAttrib3fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3fvNV)
static void INLINE SET_VertexAttrib3fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3fvNV, fn);
}

#define CALL_VertexAttrib3sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib3sNV, parameters)
#define GET_VertexAttrib3sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3sNV)
static void INLINE SET_VertexAttrib3sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3sNV, fn);
}

#define CALL_VertexAttrib3svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib3svNV, parameters)
#define GET_VertexAttrib3svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib3svNV)
static void INLINE SET_VertexAttrib3svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib3svNV, fn);
}

#define CALL_VertexAttrib4dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_VertexAttrib4dNV, parameters)
#define GET_VertexAttrib4dNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dNV)
static void INLINE SET_VertexAttrib4dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dNV, fn);
}

#define CALL_VertexAttrib4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), _gloffset_VertexAttrib4dvNV, parameters)
#define GET_VertexAttrib4dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4dvNV)
static void INLINE SET_VertexAttrib4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4dvNV, fn);
}

#define CALL_VertexAttrib4fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_VertexAttrib4fNV, parameters)
#define GET_VertexAttrib4fNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fNV)
static void INLINE SET_VertexAttrib4fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fNV, fn);
}

#define CALL_VertexAttrib4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_VertexAttrib4fvNV, parameters)
#define GET_VertexAttrib4fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4fvNV)
static void INLINE SET_VertexAttrib4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4fvNV, fn);
}

#define CALL_VertexAttrib4sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort, GLshort)), _gloffset_VertexAttrib4sNV, parameters)
#define GET_VertexAttrib4sNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4sNV)
static void INLINE SET_VertexAttrib4sNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLshort, GLshort, GLshort, GLshort)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4sNV, fn);
}

#define CALL_VertexAttrib4svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttrib4svNV, parameters)
#define GET_VertexAttrib4svNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4svNV)
static void INLINE SET_VertexAttrib4svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4svNV, fn);
}

#define CALL_VertexAttrib4ubNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)), _gloffset_VertexAttrib4ubNV, parameters)
#define GET_VertexAttrib4ubNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ubNV)
static void INLINE SET_VertexAttrib4ubNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ubNV, fn);
}

#define CALL_VertexAttrib4ubvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttrib4ubvNV, parameters)
#define GET_VertexAttrib4ubvNV(disp) GET_by_offset(disp, _gloffset_VertexAttrib4ubvNV)
static void INLINE SET_VertexAttrib4ubvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttrib4ubvNV, fn);
}

#define CALL_VertexAttribPointerNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_VertexAttribPointerNV, parameters)
#define GET_VertexAttribPointerNV(disp) GET_by_offset(disp, _gloffset_VertexAttribPointerNV)
static void INLINE SET_VertexAttribPointerNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexAttribPointerNV, fn);
}

#define CALL_VertexAttribs1dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs1dvNV, parameters)
#define GET_VertexAttribs1dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs1dvNV)
static void INLINE SET_VertexAttribs1dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs1dvNV, fn);
}

#define CALL_VertexAttribs1fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs1fvNV, parameters)
#define GET_VertexAttribs1fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs1fvNV)
static void INLINE SET_VertexAttribs1fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs1fvNV, fn);
}

#define CALL_VertexAttribs1svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs1svNV, parameters)
#define GET_VertexAttribs1svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs1svNV)
static void INLINE SET_VertexAttribs1svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs1svNV, fn);
}

#define CALL_VertexAttribs2dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs2dvNV, parameters)
#define GET_VertexAttribs2dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs2dvNV)
static void INLINE SET_VertexAttribs2dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs2dvNV, fn);
}

#define CALL_VertexAttribs2fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs2fvNV, parameters)
#define GET_VertexAttribs2fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs2fvNV)
static void INLINE SET_VertexAttribs2fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs2fvNV, fn);
}

#define CALL_VertexAttribs2svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs2svNV, parameters)
#define GET_VertexAttribs2svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs2svNV)
static void INLINE SET_VertexAttribs2svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs2svNV, fn);
}

#define CALL_VertexAttribs3dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs3dvNV, parameters)
#define GET_VertexAttribs3dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs3dvNV)
static void INLINE SET_VertexAttribs3dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs3dvNV, fn);
}

#define CALL_VertexAttribs3fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs3fvNV, parameters)
#define GET_VertexAttribs3fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs3fvNV)
static void INLINE SET_VertexAttribs3fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs3fvNV, fn);
}

#define CALL_VertexAttribs3svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs3svNV, parameters)
#define GET_VertexAttribs3svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs3svNV)
static void INLINE SET_VertexAttribs3svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs3svNV, fn);
}

#define CALL_VertexAttribs4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), _gloffset_VertexAttribs4dvNV, parameters)
#define GET_VertexAttribs4dvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4dvNV)
static void INLINE SET_VertexAttribs4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4dvNV, fn);
}

#define CALL_VertexAttribs4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), _gloffset_VertexAttribs4fvNV, parameters)
#define GET_VertexAttribs4fvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4fvNV)
static void INLINE SET_VertexAttribs4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4fvNV, fn);
}

#define CALL_VertexAttribs4svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), _gloffset_VertexAttribs4svNV, parameters)
#define GET_VertexAttribs4svNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4svNV)
static void INLINE SET_VertexAttribs4svNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4svNV, fn);
}

#define CALL_VertexAttribs4ubvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *)), _gloffset_VertexAttribs4ubvNV, parameters)
#define GET_VertexAttribs4ubvNV(disp) GET_by_offset(disp, _gloffset_VertexAttribs4ubvNV)
static void INLINE SET_VertexAttribs4ubvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttribs4ubvNV, fn);
}

#define CALL_GetTexBumpParameterfvATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), _gloffset_GetTexBumpParameterfvATI, parameters)
#define GET_GetTexBumpParameterfvATI(disp) GET_by_offset(disp, _gloffset_GetTexBumpParameterfvATI)
static void INLINE SET_GetTexBumpParameterfvATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetTexBumpParameterfvATI, fn);
}

#define CALL_GetTexBumpParameterivATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *)), _gloffset_GetTexBumpParameterivATI, parameters)
#define GET_GetTexBumpParameterivATI(disp) GET_by_offset(disp, _gloffset_GetTexBumpParameterivATI)
static void INLINE SET_GetTexBumpParameterivATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexBumpParameterivATI, fn);
}

#define CALL_TexBumpParameterfvATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), _gloffset_TexBumpParameterfvATI, parameters)
#define GET_TexBumpParameterfvATI(disp) GET_by_offset(disp, _gloffset_TexBumpParameterfvATI)
static void INLINE SET_TexBumpParameterfvATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_TexBumpParameterfvATI, fn);
}

#define CALL_TexBumpParameterivATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_TexBumpParameterivATI, parameters)
#define GET_TexBumpParameterivATI(disp) GET_by_offset(disp, _gloffset_TexBumpParameterivATI)
static void INLINE SET_TexBumpParameterivATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexBumpParameterivATI, fn);
}

#define CALL_AlphaFragmentOp1ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_AlphaFragmentOp1ATI, parameters)
#define GET_AlphaFragmentOp1ATI(disp) GET_by_offset(disp, _gloffset_AlphaFragmentOp1ATI)
static void INLINE SET_AlphaFragmentOp1ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AlphaFragmentOp1ATI, fn);
}

#define CALL_AlphaFragmentOp2ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_AlphaFragmentOp2ATI, parameters)
#define GET_AlphaFragmentOp2ATI(disp) GET_by_offset(disp, _gloffset_AlphaFragmentOp2ATI)
static void INLINE SET_AlphaFragmentOp2ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AlphaFragmentOp2ATI, fn);
}

#define CALL_AlphaFragmentOp3ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_AlphaFragmentOp3ATI, parameters)
#define GET_AlphaFragmentOp3ATI(disp) GET_by_offset(disp, _gloffset_AlphaFragmentOp3ATI)
static void INLINE SET_AlphaFragmentOp3ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_AlphaFragmentOp3ATI, fn);
}

#define CALL_BeginFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_BeginFragmentShaderATI, parameters)
#define GET_BeginFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_BeginFragmentShaderATI)
static void INLINE SET_BeginFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_BeginFragmentShaderATI, fn);
}

#define CALL_BindFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_BindFragmentShaderATI, parameters)
#define GET_BindFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_BindFragmentShaderATI)
static void INLINE SET_BindFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_BindFragmentShaderATI, fn);
}

#define CALL_ColorFragmentOp1ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_ColorFragmentOp1ATI, parameters)
#define GET_ColorFragmentOp1ATI(disp) GET_by_offset(disp, _gloffset_ColorFragmentOp1ATI)
static void INLINE SET_ColorFragmentOp1ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorFragmentOp1ATI, fn);
}

#define CALL_ColorFragmentOp2ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_ColorFragmentOp2ATI, parameters)
#define GET_ColorFragmentOp2ATI(disp) GET_by_offset(disp, _gloffset_ColorFragmentOp2ATI)
static void INLINE SET_ColorFragmentOp2ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorFragmentOp2ATI, fn);
}

#define CALL_ColorFragmentOp3ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_ColorFragmentOp3ATI, parameters)
#define GET_ColorFragmentOp3ATI(disp) GET_by_offset(disp, _gloffset_ColorFragmentOp3ATI)
static void INLINE SET_ColorFragmentOp3ATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ColorFragmentOp3ATI, fn);
}

#define CALL_DeleteFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_DeleteFragmentShaderATI, parameters)
#define GET_DeleteFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_DeleteFragmentShaderATI)
static void INLINE SET_DeleteFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_DeleteFragmentShaderATI, fn);
}

#define CALL_EndFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndFragmentShaderATI, parameters)
#define GET_EndFragmentShaderATI(disp) GET_by_offset(disp, _gloffset_EndFragmentShaderATI)
static void INLINE SET_EndFragmentShaderATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndFragmentShaderATI, fn);
}

#define CALL_GenFragmentShadersATI(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLuint)), _gloffset_GenFragmentShadersATI, parameters)
#define GET_GenFragmentShadersATI(disp) GET_by_offset(disp, _gloffset_GenFragmentShadersATI)
static void INLINE SET_GenFragmentShadersATI(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_GenFragmentShadersATI, fn);
}

#define CALL_PassTexCoordATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLenum)), _gloffset_PassTexCoordATI, parameters)
#define GET_PassTexCoordATI(disp) GET_by_offset(disp, _gloffset_PassTexCoordATI)
static void INLINE SET_PassTexCoordATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_PassTexCoordATI, fn);
}

#define CALL_SampleMapATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLenum)), _gloffset_SampleMapATI, parameters)
#define GET_SampleMapATI(disp) GET_by_offset(disp, _gloffset_SampleMapATI)
static void INLINE SET_SampleMapATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_SampleMapATI, fn);
}

#define CALL_SetFragmentShaderConstantATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), _gloffset_SetFragmentShaderConstantATI, parameters)
#define GET_SetFragmentShaderConstantATI(disp) GET_by_offset(disp, _gloffset_SetFragmentShaderConstantATI)
static void INLINE SET_SetFragmentShaderConstantATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_SetFragmentShaderConstantATI, fn);
}

#define CALL_PointParameteriNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), _gloffset_PointParameteriNV, parameters)
#define GET_PointParameteriNV(disp) GET_by_offset(disp, _gloffset_PointParameteriNV)
static void INLINE SET_PointParameteriNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_PointParameteriNV, fn);
}

#define CALL_PointParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), _gloffset_PointParameterivNV, parameters)
#define GET_PointParameterivNV(disp) GET_by_offset(disp, _gloffset_PointParameterivNV)
static void INLINE SET_PointParameterivNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_PointParameterivNV, fn);
}

#define CALL_ActiveStencilFaceEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ActiveStencilFaceEXT, parameters)
#define GET_ActiveStencilFaceEXT(disp) GET_by_offset(disp, _gloffset_ActiveStencilFaceEXT)
static void INLINE SET_ActiveStencilFaceEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ActiveStencilFaceEXT, fn);
}

#define CALL_BindVertexArrayAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_BindVertexArrayAPPLE, parameters)
#define GET_BindVertexArrayAPPLE(disp) GET_by_offset(disp, _gloffset_BindVertexArrayAPPLE)
static void INLINE SET_BindVertexArrayAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_BindVertexArrayAPPLE, fn);
}

#define CALL_DeleteVertexArraysAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteVertexArraysAPPLE, parameters)
#define GET_DeleteVertexArraysAPPLE(disp) GET_by_offset(disp, _gloffset_DeleteVertexArraysAPPLE)
static void INLINE SET_DeleteVertexArraysAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteVertexArraysAPPLE, fn);
}

#define CALL_GenVertexArraysAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenVertexArraysAPPLE, parameters)
#define GET_GenVertexArraysAPPLE(disp) GET_by_offset(disp, _gloffset_GenVertexArraysAPPLE)
static void INLINE SET_GenVertexArraysAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenVertexArraysAPPLE, fn);
}

#define CALL_IsVertexArrayAPPLE(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsVertexArrayAPPLE, parameters)
#define GET_IsVertexArrayAPPLE(disp) GET_by_offset(disp, _gloffset_IsVertexArrayAPPLE)
static void INLINE SET_IsVertexArrayAPPLE(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsVertexArrayAPPLE, fn);
}

#define CALL_GetProgramNamedParameterdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLdouble *)), _gloffset_GetProgramNamedParameterdvNV, parameters)
#define GET_GetProgramNamedParameterdvNV(disp) GET_by_offset(disp, _gloffset_GetProgramNamedParameterdvNV)
static void INLINE SET_GetProgramNamedParameterdvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLdouble *)) {
   SET_by_offset(disp, _gloffset_GetProgramNamedParameterdvNV, fn);
}

#define CALL_GetProgramNamedParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLfloat *)), _gloffset_GetProgramNamedParameterfvNV, parameters)
#define GET_GetProgramNamedParameterfvNV(disp) GET_by_offset(disp, _gloffset_GetProgramNamedParameterfvNV)
static void INLINE SET_GetProgramNamedParameterfvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLfloat *)) {
   SET_by_offset(disp, _gloffset_GetProgramNamedParameterfvNV, fn);
}

#define CALL_ProgramNamedParameter4dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLdouble, GLdouble, GLdouble, GLdouble)), _gloffset_ProgramNamedParameter4dNV, parameters)
#define GET_ProgramNamedParameter4dNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4dNV)
static void INLINE SET_ProgramNamedParameter4dNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLdouble, GLdouble, GLdouble, GLdouble)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4dNV, fn);
}

#define CALL_ProgramNamedParameter4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, const GLdouble *)), _gloffset_ProgramNamedParameter4dvNV, parameters)
#define GET_ProgramNamedParameter4dvNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4dvNV)
static void INLINE SET_ProgramNamedParameter4dvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, const GLdouble *)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4dvNV, fn);
}

#define CALL_ProgramNamedParameter4fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLfloat, GLfloat, GLfloat, GLfloat)), _gloffset_ProgramNamedParameter4fNV, parameters)
#define GET_ProgramNamedParameter4fNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4fNV)
static void INLINE SET_ProgramNamedParameter4fNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, GLfloat, GLfloat, GLfloat, GLfloat)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4fNV, fn);
}

#define CALL_ProgramNamedParameter4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, const GLfloat *)), _gloffset_ProgramNamedParameter4fvNV, parameters)
#define GET_ProgramNamedParameter4fvNV(disp) GET_by_offset(disp, _gloffset_ProgramNamedParameter4fvNV)
static void INLINE SET_ProgramNamedParameter4fvNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const GLubyte *, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramNamedParameter4fvNV, fn);
}

#define CALL_PrimitiveRestartIndexNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_PrimitiveRestartIndexNV, parameters)
#define GET_PrimitiveRestartIndexNV(disp) GET_by_offset(disp, _gloffset_PrimitiveRestartIndexNV)
static void INLINE SET_PrimitiveRestartIndexNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_PrimitiveRestartIndexNV, fn);
}

#define CALL_PrimitiveRestartNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_PrimitiveRestartNV, parameters)
#define GET_PrimitiveRestartNV(disp) GET_by_offset(disp, _gloffset_PrimitiveRestartNV)
static void INLINE SET_PrimitiveRestartNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_PrimitiveRestartNV, fn);
}

#define CALL_DepthBoundsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampd, GLclampd)), _gloffset_DepthBoundsEXT, parameters)
#define GET_DepthBoundsEXT(disp) GET_by_offset(disp, _gloffset_DepthBoundsEXT)
static void INLINE SET_DepthBoundsEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLclampd, GLclampd)) {
   SET_by_offset(disp, _gloffset_DepthBoundsEXT, fn);
}

#define CALL_BlendEquationSeparateEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), _gloffset_BlendEquationSeparateEXT, parameters)
#define GET_BlendEquationSeparateEXT(disp) GET_by_offset(disp, _gloffset_BlendEquationSeparateEXT)
static void INLINE SET_BlendEquationSeparateEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum)) {
   SET_by_offset(disp, _gloffset_BlendEquationSeparateEXT, fn);
}

#define CALL_BindFramebufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindFramebufferEXT, parameters)
#define GET_BindFramebufferEXT(disp) GET_by_offset(disp, _gloffset_BindFramebufferEXT)
static void INLINE SET_BindFramebufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindFramebufferEXT, fn);
}

#define CALL_BindRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_BindRenderbufferEXT, parameters)
#define GET_BindRenderbufferEXT(disp) GET_by_offset(disp, _gloffset_BindRenderbufferEXT)
static void INLINE SET_BindRenderbufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_BindRenderbufferEXT, fn);
}

#define CALL_CheckFramebufferStatusEXT(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLenum)), _gloffset_CheckFramebufferStatusEXT, parameters)
#define GET_CheckFramebufferStatusEXT(disp) GET_by_offset(disp, _gloffset_CheckFramebufferStatusEXT)
static void INLINE SET_CheckFramebufferStatusEXT(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_CheckFramebufferStatusEXT, fn);
}

#define CALL_DeleteFramebuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteFramebuffersEXT, parameters)
#define GET_DeleteFramebuffersEXT(disp) GET_by_offset(disp, _gloffset_DeleteFramebuffersEXT)
static void INLINE SET_DeleteFramebuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteFramebuffersEXT, fn);
}

#define CALL_DeleteRenderbuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), _gloffset_DeleteRenderbuffersEXT, parameters)
#define GET_DeleteRenderbuffersEXT(disp) GET_by_offset(disp, _gloffset_DeleteRenderbuffersEXT)
static void INLINE SET_DeleteRenderbuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_DeleteRenderbuffersEXT, fn);
}

#define CALL_FramebufferRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint)), _gloffset_FramebufferRenderbufferEXT, parameters)
#define GET_FramebufferRenderbufferEXT(disp) GET_by_offset(disp, _gloffset_FramebufferRenderbufferEXT)
static void INLINE SET_FramebufferRenderbufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_FramebufferRenderbufferEXT, fn);
}

#define CALL_FramebufferTexture1DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint)), _gloffset_FramebufferTexture1DEXT, parameters)
#define GET_FramebufferTexture1DEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTexture1DEXT)
static void INLINE SET_FramebufferTexture1DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture1DEXT, fn);
}

#define CALL_FramebufferTexture2DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint)), _gloffset_FramebufferTexture2DEXT, parameters)
#define GET_FramebufferTexture2DEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTexture2DEXT)
static void INLINE SET_FramebufferTexture2DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture2DEXT, fn);
}

#define CALL_FramebufferTexture3DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint, GLint)), _gloffset_FramebufferTexture3DEXT, parameters)
#define GET_FramebufferTexture3DEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTexture3DEXT)
static void INLINE SET_FramebufferTexture3DEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLuint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTexture3DEXT, fn);
}

#define CALL_GenFramebuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenFramebuffersEXT, parameters)
#define GET_GenFramebuffersEXT(disp) GET_by_offset(disp, _gloffset_GenFramebuffersEXT)
static void INLINE SET_GenFramebuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenFramebuffersEXT, fn);
}

#define CALL_GenRenderbuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), _gloffset_GenRenderbuffersEXT, parameters)
#define GET_GenRenderbuffersEXT(disp) GET_by_offset(disp, _gloffset_GenRenderbuffersEXT)
static void INLINE SET_GenRenderbuffersEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLsizei, GLuint *)) {
   SET_by_offset(disp, _gloffset_GenRenderbuffersEXT, fn);
}

#define CALL_GenerateMipmapEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_GenerateMipmapEXT, parameters)
#define GET_GenerateMipmapEXT(disp) GET_by_offset(disp, _gloffset_GenerateMipmapEXT)
static void INLINE SET_GenerateMipmapEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_GenerateMipmapEXT, fn);
}

#define CALL_GetFramebufferAttachmentParameterivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLint *)), _gloffset_GetFramebufferAttachmentParameterivEXT, parameters)
#define GET_GetFramebufferAttachmentParameterivEXT(disp) GET_by_offset(disp, _gloffset_GetFramebufferAttachmentParameterivEXT)
static void INLINE SET_GetFramebufferAttachmentParameterivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetFramebufferAttachmentParameterivEXT, fn);
}

#define CALL_GetRenderbufferParameterivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetRenderbufferParameterivEXT, parameters)
#define GET_GetRenderbufferParameterivEXT(disp) GET_by_offset(disp, _gloffset_GetRenderbufferParameterivEXT)
static void INLINE SET_GetRenderbufferParameterivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetRenderbufferParameterivEXT, fn);
}

#define CALL_IsFramebufferEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsFramebufferEXT, parameters)
#define GET_IsFramebufferEXT(disp) GET_by_offset(disp, _gloffset_IsFramebufferEXT)
static void INLINE SET_IsFramebufferEXT(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsFramebufferEXT, fn);
}

#define CALL_IsRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), _gloffset_IsRenderbufferEXT, parameters)
#define GET_IsRenderbufferEXT(disp) GET_by_offset(disp, _gloffset_IsRenderbufferEXT)
static void INLINE SET_IsRenderbufferEXT(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_IsRenderbufferEXT, fn);
}

#define CALL_RenderbufferStorageEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLsizei)), _gloffset_RenderbufferStorageEXT, parameters)
#define GET_RenderbufferStorageEXT(disp) GET_by_offset(disp, _gloffset_RenderbufferStorageEXT)
static void INLINE SET_RenderbufferStorageEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLsizei, GLsizei)) {
   SET_by_offset(disp, _gloffset_RenderbufferStorageEXT, fn);
}

#define CALL_BlitFramebufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)), _gloffset_BlitFramebufferEXT, parameters)
#define GET_BlitFramebufferEXT(disp) GET_by_offset(disp, _gloffset_BlitFramebufferEXT)
static void INLINE SET_BlitFramebufferEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)) {
   SET_by_offset(disp, _gloffset_BlitFramebufferEXT, fn);
}

#define CALL_BufferParameteriAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint)), _gloffset_BufferParameteriAPPLE, parameters)
#define GET_BufferParameteriAPPLE(disp) GET_by_offset(disp, _gloffset_BufferParameteriAPPLE)
static void INLINE SET_BufferParameteriAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint)) {
   SET_by_offset(disp, _gloffset_BufferParameteriAPPLE, fn);
}

#define CALL_FlushMappedBufferRangeAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptr, GLsizeiptr)), _gloffset_FlushMappedBufferRangeAPPLE, parameters)
#define GET_FlushMappedBufferRangeAPPLE(disp) GET_by_offset(disp, _gloffset_FlushMappedBufferRangeAPPLE)
static void INLINE SET_FlushMappedBufferRangeAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_FlushMappedBufferRangeAPPLE, fn);
}

#define CALL_BindFragDataLocationEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, const GLchar *)), _gloffset_BindFragDataLocationEXT, parameters)
#define GET_BindFragDataLocationEXT(disp) GET_by_offset(disp, _gloffset_BindFragDataLocationEXT)
static void INLINE SET_BindFragDataLocationEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, const GLchar *)) {
   SET_by_offset(disp, _gloffset_BindFragDataLocationEXT, fn);
}

#define CALL_GetFragDataLocationEXT(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLuint, const GLchar *)), _gloffset_GetFragDataLocationEXT, parameters)
#define GET_GetFragDataLocationEXT(disp) GET_by_offset(disp, _gloffset_GetFragDataLocationEXT)
static void INLINE SET_GetFragDataLocationEXT(struct _glapi_table *disp, GLint (GLAPIENTRYP fn)(GLuint, const GLchar *)) {
   SET_by_offset(disp, _gloffset_GetFragDataLocationEXT, fn);
}

#define CALL_GetUniformuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLuint *)), _gloffset_GetUniformuivEXT, parameters)
#define GET_GetUniformuivEXT(disp) GET_by_offset(disp, _gloffset_GetUniformuivEXT)
static void INLINE SET_GetUniformuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetUniformuivEXT, fn);
}

#define CALL_GetVertexAttribIivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), _gloffset_GetVertexAttribIivEXT, parameters)
#define GET_GetVertexAttribIivEXT(disp) GET_by_offset(disp, _gloffset_GetVertexAttribIivEXT)
static void INLINE SET_GetVertexAttribIivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribIivEXT, fn);
}

#define CALL_GetVertexAttribIuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint *)), _gloffset_GetVertexAttribIuivEXT, parameters)
#define GET_GetVertexAttribIuivEXT(disp) GET_by_offset(disp, _gloffset_GetVertexAttribIuivEXT)
static void INLINE SET_GetVertexAttribIuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetVertexAttribIuivEXT, fn);
}

#define CALL_Uniform1uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint)), _gloffset_Uniform1uiEXT, parameters)
#define GET_Uniform1uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform1uiEXT)
static void INLINE SET_Uniform1uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform1uiEXT, fn);
}

#define CALL_Uniform1uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform1uivEXT, parameters)
#define GET_Uniform1uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform1uivEXT)
static void INLINE SET_Uniform1uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform1uivEXT, fn);
}

#define CALL_Uniform2uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint, GLuint)), _gloffset_Uniform2uiEXT, parameters)
#define GET_Uniform2uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform2uiEXT)
static void INLINE SET_Uniform2uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform2uiEXT, fn);
}

#define CALL_Uniform2uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform2uivEXT, parameters)
#define GET_Uniform2uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform2uivEXT)
static void INLINE SET_Uniform2uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform2uivEXT, fn);
}

#define CALL_Uniform3uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint, GLuint, GLuint)), _gloffset_Uniform3uiEXT, parameters)
#define GET_Uniform3uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform3uiEXT)
static void INLINE SET_Uniform3uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform3uiEXT, fn);
}

#define CALL_Uniform3uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform3uivEXT, parameters)
#define GET_Uniform3uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform3uivEXT)
static void INLINE SET_Uniform3uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform3uivEXT, fn);
}

#define CALL_Uniform4uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLuint, GLuint, GLuint, GLuint)), _gloffset_Uniform4uiEXT, parameters)
#define GET_Uniform4uiEXT(disp) GET_by_offset(disp, _gloffset_Uniform4uiEXT)
static void INLINE SET_Uniform4uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_Uniform4uiEXT, fn);
}

#define CALL_Uniform4uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLuint *)), _gloffset_Uniform4uivEXT, parameters)
#define GET_Uniform4uivEXT(disp) GET_by_offset(disp, _gloffset_Uniform4uivEXT)
static void INLINE SET_Uniform4uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLsizei, const GLuint *)) {
   SET_by_offset(disp, _gloffset_Uniform4uivEXT, fn);
}

#define CALL_VertexAttribI1iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint)), _gloffset_VertexAttribI1iEXT, parameters)
#define GET_VertexAttribI1iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1iEXT)
static void INLINE SET_VertexAttribI1iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1iEXT, fn);
}

#define CALL_VertexAttribI1ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI1ivEXT, parameters)
#define GET_VertexAttribI1ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1ivEXT)
static void INLINE SET_VertexAttribI1ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1ivEXT, fn);
}

#define CALL_VertexAttribI1uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), _gloffset_VertexAttribI1uiEXT, parameters)
#define GET_VertexAttribI1uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1uiEXT)
static void INLINE SET_VertexAttribI1uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1uiEXT, fn);
}

#define CALL_VertexAttribI1uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI1uivEXT, parameters)
#define GET_VertexAttribI1uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI1uivEXT)
static void INLINE SET_VertexAttribI1uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI1uivEXT, fn);
}

#define CALL_VertexAttribI2iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLint)), _gloffset_VertexAttribI2iEXT, parameters)
#define GET_VertexAttribI2iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2iEXT)
static void INLINE SET_VertexAttribI2iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2iEXT, fn);
}

#define CALL_VertexAttribI2ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI2ivEXT, parameters)
#define GET_VertexAttribI2ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2ivEXT)
static void INLINE SET_VertexAttribI2ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2ivEXT, fn);
}

#define CALL_VertexAttribI2uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint)), _gloffset_VertexAttribI2uiEXT, parameters)
#define GET_VertexAttribI2uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2uiEXT)
static void INLINE SET_VertexAttribI2uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2uiEXT, fn);
}

#define CALL_VertexAttribI2uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI2uivEXT, parameters)
#define GET_VertexAttribI2uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI2uivEXT)
static void INLINE SET_VertexAttribI2uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI2uivEXT, fn);
}

#define CALL_VertexAttribI3iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLint, GLint)), _gloffset_VertexAttribI3iEXT, parameters)
#define GET_VertexAttribI3iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3iEXT)
static void INLINE SET_VertexAttribI3iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3iEXT, fn);
}

#define CALL_VertexAttribI3ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI3ivEXT, parameters)
#define GET_VertexAttribI3ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3ivEXT)
static void INLINE SET_VertexAttribI3ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3ivEXT, fn);
}

#define CALL_VertexAttribI3uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint)), _gloffset_VertexAttribI3uiEXT, parameters)
#define GET_VertexAttribI3uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3uiEXT)
static void INLINE SET_VertexAttribI3uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3uiEXT, fn);
}

#define CALL_VertexAttribI3uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI3uivEXT, parameters)
#define GET_VertexAttribI3uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI3uivEXT)
static void INLINE SET_VertexAttribI3uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI3uivEXT, fn);
}

#define CALL_VertexAttribI4bvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), _gloffset_VertexAttribI4bvEXT, parameters)
#define GET_VertexAttribI4bvEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4bvEXT)
static void INLINE SET_VertexAttribI4bvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLbyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4bvEXT, fn);
}

#define CALL_VertexAttribI4iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLint, GLint, GLint)), _gloffset_VertexAttribI4iEXT, parameters)
#define GET_VertexAttribI4iEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4iEXT)
static void INLINE SET_VertexAttribI4iEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4iEXT, fn);
}

#define CALL_VertexAttribI4ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), _gloffset_VertexAttribI4ivEXT, parameters)
#define GET_VertexAttribI4ivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4ivEXT)
static void INLINE SET_VertexAttribI4ivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4ivEXT, fn);
}

#define CALL_VertexAttribI4svEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), _gloffset_VertexAttribI4svEXT, parameters)
#define GET_VertexAttribI4svEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4svEXT)
static void INLINE SET_VertexAttribI4svEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLshort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4svEXT, fn);
}

#define CALL_VertexAttribI4ubvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), _gloffset_VertexAttribI4ubvEXT, parameters)
#define GET_VertexAttribI4ubvEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4ubvEXT)
static void INLINE SET_VertexAttribI4ubvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLubyte *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4ubvEXT, fn);
}

#define CALL_VertexAttribI4uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint, GLuint)), _gloffset_VertexAttribI4uiEXT, parameters)
#define GET_VertexAttribI4uiEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4uiEXT)
static void INLINE SET_VertexAttribI4uiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4uiEXT, fn);
}

#define CALL_VertexAttribI4uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), _gloffset_VertexAttribI4uivEXT, parameters)
#define GET_VertexAttribI4uivEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4uivEXT)
static void INLINE SET_VertexAttribI4uivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLuint *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4uivEXT, fn);
}

#define CALL_VertexAttribI4usvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), _gloffset_VertexAttribI4usvEXT, parameters)
#define GET_VertexAttribI4usvEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribI4usvEXT)
static void INLINE SET_VertexAttribI4usvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, const GLushort *)) {
   SET_by_offset(disp, _gloffset_VertexAttribI4usvEXT, fn);
}

#define CALL_VertexAttribIPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)), _gloffset_VertexAttribIPointerEXT, parameters)
#define GET_VertexAttribIPointerEXT(disp) GET_by_offset(disp, _gloffset_VertexAttribIPointerEXT)
static void INLINE SET_VertexAttribIPointerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)) {
   SET_by_offset(disp, _gloffset_VertexAttribIPointerEXT, fn);
}

#define CALL_FramebufferTextureLayerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint, GLint, GLint)), _gloffset_FramebufferTextureLayerEXT, parameters)
#define GET_FramebufferTextureLayerEXT(disp) GET_by_offset(disp, _gloffset_FramebufferTextureLayerEXT)
static void INLINE SET_FramebufferTextureLayerEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_FramebufferTextureLayerEXT, fn);
}

#define CALL_ColorMaskIndexedEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean)), _gloffset_ColorMaskIndexedEXT, parameters)
#define GET_ColorMaskIndexedEXT(disp) GET_by_offset(disp, _gloffset_ColorMaskIndexedEXT)
static void INLINE SET_ColorMaskIndexedEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLboolean, GLboolean, GLboolean, GLboolean)) {
   SET_by_offset(disp, _gloffset_ColorMaskIndexedEXT, fn);
}

#define CALL_DisableIndexedEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_DisableIndexedEXT, parameters)
#define GET_DisableIndexedEXT(disp) GET_by_offset(disp, _gloffset_DisableIndexedEXT)
static void INLINE SET_DisableIndexedEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_DisableIndexedEXT, fn);
}

#define CALL_EnableIndexedEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_EnableIndexedEXT, parameters)
#define GET_EnableIndexedEXT(disp) GET_by_offset(disp, _gloffset_EnableIndexedEXT)
static void INLINE SET_EnableIndexedEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_EnableIndexedEXT, fn);
}

#define CALL_GetBooleanIndexedvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLboolean *)), _gloffset_GetBooleanIndexedvEXT, parameters)
#define GET_GetBooleanIndexedvEXT(disp) GET_by_offset(disp, _gloffset_GetBooleanIndexedvEXT)
static void INLINE SET_GetBooleanIndexedvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLboolean *)) {
   SET_by_offset(disp, _gloffset_GetBooleanIndexedvEXT, fn);
}

#define CALL_GetIntegerIndexedvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLint *)), _gloffset_GetIntegerIndexedvEXT, parameters)
#define GET_GetIntegerIndexedvEXT(disp) GET_by_offset(disp, _gloffset_GetIntegerIndexedvEXT)
static void INLINE SET_GetIntegerIndexedvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLint *)) {
   SET_by_offset(disp, _gloffset_GetIntegerIndexedvEXT, fn);
}

#define CALL_IsEnabledIndexedEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_IsEnabledIndexedEXT, parameters)
#define GET_IsEnabledIndexedEXT(disp) GET_by_offset(disp, _gloffset_IsEnabledIndexedEXT)
static void INLINE SET_IsEnabledIndexedEXT(struct _glapi_table *disp, GLboolean (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_IsEnabledIndexedEXT, fn);
}

#define CALL_ClearColorIiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), _gloffset_ClearColorIiEXT, parameters)
#define GET_ClearColorIiEXT(disp) GET_by_offset(disp, _gloffset_ClearColorIiEXT)
static void INLINE SET_ClearColorIiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLint, GLint, GLint, GLint)) {
   SET_by_offset(disp, _gloffset_ClearColorIiEXT, fn);
}

#define CALL_ClearColorIuiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint, GLuint)), _gloffset_ClearColorIuiEXT, parameters)
#define GET_ClearColorIuiEXT(disp) GET_by_offset(disp, _gloffset_ClearColorIuiEXT)
static void INLINE SET_ClearColorIuiEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_ClearColorIuiEXT, fn);
}

#define CALL_GetTexParameterIivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), _gloffset_GetTexParameterIivEXT, parameters)
#define GET_GetTexParameterIivEXT(disp) GET_by_offset(disp, _gloffset_GetTexParameterIivEXT)
static void INLINE SET_GetTexParameterIivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetTexParameterIivEXT, fn);
}

#define CALL_GetTexParameterIuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLuint *)), _gloffset_GetTexParameterIuivEXT, parameters)
#define GET_GetTexParameterIuivEXT(disp) GET_by_offset(disp, _gloffset_GetTexParameterIuivEXT)
static void INLINE SET_GetTexParameterIuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLuint *)) {
   SET_by_offset(disp, _gloffset_GetTexParameterIuivEXT, fn);
}

#define CALL_TexParameterIivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLint *)), _gloffset_TexParameterIivEXT, parameters)
#define GET_TexParameterIivEXT(disp) GET_by_offset(disp, _gloffset_TexParameterIivEXT)
static void INLINE SET_TexParameterIivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLint *)) {
   SET_by_offset(disp, _gloffset_TexParameterIivEXT, fn);
}

#define CALL_TexParameterIuivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, const GLuint *)), _gloffset_TexParameterIuivEXT, parameters)
#define GET_TexParameterIuivEXT(disp) GET_by_offset(disp, _gloffset_TexParameterIuivEXT)
static void INLINE SET_TexParameterIuivEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, const GLuint *)) {
   SET_by_offset(disp, _gloffset_TexParameterIuivEXT, fn);
}

#define CALL_BeginConditionalRenderNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum)), _gloffset_BeginConditionalRenderNV, parameters)
#define GET_BeginConditionalRenderNV(disp) GET_by_offset(disp, _gloffset_BeginConditionalRenderNV)
static void INLINE SET_BeginConditionalRenderNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_BeginConditionalRenderNV, fn);
}

#define CALL_EndConditionalRenderNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndConditionalRenderNV, parameters)
#define GET_EndConditionalRenderNV(disp) GET_by_offset(disp, _gloffset_EndConditionalRenderNV)
static void INLINE SET_EndConditionalRenderNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndConditionalRenderNV, fn);
}

#define CALL_BeginTransformFeedbackEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_BeginTransformFeedbackEXT, parameters)
#define GET_BeginTransformFeedbackEXT(disp) GET_by_offset(disp, _gloffset_BeginTransformFeedbackEXT)
static void INLINE SET_BeginTransformFeedbackEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_BeginTransformFeedbackEXT, fn);
}

#define CALL_BindBufferBaseEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint)), _gloffset_BindBufferBaseEXT, parameters)
#define GET_BindBufferBaseEXT(disp) GET_by_offset(disp, _gloffset_BindBufferBaseEXT)
static void INLINE SET_BindBufferBaseEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint)) {
   SET_by_offset(disp, _gloffset_BindBufferBaseEXT, fn);
}

#define CALL_BindBufferOffsetEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLintptr)), _gloffset_BindBufferOffsetEXT, parameters)
#define GET_BindBufferOffsetEXT(disp) GET_by_offset(disp, _gloffset_BindBufferOffsetEXT)
static void INLINE SET_BindBufferOffsetEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLintptr)) {
   SET_by_offset(disp, _gloffset_BindBufferOffsetEXT, fn);
}

#define CALL_BindBufferRangeEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr)), _gloffset_BindBufferRangeEXT, parameters)
#define GET_BindBufferRangeEXT(disp) GET_by_offset(disp, _gloffset_BindBufferRangeEXT)
static void INLINE SET_BindBufferRangeEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLuint, GLintptr, GLsizeiptr)) {
   SET_by_offset(disp, _gloffset_BindBufferRangeEXT, fn);
}

#define CALL_EndTransformFeedbackEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_EndTransformFeedbackEXT, parameters)
#define GET_EndTransformFeedbackEXT(disp) GET_by_offset(disp, _gloffset_EndTransformFeedbackEXT)
static void INLINE SET_EndTransformFeedbackEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_EndTransformFeedbackEXT, fn);
}

#define CALL_GetTransformFeedbackVaryingEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *)), _gloffset_GetTransformFeedbackVaryingEXT, parameters)
#define GET_GetTransformFeedbackVaryingEXT(disp) GET_by_offset(disp, _gloffset_GetTransformFeedbackVaryingEXT)
static void INLINE SET_GetTransformFeedbackVaryingEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLuint, GLsizei, GLsizei *, GLsizei *, GLenum *, GLchar *)) {
   SET_by_offset(disp, _gloffset_GetTransformFeedbackVaryingEXT, fn);
}

#define CALL_TransformFeedbackVaryingsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const char **, GLenum)), _gloffset_TransformFeedbackVaryingsEXT, parameters)
#define GET_TransformFeedbackVaryingsEXT(disp) GET_by_offset(disp, _gloffset_TransformFeedbackVaryingsEXT)
static void INLINE SET_TransformFeedbackVaryingsEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLsizei, const char **, GLenum)) {
   SET_by_offset(disp, _gloffset_TransformFeedbackVaryingsEXT, fn);
}

#define CALL_ProvokingVertexEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), _gloffset_ProvokingVertexEXT, parameters)
#define GET_ProvokingVertexEXT(disp) GET_by_offset(disp, _gloffset_ProvokingVertexEXT)
static void INLINE SET_ProvokingVertexEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum)) {
   SET_by_offset(disp, _gloffset_ProvokingVertexEXT, fn);
}

#define CALL_GetTexParameterPointervAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid **)), _gloffset_GetTexParameterPointervAPPLE, parameters)
#define GET_GetTexParameterPointervAPPLE(disp) GET_by_offset(disp, _gloffset_GetTexParameterPointervAPPLE)
static void INLINE SET_GetTexParameterPointervAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLvoid **)) {
   SET_by_offset(disp, _gloffset_GetTexParameterPointervAPPLE, fn);
}

#define CALL_TextureRangeAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLvoid *)), _gloffset_TextureRangeAPPLE, parameters)
#define GET_TextureRangeAPPLE(disp) GET_by_offset(disp, _gloffset_TextureRangeAPPLE)
static void INLINE SET_TextureRangeAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLsizei, GLvoid *)) {
   SET_by_offset(disp, _gloffset_TextureRangeAPPLE, fn);
}

#define CALL_GetObjectParameterivAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLint *)), _gloffset_GetObjectParameterivAPPLE, parameters)
#define GET_GetObjectParameterivAPPLE(disp) GET_by_offset(disp, _gloffset_GetObjectParameterivAPPLE)
static void INLINE SET_GetObjectParameterivAPPLE(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLenum, GLint *)) {
   SET_by_offset(disp, _gloffset_GetObjectParameterivAPPLE, fn);
}

#define CALL_ObjectPurgeableAPPLE(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLenum, GLuint, GLenum)), _gloffset_ObjectPurgeableAPPLE, parameters)
#define GET_ObjectPurgeableAPPLE(disp) GET_by_offset(disp, _gloffset_ObjectPurgeableAPPLE)
static void INLINE SET_ObjectPurgeableAPPLE(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLenum, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_ObjectPurgeableAPPLE, fn);
}

#define CALL_ObjectUnpurgeableAPPLE(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLenum, GLuint, GLenum)), _gloffset_ObjectUnpurgeableAPPLE, parameters)
#define GET_ObjectUnpurgeableAPPLE(disp) GET_by_offset(disp, _gloffset_ObjectUnpurgeableAPPLE)
static void INLINE SET_ObjectUnpurgeableAPPLE(struct _glapi_table *disp, GLenum (GLAPIENTRYP fn)(GLenum, GLuint, GLenum)) {
   SET_by_offset(disp, _gloffset_ObjectUnpurgeableAPPLE, fn);
}

#define CALL_ActiveProgramEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), _gloffset_ActiveProgramEXT, parameters)
#define GET_ActiveProgramEXT(disp) GET_by_offset(disp, _gloffset_ActiveProgramEXT)
static void INLINE SET_ActiveProgramEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint)) {
   SET_by_offset(disp, _gloffset_ActiveProgramEXT, fn);
}

#define CALL_CreateShaderProgramEXT(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLenum, const GLchar *)), _gloffset_CreateShaderProgramEXT, parameters)
#define GET_CreateShaderProgramEXT(disp) GET_by_offset(disp, _gloffset_CreateShaderProgramEXT)
static void INLINE SET_CreateShaderProgramEXT(struct _glapi_table *disp, GLuint (GLAPIENTRYP fn)(GLenum, const GLchar *)) {
   SET_by_offset(disp, _gloffset_CreateShaderProgramEXT, fn);
}

#define CALL_UseShaderProgramEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), _gloffset_UseShaderProgramEXT, parameters)
#define GET_UseShaderProgramEXT(disp) GET_by_offset(disp, _gloffset_UseShaderProgramEXT)
static void INLINE SET_UseShaderProgramEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint)) {
   SET_by_offset(disp, _gloffset_UseShaderProgramEXT, fn);
}

#define CALL_TextureBarrierNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), _gloffset_TextureBarrierNV, parameters)
#define GET_TextureBarrierNV(disp) GET_by_offset(disp, _gloffset_TextureBarrierNV)
static void INLINE SET_TextureBarrierNV(struct _glapi_table *disp, void (GLAPIENTRYP fn)(void)) {
   SET_by_offset(disp, _gloffset_TextureBarrierNV, fn);
}

#define CALL_StencilFuncSeparateATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLuint)), _gloffset_StencilFuncSeparateATI, parameters)
#define GET_StencilFuncSeparateATI(disp) GET_by_offset(disp, _gloffset_StencilFuncSeparateATI)
static void INLINE SET_StencilFuncSeparateATI(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLenum, GLint, GLuint)) {
   SET_by_offset(disp, _gloffset_StencilFuncSeparateATI, fn);
}

#define CALL_ProgramEnvParameters4fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), _gloffset_ProgramEnvParameters4fvEXT, parameters)
#define GET_ProgramEnvParameters4fvEXT(disp) GET_by_offset(disp, _gloffset_ProgramEnvParameters4fvEXT)
static void INLINE SET_ProgramEnvParameters4fvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramEnvParameters4fvEXT, fn);
}

#define CALL_ProgramLocalParameters4fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), _gloffset_ProgramLocalParameters4fvEXT, parameters)
#define GET_ProgramLocalParameters4fvEXT(disp) GET_by_offset(disp, _gloffset_ProgramLocalParameters4fvEXT)
static void INLINE SET_ProgramLocalParameters4fvEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLuint, GLsizei, const GLfloat *)) {
   SET_by_offset(disp, _gloffset_ProgramLocalParameters4fvEXT, fn);
}

#define CALL_GetQueryObjecti64vEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint64EXT *)), _gloffset_GetQueryObjecti64vEXT, parameters)
#define GET_GetQueryObjecti64vEXT(disp) GET_by_offset(disp, _gloffset_GetQueryObjecti64vEXT)
static void INLINE SET_GetQueryObjecti64vEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLint64EXT *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjecti64vEXT, fn);
}

#define CALL_GetQueryObjectui64vEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint64EXT *)), _gloffset_GetQueryObjectui64vEXT, parameters)
#define GET_GetQueryObjectui64vEXT(disp) GET_by_offset(disp, _gloffset_GetQueryObjectui64vEXT)
static void INLINE SET_GetQueryObjectui64vEXT(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLuint, GLenum, GLuint64EXT *)) {
   SET_by_offset(disp, _gloffset_GetQueryObjectui64vEXT, fn);
}

#define CALL_EGLImageTargetRenderbufferStorageOES(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLvoid *)), _gloffset_EGLImageTargetRenderbufferStorageOES, parameters)
#define GET_EGLImageTargetRenderbufferStorageOES(disp) GET_by_offset(disp, _gloffset_EGLImageTargetRenderbufferStorageOES)
static void INLINE SET_EGLImageTargetRenderbufferStorageOES(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_EGLImageTargetRenderbufferStorageOES, fn);
}

#define CALL_EGLImageTargetTexture2DOES(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLvoid *)), _gloffset_EGLImageTargetTexture2DOES, parameters)
#define GET_EGLImageTargetTexture2DOES(disp) GET_by_offset(disp, _gloffset_EGLImageTargetTexture2DOES)
static void INLINE SET_EGLImageTargetTexture2DOES(struct _glapi_table *disp, void (GLAPIENTRYP fn)(GLenum, GLvoid *)) {
   SET_by_offset(disp, _gloffset_EGLImageTargetTexture2DOES, fn);
}


#endif /* !defined( _GLAPI_DISPATCH_H_ ) */
