# src/mesa/drivers/dri/nouveau/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

CFLAGS += $(shell pkg-config libdrm libdrm_nouveau --cflags)
DRI_LIB_DEPS += $(shell pkg-config libdrm_nouveau --libs)

LIBNAME = nouveau_vieux_dri.so

MINIGLX_SOURCES =

DRIVER_SOURCES = \
	nouveau_screen.c \
	nouveau_context.c \
	nouveau_fbo.c \
	nouveau_driver.c \
	nouveau_state.c \
	nouveau_bufferobj.c \
	nouveau_span.c \
	nouveau_bo_state.c \
	nouveau_texture.c \
	nouveau_surface.c \
	nv04_context.c \
	nv04_render.c \
	nv04_state_fb.c \
	nv04_state_raster.c \
	nv04_state_tex.c \
	nv04_state_frag.c \
	nv04_surface.c \
	nv10_context.c \
	nv10_render.c \
	nv10_state_fb.c \
	nv10_state_polygon.c \
	nv10_state_raster.c \
	nv10_state_tex.c \
	nv10_state_frag.c \
	nv10_state_tnl.c \
	nv20_context.c \
	nv20_render.c \
	nv20_state_fb.c \
	nv20_state_polygon.c \
	nv20_state_raster.c \
	nv20_state_tex.c \
	nv20_state_frag.c \
	nv20_state_tnl.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES =


include ../Makefile.template

symlinks:
