# src/mesa/drivers/dri/Makefile

TOP = ../../../..

include $(TOP)/configs/current

#SUBDIRS = fb ffb gamma i810 i830 mach64 mga r128 r200 radeon sis tdfx unichrome
SUBDIRS = fb gamma i810 i830 mach64 mga r128 r200 radeon sis tdfx


default: $(LIB_DIR) subdirs


$(LIB_DIR):
	-mkdir $(LIB_DIR)


subdirs:
	@for dir in $(SUBDIRS) ; do \
		pushd $$dir ; \
		make ; \
		popd ; \
	done


clean:
	@for dir in $(SUBDIRS) ; do \
		pushd $$dir ; \
		make clean ; \
		popd ; \
	done
