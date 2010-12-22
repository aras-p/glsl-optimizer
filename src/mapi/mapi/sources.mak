# src/mapi/mapi/sources.mak
#
# mapi may be used in several ways
#
#  - In default mode, mapi implements the interface defined by mapi.h.  To use
#    this mode, compile MAPI_SOURCES.
#
#  - In util mode, mapi provides utility functions for use with glapi.  To use
#    this mode, compile MAPI_UTIL_SOURCES with MAPI_MODE_UTIL defined.

MAPI_UTIL_SOURCES = \
	u_current.c \
	u_execmem.c \
	u_thread.c

MAPI_SOURCES = \
	entry.c \
	mapi.c \
	stub.c \
	table.c \
	$(MAPI_UTIL_SOURCES)
