# Makefile
# Created by IBM WorkFrame/2 MakeMake at 12:46:25 on 3 June 2003
#
# The actions included in this make file are:
#  Compile::C++ Compiler
#  Link::Linker
#  Lib::Import Lib

.SUFFIXES:

.SUFFIXES: \
    .LIB .cpp .dll .obj 

.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /I..\ /I..\X86 /I..\GL /I..\swrast /I..\swrast_setup /Ss /Wcmpcndcnscnvcpydclenuextgeninilanobsordparporppcprorearettrdtruund /Tx /O /Gm /Ge- /G5 /Gf /Gi /Oi /C %s

{G:\EVGEN\MESA5\src-glut.os2pm}.cpp.obj:
    @echo " Compile::C++ Compiler "
    icc.exe /I..\ /I..\X86 /I..\GL /I..\swrast /I..\swrast_setup /Ss /Wcmpcndcnscnvcpydclenuextgeninilanobsordparporppcprorearettrdtruund /Tx /O /Gm /Ge- /G5 /Gf /Gi /Oi /C %s

.dll.LIB:
    @echo " Lib::Import Lib "
    implib.exe %|dpfF.LIB %s

{G:\EVGEN\MESA5\src-glut.os2pm}.dll.LIB:
    @echo " Lib::Import Lib "
    implib.exe %|dpfF.LIB %s

all: \
    .\libGlut.LIB

.\libGlut.dll: \
    .\os2_winproc.obj \
    .\WarpWin.obj \
    .\glutOverlay.obj \
    .\glut_8x13.obj \
    .\glut_9x15.obj \
    .\glut_bitmap.obj \
    .\glut_cindex.obj \
    .\glut_cmap.obj \
    .\glut_cursor.obj \
    .\glut_event.obj \
    .\glut_ext.obj \
    .\glut_fullscrn.obj \
    .\glut_gamemode.obj \
    .\glut_get.obj \
    .\glut_hel10.obj \
    .\glut_hel12.obj \
    .\glut_hel18.obj \
    .\glut_init.obj \
    .\glut_input.obj \
    .\glut_key.obj \
    .\glut_keyctrl.obj \
    .\glut_keyup.obj \
    .\glut_mesa.obj \
    .\glut_modifier.obj \
    .\glut_roman.obj \
    .\glut_shapes.obj \
    .\glut_stroke.obj \
    .\glut_swap.obj \
    .\glut_teapot.obj \
    .\glut_tr24.obj \
    .\glut_util.obj \
    .\glut_vidresize.obj \
    .\glut_warp.obj \
    .\glut_win.obj \
    .\glut_winmisc.obj \
    .\os2_glx.obj \
    .\os2_menu.obj \
    ..\si-glu\libGLU.lib \
    ..\MesaDll\MesaGL2.lib \
    ..\drv\DrvLoad\MesaDrvLoad.lib \
    {$(LIB)}libGlut.DEF
    @echo " Link::Linker "
    icc.exe @<<
     /B" /dbgpack /exepack:2 /st:200000 /packd /optfunc"
     /FelibGlut.dll 
     ..\si-glu\libGLU.lib 
     ..\MesaDll\MesaGL2.lib 
     ..\drv\DrvLoad\MesaDrvLoad.lib 
     libGlut.DEF
     .\os2_winproc.obj
     .\WarpWin.obj
     .\glutOverlay.obj
     .\glut_8x13.obj
     .\glut_9x15.obj
     .\glut_bitmap.obj
     .\glut_cindex.obj
     .\glut_cmap.obj
     .\glut_cursor.obj
     .\glut_event.obj
     .\glut_ext.obj
     .\glut_fullscrn.obj
     .\glut_gamemode.obj
     .\glut_get.obj
     .\glut_hel10.obj
     .\glut_hel12.obj
     .\glut_hel18.obj
     .\glut_init.obj
     .\glut_input.obj
     .\glut_key.obj
     .\glut_keyctrl.obj
     .\glut_keyup.obj
     .\glut_mesa.obj
     .\glut_modifier.obj
     .\glut_roman.obj
     .\glut_shapes.obj
     .\glut_stroke.obj
     .\glut_swap.obj
     .\glut_teapot.obj
     .\glut_tr24.obj
     .\glut_util.obj
     .\glut_vidresize.obj
     .\glut_warp.obj
     .\glut_win.obj
     .\glut_winmisc.obj
     .\os2_glx.obj
     .\os2_menu.obj
<<

.\os2_winproc.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\os2_winproc.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h \
    gl\os2mesa.h \
    gl\gl.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}gl_mangle.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}GL/os2_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}os2_config.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}os2mesadef.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}context.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glapi.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}mtypes.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glheader.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}config.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glapitable.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glthread.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}math/m_matrix.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}Trace/tr_context.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}dd.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}conf.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}GL/os2_config.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}GL/glext.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}Xthreads.h

.\os2_menu.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\os2_menu.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\os2_glx.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\os2_glx.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    gl\os2mesa.h \
    gl\gl.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}gl_mangle.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}GL/os2_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}os2_config.h

.\glut_winmisc.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_winmisc.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_win.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_win.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutos2.h \
    gl\os2mesa.h \
    gl\gl.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}gl_mangle.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}GL/os2_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}os2_config.h

.\glut_warp.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_warp.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_vidresize.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_vidresize.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_util.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_util.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_tr24.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_tr24.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h

.\glut_teapot.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_teapot.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_swap.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_swap.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_stroke.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_stroke.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutstroke.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_shapes.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_shapes.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_roman.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_roman.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutstroke.h

.\glut_modifier.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_modifier.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_mesa.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_mesa.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_keyup.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_keyup.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_keyctrl.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_keyctrl.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_key.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_key.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_input.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_input.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_init.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_init.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_hel18.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_hel18.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h

.\glut_hel12.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_hel12.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h

.\glut_hel10.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_hel10.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h

.\glut_get.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_get.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_gamemode.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_gamemode.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_fullscrn.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_fullscrn.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_ext.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_ext.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_event.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_event.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_cursor.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_cursor.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_cmap.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_cmap.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}layerutil.h

.\glut_cindex.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_cindex.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_bitmap.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_bitmap.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\glut_9x15.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_9x15.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h

.\glut_8x13.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glut_8x13.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h

.\glutOverlay.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\glutOverlay.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutint.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutbitmap.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutstroke.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}glutwin32.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_x11.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}win32_glx.h

.\WarpWin.obj: \
    G:\EVGEN\MESA5\src-glut.os2pm\WarpWin.cpp \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpWin.h \
    {G:\EVGEN\MESA5\src-glut.os2pm;..\;..\X86;..\GL;..\swrast;..\swrast_setup;$(INCLUDE);}WarpGL.h

.\libGlut.LIB: \
    .\libGlut.dll
