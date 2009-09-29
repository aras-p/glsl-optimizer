TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = util

C_SOURCES = \
	u_debug.c \
	u_debug_dump.c \
	u_debug_symbol.c \
	u_debug_stack.c \
	u_blit.c \
	u_cache.c \
	u_cpu_detect.c \
	u_draw_quad.c \
	u_format.c \
	u_format_access.c \
	u_format_table.c \
	u_gen_mipmap.c \
	u_handle_table.c \
	u_hash_table.c \
	u_hash.c \
	u_keymap.c \
	u_linear.c \
	u_network.c \
	u_math.c \
	u_mm.c \
	u_rect.c \
	u_simple_shaders.c \
	u_snprintf.c \
	u_stream_stdc.c \
	u_stream_wd.c \
	u_surface.c \
	u_tile.c \
	u_time.c \
	u_timed_winsys.c \
	u_upload_mgr.c \
	u_simple_screen.c

include ../../Makefile.template

u_format_table.c: u_format_table.py u_format_parse.py u_format.csv
	python u_format_table.py u_format.csv > $@

u_format_access.c: u_format_access.py u_format_parse.py u_format.csv
	python u_format_access.py u_format.csv > $@
