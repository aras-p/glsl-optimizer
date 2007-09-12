# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE)) ; \
	done

install:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) $(TOP)/$(LIB_DIR)/libGLU.* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)

clean:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE) clean) ; \
	done
