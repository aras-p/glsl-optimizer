# src/mesa/drivers/dri/Makefile

TOP = ../../../..

include $(TOP)/configs/current



default: $(TOP)/$(LIB_DIR) subdirs


$(TOP)/$(LIB_DIR):
	-mkdir $(TOP)/$(LIB_DIR)


subdirs:
	@for dir in $(DRI_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done

pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@LIB_DIR@,$(LIB_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),' \
	-e 's,@DRI_DRIVER_DIR@,$(DRI_DRIVER_SEARCH_DIR),'

dri.pc: dri.pc.in
	$(pcedit) $< > $@


install: dri.pc
	@for dir in $(DRI_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) install) || exit 1 ; \
		fi \
	done
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GL/internal
	$(INSTALL) -m 0644 $(TOP)/include/GL/internal/dri_interface.h $(DESTDIR)$(INSTALL_DIR)/include/GL/internal
	$(INSTALL) -m 0644 $(TOP)/include/GL/internal/dri_sarea.h $(DESTDIR)$(INSTALL_DIR)/include/GL/internal
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig
	$(INSTALL) -m 0644 dri.pc $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig


clean:
	-@for dir in $(DRI_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) ; \
		fi \
	done
	-rm -f common/*.o
