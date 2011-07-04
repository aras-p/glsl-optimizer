# src/gbm/backends/dri/Makefile

TOP = ../../../..
include $(TOP)/configs/current

GBM_BACKEND = gbm_dri
GBM_SOURCES = gbm_dri.c driver_name.c

GBM_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/gbm/main \

GBM_LIBS = $(LIBUDEV_LIBS) $(LIBDRM_LIB) -L$(TOP)/lib -lglapi 

GBM_CFLAGS = \
	-DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\" \
	$(LIBUDEV_CFLAGS) \
	$(LIBDRM_CFLAGS)

GBM_BUILTIN=true

include ../Makefile.template
