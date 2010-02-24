### Lists of source files, included by Makefiles

MAIN_SOURCES = \
	main/api_arrayelt.c \
	main/api_exec.c \
	main/api_loopback.c \
	main/api_noop.c \
	main/api_validate.c \
	main/accum.c \
	main/attrib.c \
	main/arrayobj.c \
	main/blend.c \
	main/bufferobj.c \
	main/buffers.c \
	main/clear.c \
	main/clip.c \
	main/colortab.c \
	main/condrender.c \
	main/context.c \
	main/convolve.c \
	main/cpuinfo.c \
	main/debug.c \
	main/depth.c \
	main/depthstencil.c \
	main/dlist.c \
	main/dlopen.c \
	main/drawpix.c \
	main/enable.c \
	main/enums.c \
	main/eval.c \
	main/execmem.c \
	main/extensions.c \
	main/fbobject.c \
	main/feedback.c \
	main/ffvertex_prog.c \
	main/fog.c \
	main/formats.c \
	main/framebuffer.c \
	main/get.c \
	main/getstring.c \
	main/hash.c \
	main/hint.c \
	main/histogram.c \
	main/image.c \
	main/imports.c \
	main/light.c \
	main/lines.c \
	main/matrix.c \
	main/mipmap.c \
	main/mm.c \
	main/multisample.c \
	main/pixel.c \
	main/pixelstore.c \
	main/points.c \
	main/polygon.c \
	main/queryobj.c \
	main/rastpos.c \
	main/rbadaptors.c \
	main/readpix.c \
	main/remap.c \
	main/renderbuffer.c \
	main/scissor.c \
	main/shaders.c \
	main/shared.c \
	main/state.c \
	main/stencil.c \
	main/syncobj.c \
	main/texcompress.c \
	main/texcompress_s3tc.c \
	main/texcompress_fxt1.c \
	main/texenv.c \
	main/texenvprogram.c \
	main/texfetch.c \
	main/texformat.c \
	main/texgen.c \
	main/texgetimage.c \
	main/teximage.c \
	main/texobj.c \
	main/texparam.c \
	main/texrender.c \
	main/texstate.c \
	main/texstore.c \
	main/varray.c \
	main/version.c \
	main/viewport.c \
	main/vtxfmt.c

GLAPI_SOURCES = \
	glapi/glapi.c \
	glapi/glapi_dispatch.c \
	glapi/glapi_getproc.c \
	glapi/glapi_nop.c \
	glapi/glthread.c

MATH_SOURCES = \
	math/m_debug_clip.c \
	math/m_debug_norm.c \
	math/m_debug_xform.c \
	math/m_eval.c \
	math/m_matrix.c \
	math/m_translate.c \
	math/m_vector.c

MATH_XFORM_SOURCES = \
	math/m_xform.c

SWRAST_SOURCES = \
	swrast/s_aaline.c \
	swrast/s_aatriangle.c \
	swrast/s_accum.c \
	swrast/s_alpha.c \
	swrast/s_atifragshader.c \
	swrast/s_bitmap.c \
	swrast/s_blend.c \
	swrast/s_blit.c \
	swrast/s_clear.c \
	swrast/s_copypix.c \
	swrast/s_context.c \
	swrast/s_depth.c \
	swrast/s_drawpix.c \
	swrast/s_feedback.c \
	swrast/s_fog.c \
	swrast/s_fragprog.c \
	swrast/s_lines.c \
	swrast/s_logic.c \
	swrast/s_masking.c \
	swrast/s_points.c \
	swrast/s_readpix.c \
	swrast/s_span.c \
	swrast/s_stencil.c \
	swrast/s_texcombine.c \
	swrast/s_texfilter.c \
	swrast/s_triangle.c \
	swrast/s_zoom.c

SWRAST_SETUP_SOURCES = \
	swrast_setup/ss_context.c \
	swrast_setup/ss_triangle.c 

TNL_SOURCES = \
	tnl/t_context.c \
	tnl/t_pipeline.c \
	tnl/t_draw.c \
	tnl/t_rasterpos.c \
	tnl/t_vb_program.c \
	tnl/t_vb_render.c \
	tnl/t_vb_texgen.c \
	tnl/t_vb_texmat.c \
	tnl/t_vb_vertex.c \
	tnl/t_vb_cull.c \
	tnl/t_vb_fog.c \
	tnl/t_vb_light.c \
	tnl/t_vb_normals.c \
	tnl/t_vb_points.c \
	tnl/t_vp_build.c \
	tnl/t_vertex.c \
	tnl/t_vertex_sse.c \
	tnl/t_vertex_generic.c 

VBO_SOURCES = \
	vbo/vbo_context.c \
	vbo/vbo_exec.c \
	vbo/vbo_exec_api.c \
	vbo/vbo_exec_array.c \
	vbo/vbo_exec_draw.c \
	vbo/vbo_exec_eval.c \
	vbo/vbo_rebase.c \
	vbo/vbo_split.c \
	vbo/vbo_split_copy.c \
	vbo/vbo_split_inplace.c \
	vbo/vbo_save.c \
	vbo/vbo_save_api.c \
	vbo/vbo_save_draw.c \
	vbo/vbo_save_loopback.c 

STATETRACKER_SOURCES = \
	state_tracker/st_atom.c \
	state_tracker/st_atom_blend.c \
	state_tracker/st_atom_clip.c \
	state_tracker/st_atom_constbuf.c \
	state_tracker/st_atom_depth.c \
	state_tracker/st_atom_framebuffer.c \
	state_tracker/st_atom_pixeltransfer.c \
	state_tracker/st_atom_sampler.c \
	state_tracker/st_atom_scissor.c \
	state_tracker/st_atom_shader.c \
	state_tracker/st_atom_rasterizer.c \
	state_tracker/st_atom_stipple.c \
	state_tracker/st_atom_texture.c \
	state_tracker/st_atom_viewport.c \
	state_tracker/st_cb_accum.c \
	state_tracker/st_cb_bitmap.c \
	state_tracker/st_cb_blit.c \
	state_tracker/st_cb_bufferobjects.c \
	state_tracker/st_cb_clear.c \
	state_tracker/st_cb_condrender.c \
	state_tracker/st_cb_flush.c \
	state_tracker/st_cb_drawpixels.c \
	state_tracker/st_cb_fbo.c \
	state_tracker/st_cb_feedback.c \
	state_tracker/st_cb_program.c \
	state_tracker/st_cb_queryobj.c \
	state_tracker/st_cb_rasterpos.c \
	state_tracker/st_cb_readpixels.c \
	state_tracker/st_cb_strings.c \
	state_tracker/st_cb_texture.c \
	state_tracker/st_context.c \
	state_tracker/st_debug.c \
	state_tracker/st_draw.c \
	state_tracker/st_draw_feedback.c \
	state_tracker/st_extensions.c \
	state_tracker/st_format.c \
	state_tracker/st_framebuffer.c \
	state_tracker/st_gen_mipmap.c \
	state_tracker/st_mesa_to_tgsi.c \
	state_tracker/st_program.c \
	state_tracker/st_texture.c

SHADER_SOURCES = \
	shader/arbprogparse.c \
	shader/arbprogram.c \
	shader/atifragshader.c \
	shader/hash_table.c \
	shader/lex.yy.c \
	shader/nvfragparse.c \
	shader/nvprogram.c \
	shader/nvvertparse.c \
	shader/program.c \
	shader/program_parse.tab.c \
	shader/program_parse_extra.c \
	shader/prog_cache.c \
	shader/prog_execute.c \
	shader/prog_instruction.c \
	shader/prog_noise.c \
	shader/prog_optimize.c \
	shader/prog_parameter.c \
	shader/prog_parameter_layout.c \
	shader/prog_print.c \
	shader/prog_statevars.c \
	shader/prog_uniform.c \
	shader/programopt.c \
	shader/symbol_table.c \
	shader/shader_api.c

SLANG_SOURCES =	\
	shader/slang/slang_builtin.c	\
	shader/slang/slang_codegen.c	\
	shader/slang/slang_compile.c	\
	shader/slang/slang_compile_function.c	\
	shader/slang/slang_compile_operation.c	\
	shader/slang/slang_compile_struct.c	\
	shader/slang/slang_compile_variable.c	\
	shader/slang/slang_emit.c	\
	shader/slang/slang_ir.c	\
	shader/slang/slang_label.c	\
	shader/slang/slang_link.c	\
	shader/slang/slang_log.c	\
	shader/slang/slang_mem.c	\
	shader/slang/slang_print.c	\
	shader/slang/slang_simplify.c	\
	shader/slang/slang_storage.c	\
	shader/slang/slang_typeinfo.c	\
	shader/slang/slang_vartable.c	\
	shader/slang/slang_utility.c

ASM_C_SOURCES =	\
	x86/common_x86.c \
	x86/x86_xform.c \
	x86/3dnow.c \
	x86/sse.c \
	x86/rtasm/x86sse.c \
	sparc/sparc.c \
	ppc/common_ppc.c \
	x86-64/x86-64.c

X86_SOURCES =			\
	x86/common_x86_asm.S	\
	x86/x86_xform2.S	\
	x86/x86_xform3.S	\
	x86/x86_xform4.S	\
	x86/x86_cliptest.S	\
	x86/mmx_blend.S		\
	x86/3dnow_xform1.S	\
	x86/3dnow_xform2.S	\
	x86/3dnow_xform3.S	\
	x86/3dnow_xform4.S	\
	x86/3dnow_normal.S	\
	x86/sse_xform1.S	\
	x86/sse_xform2.S	\
	x86/sse_xform3.S	\
	x86/sse_xform4.S	\
	x86/sse_normal.S	\
	x86/read_rgba_span_x86.S

X86_API =			\
	x86/glapi_x86.S

X86-64_SOURCES =		\
	x86-64/xform4.S

X86-64_API =			\
	x86-64/glapi_x86-64.S

SPARC_SOURCES =			\
	sparc/clip.S		\
	sparc/norm.S		\
	sparc/xform.S

SPARC_API =			\
	sparc/glapi_sparc.S

COMMON_DRIVER_SOURCES =			\
	drivers/common/driverfuncs.c	\
	drivers/common/meta.c


# Sources for building non-Gallium drivers
MESA_SOURCES = \
	$(MAIN_SOURCES)		\
	$(MATH_SOURCES)		\
	$(MATH_XFORM_SOURCES)	\
	$(VBO_SOURCES)		\
	$(TNL_SOURCES)		\
	$(SHADER_SOURCES)	\
	$(SWRAST_SOURCES)	\
	$(SWRAST_SETUP_SOURCES)	\
	$(COMMON_DRIVER_SOURCES)\
	$(ASM_C_SOURCES)	\
	$(SLANG_SOURCES)

# Sources for building Gallium drivers
MESA_GALLIUM_SOURCES = \
	$(MAIN_SOURCES)		\
	$(MATH_SOURCES)		\
	$(VBO_SOURCES)		\
	$(STATETRACKER_SOURCES)	\
	$(SHADER_SOURCES)	\
	ppc/common_ppc.c	\
	x86/common_x86.c	\
	$(SLANG_SOURCES)

# All the core C sources, for dependency checking
ALL_SOURCES = \
	$(MESA_SOURCES)		\
	$(GLAPI_SOURCES)	\
	$(MESA_ASM_SOURCES)	\
	$(STATETRACKER_SOURCES)


### Object files

MESA_OBJECTS = \
	$(MESA_SOURCES:.c=.o) \
	$(MESA_ASM_SOURCES:.S=.o)

MESA_GALLIUM_OBJECTS = \
	$(MESA_GALLIUM_SOURCES:.c=.o) \
	$(MESA_ASM_SOURCES:.S=.o)

GLAPI_OBJECTS = \
	$(GLAPI_SOURCES:.c=.o) \
	$(GLAPI_ASM_SOURCES:.S=.o)


COMMON_DRIVER_OBJECTS = $(COMMON_DRIVER_SOURCES:.c=.o)


### Other archives/libraries

GLSL_LIBS = \
	$(TOP)/src/glsl/pp/libglslpp.a \
	$(TOP)/src/glsl/cl/libglslcl.a


### Include directories

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary
