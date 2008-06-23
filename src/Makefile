# src/Makefile

TOP = ..

include $(TOP)/configs/current

SUBDIRS = $(SRC_DIRS)


default: message $(TOP)/$(LIB_DIR) subdirs


message:
	@echo "Making sources for" $(CONFIG_NAME)


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1; \
		fi \
	done

install: $(TOP)/$(LIB_DIR)
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) install) || exit 1 ; \
		fi \
	done

$(TOP)/$(LIB_DIR):
	-@test -d $(TOP)/$(LIB_DIR) || mkdir -p $(TOP)/$(LIB_DIR)


clean:
	-@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) ; \
		fi \
	done
