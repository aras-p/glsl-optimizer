# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE)) ; \
	done

# GLU pkg-config file
pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@LIB_DIR@,$(LIB_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),'
glu.pc: glu.pc.in
	$(pcedit) $< > $@

install: glu.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig
	$(INSTALL) $(TOP)/$(LIB_DIR)/libGLU.* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -m 644 glu.pc $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig

clean:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE) clean) ; \
	done
