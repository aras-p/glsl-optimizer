# Makefile for GLUT-based demo programs for VMS
# contributed by Jouk Jansen  joukj@crys.chem.uva.nl


.first
	define gl [-.include.gl]

.include [-]mms-config.

##### MACROS #####

INCDIR = ([-.include],[-.util])
CFLAGS = /include=$(INCDIR)/prefix=all/name=(as_is,short)

.ifdef SHARE
GL_LIBS = $(XLIBS)
.else
GL_LIBS = [-.lib]libGLUT/l,libMesaGLU/l,libMesaGL/l,$(XLIBS)
.endif

LIB_DEP = [-.lib]$(GL_LIB) [-.lib]$(GLU_LIB) [-.lib]$(GLUT_LIB)

PROGS = cva.exe,\
	dinoshade.exe,\
	fogcoord.exe,\
	manytex.exe,\
	multipal.exe,\
	projtex.exe,\
	seccolor.exe,\
	sharedtex.exe,\
	texline.exe,\
	texwrap.exe,\
	vptest1.exe,\
	vptest2.exe,\
	vptest3.exe,\
	vptorus.exe,\
	vpwarpmesh.exe

##### RULES #####
.obj.exe :
	cxxlink $(MMS$TARGET_NAME),$(GL_LIBS)

##### TARGETS #####
default :
	$(MMS)$(MMSQUALIFIERS) $(PROGS)

clean :
	delete *.obj;*

realclean :
	delete $(PROGS)
	delete *.obj;*

cva.exe : cva.obj $(LIB_DEP)
dinoshade.exe : dinoshade.obj $(LIB_DEP)
fogcoord.exe : fogcoord.obj $(LIB_DEP)
manytex.exe : manytex.obj $(LIB_DEP)
multipal.exe : multipal.obj $(LIB_DEP)
projtex.exe : projtex.obj $(LIB_DEP)
seccolor.exe : seccolor.obj $(LIB_DEP)
sharedtex.exe : sharedtex.obj $(LIB_DEP)
texline.exe : texline.obj $(LIB_DEP)
texwrap.exe : texwrap.obj $(LIB_DEP)
vptest1.exe : vptest1.obj $(LIB_DEP)
vptest2.exe : vptest2.obj $(LIB_DEP)
vptest3.exe : vptest3.obj $(LIB_DEP)
vptorus.exe : vptorus.obj $(LIB_DEP)
vpwarpmesh.exe : vpwarpmesh.obj $(LIB_DEP)
