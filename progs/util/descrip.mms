# Makefile for GLUT-based demo programs for VMS
# contributed by Jouk Jansen  joukj@crys.chem.uva.nl


.first
	define gl [--.include.gl]

.include [--]mms-config.

##### MACROS #####

INCDIR = ([--.include],[-.util])
CFLAGS = /include=$(INCDIR)/prefix=all/name=(as_is,short)

.ifdef SHARE
GL_LIBS = $(XLIBS)
LIB_DEP = [--.lib]$(GL_SHAR) [--.lib]$(GLU_SHAR) [--.lib]$(GLUT_SHAR)
.else
GL_LIBS = [--.lib]libGLUT/l,libMesaGLU/l,libMesaGL/l,$(XLIBS)
LIB_DEP = [--.lib]$(GL_LIB) [--.lib]$(GLU_LIB) [--.lib]$(GLUT_LIB)
.endif


OBJS =readtex.obj,showbuffer.obj


##### RULES #####
.obj.exe :
	cxxlink $(MMS$TARGET_NAME),$(GL_LIBS)

##### TARGETS #####
default :
	$(MMS)$(MMSQUALIFIERS) $(OBJS)

clean :
	delete *.obj;*

realclean :
	delete *.obj;*

readtex.obj : readtex.c
showbuffer.obj : showbuffer.c
