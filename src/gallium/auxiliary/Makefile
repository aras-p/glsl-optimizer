TOP = ../../..
include $(TOP)/configs/current

LIBNAME = gallium

C_SOURCES = \
	cso_cache/cso_context.c \
	cso_cache/cso_cache.c \
	cso_cache/cso_hash.c \
	draw/draw_context.c \
	draw/draw_gs.c \
	draw/draw_pipe.c \
	draw/draw_pipe_aaline.c \
	draw/draw_pipe_aapoint.c \
	draw/draw_pipe_clip.c \
	draw/draw_pipe_cull.c \
	draw/draw_pipe_flatshade.c \
	draw/draw_pipe_offset.c \
	draw/draw_pipe_pstipple.c \
	draw/draw_pipe_stipple.c \
	draw/draw_pipe_twoside.c \
	draw/draw_pipe_unfilled.c \
	draw/draw_pipe_util.c \
	draw/draw_pipe_validate.c \
	draw/draw_pipe_vbuf.c \
	draw/draw_pipe_wide_line.c \
	draw/draw_pipe_wide_point.c \
	draw/draw_pt.c \
	draw/draw_pt_elts.c \
	draw/draw_pt_emit.c \
	draw/draw_pt_fetch.c \
	draw/draw_pt_fetch_emit.c \
	draw/draw_pt_fetch_shade_emit.c \
	draw/draw_pt_fetch_shade_pipeline.c \
	draw/draw_pt_post_vs.c \
	draw/draw_pt_util.c \
	draw/draw_pt_varray.c \
	draw/draw_pt_vcache.c \
	draw/draw_vertex.c \
	draw/draw_vs.c \
	draw/draw_vs_varient.c \
	draw/draw_vs_aos.c \
	draw/draw_vs_aos_io.c \
	draw/draw_vs_aos_machine.c \
	draw/draw_vs_exec.c \
	draw/draw_vs_llvm.c \
	draw/draw_vs_ppc.c \
	draw/draw_vs_sse.c \
	indices/u_indices_gen.c \
	indices/u_unfilled_gen.c \
	os/os_misc.c \
	os/os_stream_stdc.c \
	os/os_stream_wd.c \
	os/os_time.c \
	pipebuffer/pb_buffer_malloc.c \
	pipebuffer/pb_bufmgr_alt.c \
	pipebuffer/pb_bufmgr_cache.c \
	pipebuffer/pb_bufmgr_debug.c \
	pipebuffer/pb_bufmgr_mm.c \
	pipebuffer/pb_bufmgr_ondemand.c \
	pipebuffer/pb_bufmgr_pool.c \
	pipebuffer/pb_bufmgr_slab.c \
	pipebuffer/pb_validate.c \
	rbug/rbug_connection.c \
	rbug/rbug_core.c \
	rbug/rbug_texture.c \
	rbug/rbug_context.c \
	rbug/rbug_shader.c \
	rbug/rbug_demarshal.c \
	rtasm/rtasm_cpu.c \
	rtasm/rtasm_execmem.c \
	rtasm/rtasm_x86sse.c \
	rtasm/rtasm_ppc.c \
	rtasm/rtasm_ppc_spe.c \
	tgsi/tgsi_sanity.c \
	tgsi/tgsi_build.c \
	tgsi/tgsi_dump.c \
	tgsi/tgsi_exec.c \
	tgsi/tgsi_info.c \
	tgsi/tgsi_iterate.c \
	tgsi/tgsi_parse.c \
	tgsi/tgsi_ppc.c \
	tgsi/tgsi_scan.c \
	tgsi/tgsi_sse2.c \
	tgsi/tgsi_text.c \
	tgsi/tgsi_transform.c \
	tgsi/tgsi_ureg.c \
	tgsi/tgsi_util.c \
	translate/translate_generic.c \
	translate/translate_sse.c \
	translate/translate.c \
	translate/translate_cache.c \
	util/u_debug.c \
	util/u_debug_dump.c \
	util/u_debug_symbol.c \
	util/u_debug_stack.c \
	util/u_bitmask.c \
	util/u_blit.c \
	util/u_blitter.c \
	util/u_cache.c \
	util/u_cpu_detect.c \
	util/u_dl.c \
	util/u_draw_quad.c \
	util/u_format.c \
	util/u_format_access.c \
	util/u_format_table.c \
	util/u_gen_mipmap.c \
	util/u_handle_table.c \
	util/u_hash_table.c \
	util/u_hash.c \
	util/u_keymap.c \
	util/u_linear.c \
	util/u_network.c \
	util/u_math.c \
	util/u_mm.c \
	util/u_rect.c \
	util/u_ringbuffer.c \
	util/u_simple_shaders.c \
	util/u_snprintf.c \
	util/u_surface.c \
	util/u_texture.c \
	util/u_tile.c \
	util/u_timed_winsys.c \
	util/u_upload_mgr.c \
	util/u_simple_screen.c \
	vl/vl_bitstream_parser.c \
	vl/vl_mpeg12_mc_renderer.c \
	vl/vl_compositor.c \
	vl/vl_csc.c \
	vl/vl_shader_build.c

GALLIVM_SOURCES = \
	gallivm/gallivm.cpp  \
	gallivm/gallivm_cpu.cpp \
	gallivm/instructions.cpp  \
	gallivm/loweringpass.cpp \
	gallivm/tgsitollvm.cpp \
	gallivm/storage.cpp \
	gallivm/storagesoa.cpp \
	gallivm/instructionssoa.cpp

INC_SOURCES = \
	gallivm/gallivm_builtins.cpp \
	gallivm/gallivmsoabuiltins.cpp

# XXX: gallivm doesn't build correctly so disable for now
#ifeq ($(MESA_LLVM),1)
#DEFINES += -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS
#CPP_SOURCES += \
#	$(GALLIVM_SOURCES)
#endif


include ../Makefile.template


gallivm/gallivm_builtins.cpp: gallivm/llvm_builtins.c
	clang --emit-llvm < $< |llvm-as|opt -std-compile-opts > temp1.bin
	(echo "static const unsigned char llvm_builtins_data[] = {"; od -txC temp1.bin | sed -e "s/^[0-9]*//" -e s"/ \([0-9a-f][0-9a-f]\)/0x\1,/g" -e"\$$d" | sed -e"\$$s/,$$/,0x00};/") >$@
	rm temp1.bin

gallivm/gallivmsoabuiltins.cpp: gallivm/soabuiltins.c
	clang --emit-llvm < $< |llvm-as|opt -std-compile-opts > temp2.bin
	(echo "static const unsigned char soabuiltins_data[] = {"; od -txC temp2.bin | sed -e "s/^[0-9]*//" -e s"/ \([0-9a-f][0-9a-f]\)/0x\1,/g" -e"\$$d" | sed -e"\$$s/,$$/,0x00};/") >$@
	rm temp2.bin


indices/u_indices_gen.c: indices/u_indices_gen.py
	python $< > $@

indices/u_unfilled_gen.c: indices/u_unfilled_gen.py
	python $< > $@

util/u_format_table.c: util/u_format_table.py util/u_format_parse.py util/u_format.csv
	python util/u_format_table.py util/u_format.csv > $@

util/u_format_access.c: util/u_format_access.py util/u_format_parse.py util/u_format.csv
	python util/u_format_access.py util/u_format.csv > $@
