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
# ADD LINK32 msvcrt.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../lib/OPENGL32.LIB Release/GLUCC.LIB /nologo /dll /machine:I386 /nodefaultlib /out:"Release/GLU32.DLL"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Cmds=cl @ccRelease.txt	LIB /OUT:Release/GLUCC.LIB @ccReleaseObj.txt
PostBuild_Desc=Copy import lib and dll
PostBuild_Cmds=if not exist ..\..\..\lib md ..\..\..\lib	copy Release\GLU32.LIB ..\..\..\lib	copy Release\GLU32.DLL ..\..\..\lib
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
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../include" /I "./include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "GLU_EXPORTS" /D "BUILD_GL32" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcrtd.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ../../../lib/OPENGL32.LIB Debug/GLUCC.LIB /nologo /dll /debug /machine:I386 /nodefaultlib /out:"Debug/GLU32.DLL" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PreLink_Desc=C++ compilations
PreLink_Cmds=cl @ccDebug.txt	LIB /OUT:Debug/GLUCC.LIB @ccDebugObj.txt
PostBuild_Desc=Copy import lib and dll
PostBuild_Cmds=if not exist ..\..\..\lib md ..\..\..\lib	copy Debug\GLU32.LIB ..\..\..\lib	copy Debug\GLU32.DLL ..\..\..\lib
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "glu - Win32 Release"
# Name "glu - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\libnurbs\internals\arc.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\arcsorter.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\arctess.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\backend.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\basiccrveval.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\basicsurfeval.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\bezierEval.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\bezierPatch.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\bezierPatchMesh.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\bin.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\bufpool.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\cachingeval.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\ccw.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\coveandtiler.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\curve.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\curvelist.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\curvesub.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\dataTransform.cc
# End Source File
# Begin Source File

SOURCE=.\libtess\dict.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\directedLine.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\displaylist.cc
# End Source File
# Begin Source File

SOURCE=.\libutil\error.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\flist.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\flistsorter.cc
# End Source File
# Begin Source File

SOURCE=.\libtess\geom.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glcurveval.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glinterface.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glrenderer.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glsurfeval.cc
# End Source File
# Begin Source File

SOURCE=.\glu.def
# End Source File
# Begin Source File

SOURCE=.\libutil\glue.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\gridWrap.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\hull.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\incurveeval.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\insurfeval.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\intersect.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\knotvector.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mapdesc.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mapdescv.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\maplist.cc
# End Source File
# Begin Source File

SOURCE=.\libtess\memalloc.c
# End Source File
# Begin Source File

SOURCE=.\libtess\mesh.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mesher.cc
# End Source File
# Begin Source File

SOURCE=.\libutil\mipmap.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\monoChain.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\monoPolyPart.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\monotonizer.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\monoTriangulation.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\monoTriangulationBackend.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mycode.cc
# End Source File
# Begin Source File

SOURCE=.\libtess\normal.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\nurbsinterfac.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\nurbstess.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\partitionX.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\partitionY.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\patch.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\patchlist.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\polyDBG.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\polyUtil.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\primitiveStream.cc
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

SOURCE=.\libnurbs\nurbtess\quicksort.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\quilt.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\reader.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\rectBlock.cc
# End Source File
# Begin Source File

SOURCE=.\libutil\registry.c
# End Source File
# Begin Source File

SOURCE=.\libtess\render.c
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\renderhints.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleComp.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleCompBot.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleCompRight.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleCompTop.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampledLine.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleMonoPoly.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\searchTree.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\slicer.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\sorter.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\splitarcs.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\subdivider.cc
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
# Begin Source File

SOURCE=.\libnurbs\internals\tobezier.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\trimline.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\trimregion.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\trimvertpool.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\uarray.cc
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\varray.cc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\libnurbs\internals\arc.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\arcsorter.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\arctess.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\backend.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\basiccrveval.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\basicsurfeval.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\bezierarc.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\bezierEval.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\bezierPatch.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\bezierPatchMesh.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\bin.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\bufpool.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\cachingeval.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\coveandtiler.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\curve.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\curvelist.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\dataTransform.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\defines.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\definitions.h
# End Source File
# Begin Source File

SOURCE=".\libtess\dict-list.h"
# End Source File
# Begin Source File

SOURCE=.\libtess\dict.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\directedLine.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\displaylist.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\displaymode.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\flist.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\flistsorter.h
# End Source File
# Begin Source File

SOURCE=.\libtess\geom.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glcurveval.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glimports.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\glimports.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glrenderer.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\glsurfeval.h
# End Source File
# Begin Source File

SOURCE=.\libutil\gluint.h
# End Source File
# Begin Source File

SOURCE=.\include\gluos.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\gridline.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\gridtrimvertex.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\gridvertex.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\gridWrap.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\hull.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\jarcloc.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\knotvector.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mapdesc.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\maplist.h
# End Source File
# Begin Source File

SOURCE=.\libtess\memalloc.h
# End Source File
# Begin Source File

SOURCE=.\libtess\mesh.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mesher.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\monoChain.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\monoPolyPart.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\monotonizer.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\monoTriangulation.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\myassert.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mymath.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mysetjmp.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\mystdio.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\mystdio.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\interface\mystdlib.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\mystdlib.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\mystring.h
# End Source File
# Begin Source File

SOURCE=.\libtess\normal.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\nurbsconsts.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\nurbstess.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\partitionX.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\partitionY.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\patch.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\patchlist.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\polyDBG.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\polyUtil.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\primitiveStream.h
# End Source File
# Begin Source File

SOURCE=".\libtess\priorityq-sort.h"
# End Source File
# Begin Source File

SOURCE=.\libtess\priorityq.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\pwlarc.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\quicksort.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\quilt.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\reader.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\rectBlock.h
# End Source File
# Begin Source File

SOURCE=.\libtess\render.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\renderhints.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleComp.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleCompBot.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleCompRight.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleCompTop.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampledLine.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\sampleMonoPoly.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\searchTree.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\simplemath.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\slicer.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\sorter.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\subdivider.h
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
# Begin Source File

SOURCE=.\libnurbs\internals\trimline.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\trimregion.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\trimvertex.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\trimvertpool.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\types.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\uarray.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\internals\varray.h
# End Source File
# Begin Source File

SOURCE=.\libnurbs\nurbtess\zlassert.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\ccDebug.txt
# End Source File
# Begin Source File

SOURCE=.\ccDebugObj.txt
# End Source File
# Begin Source File

SOURCE=.\ccRelease.txt
# End Source File
# Begin Source File

SOURCE=.\ccReleaseObj.txt
# End Source File
# End Target
# End Project
