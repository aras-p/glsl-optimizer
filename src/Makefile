# src/Makefile

TOP = ..

include $(TOP)/configs/current

SUBDIRS = $(SRC_DIRS)


default: $(LIB_DIR) subdirs


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			pushd $$dir ; \
			make ; \
			popd ; \
		fi \
	done


$(LIB_DIR):
	-mkdir $(LIB_DIR)


clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			pushd $$dir ; \
			make clean ; \
			popd ; \
		fi \
	done
