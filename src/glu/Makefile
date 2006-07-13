# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE)) ; \
	done

install:
	$(INSTALL) -d $(INSTALL_DIR)/$(LIB_DIR)
	$(COPY_LIBS) $(TOP)/$(LIB_DIR)/libGLU.* $(INSTALL_DIR)/$(LIB_DIR)

clean:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE) clean) ; \
	done
