# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE)) ; \
	done

install:
	$(INSTALL) -d $(INSTALL_DIR)/lib
	$(COPY_LIBS) $(TOP)/lib/libGLU.* $(INSTALL_DIR)/lib

clean:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE) clean) ; \
	done
