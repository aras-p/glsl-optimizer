# Microsoft Developer Studio Project File - Name="glu" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=glu - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "glu.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "glu.mak" CFG="glu - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "glu - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "glu - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "glu - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLU_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../../include" /I "./include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLU_EXPORTS" /D "BUILD_GL32" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 msvcrt.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../lib/OPENGL32.LIB /nologo /dll /machine:I386 /nodefaultlib /out:"Release/GLU32.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy import lib and dll
PostBuild_Cmds=if not exist ..\..\..\lib md ..\..\..\lib	if not exist ..\..\..\libexec md ..\..\..\libexec	copy Release\GLU32.LIB ..\..\..\lib	copy Release\GLU32.DLL ..\..\..\libexec
# End Special Build Tool

!ELSEIF  "$(CFG)" == "glu - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLU_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../include" /I "./include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLU_EXPORTS" /D "BUILD_GL32" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../lib/OPENGL32.LIB /nologo /dll /debug /machine:I386 /nodefaultlib /out:"Debug/GLU32.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy import lib and dll
PostBuild_Cmds=if not exist ..\..\..\lib md ..\..\..\lib	if not exist ..\..\..\libexec md ..\..\..\libexec	copy Debug\GLU32.LIB ..\..\..\lib	copy Debug\GLU32.DLL ..\..\..\libexec
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "glu - Win32 Release"
# Name "glu - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libtess\dict.c
# End Source File
# Begin Source File

SOURCE=.\libutil\error.c
# End Source File
# Begin Source File

SOURCE=.\libtess\geom.c
# End Source File
# Begin Source File

SOURCE=.\libutil\glue.c
# End Source File
# Begin Source File

SOURCE=.\libtess\memalloc.c
# End Source File
# Begin Source File

SOURCE=.\libtess\mesh.c
# End Source File
# Begin Source File

SOURCE=.\libutil\mipmap.c
# End Source File
# Begin Source File

SOURCE=.\libtess\normal.c
# End Source File
# Begin Source File

SOURCE=.\libtess\priorityq.c
# End Source File
# Begin Source File

SOURCE=.\libutil\project.c
# End Source File
# Begin Source File

SOURCE=.\libutil\quad.c
# End Source File
# Begin Source File

SOURCE=.\libutil\registry.c
# End Source File
# Begin Source File

SOURCE=.\libtess\render.c
# End Source File
# Begin Source File

SOURCE=.\libtess\sweep.c
# End Source File
# Begin Source File

SOURCE=.\libtess\tess.c
# End Source File
# Begin Source File

SOURCE=.\libtess\tessmono.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=".\libtess\dict-list.h"
# End Source File
# Begin Source File

SOURCE=.\libtess\dict.h
# End Source File
# Begin Source File

SOURCE=.\libtess\geom.h
# End Source File
# Begin Source File

SOURCE=.\libutil\gluint.h
# End Source File
# Begin Source File

SOURCE=.\include\gluos.h
# End Source File
# Begin Source File

SOURCE=.\libtess\memalloc.h
# End Source File
# Begin Source File

SOURCE=.\libtess\mesh.h
# End Source File
# Begin Source File

SOURCE=.\libtess\normal.h
# End Source File
# Begin Source File

SOURCE=".\libtess\priorityq-sort.h"
# End Source File
# Begin Source File

SOURCE=.\libtess\priorityq.h
# End Source File
# Begin Source File

SOURCE=.\libtess\render.h
# End Source File
# Begin Source File

SOURCE=.\libtess\sweep.h
# End Source File
# Begin Source File

SOURCE=.\libtess\tess.h
# End Source File
# Begin Source File

SOURCE=.\libtess\tessmono.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
