# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 16 June 2003

.first
	define gl [---.include.gl]
	define math [-.math]
	define array_cache [-.array_cache]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main],[-.glapi]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)

SOURCES = t_array_api.c t_array_import.c t_context.c t_eval_api.c \
	t_imm_alloc.c t_imm_api.c t_imm_debug.c t_imm_dlist.c t_imm_elt.c \
	t_imm_eval.c t_imm_exec.c t_imm_fixup.c t_pipeline.c t_vb_fog.c \
	t_vb_light.c t_vb_normals.c t_vb_points.c t_vb_program.c \
	t_vb_render.c t_vb_texgen.c t_vb_texmat.c t_vb_vertex.c

OBJECTS = t_array_api.obj,t_array_import.obj,t_context.obj,t_eval_api.obj,\
	t_imm_alloc.obj,t_imm_api.obj,t_imm_debug.obj,t_imm_dlist.obj,\
	t_imm_elt.obj,t_imm_eval.obj,t_imm_exec.obj,t_imm_fixup.obj,\
	t_pipeline.obj,t_vb_fog.obj,t_vb_light.obj,t_vb_normals.obj,\
	t_vb_points.obj,t_vb_program.obj,t_vb_render.obj,t_vb_texgen.obj,\
	t_vb_texmat.obj,t_vb_vertex.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*
