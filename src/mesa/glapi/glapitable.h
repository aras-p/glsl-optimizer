/* DO NOT EDIT - This file generated automatically with gltable.py script */
#ifndef _GLAPI_TABLE_H_
#define _GLAPI_TABLE_H_

#include <GL/gl.h>

struct _glapi_table
{
   void (*NewList)(GLuint list, GLenum mode); /* 0 */
   void (*EndList)(void); /* 1 */
   void (*CallList)(GLuint list); /* 2 */
   void (*CallLists)(GLsizei n, GLenum type, const GLvoid * lists); /* 3 */
   void (*DeleteLists)(GLuint list, GLsizei range); /* 4 */
   GLuint (*GenLists)(GLsizei range); /* 5 */
   void (*ListBase)(GLuint base); /* 6 */
   void (*Begin)(GLenum mode); /* 7 */
   void (*Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap); /* 8 */
   void (*Color3b)(GLbyte red, GLbyte green, GLbyte blue); /* 9 */
   void (*Color3bv)(const GLbyte * v); /* 10 */
   void (*Color3d)(GLdouble red, GLdouble green, GLdouble blue); /* 11 */
   void (*Color3dv)(const GLdouble * v); /* 12 */
   void (*Color3f)(GLfloat red, GLfloat green, GLfloat blue); /* 13 */
   void (*Color3fv)(const GLfloat * v); /* 14 */
   void (*Color3i)(GLint red, GLint green, GLint blue); /* 15 */
   void (*Color3iv)(const GLint * v); /* 16 */
   void (*Color3s)(GLshort red, GLshort green, GLshort blue); /* 17 */
   void (*Color3sv)(const GLshort * v); /* 18 */
   void (*Color3ub)(GLubyte red, GLubyte green, GLubyte blue); /* 19 */
   void (*Color3ubv)(const GLubyte * v); /* 20 */
   void (*Color3ui)(GLuint red, GLuint green, GLuint blue); /* 21 */
   void (*Color3uiv)(const GLuint * v); /* 22 */
   void (*Color3us)(GLushort red, GLushort green, GLushort blue); /* 23 */
   void (*Color3usv)(const GLushort * v); /* 24 */
   void (*Color4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha); /* 25 */
   void (*Color4bv)(const GLbyte * v); /* 26 */
   void (*Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha); /* 27 */
   void (*Color4dv)(const GLdouble * v); /* 28 */
   void (*Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); /* 29 */
   void (*Color4fv)(const GLfloat * v); /* 30 */
   void (*Color4i)(GLint red, GLint green, GLint blue, GLint alpha); /* 31 */
   void (*Color4iv)(const GLint * v); /* 32 */
   void (*Color4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha); /* 33 */
   void (*Color4sv)(const GLshort * v); /* 34 */
   void (*Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha); /* 35 */
   void (*Color4ubv)(const GLubyte * v); /* 36 */
   void (*Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha); /* 37 */
   void (*Color4uiv)(const GLuint * v); /* 38 */
   void (*Color4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha); /* 39 */
   void (*Color4usv)(const GLushort * v); /* 40 */
   void (*EdgeFlag)(GLboolean flag); /* 41 */
   void (*EdgeFlagv)(const GLboolean * flag); /* 42 */
   void (*End)(void); /* 43 */
   void (*Indexd)(GLdouble c); /* 44 */
   void (*Indexdv)(const GLdouble * c); /* 45 */
   void (*Indexf)(GLfloat c); /* 46 */
   void (*Indexfv)(const GLfloat * c); /* 47 */
   void (*Indexi)(GLint c); /* 48 */
   void (*Indexiv)(const GLint * c); /* 49 */
   void (*Indexs)(GLshort c); /* 50 */
   void (*Indexsv)(const GLshort * c); /* 51 */
   void (*Normal3b)(GLbyte nx, GLbyte ny, GLbyte nz); /* 52 */
   void (*Normal3bv)(const GLbyte * v); /* 53 */
   void (*Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz); /* 54 */
   void (*Normal3dv)(const GLdouble * v); /* 55 */
   void (*Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz); /* 56 */
   void (*Normal3fv)(const GLfloat * v); /* 57 */
   void (*Normal3i)(GLint nx, GLint ny, GLint nz); /* 58 */
   void (*Normal3iv)(const GLint * v); /* 59 */
   void (*Normal3s)(GLshort nx, GLshort ny, GLshort nz); /* 60 */
   void (*Normal3sv)(const GLshort * v); /* 61 */
   void (*RasterPos2d)(GLdouble x, GLdouble y); /* 62 */
   void (*RasterPos2dv)(const GLdouble * v); /* 63 */
   void (*RasterPos2f)(GLfloat x, GLfloat y); /* 64 */
   void (*RasterPos2fv)(const GLfloat * v); /* 65 */
   void (*RasterPos2i)(GLint x, GLint y); /* 66 */
   void (*RasterPos2iv)(const GLint * v); /* 67 */
   void (*RasterPos2s)(GLshort x, GLshort y); /* 68 */
   void (*RasterPos2sv)(const GLshort * v); /* 69 */
   void (*RasterPos3d)(GLdouble x, GLdouble y, GLdouble z); /* 70 */
   void (*RasterPos3dv)(const GLdouble * v); /* 71 */
   void (*RasterPos3f)(GLfloat x, GLfloat y, GLfloat z); /* 72 */
   void (*RasterPos3fv)(const GLfloat * v); /* 73 */
   void (*RasterPos3i)(GLint x, GLint y, GLint z); /* 74 */
   void (*RasterPos3iv)(const GLint * v); /* 75 */
   void (*RasterPos3s)(GLshort x, GLshort y, GLshort z); /* 76 */
   void (*RasterPos3sv)(const GLshort * v); /* 77 */
   void (*RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 78 */
   void (*RasterPos4dv)(const GLdouble * v); /* 79 */
   void (*RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 80 */
   void (*RasterPos4fv)(const GLfloat * v); /* 81 */
   void (*RasterPos4i)(GLint x, GLint y, GLint z, GLint w); /* 82 */
   void (*RasterPos4iv)(const GLint * v); /* 83 */
   void (*RasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w); /* 84 */
   void (*RasterPos4sv)(const GLshort * v); /* 85 */
   void (*Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2); /* 86 */
   void (*Rectdv)(const GLdouble * v1, const GLdouble * v2); /* 87 */
   void (*Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2); /* 88 */
   void (*Rectfv)(const GLfloat * v1, const GLfloat * v2); /* 89 */
   void (*Recti)(GLint x1, GLint y1, GLint x2, GLint y2); /* 90 */
   void (*Rectiv)(const GLint * v1, const GLint * v2); /* 91 */
   void (*Rects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2); /* 92 */
   void (*Rectsv)(const GLshort * v1, const GLshort * v2); /* 93 */
   void (*TexCoord1d)(GLdouble s); /* 94 */
   void (*TexCoord1dv)(const GLdouble * v); /* 95 */
   void (*TexCoord1f)(GLfloat s); /* 96 */
   void (*TexCoord1fv)(const GLfloat * v); /* 97 */
   void (*TexCoord1i)(GLint s); /* 98 */
   void (*TexCoord1iv)(const GLint * v); /* 99 */
   void (*TexCoord1s)(GLshort s); /* 100 */
   void (*TexCoord1sv)(const GLshort * v); /* 101 */
   void (*TexCoord2d)(GLdouble s, GLdouble t); /* 102 */
   void (*TexCoord2dv)(const GLdouble * v); /* 103 */
   void (*TexCoord2f)(GLfloat s, GLfloat t); /* 104 */
   void (*TexCoord2fv)(const GLfloat * v); /* 105 */
   void (*TexCoord2i)(GLint s, GLint t); /* 106 */
   void (*TexCoord2iv)(const GLint * v); /* 107 */
   void (*TexCoord2s)(GLshort s, GLshort t); /* 108 */
   void (*TexCoord2sv)(const GLshort * v); /* 109 */
   void (*TexCoord3d)(GLdouble s, GLdouble t, GLdouble r); /* 110 */
   void (*TexCoord3dv)(const GLdouble * v); /* 111 */
   void (*TexCoord3f)(GLfloat s, GLfloat t, GLfloat r); /* 112 */
   void (*TexCoord3fv)(const GLfloat * v); /* 113 */
   void (*TexCoord3i)(GLint s, GLint t, GLint r); /* 114 */
   void (*TexCoord3iv)(const GLint * v); /* 115 */
   void (*TexCoord3s)(GLshort s, GLshort t, GLshort r); /* 116 */
   void (*TexCoord3sv)(const GLshort * v); /* 117 */
   void (*TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q); /* 118 */
   void (*TexCoord4dv)(const GLdouble * v); /* 119 */
   void (*TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q); /* 120 */
   void (*TexCoord4fv)(const GLfloat * v); /* 121 */
   void (*TexCoord4i)(GLint s, GLint t, GLint r, GLint q); /* 122 */
   void (*TexCoord4iv)(const GLint * v); /* 123 */
   void (*TexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q); /* 124 */
   void (*TexCoord4sv)(const GLshort * v); /* 125 */
   void (*Vertex2d)(GLdouble x, GLdouble y); /* 126 */
   void (*Vertex2dv)(const GLdouble * v); /* 127 */
   void (*Vertex2f)(GLfloat x, GLfloat y); /* 128 */
   void (*Vertex2fv)(const GLfloat * v); /* 129 */
   void (*Vertex2i)(GLint x, GLint y); /* 130 */
   void (*Vertex2iv)(const GLint * v); /* 131 */
   void (*Vertex2s)(GLshort x, GLshort y); /* 132 */
   void (*Vertex2sv)(const GLshort * v); /* 133 */
   void (*Vertex3d)(GLdouble x, GLdouble y, GLdouble z); /* 134 */
   void (*Vertex3dv)(const GLdouble * v); /* 135 */
   void (*Vertex3f)(GLfloat x, GLfloat y, GLfloat z); /* 136 */
   void (*Vertex3fv)(const GLfloat * v); /* 137 */
   void (*Vertex3i)(GLint x, GLint y, GLint z); /* 138 */
   void (*Vertex3iv)(const GLint * v); /* 139 */
   void (*Vertex3s)(GLshort x, GLshort y, GLshort z); /* 140 */
   void (*Vertex3sv)(const GLshort * v); /* 141 */
   void (*Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 142 */
   void (*Vertex4dv)(const GLdouble * v); /* 143 */
   void (*Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 144 */
   void (*Vertex4fv)(const GLfloat * v); /* 145 */
   void (*Vertex4i)(GLint x, GLint y, GLint z, GLint w); /* 146 */
   void (*Vertex4iv)(const GLint * v); /* 147 */
   void (*Vertex4s)(GLshort x, GLshort y, GLshort z, GLshort w); /* 148 */
   void (*Vertex4sv)(const GLshort * v); /* 149 */
   void (*ClipPlane)(GLenum plane, const GLdouble * equation); /* 150 */
   void (*ColorMaterial)(GLenum face, GLenum mode); /* 151 */
   void (*CullFace)(GLenum mode); /* 152 */
   void (*Fogf)(GLenum pname, GLfloat param); /* 153 */
   void (*Fogfv)(GLenum pname, const GLfloat * params); /* 154 */
   void (*Fogi)(GLenum pname, GLint param); /* 155 */
   void (*Fogiv)(GLenum pname, const GLint * params); /* 156 */
   void (*FrontFace)(GLenum mode); /* 157 */
   void (*Hint)(GLenum target, GLenum mode); /* 158 */
   void (*Lightf)(GLenum light, GLenum pname, GLfloat param); /* 159 */
   void (*Lightfv)(GLenum light, GLenum pname, const GLfloat * params); /* 160 */
   void (*Lighti)(GLenum light, GLenum pname, GLint param); /* 161 */
   void (*Lightiv)(GLenum light, GLenum pname, const GLint * params); /* 162 */
   void (*LightModelf)(GLenum pname, GLfloat param); /* 163 */
   void (*LightModelfv)(GLenum pname, const GLfloat * params); /* 164 */
   void (*LightModeli)(GLenum pname, GLint param); /* 165 */
   void (*LightModeliv)(GLenum pname, const GLint * params); /* 166 */
   void (*LineStipple)(GLint factor, GLushort pattern); /* 167 */
   void (*LineWidth)(GLfloat width); /* 168 */
   void (*Materialf)(GLenum face, GLenum pname, GLfloat param); /* 169 */
   void (*Materialfv)(GLenum face, GLenum pname, const GLfloat * params); /* 170 */
   void (*Materiali)(GLenum face, GLenum pname, GLint param); /* 171 */
   void (*Materialiv)(GLenum face, GLenum pname, const GLint * params); /* 172 */
   void (*PointSize)(GLfloat size); /* 173 */
   void (*PolygonMode)(GLenum face, GLenum mode); /* 174 */
   void (*PolygonStipple)(const GLubyte * mask); /* 175 */
   void (*Scissor)(GLint x, GLint y, GLsizei width, GLsizei height); /* 176 */
   void (*ShadeModel)(GLenum mode); /* 177 */
   void (*TexParameterf)(GLenum target, GLenum pname, GLfloat param); /* 178 */
   void (*TexParameterfv)(GLenum target, GLenum pname, const GLfloat * params); /* 179 */
   void (*TexParameteri)(GLenum target, GLenum pname, GLint param); /* 180 */
   void (*TexParameteriv)(GLenum target, GLenum pname, const GLint * params); /* 181 */
   void (*TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 182 */
   void (*TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 183 */
   void (*TexEnvf)(GLenum target, GLenum pname, GLfloat param); /* 184 */
   void (*TexEnvfv)(GLenum target, GLenum pname, const GLfloat * params); /* 185 */
   void (*TexEnvi)(GLenum target, GLenum pname, GLint param); /* 186 */
   void (*TexEnviv)(GLenum target, GLenum pname, const GLint * params); /* 187 */
   void (*TexGend)(GLenum coord, GLenum pname, GLdouble param); /* 188 */
   void (*TexGendv)(GLenum coord, GLenum pname, const GLdouble * params); /* 189 */
   void (*TexGenf)(GLenum coord, GLenum pname, GLfloat param); /* 190 */
   void (*TexGenfv)(GLenum coord, GLenum pname, const GLfloat * params); /* 191 */
   void (*TexGeni)(GLenum coord, GLenum pname, GLint param); /* 192 */
   void (*TexGeniv)(GLenum coord, GLenum pname, const GLint * params); /* 193 */
   void (*FeedbackBuffer)(GLsizei size, GLenum type, GLfloat * buffer); /* 194 */
   void (*SelectBuffer)(GLsizei size, GLuint * buffer); /* 195 */
   GLint (*RenderMode)(GLenum mode); /* 196 */
   void (*InitNames)(void); /* 197 */
   void (*LoadName)(GLuint name); /* 198 */
   void (*PassThrough)(GLfloat token); /* 199 */
   void (*PopName)(void); /* 200 */
   void (*PushName)(GLuint name); /* 201 */
   void (*DrawBuffer)(GLenum mode); /* 202 */
   void (*Clear)(GLbitfield mask); /* 203 */
   void (*ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); /* 204 */
   void (*ClearIndex)(GLfloat c); /* 205 */
   void (*ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha); /* 206 */
   void (*ClearStencil)(GLint s); /* 207 */
   void (*ClearDepth)(GLclampd depth); /* 208 */
   void (*StencilMask)(GLuint mask); /* 209 */
   void (*ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha); /* 210 */
   void (*DepthMask)(GLboolean flag); /* 211 */
   void (*IndexMask)(GLuint mask); /* 212 */
   void (*Accum)(GLenum op, GLfloat value); /* 213 */
   void (*Disable)(GLenum cap); /* 214 */
   void (*Enable)(GLenum cap); /* 215 */
   void (*Finish)(void); /* 216 */
   void (*Flush)(void); /* 217 */
   void (*PopAttrib)(void); /* 218 */
   void (*PushAttrib)(GLbitfield mask); /* 219 */
   void (*Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points); /* 220 */
   void (*Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points); /* 221 */
   void (*Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points); /* 222 */
   void (*Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points); /* 223 */
   void (*MapGrid1d)(GLint un, GLdouble u1, GLdouble u2); /* 224 */
   void (*MapGrid1f)(GLint un, GLfloat u1, GLfloat u2); /* 225 */
   void (*MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2); /* 226 */
   void (*MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2); /* 227 */
   void (*EvalCoord1d)(GLdouble u); /* 228 */
   void (*EvalCoord1dv)(const GLdouble * u); /* 229 */
   void (*EvalCoord1f)(GLfloat u); /* 230 */
   void (*EvalCoord1fv)(const GLfloat * u); /* 231 */
   void (*EvalCoord2d)(GLdouble u, GLdouble v); /* 232 */
   void (*EvalCoord2dv)(const GLdouble * u); /* 233 */
   void (*EvalCoord2f)(GLfloat u, GLfloat v); /* 234 */
   void (*EvalCoord2fv)(const GLfloat * u); /* 235 */
   void (*EvalMesh1)(GLenum mode, GLint i1, GLint i2); /* 236 */
   void (*EvalPoint1)(GLint i); /* 237 */
   void (*EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2); /* 238 */
   void (*EvalPoint2)(GLint i, GLint j); /* 239 */
   void (*AlphaFunc)(GLenum func, GLclampf ref); /* 240 */
   void (*BlendFunc)(GLenum sfactor, GLenum dfactor); /* 241 */
   void (*LogicOp)(GLenum opcode); /* 242 */
   void (*StencilFunc)(GLenum func, GLint ref, GLuint mask); /* 243 */
   void (*StencilOp)(GLenum fail, GLenum zfail, GLenum zpass); /* 244 */
   void (*DepthFunc)(GLenum func); /* 245 */
   void (*PixelZoom)(GLfloat xfactor, GLfloat yfactor); /* 246 */
   void (*PixelTransferf)(GLenum pname, GLfloat param); /* 247 */
   void (*PixelTransferi)(GLenum pname, GLint param); /* 248 */
   void (*PixelStoref)(GLenum pname, GLfloat param); /* 249 */
   void (*PixelStorei)(GLenum pname, GLint param); /* 250 */
   void (*PixelMapfv)(GLenum map, GLint mapsize, const GLfloat * values); /* 251 */
   void (*PixelMapuiv)(GLenum map, GLint mapsize, const GLuint * values); /* 252 */
   void (*PixelMapusv)(GLenum map, GLint mapsize, const GLushort * values); /* 253 */
   void (*ReadBuffer)(GLenum mode); /* 254 */
   void (*CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type); /* 255 */
   void (*ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels); /* 256 */
   void (*DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels); /* 257 */
   void (*GetBooleanv)(GLenum pname, GLboolean * params); /* 258 */
   void (*GetClipPlane)(GLenum plane, GLdouble * equation); /* 259 */
   void (*GetDoublev)(GLenum pname, GLdouble * params); /* 260 */
   GLenum (*GetError)(void); /* 261 */
   void (*GetFloatv)(GLenum pname, GLfloat * params); /* 262 */
   void (*GetIntegerv)(GLenum pname, GLint * params); /* 263 */
   void (*GetLightfv)(GLenum light, GLenum pname, GLfloat * params); /* 264 */
   void (*GetLightiv)(GLenum light, GLenum pname, GLint * params); /* 265 */
   void (*GetMapdv)(GLenum target, GLenum query, GLdouble * v); /* 266 */
   void (*GetMapfv)(GLenum target, GLenum query, GLfloat * v); /* 267 */
   void (*GetMapiv)(GLenum target, GLenum query, GLint * v); /* 268 */
   void (*GetMaterialfv)(GLenum face, GLenum pname, GLfloat * params); /* 269 */
   void (*GetMaterialiv)(GLenum face, GLenum pname, GLint * params); /* 270 */
   void (*GetPixelMapfv)(GLenum map, GLfloat * values); /* 271 */
   void (*GetPixelMapuiv)(GLenum map, GLuint * values); /* 272 */
   void (*GetPixelMapusv)(GLenum map, GLushort * values); /* 273 */
   void (*GetPolygonStipple)(GLubyte * mask); /* 274 */
   const GLubyte * (*GetString)(GLenum name); /* 275 */
   void (*GetTexEnvfv)(GLenum target, GLenum pname, GLfloat * params); /* 276 */
   void (*GetTexEnviv)(GLenum target, GLenum pname, GLint * params); /* 277 */
   void (*GetTexGendv)(GLenum coord, GLenum pname, GLdouble * params); /* 278 */
   void (*GetTexGenfv)(GLenum coord, GLenum pname, GLfloat * params); /* 279 */
   void (*GetTexGeniv)(GLenum coord, GLenum pname, GLint * params); /* 280 */
   void (*GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels); /* 281 */
   void (*GetTexParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 282 */
   void (*GetTexParameteriv)(GLenum target, GLenum pname, GLint * params); /* 283 */
   void (*GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat * params); /* 284 */
   void (*GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint * params); /* 285 */
   GLboolean (*IsEnabled)(GLenum cap); /* 286 */
   GLboolean (*IsList)(GLuint list); /* 287 */
   void (*DepthRange)(GLclampd zNear, GLclampd zFar); /* 288 */
   void (*Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar); /* 289 */
   void (*LoadIdentity)(void); /* 290 */
   void (*LoadMatrixf)(const GLfloat * m); /* 291 */
   void (*LoadMatrixd)(const GLdouble * m); /* 292 */
   void (*MatrixMode)(GLenum mode); /* 293 */
   void (*MultMatrixf)(const GLfloat * m); /* 294 */
   void (*MultMatrixd)(const GLdouble * m); /* 295 */
   void (*Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar); /* 296 */
   void (*PopMatrix)(void); /* 297 */
   void (*PushMatrix)(void); /* 298 */
   void (*Rotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z); /* 299 */
   void (*Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z); /* 300 */
   void (*Scaled)(GLdouble x, GLdouble y, GLdouble z); /* 301 */
   void (*Scalef)(GLfloat x, GLfloat y, GLfloat z); /* 302 */
   void (*Translated)(GLdouble x, GLdouble y, GLdouble z); /* 303 */
   void (*Translatef)(GLfloat x, GLfloat y, GLfloat z); /* 304 */
   void (*Viewport)(GLint x, GLint y, GLsizei width, GLsizei height); /* 305 */
   void (*ArrayElement)(GLint i); /* 306 */
   void (*BindTexture)(GLenum target, GLuint texture); /* 307 */
   void (*ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 308 */
   void (*DisableClientState)(GLenum array); /* 309 */
   void (*DrawArrays)(GLenum mode, GLint first, GLsizei count); /* 310 */
   void (*DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices); /* 311 */
   void (*EdgeFlagPointer)(GLsizei stride, const GLvoid * pointer); /* 312 */
   void (*EnableClientState)(GLenum array); /* 313 */
   void (*IndexPointer)(GLenum type, GLsizei stride, const GLvoid * pointer); /* 314 */
   void (*Indexub)(GLubyte c); /* 315 */
   void (*Indexubv)(const GLubyte * c); /* 316 */
   void (*InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid * pointer); /* 317 */
   void (*NormalPointer)(GLenum type, GLsizei stride, const GLvoid * pointer); /* 318 */
   void (*PolygonOffset)(GLfloat factor, GLfloat units); /* 319 */
   void (*TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 320 */
   void (*VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 321 */
   GLboolean (*AreTexturesResident)(GLsizei n, const GLuint * textures, GLboolean * residences); /* 322 */
   void (*CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border); /* 323 */
   void (*CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border); /* 324 */
   void (*CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width); /* 325 */
   void (*CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height); /* 326 */
   void (*DeleteTextures)(GLsizei n, const GLuint * textures); /* 327 */
   void (*GenTextures)(GLsizei n, GLuint * textures); /* 328 */
   void (*GetPointerv)(GLenum pname, GLvoid ** params); /* 329 */
   GLboolean (*IsTexture)(GLuint texture); /* 330 */
   void (*PrioritizeTextures)(GLsizei n, const GLuint * textures, const GLclampf * priorities); /* 331 */
   void (*TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels); /* 332 */
   void (*TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels); /* 333 */
   void (*PopClientAttrib)(void); /* 334 */
   void (*PushClientAttrib)(GLbitfield mask); /* 335 */
   void (*BlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha); /* 336 */
   void (*BlendEquation)(GLenum mode); /* 337 */
   void (*DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices); /* 338 */
   void (*ColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table); /* 339 */
   void (*ColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat * params); /* 340 */
   void (*ColorTableParameteriv)(GLenum target, GLenum pname, const GLint * params); /* 341 */
   void (*CopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width); /* 342 */
   void (*GetColorTable)(GLenum target, GLenum format, GLenum type, GLvoid * table); /* 343 */
   void (*GetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 344 */
   void (*GetColorTableParameteriv)(GLenum target, GLenum pname, GLint * params); /* 345 */
   void (*ColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data); /* 346 */
   void (*CopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width); /* 347 */
   void (*ConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image); /* 348 */
   void (*ConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image); /* 349 */
   void (*ConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params); /* 350 */
   void (*ConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat * params); /* 351 */
   void (*ConvolutionParameteri)(GLenum target, GLenum pname, GLint params); /* 352 */
   void (*ConvolutionParameteriv)(GLenum target, GLenum pname, const GLint * params); /* 353 */
   void (*CopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width); /* 354 */
   void (*CopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height); /* 355 */
   void (*GetConvolutionFilter)(GLenum target, GLenum format, GLenum type, GLvoid * image); /* 356 */
   void (*GetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 357 */
   void (*GetConvolutionParameteriv)(GLenum target, GLenum pname, GLint * params); /* 358 */
   void (*GetSeparableFilter)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span); /* 359 */
   void (*SeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column); /* 360 */
   void (*GetHistogram)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 361 */
   void (*GetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 362 */
   void (*GetHistogramParameteriv)(GLenum target, GLenum pname, GLint * params); /* 363 */
   void (*GetMinmax)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 364 */
   void (*GetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat * params); /* 365 */
   void (*GetMinmaxParameteriv)(GLenum target, GLenum pname, GLint * params); /* 366 */
   void (*Histogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink); /* 367 */
   void (*Minmax)(GLenum target, GLenum internalformat, GLboolean sink); /* 368 */
   void (*ResetHistogram)(GLenum target); /* 369 */
   void (*ResetMinmax)(GLenum target); /* 370 */
   void (*TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 371 */
   void (*TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels); /* 372 */
   void (*CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height); /* 373 */
   void (*ActiveTextureARB)(GLenum texture); /* 374 */
   void (*ClientActiveTextureARB)(GLenum texture); /* 375 */
   void (*MultiTexCoord1dARB)(GLenum target, GLdouble s); /* 376 */
   void (*MultiTexCoord1dvARB)(GLenum target, const GLdouble * v); /* 377 */
   void (*MultiTexCoord1fARB)(GLenum target, GLfloat s); /* 378 */
   void (*MultiTexCoord1fvARB)(GLenum target, const GLfloat * v); /* 379 */
   void (*MultiTexCoord1iARB)(GLenum target, GLint s); /* 380 */
   void (*MultiTexCoord1ivARB)(GLenum target, const GLint * v); /* 381 */
   void (*MultiTexCoord1sARB)(GLenum target, GLshort s); /* 382 */
   void (*MultiTexCoord1svARB)(GLenum target, const GLshort * v); /* 383 */
   void (*MultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t); /* 384 */
   void (*MultiTexCoord2dvARB)(GLenum target, const GLdouble * v); /* 385 */
   void (*MultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t); /* 386 */
   void (*MultiTexCoord2fvARB)(GLenum target, const GLfloat * v); /* 387 */
   void (*MultiTexCoord2iARB)(GLenum target, GLint s, GLint t); /* 388 */
   void (*MultiTexCoord2ivARB)(GLenum target, const GLint * v); /* 389 */
   void (*MultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t); /* 390 */
   void (*MultiTexCoord2svARB)(GLenum target, const GLshort * v); /* 391 */
   void (*MultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r); /* 392 */
   void (*MultiTexCoord3dvARB)(GLenum target, const GLdouble * v); /* 393 */
   void (*MultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r); /* 394 */
   void (*MultiTexCoord3fvARB)(GLenum target, const GLfloat * v); /* 395 */
   void (*MultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r); /* 396 */
   void (*MultiTexCoord3ivARB)(GLenum target, const GLint * v); /* 397 */
   void (*MultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r); /* 398 */
   void (*MultiTexCoord3svARB)(GLenum target, const GLshort * v); /* 399 */
   void (*MultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q); /* 400 */
   void (*MultiTexCoord4dvARB)(GLenum target, const GLdouble * v); /* 401 */
   void (*MultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q); /* 402 */
   void (*MultiTexCoord4fvARB)(GLenum target, const GLfloat * v); /* 403 */
   void (*MultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q); /* 404 */
   void (*MultiTexCoord4ivARB)(GLenum target, const GLint * v); /* 405 */
   void (*MultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q); /* 406 */
   void (*MultiTexCoord4svARB)(GLenum target, const GLshort * v); /* 407 */
   void (*LoadTransposeMatrixfARB)(const GLfloat * m); /* 408 */
   void (*LoadTransposeMatrixdARB)(const GLdouble * m); /* 409 */
   void (*MultTransposeMatrixfARB)(const GLfloat * m); /* 410 */
   void (*MultTransposeMatrixdARB)(const GLdouble * m); /* 411 */
   void (*SampleCoverageARB)(GLclampf value, GLboolean invert); /* 412 */
   void (*__unused413)(void); /* 413 */
   void (*PolygonOffsetEXT)(GLfloat factor, GLfloat bias); /* 414 */
   void (*GetTexFilterFuncSGIS)(GLenum target, GLenum filter, GLfloat * weights); /* 415 */
   void (*TexFilterFuncSGIS)(GLenum target, GLenum filter, GLsizei n, const GLfloat * weights); /* 416 */
   void (*GetHistogramEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 417 */
   void (*GetHistogramParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 418 */
   void (*GetHistogramParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 419 */
   void (*GetMinmaxEXT)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values); /* 420 */
   void (*GetMinmaxParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 421 */
   void (*GetMinmaxParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 422 */
   void (*GetConvolutionFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid * image); /* 423 */
   void (*GetConvolutionParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 424 */
   void (*GetConvolutionParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 425 */
   void (*GetSeparableFilterEXT)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span); /* 426 */
   void (*GetColorTableSGI)(GLenum target, GLenum format, GLenum type, GLvoid * table); /* 427 */
   void (*GetColorTableParameterfvSGI)(GLenum target, GLenum pname, GLfloat * params); /* 428 */
   void (*GetColorTableParameterivSGI)(GLenum target, GLenum pname, GLint * params); /* 429 */
   void (*PixelTexGenSGIX)(GLenum mode); /* 430 */
   void (*PixelTexGenParameteriSGIS)(GLenum pname, GLint param); /* 431 */
   void (*PixelTexGenParameterivSGIS)(GLenum pname, const GLint * params); /* 432 */
   void (*PixelTexGenParameterfSGIS)(GLenum pname, GLfloat param); /* 433 */
   void (*PixelTexGenParameterfvSGIS)(GLenum pname, const GLfloat * params); /* 434 */
   void (*GetPixelTexGenParameterivSGIS)(GLenum pname, GLint * params); /* 435 */
   void (*GetPixelTexGenParameterfvSGIS)(GLenum pname, GLfloat * params); /* 436 */
   void (*TexImage4DSGIS)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid * pixels); /* 437 */
   void (*TexSubImage4DSGIS)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid * pixels); /* 438 */
   GLboolean (*AreTexturesResidentEXT)(GLsizei n, const GLuint * textures, GLboolean * residences); /* 439 */
   void (*GenTexturesEXT)(GLsizei n, GLuint * textures); /* 440 */
   GLboolean (*IsTextureEXT)(GLuint texture); /* 441 */
   void (*DetailTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat * points); /* 442 */
   void (*GetDetailTexFuncSGIS)(GLenum target, GLfloat * points); /* 443 */
   void (*SharpenTexFuncSGIS)(GLenum target, GLsizei n, const GLfloat * points); /* 444 */
   void (*GetSharpenTexFuncSGIS)(GLenum target, GLfloat * points); /* 445 */
   void (*SampleMaskSGIS)(GLclampf value, GLboolean invert); /* 446 */
   void (*SamplePatternSGIS)(GLenum pattern); /* 447 */
   void (*ColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 448 */
   void (*EdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean * pointer); /* 449 */
   void (*IndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 450 */
   void (*NormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 451 */
   void (*TexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 452 */
   void (*VertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer); /* 453 */
   void (*SpriteParameterfSGIX)(GLenum pname, GLfloat param); /* 454 */
   void (*SpriteParameterfvSGIX)(GLenum pname, const GLfloat * params); /* 455 */
   void (*SpriteParameteriSGIX)(GLenum pname, GLint param); /* 456 */
   void (*SpriteParameterivSGIX)(GLenum pname, const GLint * params); /* 457 */
   void (*PointParameterfEXT)(GLenum pname, GLfloat param); /* 458 */
   void (*PointParameterfvEXT)(GLenum pname, const GLfloat * params); /* 459 */
   GLint (*GetInstrumentsSGIX)(void); /* 460 */
   void (*InstrumentsBufferSGIX)(GLsizei size, GLint * buffer); /* 461 */
   GLint (*PollInstrumentsSGIX)(GLint * marker_p); /* 462 */
   void (*ReadInstrumentsSGIX)(GLint marker); /* 463 */
   void (*StartInstrumentsSGIX)(void); /* 464 */
   void (*StopInstrumentsSGIX)(GLint marker); /* 465 */
   void (*FrameZoomSGIX)(GLint factor); /* 466 */
   void (*TagSampleBufferSGIX)(void); /* 467 */
   void (*ReferencePlaneSGIX)(const GLdouble * equation); /* 468 */
   void (*FlushRasterSGIX)(void); /* 469 */
   void (*GetListParameterfvSGIX)(GLuint list, GLenum pname, GLfloat * params); /* 470 */
   void (*GetListParameterivSGIX)(GLuint list, GLenum pname, GLint * params); /* 471 */
   void (*ListParameterfSGIX)(GLuint list, GLenum pname, GLfloat param); /* 472 */
   void (*ListParameterfvSGIX)(GLuint list, GLenum pname, const GLfloat * params); /* 473 */
   void (*ListParameteriSGIX)(GLuint list, GLenum pname, GLint param); /* 474 */
   void (*ListParameterivSGIX)(GLuint list, GLenum pname, const GLint * params); /* 475 */
   void (*FragmentColorMaterialSGIX)(GLenum face, GLenum mode); /* 476 */
   void (*FragmentLightfSGIX)(GLenum light, GLenum pname, GLfloat param); /* 477 */
   void (*FragmentLightfvSGIX)(GLenum light, GLenum pname, const GLfloat * params); /* 478 */
   void (*FragmentLightiSGIX)(GLenum light, GLenum pname, GLint param); /* 479 */
   void (*FragmentLightivSGIX)(GLenum light, GLenum pname, const GLint * params); /* 480 */
   void (*FragmentLightModelfSGIX)(GLenum pname, GLfloat param); /* 481 */
   void (*FragmentLightModelfvSGIX)(GLenum pname, const GLfloat * params); /* 482 */
   void (*FragmentLightModeliSGIX)(GLenum pname, GLint param); /* 483 */
   void (*FragmentLightModelivSGIX)(GLenum pname, const GLint * params); /* 484 */
   void (*FragmentMaterialfSGIX)(GLenum face, GLenum pname, GLfloat param); /* 485 */
   void (*FragmentMaterialfvSGIX)(GLenum face, GLenum pname, const GLfloat * params); /* 486 */
   void (*FragmentMaterialiSGIX)(GLenum face, GLenum pname, GLint param); /* 487 */
   void (*FragmentMaterialivSGIX)(GLenum face, GLenum pname, const GLint * params); /* 488 */
   void (*GetFragmentLightfvSGIX)(GLenum light, GLenum pname, GLfloat * params); /* 489 */
   void (*GetFragmentLightivSGIX)(GLenum light, GLenum pname, GLint * params); /* 490 */
   void (*GetFragmentMaterialfvSGIX)(GLenum face, GLenum pname, GLfloat * params); /* 491 */
   void (*GetFragmentMaterialivSGIX)(GLenum face, GLenum pname, GLint * params); /* 492 */
   void (*LightEnviSGIX)(GLenum pname, GLint param); /* 493 */
   void (*VertexWeightfEXT)(GLfloat weight); /* 494 */
   void (*VertexWeightfvEXT)(const GLfloat * weight); /* 495 */
   void (*VertexWeightPointerEXT)(GLsizei size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 496 */
   void (*FlushVertexArrayRangeNV)(void); /* 497 */
   void (*VertexArrayRangeNV)(GLsizei length, const GLvoid * pointer); /* 498 */
   void (*CombinerParameterfvNV)(GLenum pname, const GLfloat * params); /* 499 */
   void (*CombinerParameterfNV)(GLenum pname, GLfloat param); /* 500 */
   void (*CombinerParameterivNV)(GLenum pname, const GLint * params); /* 501 */
   void (*CombinerParameteriNV)(GLenum pname, GLint param); /* 502 */
   void (*CombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage); /* 503 */
   void (*CombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum); /* 504 */
   void (*FinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage); /* 505 */
   void (*GetCombinerInputParameterfvNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params); /* 506 */
   void (*GetCombinerInputParameterivNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params); /* 507 */
   void (*GetCombinerOutputParameterfvNV)(GLenum stage, GLenum portion, GLenum pname, GLfloat * params); /* 508 */
   void (*GetCombinerOutputParameterivNV)(GLenum stage, GLenum portion, GLenum pname, GLint * params); /* 509 */
   void (*GetFinalCombinerInputParameterfvNV)(GLenum variable, GLenum pname, GLfloat * params); /* 510 */
   void (*GetFinalCombinerInputParameterivNV)(GLenum variable, GLenum pname, GLint * params); /* 511 */
   void (*ResizeBuffersMESA)(void); /* 512 */
   void (*WindowPos2dMESA)(GLdouble x, GLdouble y); /* 513 */
   void (*WindowPos2dvMESA)(const GLdouble * v); /* 514 */
   void (*WindowPos2fMESA)(GLfloat x, GLfloat y); /* 515 */
   void (*WindowPos2fvMESA)(const GLfloat * v); /* 516 */
   void (*WindowPos2iMESA)(GLint x, GLint y); /* 517 */
   void (*WindowPos2ivMESA)(const GLint * v); /* 518 */
   void (*WindowPos2sMESA)(GLshort x, GLshort y); /* 519 */
   void (*WindowPos2svMESA)(const GLshort * v); /* 520 */
   void (*WindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z); /* 521 */
   void (*WindowPos3dvMESA)(const GLdouble * v); /* 522 */
   void (*WindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z); /* 523 */
   void (*WindowPos3fvMESA)(const GLfloat * v); /* 524 */
   void (*WindowPos3iMESA)(GLint x, GLint y, GLint z); /* 525 */
   void (*WindowPos3ivMESA)(const GLint * v); /* 526 */
   void (*WindowPos3sMESA)(GLshort x, GLshort y, GLshort z); /* 527 */
   void (*WindowPos3svMESA)(const GLshort * v); /* 528 */
   void (*WindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 529 */
   void (*WindowPos4dvMESA)(const GLdouble * v); /* 530 */
   void (*WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 531 */
   void (*WindowPos4fvMESA)(const GLfloat * v); /* 532 */
   void (*WindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w); /* 533 */
   void (*WindowPos4ivMESA)(const GLint * v); /* 534 */
   void (*WindowPos4sMESA)(GLshort x, GLshort y, GLshort z, GLshort w); /* 535 */
   void (*WindowPos4svMESA)(const GLshort * v); /* 536 */
   void (*BlendFuncSeparateEXT)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha); /* 537 */
   void (*IndexMaterialEXT)(GLenum face, GLenum mode); /* 538 */
   void (*IndexFuncEXT)(GLenum func, GLclampf ref); /* 539 */
   void (*LockArraysEXT)(GLint first, GLsizei count); /* 540 */
   void (*UnlockArraysEXT)(void); /* 541 */
   void (*CullParameterdvEXT)(GLenum pname, GLdouble * params); /* 542 */
   void (*CullParameterfvEXT)(GLenum pname, GLfloat * params); /* 543 */
   void (*HintPGI)(GLenum target, GLint mode); /* 544 */
   void (*FogCoordfEXT)(GLfloat coord); /* 545 */
   void (*FogCoordfvEXT)(const GLfloat * coord); /* 546 */
   void (*FogCoorddEXT)(GLdouble coord); /* 547 */
   void (*FogCoorddvEXT)(const GLdouble * coord); /* 548 */
   void (*FogCoordPointerEXT)(GLenum type, GLsizei stride, const GLvoid * pointer); /* 549 */
   void (*GetColorTableEXT)(GLenum target, GLenum format, GLenum type, GLvoid * data); /* 550 */
   void (*GetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint * params); /* 551 */
   void (*GetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params); /* 552 */
   void (*TbufferMask3DFX)(GLuint mask); /* 553 */
   void (*CompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data); /* 554 */
   void (*CompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data); /* 555 */
   void (*CompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data); /* 556 */
   void (*CompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data); /* 557 */
   void (*CompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data); /* 558 */
   void (*CompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data); /* 559 */
   void (*GetCompressedTexImageARB)(GLenum target, GLint level, GLvoid * img); /* 560 */
   void (*SecondaryColor3bEXT)(GLbyte red, GLbyte green, GLbyte blue); /* 561 */
   void (*SecondaryColor3bvEXT)(const GLbyte * v); /* 562 */
   void (*SecondaryColor3dEXT)(GLdouble red, GLdouble green, GLdouble blue); /* 563 */
   void (*SecondaryColor3dvEXT)(const GLdouble * v); /* 564 */
   void (*SecondaryColor3fEXT)(GLfloat red, GLfloat green, GLfloat blue); /* 565 */
   void (*SecondaryColor3fvEXT)(const GLfloat * v); /* 566 */
   void (*SecondaryColor3iEXT)(GLint red, GLint green, GLint blue); /* 567 */
   void (*SecondaryColor3ivEXT)(const GLint * v); /* 568 */
   void (*SecondaryColor3sEXT)(GLshort red, GLshort green, GLshort blue); /* 569 */
   void (*SecondaryColor3svEXT)(const GLshort * v); /* 570 */
   void (*SecondaryColor3ubEXT)(GLubyte red, GLubyte green, GLubyte blue); /* 571 */
   void (*SecondaryColor3ubvEXT)(const GLubyte * v); /* 572 */
   void (*SecondaryColor3uiEXT)(GLuint red, GLuint green, GLuint blue); /* 573 */
   void (*SecondaryColor3uivEXT)(const GLuint * v); /* 574 */
   void (*SecondaryColor3usEXT)(GLushort red, GLushort green, GLushort blue); /* 575 */
   void (*SecondaryColor3usvEXT)(const GLushort * v); /* 576 */
   void (*SecondaryColorPointerEXT)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 577 */
   GLboolean (*AreProgramsResidentNV)(GLsizei n, const GLuint * ids, GLboolean * residences); /* 578 */
   void (*BindProgramNV)(GLenum target, GLuint id); /* 579 */
   void (*DeleteProgramsNV)(GLsizei n, const GLuint * ids); /* 580 */
   void (*ExecuteProgramNV)(GLenum target, GLuint id, const GLfloat * params); /* 581 */
   void (*GenProgramsNV)(GLsizei n, GLuint * ids); /* 582 */
   void (*GetProgramParameterdvNV)(GLenum target, GLuint index, GLenum pname, GLdouble * params); /* 583 */
   void (*GetProgramParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat * params); /* 584 */
   void (*GetProgramivNV)(GLuint id, GLenum pname, GLint * params); /* 585 */
   void (*GetProgramStringNV)(GLuint id, GLenum pname, GLubyte * program); /* 586 */
   void (*GetTrackMatrixivNV)(GLenum target, GLuint address, GLenum pname, GLint * params); /* 587 */
   void (*GetVertexAttribdvNV)(GLuint index, GLenum pname, GLdouble * params); /* 588 */
   void (*GetVertexAttribfvNV)(GLuint index, GLenum pname, GLfloat * params); /* 589 */
   void (*GetVertexAttribivNV)(GLuint index, GLenum pname, GLint * params); /* 590 */
   void (*GetVertexAttribPointervNV)(GLuint index, GLenum pname, GLvoid ** pointer); /* 591 */
   GLboolean (*IsProgramNV)(GLuint id); /* 592 */
   void (*LoadProgramNV)(GLenum target, GLuint id, GLsizei len, const GLubyte * program); /* 593 */
   void (*ProgramParameter4dNV)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 594 */
   void (*ProgramParameter4dvNV)(GLenum target, GLuint index, const GLdouble * params); /* 595 */
   void (*ProgramParameter4fNV)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 596 */
   void (*ProgramParameter4fvNV)(GLenum target, GLuint index, const GLfloat * params); /* 597 */
   void (*ProgramParameters4dvNV)(GLenum target, GLuint index, GLuint num, const GLdouble * params); /* 598 */
   void (*ProgramParameters4fvNV)(GLenum target, GLuint index, GLuint num, const GLfloat * params); /* 599 */
   void (*RequestResidentProgramsNV)(GLsizei n, const GLuint * ids); /* 600 */
   void (*TrackMatrixNV)(GLenum target, GLuint address, GLenum matrix, GLenum transform); /* 601 */
   void (*VertexAttribPointerNV)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer); /* 602 */
   void (*VertexAttrib1dNV)(GLuint index, GLdouble x); /* 603 */
   void (*VertexAttrib1dvNV)(GLuint index, const GLdouble * v); /* 604 */
   void (*VertexAttrib1fNV)(GLuint index, GLfloat x); /* 605 */
   void (*VertexAttrib1fvNV)(GLuint index, const GLfloat * v); /* 606 */
   void (*VertexAttrib1sNV)(GLuint index, GLshort x); /* 607 */
   void (*VertexAttrib1svNV)(GLuint index, const GLshort * v); /* 608 */
   void (*VertexAttrib2dNV)(GLuint index, GLdouble x, GLdouble y); /* 609 */
   void (*VertexAttrib2dvNV)(GLuint index, const GLdouble * v); /* 610 */
   void (*VertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y); /* 611 */
   void (*VertexAttrib2fvNV)(GLuint index, const GLfloat * v); /* 612 */
   void (*VertexAttrib2sNV)(GLuint index, GLshort x, GLshort y); /* 613 */
   void (*VertexAttrib2svNV)(GLuint index, const GLshort * v); /* 614 */
   void (*VertexAttrib3dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z); /* 615 */
   void (*VertexAttrib3dvNV)(GLuint index, const GLdouble * v); /* 616 */
   void (*VertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z); /* 617 */
   void (*VertexAttrib3fvNV)(GLuint index, const GLfloat * v); /* 618 */
   void (*VertexAttrib3sNV)(GLuint index, GLshort x, GLshort y, GLshort z); /* 619 */
   void (*VertexAttrib3svNV)(GLuint index, const GLshort * v); /* 620 */
   void (*VertexAttrib4dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w); /* 621 */
   void (*VertexAttrib4dvNV)(GLuint index, const GLdouble * v); /* 622 */
   void (*VertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w); /* 623 */
   void (*VertexAttrib4fvNV)(GLuint index, const GLfloat * v); /* 624 */
   void (*VertexAttrib4sNV)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w); /* 625 */
   void (*VertexAttrib4svNV)(GLuint index, const GLshort * v); /* 626 */
   void (*VertexAttrib4ubNV)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w); /* 627 */
   void (*VertexAttrib4ubvNV)(GLuint index, const GLubyte * v); /* 628 */
   void (*VertexAttribs1dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 629 */
   void (*VertexAttribs1fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 630 */
   void (*VertexAttribs1svNV)(GLuint index, GLsizei n, const GLshort * v); /* 631 */
   void (*VertexAttribs2dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 632 */
   void (*VertexAttribs2fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 633 */
   void (*VertexAttribs2svNV)(GLuint index, GLsizei n, const GLshort * v); /* 634 */
   void (*VertexAttribs3dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 635 */
   void (*VertexAttribs3fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 636 */
   void (*VertexAttribs3svNV)(GLuint index, GLsizei n, const GLshort * v); /* 637 */
   void (*VertexAttribs4dvNV)(GLuint index, GLsizei n, const GLdouble * v); /* 638 */
   void (*VertexAttribs4fvNV)(GLuint index, GLsizei n, const GLfloat * v); /* 639 */
   void (*VertexAttribs4svNV)(GLuint index, GLsizei n, const GLshort * v); /* 640 */
   void (*VertexAttribs4ubvNV)(GLuint index, GLsizei n, const GLubyte * v); /* 641 */
   void (*PointParameteriNV)(GLenum pname, GLint params); /* 642 */
   void (*PointParameterivNV)(GLenum pname, const GLint * params); /* 643 */
   void (*MultiDrawArraysEXT)(GLenum mode, GLint * first, GLsizei * count, GLsizei primcount); /* 644 */
   void (*MultiDrawElementsEXT)(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount); /* 645 */
   void (*ActiveStencilFaceEXT)(GLenum face); /* 646 */
   void (*DeleteFencesNV)(GLsizei n, const GLuint * fences); /* 647 */
   void (*GenFencesNV)(GLsizei n, GLuint * fences); /* 648 */
   GLboolean (*IsFenceNV)(GLuint fence); /* 649 */
   GLboolean (*TestFenceNV)(GLuint fence); /* 650 */
   void (*GetFenceivNV)(GLuint fence, GLenum pname, GLint * params); /* 651 */
   void (*FinishFenceNV)(GLuint fence); /* 652 */
   void (*SetFenceNV)(GLuint fence, GLenum condition); /* 653 */
};

#endif
