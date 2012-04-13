include Makefile.sources

noinst_LIBRARIES = libr600.a

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include \
	$(RADEON_CFLAGS) \
	$(DEFINES) \
	$(ASM_FLAGS) \
	$(PIC_FLAGS) \
	$(VISIBILITY_CFLAGS)

libr600_a_SOURCES = \
	$(C_SOURCES)
