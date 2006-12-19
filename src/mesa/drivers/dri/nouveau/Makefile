# src/mesa/drivers/dri/nouveau/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = nouveau_dri.so

MINIGLX_SOURCES = 

DRIVER_SOURCES = \
	nouveau_buffers.c        \
	nouveau_card.c           \
	nouveau_context.c        \
	nouveau_driver.c         \
	nouveau_fifo.c           \
	nouveau_lock.c           \
	nouveau_object.c         \
	nouveau_screen.c         \
	nouveau_span.c           \
	nouveau_state.c          \
	nouveau_shader.c         \
	nouveau_shader_0_arb.c   \
	nouveau_shader_1.c       \
	nouveau_shader_2.c       \
	nouveau_tex.c            \
	nouveau_swtcl.c          \
	nv10_swtcl.c             \
	nv10_state.c             \
	nv20_state.c             \
	nv30_state.c             \
	nouveau_state_cache.c    \
	nv20_vertprog.c          \
	nv30_fragprog.c          \
	nv30_vertprog.c          \
	nv40_fragprog.c          \
	nv40_vertprog.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 

ASM_SOURCES = 


include ../Makefile.template

symlinks:
