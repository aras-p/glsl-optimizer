# Microsoft Developer Studio Project File - Name="core" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=core - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "core.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "core.mak" CFG="core - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "core - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "core - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "core - Win32 Release"

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
# ADD CPP /nologo /Gd /W3 /GX /O2 /I "../../../../../include" /I "../../../../../src/mesa" /I "../../../../../src/mesa/main" /I "../../../../../src/mesa/glapi" /I "../../../../../src/mesa/math" /I "../../../../../src/mesa/transform" /I "../../../../../src/mesa/swrast" /I "../../../../../src/mesa/swrast_setup" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "BUILD_GL32" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "core - Win32 Debug"

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
# ADD CPP /nologo /Gd /W3 /Gm /GX /ZI /Od /I "../../../../../src/mesa/main" /I "../../../../../src/mesa/glapi" /I "../../../../../src/mesa/math" /I "../../../../../src/mesa/transform" /I "../../../../../src/mesa/swrast" /I "../../../../../src/mesa/swrast_setup" /I "../../../../../include" /I "../../../../../src/mesa" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "BUILD_GL32" /YX /FD /GZ /c
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

# Name "core - Win32 Release"
# Name "core - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\array_cache\ac_context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\array_cache\ac_import.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\accum.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_arrayelt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_loopback.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_noop.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_validate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\arbprogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\attrib.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\blend.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\bufferobj.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\buffers.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\clip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\colortab.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\context.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\convolve.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\debug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\depth.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\dispatch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\dlist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\drawpix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\enable.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\enums.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\eval.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\extensions.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\feedback.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\fog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\get.c
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glthread.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\hash.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\hint.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\histogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\image.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\imports.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\light.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\lines.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_debug_clip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_debug_norm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_debug_xform.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_eval.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_translate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_vector.c
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_xform.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\matrix.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvfragparse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvprogram.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvvertexec.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvvertparse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\occlude.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\pixel.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\points.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\polygon.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\rastpos.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\state.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\stencil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texcompress.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texformat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\teximage.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texobj.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texstate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texstore.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texutil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\varray.c
# End Source File
# Begin Source File

SOURCE=..\..\..\main\vtxfmt.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\array_cache\ac_context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\array_cache\acache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\accum.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_arrayelt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_eval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_loopback.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_noop.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\api_validate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\arbprogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\arbvertparse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\attrib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\blend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\bufferobj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\buffers.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\clip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\colormac.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\colortab.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\config.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\context.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\convolve.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\dd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\debug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\depth.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\dlist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\drawpix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\enable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\enums.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\eval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\extensions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\feedback.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\fog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\get.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glapioffsets.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glapitable.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glapitemp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\glheader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glprocs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\glapi\glthread.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\hash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\hint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\histogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\image.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\imports.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\light.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\lines.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_clip_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_copy_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_debug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_debug_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_dotprod_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_eval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_norm_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_trans_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_translate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_vector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_xform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\m_xform_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\macros.h
# End Source File
# Begin Source File

SOURCE=..\..\..\math\mathmod.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\matrix.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\mtypes.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvfragparse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvfragprog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvprogram.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvvertexec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvvertparse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\nvvertprog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\occlude.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\pixel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\points.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\polygon.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\rastpos.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\simple_list.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\state.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\stencil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texcompress.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texformat.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texformat_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\teximage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texobj.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texstate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texstore.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texutil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\texutil_tmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\varray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\version.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\vtxfmt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\main\vtxfmt_tmp.h
# End Source File
# End Group
# End Target
# End Project
