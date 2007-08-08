
TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = i915simple

DRIVER_SOURCES = \
	i915_blit.c \
	i915_buffer.c \
	i915_clear.c \
	i915_flush.c \
	i915_context.c \
	i915_context.c \
	i915_debug.c \
	i915_regions.c \
	i915_state.c \
	i915_state_derived.c \
	i915_state_emit.c \
	i915_state_fragprog.c \
	i915_prim_emit.c \
	i915_surface.c 

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:

