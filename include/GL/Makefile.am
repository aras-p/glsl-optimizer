## Process this file with automake to produce Makefile.in

GLincludedir = $(includedir)/GL

INC_FX = fxmesa.h
INC_GGI = ggimesa.h
INC_OSMESA = osmesa.h
INC_SVGA = svgamesa.h
INC_X11 = glx.h glxext.h glx_mangle.h xmesa.h xmesa_x.h xmesa_xf86.h
INC_GLUT = glut.h glutf90.h

if HAVE_FX
sel_inc_fx = $(INC_FX)
endif

if HAVE_GGI
sel_inc_ggi = $(INC_GGI)
endif

if HAVE_OSMESA
sel_inc_osmesa = $(INC_OSMESA)
endif

if HAVE_SVGA
sel_inc_svga = $(INC_SVGA)
endif

if HAVE_X11
sel_inc_x11 = $(INC_X11)
endif

if NEED_GLUT
sel_inc_glut = $(INC_GLUT)
endif

EXTRA_HEADERS = amesa.h dosmesa.h foomesa.h glut_h.dja mesa_wgl.h mglmesa.h \
	vms_x_fix.h wmesa.h \
	$(INC_FX) $(INC_GGI) $(INC_OSMESA) $(INC_SVGA) $(INC_X11) $(INC_GLUT)

GLinclude_HEADERS = gl.h glext.h gl_mangle.h glu.h glu_mangle.h \
	$(sel_inc_fx) $(sel_inc_ggi) $(sel_inc_osmesa) $(sel_inc_svga) \
	$(sel_inc_x11) $(sel_inc_glut)
include $(top_srcdir)/common_rules.make
