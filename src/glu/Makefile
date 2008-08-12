# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir && $(MAKE)) || exit 1 ; \
	done

# GLU pkg-config file
pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),'
glu.pc: glu.pc.in
	$(pcedit) $< > $@

install: glu.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) $(TOP)/$(LIB_DIR)/$(GLU_LIB_GLOB) $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 glu.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

clean:
	-@for dir in $(SUBDIRS) ; do \
		(cd $$dir && $(MAKE) clean) ; \
	done
	-rm -f *.pc
