# Makefile for GLUT-based demo programs for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last update : 30 November 2007

.first
	define gl [--.include.gl]

.include [--]mms-config.

##### MACROS #####

INCDIR = ([--.include],[-.util])
CFLAGS =/include=$(INCDIR)/prefix=all/name=(as_is,short)/float=ieee/ieee=denorm

.ifdef SHARE
GL_LIBS = $(XLIBS)
LIB_DEP = [--.lib]$(GL_SHAR) [--.lib]$(GLU_SHAR) [--.lib]$(GLUT_SHAR)
.else
GL_LIBS = [--.lib]libGLUT/l,libMesaGLU/l,libMesaGL/l,$(XLIBS)
LIB_DEP = [--.lib]$(GL_LIB) [--.lib]$(GLU_LIB) [--.lib]$(GLUT_LIB)
.endif


PROGS = bounce.exe,clearspd.exe,drawpix.exe,gamma.exe,gears.exe,\
	glinfo.exe,glutfx.exe,isosurf.exe,morph3d.exe,\
	paltex.exe,pointblast.exe,reflect.exe,spectex.exe,stex3d.exe,\
	tessdemo.exe,texcyl.exe,texobj.exe,trispd.exe,winpos.exe


##### RULES #####
.obj.exe :
	cxxlink $(MMS$TARGET_NAME),$(GL_LIBS)

##### TARGETS #####
default :
	$(MMS)$(MMSQUALIFIERS) $(PROGS)

clean :
	delete *.obj;*

realclean :
	delete *.exe;*
	delete *.obj;*

bounce.exe : bounce.obj $(LIB_DEP)
clearspd.exe : clearspd.obj $(LIB_DEP)
drawpix.exe : drawpix.obj $(LIB_DEP) [-.util]readtex.obj
	cxxlink $(MMS$TARGET_NAME),[-.util]readtex.obj,$(GL_LIBS)
gamma.exe : gamma.obj $(LIB_DEP)
gears.exe : gears.obj $(LIB_DEP)
glinfo.exe : glinfo.obj $(LIB_DEP)
glutfx.exe : glutfx.obj $(LIB_DEP)
isosurf.exe : isosurf.obj $(LIB_DEP) [-.util]readtex.obj
	cxxlink $(MMS$TARGET_NAME),[-.util]readtex.obj,$(GL_LIBS)
morph3d.exe : morph3d.obj $(LIB_DEP)
paltex.exe : paltex.obj $(LIB_DEP)
pointblast.exe : pointblast.obj $(LIB_DEP)
reflect.exe : reflect.obj [-.util]readtex.obj [-.util]showbuffer.obj\
	$(LIB_DEP)
	cxxlink $(MMS$TARGET_NAME),[-.util]readtex,showbuffer,$(GL_LIBS)
spectex.exe : spectex.obj $(LIB_DEP)
stex3d.exe : stex3d.obj $(LIB_DEP)
tessdemo.exe : tessdemo.obj $(LIB_DEP)
texcyl.exe : texcyl.obj [-.util]readtex.obj $(LIB_DEP)
	cxxlink $(MMS$TARGET_NAME),[-.util]readtex.obj,$(GL_LIBS)
texobj.exe : texobj.obj $(LIB_DEP)
trispd.exe : trispd.obj $(LIB_DEP)
winpos.exe : winpos.obj [-.util]readtex.obj $(LIB_DEP)
	cxxlink $(MMS$TARGET_NAME),[-.util]readtex.obj,$(GL_LIBS)


bounce.obj : bounce.c
clearspd.obj : clearspd.c
drawpix.obj : drawpix.c
gamma.obj : gamma.c
gears.obj : gears.c
glinfo.obj : glinfo.c
glutfx.obj : glutfx.c
isosurf.obj : isosurf.c
morph3d.obj : morph3d.c
paltex.obj : paltex.c
pointblast.obj : pointblast.c
reflect.obj : reflect.c
spectex.obj : spectex.c
stex3d.obj : stex3d.c
tessdemo.obj : tessdemo.c
texcyl.obj : texcyl.c
texobj.obj : texobj.c
trispd.obj : trispd.c
winpos.obj : winpos.c
