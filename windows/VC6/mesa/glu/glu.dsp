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
# ADD CPP /nologo /MT /W3 /GX /O2 /I "../../../../include" /I "../../../../src/glu/sgi/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLU_EXPORTS" /D "BUILD_GL32" /FD /c
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
# ADD LINK32 msvcrt.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib Release/glucc.lib /nologo /dll /machine:I386 /nodefaultlib /out:"Release/GLU32.DLL" /libpath:"../gdi/Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=C++ Compilations
PreLink_Cmds=cl @compileRelease.txt	LIB /OUT:Release/GLUCC.LIB @objectsRelease.txt
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Release\GLU32.LIB ..\..\..\..\lib	copy Release\GLU32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Release\GLU32.DLL ..\..\..\..\progs\demos
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
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "../../../../include" /I "../../../../src/glu/sgi/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLU_EXPORTS" /D "BUILD_GL32" /FD /GZ /c
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
# ADD LINK32 msvcrtd.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib opengl32.lib Debug/glucc.lib /nologo /dll /incremental:no /debug /machine:I386 /nodefaultlib /out:"Debug/GLU32.DLL" /pdbtype:sept /libpath:"../gdi/Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=C++ Compilations
PreLink_Cmds=cl @compileDebug.txt	LIB /OUT:Debug/GLUCC.LIB @objectsDebug.txt
PostBuild_Cmds=if not exist ..\..\..\..\lib md ..\..\..\..\lib	copy Debug\GLU32.LIB ..\..\..\..\lib	copy Debug\GLU32.DLL ..\..\..\..\lib	if exist ..\..\..\..\progs\demos copy Debug\GLU32.DLL ..\..\..\..\progs\demos
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "glu - Win32 Release"
# Name "glu - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\dict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libutil\error.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\geom.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\glu.def
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libutil\glue.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\memalloc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\mesh.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libutil\mipmap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\normal.c
# End Source File
# Begin Source File

SOURCE="..\..\..\..\src\glu\sgi\libtess\priorityq-heap.c"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\priorityq.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libutil\project.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libutil\quad.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libutil\registry.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\render.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\sweep.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\tess.c
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\tessmono.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\arc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\arcsorter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\arctess.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\backend.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\basiccrveval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\basicsurfeval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\bezierarc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\bezierEval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\bezierPatch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\bezierPatchMesh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\bin.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\bufpool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\cachingeval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\coveandtiler.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\curve.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\curvelist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\dataTransform.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\defines.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\definitions.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\src\glu\sgi\libtess\dict-list.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\dict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\directedLine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\displaylist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\displaymode.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\flist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\flistsorter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\geom.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glcurveval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glimports.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\glimports.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glrenderer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glsurfeval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libutil\gluint.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\include\gluos.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\gridline.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\gridtrimvertex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\gridvertex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\gridWrap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\hull.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\jarcloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\knotvector.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mapdesc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\maplist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\memalloc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\mesh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mesher.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\monoChain.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\monoPolyPart.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\monotonizer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\monoTriangulation.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\myassert.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mymath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mysetjmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\mystdio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\mystdio.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\mystdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\mystdlib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mystring.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\normal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\nurbsconsts.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\nurbstess.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\partitionX.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\partitionY.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\patch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\patchlist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\polyDBG.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\polyUtil.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\primitiveStream.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\src\glu\sgi\libtess\priorityq-heap.h"
# End Source File
# Begin Source File

SOURCE="..\..\..\..\src\glu\sgi\libtess\priorityq-sort.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\priorityq.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\pwlarc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\quicksort.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\quilt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\reader.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\rectBlock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\render.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\renderhints.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleComp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleCompBot.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleCompRight.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleCompTop.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampledLine.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleMonoPoly.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\searchTree.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\simplemath.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\slicer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\sorter.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\subdivider.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\sweep.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\tess.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\tessmono.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\trimline.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\trimregion.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\trimvertex.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\trimvertpool.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\types.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\uarray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\varray.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\zlassert.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "C++ files"

# PROP Default_Filter ".cc"
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\arc.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\arcsorter.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\arctess.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\backend.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\basiccrveval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\basicsurfeval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\bezierEval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\bezierPatch.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\bezierPatchMesh.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\bin.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\bufpool.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\cachingeval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\ccw.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\coveandtiler.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\curve.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\curvelist.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\curvesub.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\dataTransform.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\directedLine.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\displaylist.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\flist.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\flistsorter.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glcurveval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glinterface.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glrenderer.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\glsurfeval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\gridWrap.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\hull.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\incurveeval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\interface\insurfeval.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\intersect.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\knotvector.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mapdesc.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mapdescv.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\maplist.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mesher.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\monoChain.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\monoPolyPart.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\monotonizer.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\monoTriangulation.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\monoTriangulationBackend.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\mycode.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\nurbsinterfac.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\nurbstess.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\partitionX.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\partitionY.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\patch.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\patchlist.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\polyDBG.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\polyUtil.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\primitiveStream.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\quicksort.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\quilt.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\reader.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libtess\README
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\rectBlock.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\renderhints.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleComp.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleCompBot.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleCompRight.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleCompTop.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampledLine.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\sampleMonoPoly.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\nurbtess\searchTree.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\slicer.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\sorter.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\splitarcs.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\subdivider.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\tobezier.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\trimline.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\trimregion.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\trimvertpool.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\uarray.cc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\..\..\..\src\glu\sgi\libnurbs\internals\varray.cc
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE="..\..\..\..\src\glu\sgi\libtess\alg-outline"
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\compileDebug.txt
# End Source File
# Begin Source File

SOURCE=.\compileRelease.txt
# End Source File
# Begin Source File

SOURCE=.\objectsDebug.txt
# End Source File
# Begin Source File

SOURCE=.\objectsRelease.txt
# End Source File
# End Target
# End Project
