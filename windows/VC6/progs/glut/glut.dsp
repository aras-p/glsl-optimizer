# Microsoft Developer Studio Project File - Name="glut" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=glut - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "glut.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "glut.mak" CFG="glut - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "glut - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "glut - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "glut - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLUT_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../../../include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_DLL" /D "_USRDLL" /D "GLUT_EXPORTS" /D "MESA" /D "BUILD_GL32" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 opengl32.lib glu32.lib winmm.lib msvcrt.lib oldnames.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /nodefaultlib /out:"Release/GLUT32.DLL" /libpath:"../../mesa/Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Release\GLUT32.LIB ..\..\..\..\lib	copy Release\GLUT32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Release\GLUT32.DLL ..\..\..\..\progs\demos
# End Special Build Tool

!ELSEIF  "$(CFG)" == "glut - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLUT_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../../../include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_DLL" /D "_USRDLL" /D "GLUT_EXPORTS" /D "MESA" /D "BUILD_GL32" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 winmm.lib msvcrtd.lib oldnames.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib glu32.lib /nologo /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"Debug/GLUT32.DLL" /pdbtype:sept /libpath:"../../mesa/Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Debug\GLUT32.LIB ..\..\..\..\lib	copy Debug\GLUT32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Debug\GLUT32.DLL ..\..\..\..\progs\demos
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "glut - Win32 Release"
# Name "glut - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_8x13.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_9x15.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_bitmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_bwidth.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_cindex.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_cmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_cursor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_dials.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_dstr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_event.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_ext.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_fcb.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_fullscrn.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_gamemode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_get.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_hel10.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_hel12.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_hel18.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_init.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_input.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_joy.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_key.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_keyctrl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_keyup.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_mesa.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_modifier.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_mroman.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_overlay.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_roman.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_shapes.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_space.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_stroke.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_swap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_swidth.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_tablet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_teapot.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_tr10.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_tr24.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_vidresize.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_warp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_win.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glut_winmisc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\win32_glx.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\win32_menu.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\win32_util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\win32_winproc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\win32_x11.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glutbitmap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glutint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glutstroke.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\glutwin32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\stroke.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\win32_glx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glut\glx\win32_x11.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
