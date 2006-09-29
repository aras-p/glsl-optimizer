# Microsoft Developer Studio Project File - Name="mesa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=mesa - Win32 Debug x86
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mesa.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mesa.mak" CFG="mesa - Win32 Debug x86"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mesa - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "mesa - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "mesa - Win32 Release x86" (based on "Win32 (x86) Static Library")
!MESSAGE "mesa - Win32 Debug x86" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mesa - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/main" /I "../../../../src/mesa/shader" /I "../../../../src/mesa/shader/slang" /I "../../../../src/mesa/shader/grammar" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /YX /FD /Zm1000 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/main" /I "../../../../src/mesa/shader" /I "../../../../src/mesa/shader/slang" /I "../../../../src/mesa/shader/grammar" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /Fr /FD /GZ /Zm1000 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mesa - Win32 Release x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "mesa___Win32_Release_x86"
# PROP BASE Intermediate_Dir "mesa___Win32_Release_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_x86"
# PROP Intermediate_Dir "Release_x86"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/main" /I "../../../../src/mesa/shader" /I "../../../../src/mesa/shader/slang" /I "../../../../src/mesa/shader/grammar" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /YX /FD /Zm1000 /c
# ADD CPP /nologo /W3 /GX /O2 /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/main" /I "../../../../src/mesa/shader" /I "../../../../src/mesa/shader/slang" /I "../../../../src/mesa/shader/grammar" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /D "SLANG_X86" /YX /FD /Zm1000 /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug x86"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "mesa___Win32_Debug_x86"
# PROP BASE Intermediate_Dir "mesa___Win32_Debug_x86"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_x86"
# PROP Intermediate_Dir "Debug_x86"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/main" /I "../../../../src/mesa/shader" /I "../../../../src/mesa/shader/slang" /I "../../../../src/mesa/shader/grammar" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /Fr /FD /GZ /Zm1000 /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "../../../../include" /I "../../../../src/mesa" /I "../../../../src/mesa/glapi" /I "../../../../src/mesa/main" /I "../../../../src/mesa/shader" /I "../../../../src/mesa/shader/slang" /I "../../../../src/mesa/shader/grammar" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /D "_DLL" /D "BUILD_GL32" /D "MESA_MINWARN" /D "SLANG_X86" /Fr /FD /GZ /Zm1000 /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "mesa - Win32 Release"
# Name "mesa - Win32 Debug"
# Name "mesa - Win32 Release x86"
# Name "mesa - Win32 Debug x86"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\mesa\array_cache\ac_context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\array_cache\ac_import.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\accum.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_arrayelt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_loopback.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_noop.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_validate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\arrayobj.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\arbprogparse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\arbprogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\atifragshader.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\attrib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\blend.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\bufferobj.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\buffers.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\clip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\colortab.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\convolve.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\debug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\depth.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\depthstencil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\dispatch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\dlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\drawpix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\enable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\enums.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\eval.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\execmem.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\extensions.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\fbobject.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\feedback.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\fog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\framebuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\get.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\getstring.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glthread.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\grammar\grammar.c

!IF  "$(CFG)" == "mesa - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Release x86"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug x86"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\grammar\grammar_mesa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\hash.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\hint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\histogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\image.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\imports.c

!IF  "$(CFG)" == "mesa - Win32 Release"

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug"

!ELSEIF  "$(CFG)" == "mesa - Win32 Release x86"

# ADD CPP /YX

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug x86"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\light.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\lines.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_debug_clip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_debug_norm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_debug_xform.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_eval.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_translate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_vector.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_xform.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\mipmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\mm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvfragparse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvprogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvvertexec.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvvertparse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\occlude.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\pixel.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\points.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\polygon.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\program.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\rastpos.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\rbadaptors.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\renderbuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_aaline.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_aatriangle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_accum.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_alpha.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_arbshader.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_atifragshader.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_bitmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_blend.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_blit.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_buffers.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_copypix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_depth.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_drawpix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_feedback.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_fog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_imaging.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_lines.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_logic.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_masking.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_nvfragprog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_points.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_readpix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_span.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_stencil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_texcombine.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_texfilter.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_texstore.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_triangle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_zoom.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\shaderobjects.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\shaderobjects_3dlabs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_analyse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_assignment.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_conditional.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_constructor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_typeinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_function.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_operation.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_struct.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_variable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_execute.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_execute_x86.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_export.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_library_noise.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_library_texsample.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_link.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_preprocess.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_storage.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_utility.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast_setup\ss_context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast_setup\ss_triangle.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\state.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\stencil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_array_api.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_array_import.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_pipeline.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_save_api.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_save_loopback.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_save_playback.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_arbprogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_arbshader.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_cull.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_fog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_light.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_normals.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_points.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_program.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_render.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_texgen.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_texmat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_vertex.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vertex.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vertex_generic.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vp_build.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vtx_api.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vtx_eval.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vtx_exec.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vtx_generic.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texcompress.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texcompress_fxt1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texcompress_s3tc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texenvprogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texformat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\teximage.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texobj.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texrender.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texstate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texstore.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\varray.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\vsnprintf.c

!IF  "$(CFG)" == "mesa - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Release x86"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug x86"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\vtxfmt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\x86\rtasm\x86sse.c

!IF  "$(CFG)" == "mesa - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "mesa - Win32 Release x86"

!ELSEIF  "$(CFG)" == "mesa - Win32 Debug x86"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\src\mesa\array_cache\ac_context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\array_cache\acache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\accum.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_arrayelt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_eval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_loopback.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_noop.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\api_validate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\arrayobj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\arbprogparse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\arbprogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\arbprogram_syn.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\atifragshader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\attrib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\bitset.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\blend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\bufferobj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\buffers.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\clip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\colormac.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\colortab.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\convolve.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\dd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\depth.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\depthstencil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\dlist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\drawpix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\enable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\enums.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\eval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\extensions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\fbobject.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\feedback.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\fog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\framebuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\get.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glapioffsets.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glapitable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glapitemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\glheader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glprocs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\glapi\glthread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\grammar\grammar.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\grammar\grammar_mesa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\grammar\grammar_syn.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\hash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\hint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\histogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\image.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\imports.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\light.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\lines.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_clip_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_copy_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_debug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_debug_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_dotprod_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_eval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_norm_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_trans_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_translate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_xform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\m_xform_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\macros.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\math\mathmod.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\mm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\mtypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvfragparse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvfragprog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvprogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvvertexec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvvertparse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\nvvertprog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\occlude.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\pixel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\points.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\polygon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\program.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\rastpos.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\rbadaptors.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\renderbuffer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_aaline.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_aalinetemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_aatriangle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_aatritemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_accum.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_alpha.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_arbshader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_atifragshader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_blend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_depth.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_drawpix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_feedback.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_fog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_lines.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_linetemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_logic.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_masking.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_nvfragprog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_points.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_pointtemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_span.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_spantemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_stencil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_texcombine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_texfilter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_triangle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_trispan.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_tritemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\s_zoom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\shaderobjects.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\shaderobjects_3dlabs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\simple_list.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_analyse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_assignment.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_conditional.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_constructor.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_assemble_typeinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_function.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_operation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_struct.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_compile_variable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_execute.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_export.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_library_noise.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_library_texsample.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_link.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_mesa.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_preprocess.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_storage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\slang_utility.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast_setup\ss_context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast_setup\ss_triangle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast_setup\ss_tritmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast_setup\ss_vb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\state.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\stencil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast\swrast.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\swrast_setup\swrast_setup.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_array_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_array_import.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_pipeline.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_save_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_arbprogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_cliptmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_lighttmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vb_rendertmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vertex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vp_build.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\t_vtx_api.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texcompress.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texenvprogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texformat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texformat_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\teximage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texobj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texrender.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texstate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\texstore.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\tnl\tnl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\shader\slang\traverse_wrap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\varray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\version.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\vtxfmt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\main\vtxfmt_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\mesa\x86\rtasm\x86sse.h
# End Source File
# End Group
# End Target
# End Project
