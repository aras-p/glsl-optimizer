# Makefile for demo programs for VMS
# contributed by Jouk Jansen  joukj@crys.chem.uva.nl


.first
	define gl [-.include.gl]

.include [-]mms-config.

##### MACROS #####

INCDIR = [-.include]
CFLAGS = /include=$(INCDIR)/define=(FBIND=1)

GL_LIBS = [-.lib]libMesaaux/l,libMesatk/l,libMesaGLU/l,libMesaGL/l,$(XLIBS)

LIB_DEP = [-.lib]$(GL_LIB) [-.lib]$(GLU_LIB) [-.lib]$(TK_LIB) [-.lib]$(AUX_LIB)

PROGS = bounce.exe;,gamma.exe;,gears.exe;,glxdemo.exe;,glxpixmap.exe;,\
	isosurf.exe;,offset.exe;,osdemo.exe;,spin.exe;,test0.exe;,\
	texobj.exe;,xdemo.exe;,reflect.exe;,winpos.exe;



##### RULES #####


##### TARGETS #####
default :
	mms $(PROGS)

clean :
	delete *.obj;*

realclean :
	delete $(PROGS)
	delete *.obj;*

bounce.exe; : bounce.obj $(LIB_DEP)
	link bounce,$(GL_LIBS)
	
gamma.exe; : gamma.obj $(LIB_DEP)
	link gamma,$(GL_LIBS)

gears.exe; : gears.obj $(LIB_DEP)
	link gears,$(GL_LIBS)

glxdemo.exe; : glxdemo.obj $(LIB_DEP)
	link glxdemo,$(GL_LIBS)

glxpixmap.exe; : glxpixmap.obj $(LIB_DEP)
	link glxpixmap,$(GL_LIBS)

isosurf.exe; : isosurf.obj $(LIB_DEP)
	link isosurf,$(GL_LIBS)

offset.exe; : offset.obj $(LIB_DEP)
	link offset,$(GL_LIBS)

osdemo.exe; : osdemo.obj $(LIB_DEP)
	link osdemo,$(GL_LIBS)

spin.exe; : spin.obj $(LIB_DEP)
	link spin,$(GL_LIBS)

test0.exe; : test0.obj $(LIB_DEP)
	link test0,$(GL_LIBS)

texobj.exe; : texobj.obj $(LIB_DEP)
	link texobj,$(GL_LIBS)

xdemo.exe; : xdemo.obj $(LIB_DEP)
	link xdemo,$(GL_LIBS)

reflect.exe; : reflect.obj $(LIB_DEP)
	link reflect,$(GL_LIBS)

winpos.exe; : winpos.obj $(LIB_DEP)
	link winpos,$(GL_LIBS)
