# src/mesa/shader/slang/library/Makefile

TOP = ../../../../..

include $(TOP)/configs/current

GLSL_CL = $(TOP)/src/glsl/apps/compile

#
# targets
#

.PHONY: default clean

default: builtin

clean:
	-rm -f *_gc.h

builtin: builtin_110 builtin_120

#
# builtin library sources
#

builtin_110: slang_common_builtin_gc.h slang_core_gc.h slang_fragment_builtin_gc.h slang_vertex_builtin_gc.h

builtin_120: slang_120_core_gc.h slang_builtin_120_common_gc.h slang_builtin_120_fragment_gc.h


slang_120_core_gc.h: slang_120_core.gc
	$(GLSL_CL) fragment slang_120_core.gc slang_120_core_gc.h

slang_builtin_120_common_gc.h: slang_builtin_120_common.gc
	$(GLSL_CL) fragment slang_builtin_120_common.gc slang_builtin_120_common_gc.h

slang_builtin_120_fragment_gc.h: slang_builtin_120_fragment.gc
	$(GLSL_CL) fragment slang_builtin_120_fragment.gc slang_builtin_120_fragment_gc.h

slang_common_builtin_gc.h: slang_common_builtin.gc
	$(GLSL_CL) fragment slang_common_builtin.gc slang_common_builtin_gc.h

slang_core_gc.h: slang_core.gc
	$(GLSL_CL) fragment slang_core.gc slang_core_gc.h

slang_fragment_builtin_gc.h: slang_fragment_builtin.gc
	$(GLSL_CL) fragment slang_fragment_builtin.gc slang_fragment_builtin_gc.h

slang_vertex_builtin_gc.h: slang_vertex_builtin.gc
	$(GLSL_CL) vertex slang_vertex_builtin.gc slang_vertex_builtin_gc.h

