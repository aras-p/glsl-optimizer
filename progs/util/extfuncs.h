/**
 * Utility for getting OpenGL extension function pointers
 * Meant to be #included.
 */

/* OpenGL 2.0 */
static PFNGLATTACHSHADERPROC glAttachShader_func = NULL;
static PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation_func = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader_func = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram_func = NULL;
static PFNGLCREATESHADERPROC glCreateShader_func = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram_func = NULL;
static PFNGLDELETESHADERPROC glDeleteShader_func = NULL;
static PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib_func = NULL;
static PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform_func = NULL;
static PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders_func = NULL;
static PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation_func = NULL;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_func = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_func = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv_func = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv_func = NULL;
static PFNGLGETSHADERSOURCEPROC glGetShaderSource_func = NULL;
static PFNGLGETUNIFORMFVPROC glGetUniformfv_func = NULL;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_func = NULL;
static PFNGLISPROGRAMPROC glIsProgram_func = NULL;
static PFNGLISSHADERPROC glIsShader_func = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram_func = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource_func = NULL;
static PFNGLUNIFORM1IPROC glUniform1i_func = NULL;
static PFNGLUNIFORM2IPROC glUniform2i_func = NULL;
static PFNGLUNIFORM3IPROC glUniform3i_func = NULL;
static PFNGLUNIFORM4IPROC glUniform4i_func = NULL;
static PFNGLUNIFORM1FPROC glUniform1f_func = NULL;
static PFNGLUNIFORM2FPROC glUniform2f_func = NULL;
static PFNGLUNIFORM3FPROC glUniform3f_func = NULL;
static PFNGLUNIFORM4FPROC glUniform4f_func = NULL;
static PFNGLUNIFORM1FVPROC glUniform1fv_func = NULL;
static PFNGLUNIFORM2FVPROC glUniform2fv_func = NULL;
static PFNGLUNIFORM3FVPROC glUniform3fv_func = NULL;
static PFNGLUNIFORM4FVPROC glUniform4fv_func = NULL;
static PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv_func = NULL;
static PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv_func = NULL;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv_func = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram_func = NULL;
static PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f_func = NULL;
static PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f_func = NULL;
static PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f_func = NULL;
static PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f_func = NULL;
static PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv_func = NULL;
static PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv_func = NULL;
static PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv_func = NULL;
static PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv_func = NULL;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer_func = NULL;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray_func = NULL;
static PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray_func = NULL;

/* OpenGL 2.1 */
static PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv_func = NULL;
static PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv_func = NULL;
static PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv_func = NULL;
static PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv_func = NULL;
static PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv_func = NULL;
static PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv_func = NULL;

/* OpenGL 1.4 */
static PFNGLPOINTPARAMETERFVPROC glPointParameterfv_func = NULL;
static PFNGLSECONDARYCOLOR3FVPROC glSecondaryColor3fv_func = NULL;

/* GL_ARB_vertex/fragment_program */
static PFNGLBINDPROGRAMARBPROC glBindProgramARB_func = NULL;
static PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB_func = NULL;
static PFNGLGENPROGRAMSARBPROC glGenProgramsARB_func = NULL;
static PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC glGetProgramLocalParameterdvARB_func = NULL;
static PFNGLISPROGRAMARBPROC glIsProgramARB_func = NULL;
static PFNGLPROGRAMLOCALPARAMETER4DARBPROC glProgramLocalParameter4dARB_func = NULL;
static PFNGLPROGRAMLOCALPARAMETER4FVARBPROC glProgramLocalParameter4fvARB_func = NULL;
static PFNGLPROGRAMSTRINGARBPROC glProgramStringARB_func = NULL;
static PFNGLVERTEXATTRIB1FARBPROC glVertexAttrib1fARB_func = NULL;

/* GL_APPLE_vertex_array_object */
static PFNGLBINDVERTEXARRAYAPPLEPROC glBindVertexArrayAPPLE_func = NULL;
static PFNGLDELETEVERTEXARRAYSAPPLEPROC glDeleteVertexArraysAPPLE_func = NULL;
static PFNGLGENVERTEXARRAYSAPPLEPROC glGenVertexArraysAPPLE_func = NULL;
static PFNGLISVERTEXARRAYAPPLEPROC glIsVertexArrayAPPLE_func = NULL;

/* GL_EXT_stencil_two_side */
static PFNGLACTIVESTENCILFACEEXTPROC glActiveStencilFaceEXT_func = NULL;

/* GL_ARB_buffer_object */
static PFNGLGENBUFFERSARBPROC glGenBuffersARB_func = NULL;
static PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB_func = NULL;
static PFNGLBINDBUFFERARBPROC glBindBufferARB_func = NULL;
static PFNGLBUFFERDATAARBPROC glBufferDataARB_func = NULL;
static PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB_func = NULL;
static PFNGLMAPBUFFERARBPROC glMapBufferARB_func = NULL;
static PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB_func = NULL;

/* GL_EXT_framebuffer_object */
static PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT_func = NULL;
static PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT_func = NULL;
static PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT_func = NULL;
static PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT_func = NULL;
static PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT_func = NULL;
static PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT_func = NULL;
static PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT_func = NULL;
static PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT_func = NULL;
static PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT_func = NULL;
static PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT_func = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT_func = NULL;
static PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT_func = NULL;
static PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT_func = NULL;
static PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT_func = NULL;

/* GL_ARB_framebuffer_object */
static PFNGLISRENDERBUFFERPROC glIsRenderbuffer_func = NULL;
static PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer_func = NULL;
static PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers_func = NULL;
static PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers_func = NULL;
static PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage_func = NULL;
static PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv_func = NULL;
static PFNGLISFRAMEBUFFERPROC glIsFramebuffer_func = NULL;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer_func = NULL;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers_func = NULL;
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers_func = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D_func = NULL;
static PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D_func = NULL;
static PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer_func = NULL;
static PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv_func = NULL;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap_func = NULL;
static PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer_func = NULL;
static PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample_func = NULL;
static PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer_func = NULL;


static void
GetExtensionFuncs(void)
{
   /* OpenGL 2.0 */
   glAttachShader_func = (PFNGLATTACHSHADERPROC) glutGetProcAddress("glAttachShader");
   glBindAttribLocation_func = (PFNGLBINDATTRIBLOCATIONPROC) glutGetProcAddress("glBindAttribLocation");
   glCompileShader_func = (PFNGLCOMPILESHADERPROC) glutGetProcAddress("glCompileShader");
   glCreateProgram_func = (PFNGLCREATEPROGRAMPROC) glutGetProcAddress("glCreateProgram");
   glCreateShader_func = (PFNGLCREATESHADERPROC) glutGetProcAddress("glCreateShader");
   glDeleteProgram_func = (PFNGLDELETEPROGRAMPROC) glutGetProcAddress("glDeleteProgram");
   glDeleteShader_func = (PFNGLDELETESHADERPROC) glutGetProcAddress("glDeleteShader");
   glGetActiveAttrib_func = (PFNGLGETACTIVEATTRIBPROC) glutGetProcAddress("glGetActiveAttrib");
   glGetActiveUniform_func = (PFNGLGETACTIVEUNIFORMPROC) glutGetProcAddress("glGetActiveUniform");
   glGetAttachedShaders_func = (PFNGLGETATTACHEDSHADERSPROC) glutGetProcAddress("glGetAttachedShaders");
   glGetAttribLocation_func = (PFNGLGETATTRIBLOCATIONPROC) glutGetProcAddress("glGetAttribLocation");
   glGetProgramInfoLog_func = (PFNGLGETPROGRAMINFOLOGPROC) glutGetProcAddress("glGetProgramInfoLog");
   glGetShaderInfoLog_func = (PFNGLGETSHADERINFOLOGPROC) glutGetProcAddress("glGetShaderInfoLog");
   glGetProgramiv_func = (PFNGLGETPROGRAMIVPROC) glutGetProcAddress("glGetProgramiv");
   glGetShaderiv_func = (PFNGLGETSHADERIVPROC) glutGetProcAddress("glGetShaderiv");
   glGetShaderSource_func = (PFNGLGETSHADERSOURCEPROC) glutGetProcAddress("glGetShaderSource");
   glGetUniformLocation_func = (PFNGLGETUNIFORMLOCATIONPROC) glutGetProcAddress("glGetUniformLocation");
   glGetUniformfv_func = (PFNGLGETUNIFORMFVPROC) glutGetProcAddress("glGetUniformfv");
   glIsProgram_func = (PFNGLISPROGRAMPROC) glutGetProcAddress("glIsProgram");
   glIsShader_func = (PFNGLISSHADERPROC) glutGetProcAddress("glIsShader");
   glLinkProgram_func = (PFNGLLINKPROGRAMPROC) glutGetProcAddress("glLinkProgram");
   glShaderSource_func = (PFNGLSHADERSOURCEPROC) glutGetProcAddress("glShaderSource");
   glUniform1i_func = (PFNGLUNIFORM1IPROC) glutGetProcAddress("glUniform1i");
   glUniform2i_func = (PFNGLUNIFORM2IPROC) glutGetProcAddress("glUniform2i");
   glUniform3i_func = (PFNGLUNIFORM3IPROC) glutGetProcAddress("glUniform3i");
   glUniform4i_func = (PFNGLUNIFORM4IPROC) glutGetProcAddress("glUniform3i");
   glUniform1f_func = (PFNGLUNIFORM1FPROC) glutGetProcAddress("glUniform1f");
   glUniform2f_func = (PFNGLUNIFORM2FPROC) glutGetProcAddress("glUniform2f");
   glUniform3f_func = (PFNGLUNIFORM3FPROC) glutGetProcAddress("glUniform3f");
   glUniform4f_func = (PFNGLUNIFORM4FPROC) glutGetProcAddress("glUniform4f");
   glUniform1fv_func = (PFNGLUNIFORM1FVPROC) glutGetProcAddress("glUniform1fv");
   glUniform2fv_func = (PFNGLUNIFORM2FVPROC) glutGetProcAddress("glUniform2fv");
   glUniform3fv_func = (PFNGLUNIFORM3FVPROC) glutGetProcAddress("glUniform3fv");
   glUniform4fv_func = (PFNGLUNIFORM3FVPROC) glutGetProcAddress("glUniform4fv");
   glUniformMatrix2fv_func = (PFNGLUNIFORMMATRIX2FVPROC) glutGetProcAddress("glUniformMatrix2fv");
   glUniformMatrix3fv_func = (PFNGLUNIFORMMATRIX3FVPROC) glutGetProcAddress("glUniformMatrix3fv");
   glUniformMatrix4fv_func = (PFNGLUNIFORMMATRIX4FVPROC) glutGetProcAddress("glUniformMatrix4fv");
   glUseProgram_func = (PFNGLUSEPROGRAMPROC) glutGetProcAddress("glUseProgram");
   glVertexAttrib1f_func = (PFNGLVERTEXATTRIB1FPROC) glutGetProcAddress("glVertexAttrib1f");
   glVertexAttrib2f_func = (PFNGLVERTEXATTRIB2FPROC) glutGetProcAddress("glVertexAttrib2f");
   glVertexAttrib3f_func = (PFNGLVERTEXATTRIB3FPROC) glutGetProcAddress("glVertexAttrib3f");
   glVertexAttrib4f_func = (PFNGLVERTEXATTRIB4FPROC) glutGetProcAddress("glVertexAttrib4f");
   glVertexAttrib1fv_func = (PFNGLVERTEXATTRIB1FVPROC) glutGetProcAddress("glVertexAttrib1fv");
   glVertexAttrib2fv_func = (PFNGLVERTEXATTRIB2FVPROC) glutGetProcAddress("glVertexAttrib2fv");
   glVertexAttrib3fv_func = (PFNGLVERTEXATTRIB3FVPROC) glutGetProcAddress("glVertexAttrib3fv");
   glVertexAttrib4fv_func = (PFNGLVERTEXATTRIB4FVPROC) glutGetProcAddress("glVertexAttrib4fv");

   glVertexAttribPointer_func = (PFNGLVERTEXATTRIBPOINTERPROC) glutGetProcAddress("glVertexAttribPointer");
   glEnableVertexAttribArray_func = (PFNGLENABLEVERTEXATTRIBARRAYPROC) glutGetProcAddress("glEnableVertexAttribArray");
   glDisableVertexAttribArray_func = (PFNGLDISABLEVERTEXATTRIBARRAYPROC) glutGetProcAddress("glDisableVertexAttribArray");

   /* OpenGL 2.1 */
   glUniformMatrix2x3fv_func = (PFNGLUNIFORMMATRIX2X3FVPROC) glutGetProcAddress("glUniformMatrix2x3fv");
   glUniformMatrix3x2fv_func = (PFNGLUNIFORMMATRIX3X2FVPROC) glutGetProcAddress("glUniformMatrix3x2fv");
   glUniformMatrix2x4fv_func = (PFNGLUNIFORMMATRIX2X4FVPROC) glutGetProcAddress("glUniformMatrix2x4fv");
   glUniformMatrix4x2fv_func = (PFNGLUNIFORMMATRIX4X2FVPROC) glutGetProcAddress("glUniformMatrix4x2fv");
   glUniformMatrix3x4fv_func = (PFNGLUNIFORMMATRIX3X4FVPROC) glutGetProcAddress("glUniformMatrix3x4fv");
   glUniformMatrix4x3fv_func = (PFNGLUNIFORMMATRIX4X3FVPROC) glutGetProcAddress("glUniformMatrix4x3fv");

   /* OpenGL 1.4 */
   glPointParameterfv_func = (PFNGLPOINTPARAMETERFVPROC) glutGetProcAddress("glPointParameterfv");
   glSecondaryColor3fv_func = (PFNGLSECONDARYCOLOR3FVPROC) glutGetProcAddress("glSecondaryColor3fv");

   /* GL_ARB_vertex/fragment_program */
   glBindProgramARB_func = (PFNGLBINDPROGRAMARBPROC) glutGetProcAddress("glBindProgramARB");
   glDeleteProgramsARB_func = (PFNGLDELETEPROGRAMSARBPROC) glutGetProcAddress("glDeleteProgramsARB");
   glGenProgramsARB_func = (PFNGLGENPROGRAMSARBPROC) glutGetProcAddress("glGenProgramsARB");
   glGetProgramLocalParameterdvARB_func = (PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) glutGetProcAddress("glGetProgramLocalParameterdvARB");
   glIsProgramARB_func = (PFNGLISPROGRAMARBPROC) glutGetProcAddress("glIsProgramARB");
   glProgramLocalParameter4dARB_func = (PFNGLPROGRAMLOCALPARAMETER4DARBPROC) glutGetProcAddress("glProgramLocalParameter4dARB");
   glProgramLocalParameter4fvARB_func = (PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) glutGetProcAddress("glProgramLocalParameter4fvARB");
   glProgramStringARB_func = (PFNGLPROGRAMSTRINGARBPROC) glutGetProcAddress("glProgramStringARB");
   glVertexAttrib1fARB_func = (PFNGLVERTEXATTRIB1FARBPROC) glutGetProcAddress("glVertexAttrib1fARB");

   /* GL_APPLE_vertex_array_object */
   glBindVertexArrayAPPLE_func = (PFNGLBINDVERTEXARRAYAPPLEPROC) glutGetProcAddress("glBindVertexArrayAPPLE");
   glDeleteVertexArraysAPPLE_func = (PFNGLDELETEVERTEXARRAYSAPPLEPROC) glutGetProcAddress("glDeleteVertexArraysAPPLE");
   glGenVertexArraysAPPLE_func = (PFNGLGENVERTEXARRAYSAPPLEPROC) glutGetProcAddress("glGenVertexArraysAPPLE");
   glIsVertexArrayAPPLE_func = (PFNGLISVERTEXARRAYAPPLEPROC) glutGetProcAddress("glIsVertexArrayAPPLE");

   /* GL_EXT_stencil_two_side */
   glActiveStencilFaceEXT_func = (PFNGLACTIVESTENCILFACEEXTPROC) glutGetProcAddress("glActiveStencilFaceEXT");

   /* GL_ARB_vertex_buffer_object */
   glGenBuffersARB_func = (PFNGLGENBUFFERSARBPROC) glutGetProcAddress("glGenBuffersARB");
   glDeleteBuffersARB_func = (PFNGLDELETEBUFFERSARBPROC) glutGetProcAddress("glDeleteBuffersARB");
   glBindBufferARB_func = (PFNGLBINDBUFFERARBPROC) glutGetProcAddress("glBindBufferARB");
   glBufferDataARB_func = (PFNGLBUFFERDATAARBPROC) glutGetProcAddress("glBufferDataARB");
   glBufferSubDataARB_func = (PFNGLBUFFERSUBDATAARBPROC) glutGetProcAddress("glBufferSubDataARB");
   glMapBufferARB_func = (PFNGLMAPBUFFERARBPROC) glutGetProcAddress("glMapBufferARB");
   glUnmapBufferARB_func = (PFNGLUNMAPBUFFERARBPROC) glutGetProcAddress("glUnmapBufferARB");

   /* GL_EXT_framebuffer_object */
   glIsRenderbufferEXT_func = (PFNGLISRENDERBUFFEREXTPROC) glutGetProcAddress("glIsRenderbufferEXT");
   glBindRenderbufferEXT_func = (PFNGLBINDRENDERBUFFEREXTPROC) glutGetProcAddress("glBindRenderbufferEXT");
   glDeleteRenderbuffersEXT_func = (PFNGLDELETERENDERBUFFERSEXTPROC) glutGetProcAddress("glDeleteRenderbuffersEXT");
   glGenRenderbuffersEXT_func = (PFNGLGENRENDERBUFFERSEXTPROC) glutGetProcAddress("glGenRenderbuffersEXT");
   glRenderbufferStorageEXT_func = (PFNGLRENDERBUFFERSTORAGEEXTPROC) glutGetProcAddress("glRenderbufferStorageEXT");
   glGetRenderbufferParameterivEXT_func = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) glutGetProcAddress("glGetRenderbufferParameterivEXT");
   glIsFramebufferEXT_func = (PFNGLISFRAMEBUFFEREXTPROC) glutGetProcAddress("glIsFramebufferEXT");
   glBindFramebufferEXT_func = (PFNGLBINDFRAMEBUFFEREXTPROC) glutGetProcAddress("glBindFramebufferEXT");
   glDeleteFramebuffersEXT_func = (PFNGLDELETEFRAMEBUFFERSEXTPROC) glutGetProcAddress("glDeleteFramebuffersEXT");
   glGenFramebuffersEXT_func = (PFNGLGENFRAMEBUFFERSEXTPROC) glutGetProcAddress("glGenFramebuffersEXT");
   glCheckFramebufferStatusEXT_func = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) glutGetProcAddress("glCheckFramebufferStatusEXT");
   glFramebufferTexture1DEXT_func = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) glutGetProcAddress("glFramebufferTexture1DEXT");
   glFramebufferTexture2DEXT_func = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) glutGetProcAddress("glFramebufferTexture2DEXT");
   glFramebufferTexture3DEXT_func = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) glutGetProcAddress("glFramebufferTexture3DEXT");
   glFramebufferRenderbufferEXT_func = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) glutGetProcAddress("glFramebufferRenderbufferEXT");
   glGetFramebufferAttachmentParameterivEXT_func = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) glutGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
   glGenerateMipmapEXT_func = (PFNGLGENERATEMIPMAPEXTPROC) glutGetProcAddress("glGenerateMipmapEXT");

   /* GL_ARB_framebuffer_object */
   glIsRenderbuffer_func = (PFNGLISRENDERBUFFERPROC) glutGetProcAddress("glIsRenderbuffer");
   glBindRenderbuffer_func = (PFNGLBINDRENDERBUFFERPROC) glutGetProcAddress("glBindRenderbuffer");
   glDeleteRenderbuffers_func = (PFNGLDELETERENDERBUFFERSPROC) glutGetProcAddress("glDeleteRenderbuffers");
   glGenRenderbuffers_func = (PFNGLGENRENDERBUFFERSPROC) glutGetProcAddress("glGenRenderbuffers");
   glRenderbufferStorage_func = (PFNGLRENDERBUFFERSTORAGEPROC) glutGetProcAddress("glRenderbufferStorage");
   glGetRenderbufferParameteriv_func = (PFNGLGETRENDERBUFFERPARAMETERIVPROC) glutGetProcAddress("glGetRenderbufferParameteriv");
   glIsFramebuffer_func = (PFNGLISFRAMEBUFFERPROC) glutGetProcAddress("glIsFramebuffer");
   glBindFramebuffer_func = (PFNGLBINDFRAMEBUFFERPROC) glutGetProcAddress("glBindFramebuffer");
   glDeleteFramebuffers_func = (PFNGLDELETEFRAMEBUFFERSPROC) glutGetProcAddress("glDeleteFramebuffers");
   glGenFramebuffers_func = (PFNGLGENFRAMEBUFFERSPROC) glutGetProcAddress("glGenFramebuffers");
   glCheckFramebufferStatus_func = (PFNGLCHECKFRAMEBUFFERSTATUSPROC) glutGetProcAddress("glCheckFramebufferStatus");
   glFramebufferTexture1D_func = (PFNGLFRAMEBUFFERTEXTURE1DPROC) glutGetProcAddress("glFramebufferTexture1D");
   glFramebufferTexture2D_func = (PFNGLFRAMEBUFFERTEXTURE2DPROC) glutGetProcAddress("glFramebufferTexture2D");
   glFramebufferTexture3D_func = (PFNGLFRAMEBUFFERTEXTURE3DPROC) glutGetProcAddress("glFramebufferTexture3D");
   glFramebufferRenderbuffer_func = (PFNGLFRAMEBUFFERRENDERBUFFERPROC) glutGetProcAddress("glFramebufferRenderbuffer");
   glGetFramebufferAttachmentParameteriv_func = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC) glutGetProcAddress("glGetFramebufferAttachmentParameteriv");
   glGenerateMipmap_func = (PFNGLGENERATEMIPMAPPROC) glutGetProcAddress("glGenerateMipmap");
   glBlitFramebuffer_func = (PFNGLBLITFRAMEBUFFERPROC) glutGetProcAddress("glBlitFramebuffer");
   glRenderbufferStorageMultisample_func = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC) glutGetProcAddress("glRenderbufferStorageMultisample");
   glFramebufferTextureLayer_func = (PFNGLFRAMEBUFFERTEXTURELAYERPROC) glutGetProcAddress("glFramebufferTextureLayer");
}

