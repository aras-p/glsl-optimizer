## Process this file with automake to produce Makefile.in

GLincludedir = $(includedir)/GL

if HAVE_FX
INC_FX = fxmesa.h
endif

if HAVE_GGI
INC_GGI = ggimesa.h
endif

if HAVE_OSMESA
INC_OSMESA = osmesa.h
endif

if HAVE_SVGA
INC_SVGA = svgamesa.h
endif

if HAVE_X11
INC_X11 = glx.h glx_mangle.h xmesa.h xmesa_x.h xmesa_xf86.h
endif

if NEED_GLUT
INC_GLUT = glut.h
endif

EXTRA_DIST = fxmesa.h ggimesa.h osmesa.h svgamesa.h \
	glx.h glx_mangle.h xmesa.h xmesa_x.h xmesa_xf86.h

GLinclude_HEADERS = gl.h glext.h gl_mangle.h glu.h glu_mangle.h \
	$(INC_FX) $(INC_GGI) $(INC_OSMESA) $(INC_SVGA) $(INC_X11) $(INC_GLUT)

