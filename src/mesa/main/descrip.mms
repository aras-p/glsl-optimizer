# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 5 Januari 2001

.first
	define gl [-.include.gl]

.include [-]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [-.include]
LIBDIR = [-.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)

CORE_SOURCES =accum.c \
	api_loopback.c \
	api_noop.c \
	api_validate.c \
 	attrib.c \
	blend.c \
	buffers.c \
	clip.c \
	colortab.c \
	config.c \
	context.c \
	convolve.c \
	debug.c \
	depth.c \
	dispatch.c \
	dlist.c \
	drawpix.c \
	enable.c \
	enums.c \
	eval.c \
	extensions.c \
	feedback.c \
	fog.c \
	get.c \
	glapi.c \
	glthread.c \
	hash.c \
	highpc.c \
	hint.c \
	histogram.c \
	image.c \
	imports.c \
	light.c \
	lines.c \
	lowpc.c \
	matrix.c \
	mem.c \
	mmath.c \
	pixel.c \
	points.c \
	polygon.c \
	rastpos.c \
	state.c \
	stencil.c \
	texformat.c \
	teximage.c \
	texobj.c \
	texstate.c \
	texstore.c \
	texutil.c \
	varray.c \
	vtxfmt.c \
	[.x86]x86.c

DRIVER_SOURCES = [.x]glxapi.c [.x]fakeglx.c [.x]xfonts.c \
[.x]xm_api.c [.x]xm_dd.c [.x]xm_line.c [.x]xm_span.c [.x]xm_tri.c \
[.osmesa]osmesa.c \
[.svga]svgamesa.c \
[.fx]fxapi.c [.fx]fxdd.c [.fx]fxddtex.c \
[.fx]fxddspan.c\
[.fx]fxsetup.c \
[.fx]fxtexman.c \
[.fx]fxtris.c \
[.fx]fxvb.c \
[.fx]fxglidew.c

RASTER_SOURCES = [.swrast]s_aatriangle.c \
[.swrast]s_aaline.c \
[.swrast]s_accum.c \
[.swrast]s_alpha.c \
[.swrast]s_alphabuf.c \
[.swrast]s_bitmap.c \
[.swrast]s_blend.c \
[.swrast]s_buffers.c \
[.swrast]s_copypix.c \
[.swrast]s_context.c \
[.swrast]s_depth.c \
[.swrast]s_drawpix.c \
[.swrast]s_fog.c \
[.swrast]s_feedback.c \
[.swrast]s_histogram.c \
[.swrast]s_imaging.c \
[.swrast]s_lines.c \
[.swrast]s_logic.c \
[.swrast]s_masking.c \
[.swrast]s_pb.c \
[.swrast]s_pixeltex.c \
[.swrast]s_points.c \
[.swrast]s_readpix.c \
[.swrast]s_scissor.c \
[.swrast]s_span.c \
[.swrast]s_stencil.c \
[.swrast]s_texstore.c \
[.swrast]s_texture.c \
[.swrast]s_triangle.c \
[.swrast]s_zoom.c \
[.swrast_setup]ss_context.c \
[.swrast_setup]ss_interp.c \
[.swrast_setup]ss_triangle.c \
[.swrast_setup]ss_vb.c 

ASM_SOURCES =

TNL_SOURCES=[.tnl]t_array_api.c \
[.tnl]t_array_import.c \
[.tnl]t_context.c \
[.tnl]t_eval_api.c \
[.tnl]t_imm_alloc.c \
[.tnl]t_imm_api.c \
[.tnl]t_imm_debug.c \
[.tnl]t_imm_dlist.c \
[.tnl]t_imm_elt.c \
[.tnl]t_imm_eval.c \
[.tnl]t_imm_exec.c \
[.tnl]t_imm_fixup.c \
[.tnl]t_pipeline.c \
[.tnl]t_vb_fog.c \
[.tnl]t_vb_light.c \
[.tnl]t_vb_normals.c \
[.tnl]t_vb_points.c \
[.tnl]t_vb_render.c \
[.tnl]t_vb_texgen.c \
[.tnl]t_vb_texmat.c \
[.tnl]t_vb_vertex.c

TNLDD_SOURCES=[.tnl_dd]t_dd.c \
[.tnl_dd]t_dd_vb.c

MATH_SOURCES=[.math]m_debug_xform.c \
[.math]m_debug_norm.c \
[.math]m_debug_vertex.c \
[.math]m_eval.c \
[.math]m_matrix.c \
[.math]m_translate.c \
[.math]m_vector.c \
[.math]m_vertices.c \
[.math]m_xform.c  

CACHE_SOURCES=[.array_cache]ac_context.c \
	[.array_cache]ac_import.c
 
OBJECTS1=accum.obj,\
api_loopback.obj,\
api_noop.obj,\
api_validate.obj,\
attrib.obj,\
blend.obj,\
buffers.obj,\
clip.obj,\
colortab.obj,\
config.obj,\
context.obj,\
convolve.obj,\
debug.obj,\
depth.obj,\
dispatch.obj,\
dlist.obj,\
drawpix.obj

OBJECTS2=enable.obj,\
enums.obj,\
eval.obj,\
extensions.obj,\
feedback.obj,\
fog.obj,\
get.obj,\
glapi.obj,\
glthread.obj,\
hash.obj,\
highpc.obj,\
hint.obj,\
histogram.obj,\
image.obj,\
imports.obj,\
light.obj,\
lines.obj,\
lowpc.obj,\
matrix.obj,\
mem.obj

OBJECTS3=mmath.obj,\
pixel.obj,\
points.obj,\
polygon.obj,\
rastpos.obj,\
state.obj,\
stencil.obj,\
texformat.obj,\
teximage.obj,\
texobj.obj,\
texstate.obj,\
texstore.obj,\
texutil.obj,\
varray.obj,\
vtxfmt.obj,\
[.x86]x86.obj

OBJECTS4=[.x]glxapi.obj,[.x]fakeglx.obj,[.x]xfonts.obj,\
[.x]xm_api.obj,[.x]xm_dd.obj,[.x]xm_line.obj,[.x]xm_span.obj,[.x]xm_tri.obj,\
[.osmesa]osmesa.obj,\
[.svga]svgamesa.obj

OBJECTS5=[.fx]fxapi.obj,[.fx]fxdd.obj,[.fx]fxddtex.obj

OBJECTS6=[.fx]fxddspan.obj,\
[.fx]fxsetup.obj,\
[.fx]fxtexman.obj,\
[.fx]fxtris.obj,\
[.fx]fxvb.obj,\
[.fx]fxglidew.obj

OBJECTS7=[.swrast]s_aatriangle.obj,\
[.swrast]s_accum.obj,\
[.swrast]s_alpha.obj,\
[.swrast]s_alphabuf.obj,\
[.swrast]s_bitmap.obj,\
[.swrast]s_blend.obj,\
[.swrast]s_buffers.obj,\
[.swrast]s_copypix.obj,\
[.swrast]s_context.obj,\
[.swrast]s_depth.obj

OBJECTS8=[.swrast]s_drawpix.obj,\
[.swrast]s_fog.obj,\
[.swrast]s_histogram.obj,\
[.swrast]s_imaging.obj,\
[.swrast]s_lines.obj,\
[.swrast]s_logic.obj,\
[.swrast]s_masking.obj,\
[.swrast]s_pb.obj,\
[.swrast]s_pixeltex.obj,\
[.swrast]s_points.obj

OBJECTS9=[.swrast]s_readpix.obj,\
[.swrast]s_aaline.obj,\
[.swrast]s_scissor.obj,\
[.swrast]s_span.obj,\
[.swrast]s_stencil.obj,\
[.swrast]s_texstore.obj,\
[.swrast]s_texture.obj,\
[.swrast]s_triangle.obj,\
[.swrast]s_feedback.obj,\
[.swrast]s_zoom.obj

OBJECTS10=[.swrast_setup]ss_context.obj,\
[.swrast_setup]ss_interp.obj,\
[.swrast_setup]ss_triangle.obj,\
[.swrast_setup]ss_vb.obj 

OBJECTS11=[.tnl]t_array_api.obj,\
[.tnl]t_array_import.obj,\
[.tnl]t_context.obj,\
[.tnl]t_eval_api.obj,\
[.tnl]t_imm_alloc.obj,\
[.tnl]t_imm_api.obj,\
[.tnl]t_imm_debug.obj,\
[.tnl]t_imm_dlist.obj,\
[.tnl]t_imm_elt.obj,\
[.tnl]t_imm_eval.obj,\
[.tnl]t_imm_exec.obj

OBJECTS12=[.tnl]t_imm_fixup.obj,\
[.tnl]t_pipeline.obj,\
[.tnl]t_vb_fog.obj,\
[.tnl]t_vb_light.obj,\
[.tnl]t_vb_normals.obj,\
[.tnl]t_vb_points.obj,\
[.tnl]t_vb_render.obj,\
[.tnl]t_vb_texgen.obj,\
[.tnl]t_vb_texmat.obj,\
[.tnl]t_vb_vertex.obj

OBJECTS13=[.math]m_debug_xform.obj,\
[.math]m_debug_norm.obj,\
[.math]m_debug_vertex.obj,\
[.math]m_eval.obj,\
[.math]m_matrix.obj,\
[.math]m_translate.obj,\
[.math]m_vector.obj,\
[.math]m_vertices.obj,\
[.math]m_xform.obj 

OBJECTS14=[.array_cache]ac_context.obj,\
	[.array_cache]ac_import.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS1),$(OBJECTS2) $(OBJECTS3) $(OBJECTS4)\
	$(OBJECTS5) $(OBJECTS6) $(OBJECTS7) $(OBJECTS8) $(OBJECTS9)\
	$(OBJECTS10) $(OBJECTS11) $(OBJECTS12) $(OBJECTS13) $(OBJECTS14)
.ifdef SHARE
  @ WRITE_ SYS$OUTPUT "  generating mesagl1.opt"
  @ OPEN_/WRITE FILE  mesagl1.opt
  @ WRITE_ FILE "!"
  @ WRITE_ FILE "! mesagl1.opt generated by DESCRIP.$(MMS_EXT)" 
  @ WRITE_ FILE "!"
  @ WRITE_ FILE "IDENTIFICATION=""$(VERSION)"""
  @ WRITE_ FILE "GSMATCH=LEQUAL,3,4
  @ WRITE_ FILE "$(OBJECTS1)"
  @ WRITE_ FILE "$(OBJECTS2)"
  @ WRITE_ FILE "$(OBJECTS3)"
  @ WRITE_ FILE "$(OBJECTS4)"
  @ WRITE_ FILE "$(OBJECTS5)"
  @ WRITE_ FILE "$(OBJECTS6)"
  @ WRITE_ FILE "$(OBJECTS7)"
  @ WRITE_ FILE "$(OBJECTS8)"
  @ WRITE_ FILE "$(OBJECTS9)"
  @ WRITE_ FILE "$(OBJECTS10)"
  @ WRITE_ FILE "$(OBJECTS11)"
  @ WRITE_ FILE "$(OBJECTS12)"
  @ WRITE_ FILE "$(OBJECTS13)"
  @ WRITE_ FILE "$(OBJECTS14)"
  @ write_ file "sys$share:decw$xextlibshr/share"
  @ write_ file "sys$share:decw$xlibshr/share"
  @ write_ file "sys$share:pthread$rtl/share"
  @ CLOSE_ FILE
  @ WRITE_ SYS$OUTPUT "  generating mesagl.map ..."
  @ LINK_/NODEB/NOSHARE/NOEXE/MAP=mesagl.map/FULL mesagl1.opt/OPT
  @ WRITE_ SYS$OUTPUT "  analyzing mesagl.map ..."
  @ @[-.vms]ANALYZE_MAP.COM mesagl.map mesagl.opt
  @ WRITE_ SYS$OUTPUT "  linking $(GL_LIB) ..."
  @ LINK_/NODEB/SHARE=$(GL_LIB)/MAP=mesagl.map/FULL mesagl1.opt/opt,mesagl.opt/opt
.else
  @ $(MAKELIB) $(GL_LIB) $(OBJECTS1)
  @ library $(GL_LIB) $(OBJECTS2)
  @ library $(GL_LIB) $(OBJECTS3)
  @ library $(GL_LIB) $(OBJECTS4)
  @ library $(GL_LIB) $(OBJECTS5)
  @ library $(GL_LIB) $(OBJECTS6)
  @ library $(GL_LIB) $(OBJECTS7)
  @ library $(GL_LIB) $(OBJECTS8)
  @ library $(GL_LIB) $(OBJECTS9)
  @ library $(GL_LIB) $(OBJECTS10)
  @ library $(GL_LIB) $(OBJECTS11)
  @ library $(GL_LIB) $(OBJECTS12)
  @ library $(GL_LIB) $(OBJECTS13)
  @ library $(GL_LIB) $(OBJECTS14)
.endif
  @ rename $(GL_LIB)* $(LIBDIR)

clean :
	purge
	delete *.obj;*

pixeltex.obj : pixeltex.c

imports.obj : imports.c

[.x86]x86.obj : [.x86]x86.c
	$(CC) $(CFLAGS) /obj=[.x86]x86.obj [.x86]x86.c
[.x]glxapi.obj : [.x]glxapi.c
	$(CC) $(CFLAGS) /obj=[.x]glxapi.obj [.x]glxapi.c
[.x]fakeglx.obj : [.x]fakeglx.c
	$(CC) $(CFLAGS) /obj=[.x]fakeglx.obj [.x]fakeglx.c
[.x]xfonts.obj : [.x]xfonts.c
	$(CC) $(CFLAGS) /obj=[.x]xfonts.obj [.x]xfonts.c
[.x]xm_api.obj : [.x]xm_api.c
	$(CC) $(CFLAGS) /obj=[.x]xm_api.obj [.x]xm_api.c
[.x]xm_dd.obj : [.x]xm_dd.c
	$(CC) $(CFLAGS)/nowarn /obj=[.x]xm_dd.obj [.x]xm_dd.c
[.x]xm_line.obj : [.x]xm_line.c
	$(CC) $(CFLAGS) /obj=[.x]xm_line.obj [.x]xm_line.c
[.x]xm_span.obj : [.x]xm_span.c
	$(CC) $(CFLAGS) /obj=[.x]xm_span.obj [.x]xm_span.c
[.x]xm_tri.obj : [.x]xm_tri.c
	$(CC) $(CFLAGS) /obj=[.x]xm_tri.obj [.x]xm_tri.c
[.osmesa]osmesa.obj : [.osmesa]osmesa.c
	$(CC) $(CFLAGS) /obj=[.osmesa]osmesa.obj [.osmesa]osmesa.c
[.svga]svgamesa.obj : [.svga]svgamesa.c
	$(CC) $(CFLAGS) /obj=[.svga]svgamesa.obj [.svga]svgamesa.c
[.fx]fxapi.obj : [.fx]fxapi.c
	$(CC) $(CFLAGS) /obj=[.fx]fxapi.obj [.fx]fxapi.c
[.fx]fxdd.obj : [.fx]fxdd.c
	$(CC) $(CFLAGS) /obj=[.fx]fxdd.obj [.fx]fxdd.c
[.fx]fxddtex.obj : [.fx]fxddtex.c
	$(CC) $(CFLAGS) /obj=[.fx]fxddtex.obj [.fx]fxddtex.c
[.fx]fxddspan.obj : [.fx]fxddspan.c
	$(CC) $(CFLAGS) /obj=[.fx]fxddspan.obj [.fx]fxddspan.c
[.fx]fxsetup.obj : [.fx]fxsetup.c
	$(CC) $(CFLAGS) /obj=[.fx]fxsetup.obj [.fx]fxsetup.c
[.fx]fxtexman.obj : [.fx]fxtexman.c
	$(CC) $(CFLAGS) /obj=[.fx]fxtexman.obj [.fx]fxtexman.c
[.fx]fxtris.obj : [.fx]fxtris.c
	$(CC) $(CFLAGS) /obj=[.fx]fxtris.obj [.fx]fxtris.c
[.fx]fxvb.obj : [.fx]fxvb.c
	$(CC) $(CFLAGS) /obj=[.fx]fxvb.obj [.fx]fxvb.c
[.fx]fxglidew.obj : [.fx]fxglidew.c
	$(CC) $(CFLAGS) /obj=[.fx]fxglidew.obj [.fx]fxglidew.c
[.swrast]s_aaline.obj : [.swrast]s_aaline.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_aaline.obj [.swrast]s_aaline.c
[.swrast]s_aatriangle.obj : [.swrast]s_aatriangle.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_aatriangle.obj [.swrast]s_aatriangle.c
[.swrast]s_accum.obj : [.swrast]s_accum.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_accum.obj [.swrast]s_accum.c
[.swrast]s_alpha.obj : [.swrast]s_alpha.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_alpha.obj [.swrast]s_alpha.c
[.swrast]s_alphabuf.obj : [.swrast]s_alphabuf.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_alphabuf.obj [.swrast]s_alphabuf.c
[.swrast]s_bitmap.obj : [.swrast]s_bitmap.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_bitmap.obj [.swrast]s_bitmap.c
[.swrast]s_blend.obj : [.swrast]s_blend.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_blend.obj [.swrast]s_blend.c
[.swrast]s_buffers.obj : [.swrast]s_buffers.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_buffers.obj [.swrast]s_buffers.c
[.swrast]s_copypix.obj : [.swrast]s_copypix.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_copypix.obj [.swrast]s_copypix.c
[.swrast]s_context.obj : [.swrast]s_context.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_context.obj [.swrast]s_context.c
[.swrast]s_depth.obj : [.swrast]s_depth.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_depth.obj [.swrast]s_depth.c
[.swrast]s_drawpix.obj : [.swrast]s_drawpix.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_drawpix.obj [.swrast]s_drawpix.c
[.swrast]s_feedback.obj : [.swrast]s_feedback.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_feedback.obj [.swrast]s_feedback.c
[.swrast]s_fog.obj : [.swrast]s_fog.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_fog.obj [.swrast]s_fog.c
[.swrast]s_histogram.obj : [.swrast]s_histogram.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_histogram.obj [.swrast]s_histogram.c
[.swrast]s_imaging.obj : [.swrast]s_imaging.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_imaging.obj [.swrast]s_imaging.c
[.swrast]s_lines.obj : [.swrast]s_lines.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_lines.obj [.swrast]s_lines.c
[.swrast]s_logic.obj : [.swrast]s_logic.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_logic.obj [.swrast]s_logic.c
[.swrast]s_masking.obj : [.swrast]s_masking.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_masking.obj [.swrast]s_masking.c
[.swrast]s_pb.obj : [.swrast]s_pb.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_pb.obj [.swrast]s_pb.c
[.swrast]s_pixeltex.obj : [.swrast]s_pixeltex.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_pixeltex.obj [.swrast]s_pixeltex.c
[.swrast]s_points.obj : [.swrast]s_points.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_points.obj [.swrast]s_points.c
[.swrast]s_readpix.obj : [.swrast]s_readpix.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_readpix.obj [.swrast]s_readpix.c
[.swrast]s_scissor.obj : [.swrast]s_scissor.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_scissor.obj [.swrast]s_scissor.c
[.swrast]s_span.obj : [.swrast]s_span.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_span.obj [.swrast]s_span.c
[.swrast]s_stencil.obj : [.swrast]s_stencil.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_stencil.obj [.swrast]s_stencil.c
[.swrast]s_texstore.obj : [.swrast]s_texstore.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_texstore.obj [.swrast]s_texstore.c
[.swrast]s_texture.obj : [.swrast]s_texture.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_texture.obj [.swrast]s_texture.c
[.swrast]s_triangle.obj : [.swrast]s_triangle.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_triangle.obj [.swrast]s_triangle.c
[.swrast]s_zoom.obj : [.swrast]s_zoom.c
	$(CC) $(CFLAGS) /obj=[.swrast]s_zoom.obj [.swrast]s_zoom.c
[.swrast_setup]ss_context.obj : [.swrast_setup]ss_context.c
	$(CC) $(CFLAGS) /obj=[.swrast_setup]ss_context.obj [.swrast_setup]ss_context.c
[.swrast_setup]ss_interp.obj : [.swrast_setup]ss_interp.c
	$(CC) $(CFLAGS) /obj=[.swrast_setup]ss_interp.obj [.swrast_setup]ss_interp.c
[.swrast_setup]ss_triangle.obj : [.swrast_setup]ss_triangle.c
	$(CC) $(CFLAGS) /obj=[.swrast_setup]ss_triangle.obj [.swrast_setup]ss_triangle.c
[.swrast_setup]ss_vb.obj : [.swrast_setup]ss_vb.c
	$(CC) $(CFLAGS) /obj=[.swrast_setup]ss_vb.obj [.swrast_setup]ss_vb.c
[.tnl]t_array_api.obj : [.tnl]t_array_api.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_array_api.obj [.tnl]t_array_api.c
[.tnl]t_array_import.obj : [.tnl]t_array_import.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_array_import.obj [.tnl]t_array_import.c
[.tnl]t_context.obj : [.tnl]t_context.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_context.obj [.tnl]t_context.c
[.tnl]t_eval_api.obj : [.tnl]t_eval_api.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_eval_api.obj [.tnl]t_eval_api.c
[.tnl]t_imm_alloc.obj : [.tnl]t_imm_alloc.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_alloc.obj [.tnl]t_imm_alloc.c
[.tnl]t_imm_api.obj : [.tnl]t_imm_api.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_api.obj [.tnl]t_imm_api.c
[.tnl]t_imm_debug.obj : [.tnl]t_imm_debug.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_debug.obj [.tnl]t_imm_debug.c
[.tnl]t_imm_dlist.obj : [.tnl]t_imm_dlist.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_dlist.obj [.tnl]t_imm_dlist.c
[.tnl]t_imm_elt.obj : [.tnl]t_imm_elt.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_elt.obj [.tnl]t_imm_elt.c
[.tnl]t_imm_eval.obj : [.tnl]t_imm_eval.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_eval.obj [.tnl]t_imm_eval.c
[.tnl]t_imm_exec.obj : [.tnl]t_imm_exec.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_exec.obj [.tnl]t_imm_exec.c
[.tnl]t_imm_fixup.obj : [.tnl]t_imm_fixup.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_imm_fixup.obj [.tnl]t_imm_fixup.c
[.tnl]t_pipeline.obj : [.tnl]t_pipeline.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_pipeline.obj [.tnl]t_pipeline.c
[.tnl]t_vb_fog.obj : [.tnl]t_vb_fog.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_fog.obj [.tnl]t_vb_fog.c
[.tnl]t_vb_light.obj : [.tnl]t_vb_light.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_light.obj [.tnl]t_vb_light.c
[.tnl]t_vb_normals.obj : [.tnl]t_vb_normals.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_normals.obj [.tnl]t_vb_normals.c
[.tnl]t_vb_points.obj : [.tnl]t_vb_points.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_points.obj [.tnl]t_vb_points.c
[.tnl]t_vb_render.obj : [.tnl]t_vb_render.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_render.obj [.tnl]t_vb_render.c
[.tnl]t_vb_texgen.obj : [.tnl]t_vb_texgen.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_texgen.obj [.tnl]t_vb_texgen.c
[.tnl]t_vb_texmat.obj : [.tnl]t_vb_texmat.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_texmat.obj [.tnl]t_vb_texmat.c
[.tnl]t_vb_vertex.obj : [.tnl]t_vb_vertex.c
	$(CC) $(CFLAGS) /obj=[.tnl]t_vb_vertex.obj [.tnl]t_vb_vertex.c
[.math]m_debug_xform.obj : [.math]m_debug_xform.c
	$(CC) $(CFLAGS) /obj=[.math]m_debug_xform.obj [.math]m_debug_xform.c
[.math]m_debug_norm.obj : [.math]m_debug_norm.c
	$(CC) $(CFLAGS) /obj=[.math]m_debug_norm.obj [.math]m_debug_norm.c
[.math]m_debug_vertex.obj : [.math]m_debug_vertex.c
	$(CC) $(CFLAGS) /obj=[.math]m_debug_vertex.obj [.math]m_debug_vertex.c
[.math]m_eval.obj : [.math]m_eval.c
	$(CC) $(CFLAGS) /obj=[.math]m_eval.obj [.math]m_eval.c
[.math]m_matrix.obj : [.math]m_matrix.c
	$(CC) $(CFLAGS) /obj=[.math]m_matrix.obj [.math]m_matrix.c
[.math]m_translate.obj : [.math]m_translate.c
	$(CC) $(CFLAGS) /obj=[.math]m_translate.obj [.math]m_translate.c
[.math]m_vector.obj : [.math]m_vector.c
	$(CC) $(CFLAGS) /obj=[.math]m_vector.obj [.math]m_vector.c
[.math]m_vertices.obj : [.math]m_vertices.c
	$(CC) $(CFLAGS) /obj=[.math]m_vertices.obj [.math]m_vertices.c
[.math]m_xform.obj : [.math]m_xform.c
	$(CC) $(CFLAGS) /obj=[.math]m_xform.obj [.math]m_xform.c
[.array_cache]ac_context.obj : [.array_cache]ac_context.c
	$(CC) $(CFLAGS) /obj=[.array_cache]ac_context.obj \
	[.array_cache]ac_context.c
[.array_cache]ac_import.obj : [.array_cache]ac_import.c
	$(CC) $(CFLAGS) /obj=[.array_cache]ac_import.obj \
	[.array_cache]ac_import.c
[.tnl_dd]t_dd.obj : [.tnl_dd]t_dd.c
	$(CC) $(CFLAGS) /obj=[.tnl_dd]t_dd.obj [.tnl_dd]t_dd.c
[.tnl_dd]t_dd_vb.obj : [.tnl_dd]t_dd_vb.c
	$(CC) $(CFLAGS) /obj=[.tnl_dd]t_dd_vb.obj [.tnl_dd]t_dd_vb.c
