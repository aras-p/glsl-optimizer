# src/Makefile

TOP = ..

include $(TOP)/configs/current

SUBDIRS = $(SRC_DIRS)


default: message $(LIB_DIR) subdirs


message:
	@echo "Making sources for" $(CONFIG_NAME)


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; $(MAKE)) || exit 1 ; \
		fi \
	done


$(LIB_DIR):
	-mkdir $(LIB_DIR)


clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; $(MAKE) clean) ; \
		fi \
	done
