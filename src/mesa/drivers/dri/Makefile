# src/mesa/drivers/dri/Makefile

TOP = ../../../..

include $(TOP)/configs/current



default: $(LIB_DIR) subdirs


$(LIB_DIR):
	-mkdir $(LIB_DIR)


subdirs:
	@for dir in $(DRI_DIRS) ; do \
		(cd $$dir ; make) || exit 1; \
	done


clean:
	@for dir in $(DRI_DIRS) ; do \
		(cd $$dir ; make clean) ; \
	done
