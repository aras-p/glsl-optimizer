# Top-level Mesa makefile

TOP = .

SUBDIRS = src progs


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE)) || exit 1 ; \
	done


doxygen:
	(cd doxygen ; make) ; \

clean:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE) clean) ; \
	done


realclean:
	touch $(TOP)/configs/current
	$(MAKE) clean
	-rm -rf lib*
	-rm -f $(TOP)/configs/current
	-rm -f `find . -name \*.o`
	-rm -f `find . -name \*.a`
	-rm -f `find . -name \*.so`



install:
	@echo "Installing"
	$(TOP)/bin/installmesa

# DirectFBGL module installation
linux-directfb-install:
	cd src/mesa/drivers/directfb && $(MAKE) install

# If there's no current configuration file
$(TOP)/configs/current:
	@echo
	@echo
	@echo "Please choose a configuration from the following list:"
	@ls -1 $(TOP)/configs | grep -v "current\|default\|CVS"
	@echo
	@echo "Then type 'make <config>' (ex: 'make linux-x86')"
	@echo "(ignore the following error message)"
	@exit 1


# Rules to set/install a specific build configuration
aix \
aix-64 \
aix-gcc \
aix-static \
beos \
darwin \
darwin-static \
freebsd \
freebsd-dri \
freebsd-dri-x86 \
hpux10 \
hpux10-gcc \
hpux10-static \
hpux11-32 \
hpux11-32-static \
hpux11-32-static-nothreads \
hpux11-64 \
hpux11-64-static \
hpux9 \
hpux9-gcc \
irix6-64 \
irix6-64-static \
irix6-n32 \
irix6-n32-static \
irix6-o32 \
irix6-o32-static \
linux \
linux-alpha \
linux-alpha-static \
linux-debug \
linux-directfb \
linux-dri \
linux-dri-x86 \
linux-dri-x86-64 \
linux-dri-ppc \
linux-glide \
linux-icc \
linux-icc-static \
linux-osmesa16 \
linux-osmesa16-static \
linux-osmesa32 \
linux-ppc \
linux-ppc-static \
linux-solo \
linux-solo-x86 \
linux-solo-ia64 \
linux-sparc \
linux-sparc5 \
linux-static \
linux-ultrasparc \
linux-tcc \
linux-x86 \
linux-x86-debug \
linux-x86-64 \
linux-x86-64-static \
linux-x86-glide \
linux-x86-static \
netbsd \
openbsd \
osf1 \
solaris-x86 \
solaris-x86-gcc \
sunos4 \
sunos4-gcc \
sunos4-static \
sunos5 \
sunos5-gcc \
sunos5-smp \
ultrix-gcc:
	(cd configs && rm -f current && ln -s $@ current)
	$(MAKE) default


# Rules for making release tarballs

DIRECTORY = Mesa-6.2
LIB_NAME = MesaLib-6.2
DEMO_NAME = MesaDemos-6.2

LIB_FILES =	\
	$(DIRECTORY)/Makefile*						\
	$(DIRECTORY)/descrip.mms					\
	$(DIRECTORY)/mms-config.					\
	$(DIRECTORY)/Mesa.dsw						\
	$(DIRECTORY)/bin/mklib						\
	$(DIRECTORY)/bin/installmesa					\
	$(DIRECTORY)/configs/[a-z]*					\
	$(DIRECTORY)/docs/*.html					\
	$(DIRECTORY)/docs/COPYING					\
	$(DIRECTORY)/docs/INSTALL.GNU					\
	$(DIRECTORY)/docs/README.*					\
	$(DIRECTORY)/docs/RELNOTES*					\
	$(DIRECTORY)/docs/VERSIONS					\
	$(DIRECTORY)/docs/*.spec					\
	$(DIRECTORY)/include/GL/internal/glcore.h			\
	$(DIRECTORY)/include/GL/dmesa.h					\
	$(DIRECTORY)/include/GL/amesa.h					\
	$(DIRECTORY)/include/GL/fxmesa.h				\
	$(DIRECTORY)/include/GL/ggimesa.h				\
	$(DIRECTORY)/include/GL/gl.h					\
	$(DIRECTORY)/include/GL/glext.h					\
	$(DIRECTORY)/include/GL/gl_mangle.h				\
	$(DIRECTORY)/include/GL/glu.h					\
	$(DIRECTORY)/include/GL/glu_mangle.h				\
	$(DIRECTORY)/include/GL/glx.h					\
	$(DIRECTORY)/include/GL/glxext.h				\
	$(DIRECTORY)/include/GL/glx_mangle.h				\
	$(DIRECTORY)/include/GL/mesa_wgl.h				\
	$(DIRECTORY)/include/GL/mglmesa.h				\
	$(DIRECTORY)/include/GL/osmesa.h				\
	$(DIRECTORY)/include/GL/svgamesa.h				\
	$(DIRECTORY)/include/GL/ugl*.h					\
	$(DIRECTORY)/include/GL/vms_x_fix.h				\
	$(DIRECTORY)/include/GL/wmesa.h					\
	$(DIRECTORY)/include/GL/xmesa.h					\
	$(DIRECTORY)/include/GL/xmesa_x.h				\
	$(DIRECTORY)/include/GL/xmesa_xf86.h				\
	$(DIRECTORY)/src/Makefile					\
	$(DIRECTORY)/src/descrip.mms					\
	$(DIRECTORY)/src/mesa/Makefile*					\
	$(DIRECTORY)/src/mesa/sources					\
	$(DIRECTORY)/src/mesa/descrip.mms				\
	$(DIRECTORY)/src/mesa/depend					\
	$(DIRECTORY)/src/mesa/main/*.[chS]				\
	$(DIRECTORY)/src/mesa/main/main.dsp				\
	$(DIRECTORY)/src/mesa/main/descrip.mms				\
	$(DIRECTORY)/src/mesa/glapi/*.[chS]				\
	$(DIRECTORY)/src/mesa/glapi/glapi.dsp				\
	$(DIRECTORY)/src/mesa/glapi/descrip.mms				\
	$(DIRECTORY)/src/mesa/array_cache/*.[ch]			\
	$(DIRECTORY)/src/mesa/array_cache/array_cache.dsp		\
	$(DIRECTORY)/src/mesa/array_cache/descrip.mms			\
	$(DIRECTORY)/src/mesa/math/*.[ch]				\
	$(DIRECTORY)/src/mesa/math/math.dsp				\
	$(DIRECTORY)/src/mesa/math/descrip.mms				\
	$(DIRECTORY)/src/mesa/shader/*.[ch]				\
	$(DIRECTORY)/src/mesa/shader/shader.dsp				\
	$(DIRECTORY)/src/mesa/shader/descrip.mms			\
	$(DIRECTORY)/src/mesa/swrast/*.[ch]				\
	$(DIRECTORY)/src/mesa/swrast/swrast.dsp				\
	$(DIRECTORY)/src/mesa/swrast/descrip.mms			\
	$(DIRECTORY)/src/mesa/swrast_setup/*.[ch]			\
	$(DIRECTORY)/src/mesa/swrast_setup/swrast_setup.dsp		\
	$(DIRECTORY)/src/mesa/swrast_setup/descrip.mms			\
	$(DIRECTORY)/src/mesa/tnl/*.[chS]				\
	$(DIRECTORY)/src/mesa/tnl/tnl.dsp				\
	$(DIRECTORY)/src/mesa/tnl/descrip.mms				\
	$(DIRECTORY)/src/mesa/tnl_dd/*.[ch]				\
	$(DIRECTORY)/src/mesa/tnl_dd/imm/*.[ch]				\
	$(DIRECTORY)/src/mesa/tnl_dd/imm/NOTES.imm			\
	$(DIRECTORY)/src/mesa/drivers/beos/*.cpp			\
	$(DIRECTORY)/src/mesa/drivers/beos/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/common/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/common/descrip.mms		\
	$(DIRECTORY)/src/mesa/drivers/directfb/*.[ch]		\
	$(DIRECTORY)/src/mesa/drivers/directfb/Makefile		\
	$(DIRECTORY)/src/mesa/drivers/dos/*.[chS]			\
	$(DIRECTORY)/src/mesa/drivers/dri/common/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/fbdev/glfbdev.c			\
	$(DIRECTORY)/src/mesa/drivers/glide/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/glide/*.def			\
	$(DIRECTORY)/src/mesa/drivers/ggi/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/ggi/ggimesa.conf.in		\
	$(DIRECTORY)/src/mesa/drivers/ggi/default/*.c			\
	$(DIRECTORY)/src/mesa/drivers/ggi/default/genkgi.conf.in	\
	$(DIRECTORY)/src/mesa/drivers/ggi/display/*.c			\
	$(DIRECTORY)/src/mesa/drivers/ggi/display/fbdev.conf.in		\
	$(DIRECTORY)/src/mesa/drivers/ggi/include/ggi/mesa/*.h		\
	$(DIRECTORY)/src/mesa/drivers/osmesa/Makefile.win		\
	$(DIRECTORY)/src/mesa/drivers/osmesa/descrip.mms		\
	$(DIRECTORY)/src/mesa/drivers/osmesa/osmesa.def			\
	$(DIRECTORY)/src/mesa/drivers/osmesa/osmesa.dsp			\
	$(DIRECTORY)/src/mesa/drivers/osmesa/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/svga/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/windows/*/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/windows/*/*.def			\
	$(DIRECTORY)/src/mesa/drivers/windows/*/*.dsp			\
	$(DIRECTORY)/src/mesa/drivers/x11/descrip.mms			\
	$(DIRECTORY)/src/mesa/drivers/x11/*.[ch]			\
	$(DIRECTORY)/src/mesa/sparc/*.[chS]				\
	$(DIRECTORY)/src/mesa/x86/Makefile				\
	$(DIRECTORY)/src/mesa/x86/*.[ch]				\
	$(DIRECTORY)/src/mesa/x86/*.S					\
	$(DIRECTORY)/src/glu/Makefile					\
	$(DIRECTORY)/src/glu/descrip.mms				\
	$(DIRECTORY)/src/glu/sgi/Makefile				\
	$(DIRECTORY)/src/glu/sgi/Makefile.win				\
	$(DIRECTORY)/src/glu/sgi/Makefile.DJ				\
	$(DIRECTORY)/src/glu/sgi/cc*.txt				\
	$(DIRECTORY)/src/glu/sgi/glu.def				\
	$(DIRECTORY)/src/glu/sgi/glu.dsp				\
	$(DIRECTORY)/src/glu/sgi/dummy.cc				\
	$(DIRECTORY)/src/glu/sgi/descrip.mms				\
	$(DIRECTORY)/src/glu/sgi/mesaglu.opt				\
	$(DIRECTORY)/src/glu/sgi/include/gluos.h			\
	$(DIRECTORY)/src/glu/sgi/libnurbs/interface/*.h			\
	$(DIRECTORY)/src/glu/sgi/libnurbs/interface/*.cc		\
	$(DIRECTORY)/src/glu/sgi/libnurbs/internals/*.h			\
	$(DIRECTORY)/src/glu/sgi/libnurbs/internals/*.cc		\
	$(DIRECTORY)/src/glu/sgi/libnurbs/nurbtess/*.h			\
	$(DIRECTORY)/src/glu/sgi/libnurbs/nurbtess/*.cc			\
	$(DIRECTORY)/src/glu/sgi/libtess/README				\
	$(DIRECTORY)/src/glu/sgi/libtess/alg-outline			\
	$(DIRECTORY)/src/glu/sgi/libtess/*.[ch]				\
	$(DIRECTORY)/src/glu/sgi/libutil/*.[ch]				\
	$(DIRECTORY)/src/glu/mesa/README[12]				\
	$(DIRECTORY)/src/glu/mesa/Makefile*				\
	$(DIRECTORY)/src/glu/mesa/descrip.mms				\
	$(DIRECTORY)/src/glu/mesa/mms_depend				\
	$(DIRECTORY)/src/glu/mesa/*.def					\
	$(DIRECTORY)/src/glu/mesa/depend				\
	$(DIRECTORY)/src/glu/mesa/*.[ch]				\
	$(DIRECTORY)/src/glw/*.[ch]					\
	$(DIRECTORY)/src/glw/Makefile*					\
	$(DIRECTORY)/src/glw/README					\
	$(DIRECTORY)/src/glw/depend					\
	$(DIRECTORY)/progs/Makefile					\
	$(DIRECTORY)/progs/util/README					\
	$(DIRECTORY)/progs/util/*.[ch]					\
	$(DIRECTORY)/progs/util/sampleMakefile				\
	$(DIRECTORY)/vms/analyze_map.com				\
	$(DIRECTORY)/vms/xlib.opt					\
	$(DIRECTORY)/vms/xlib_share.opt


DEMO_FILES =	\
	$(DIRECTORY)/include/GL/glut.h			\
	$(DIRECTORY)/include/GL/glutf90.h		\
	$(DIRECTORY)/src/glut/glx/Makefile*		\
	$(DIRECTORY)/src/glut/glx/depend		\
	$(DIRECTORY)/src/glut/glx/*def			\
	$(DIRECTORY)/src/glut/glx/descrip.mms		\
	$(DIRECTORY)/src/glut/glx/mms_depend		\
	$(DIRECTORY)/src/glut/glx/glut.dsp		\
	$(DIRECTORY)/src/glut/glx/*.[ch]		\
	$(DIRECTORY)/src/glut/beos/*.[ch]		\
	$(DIRECTORY)/src/glut/beos/*.cpp		\
	$(DIRECTORY)/src/glut/beos/Makefile		\
	$(DIRECTORY)/src/glut/dos/*.[ch]		\
	$(DIRECTORY)/src/glut/dos/Makefile.DJ		\
	$(DIRECTORY)/src/glut/dos/PC_HW/*.[chS]		\
	$(DIRECTORY)/src/glut/ggi/*.[ch]		\
	$(DIRECTORY)/src/glut/ggi/Makefile		\
	$(DIRECTORY)/progs/beos/*.cpp			\
	$(DIRECTORY)/progs/beos/Makefile		\
	$(DIRECTORY)/progs/images/*.rgb			\
	$(DIRECTORY)/progs/images/*.rgba		\
	$(DIRECTORY)/progs/demos/Makefile*		\
	$(DIRECTORY)/progs/demos/descrip.mms		\
	$(DIRECTORY)/progs/demos/*.[ch]			\
	$(DIRECTORY)/progs/demos/*.cxx			\
	$(DIRECTORY)/progs/demos/*.dat			\
	$(DIRECTORY)/progs/demos/README			\
	$(DIRECTORY)/progs/demos/Windows/*.dsp		\
	$(DIRECTORY)/progs/demos/Windows/*.dsw		\
	$(DIRECTORY)/progs/xdemos/Makefile*		\
	$(DIRECTORY)/progs/xdemos/descrip.mms		\
	$(DIRECTORY)/progs/xdemos/*.[chf]		\
	$(DIRECTORY)/progs/redbook/Makefile*		\
	$(DIRECTORY)/progs/redbook/README		\
	$(DIRECTORY)/progs/redbook/*.[ch]		\
	$(DIRECTORY)/progs/samples/Makefile*		\
	$(DIRECTORY)/progs/samples/README		\
	$(DIRECTORY)/progs/samples/*.c			\
	$(DIRECTORY)/progs/windml/Makefile.ugl		\
	$(DIRECTORY)/progs/windml/*.c			\
	$(DIRECTORY)/progs/windml/*.bmp			\
	$(DIRECTORY)/progs/ggi/*.c


DRI_FILES = \
	$(DIRECTORY)/src/glx/Makefile					\
	$(DIRECTORY)/src/glx/mini/*.[ch]				\
	$(DIRECTORY)/src/glx/mini/example.miniglx.conf			\
	$(DIRECTORY)/src/glx/mini/NOTES					\
	$(DIRECTORY)/src/mesa/drivers/dri/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/dri/Makefile.template		\
	$(DIRECTORY)/src/mesa/drivers/dri/common/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/dri/dri_client/imports/*.h	\
	$(DIRECTORY)/src/mesa/drivers/dri/*/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/dri/*/depend			\
	$(DIRECTORY)/src/mesa/drivers/dri/*/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/dri/*/Doxyfile			\
	$(DIRECTORY)/src/mesa/drivers/dri/*/server/*.[ch]


# Everything for new a Mesa release:
tarballs: lib_gz demo_gz lib_bz2 demo_bz2 lib_zip demo_zip md5


lib_gz:
	rm -f configs/current ; \
	cd .. ; \
	tar -cvf $(LIB_NAME).tar $(LIB_FILES) ; \
	gzip $(LIB_NAME).tar ; \
	mv $(LIB_NAME).tar.gz $(DIRECTORY)

demo_gz:
	cd .. ; \
	tar -cvf $(DEMO_NAME).tar $(DEMO_FILES) ; \
	gzip $(DEMO_NAME).tar ; \
	mv $(DEMO_NAME).tar.gz $(DIRECTORY)

lib_bz2:
	rm -f configs/current ; \
	cd .. ; \
	tar -cvf $(LIB_NAME).tar $(LIB_FILES) ; \
	bzip2 $(LIB_NAME).tar ; \
	mv $(LIB_NAME).tar.bz2 $(DIRECTORY)

demo_bz2:
	cd .. ; \
	tar -cvf $(DEMO_NAME).tar $(DEMO_FILES) ; \
	bzip2 $(DEMO_NAME).tar ; \
	mv $(DEMO_NAME).tar.bz2 $(DIRECTORY)

lib_zip:
	rm -f configs/current ; \
	-rm $(LIB_NAME).zip ; \
	cd .. ; \
	zip -r $(LIB_NAME).zip $(LIB_FILES) ; \
	mv $(LIB_NAME).zip $(DIRECTORY)

demo_zip:
	-rm $(DEMO_NAME).zip ; \
	cd .. ; \
	zip -r $(DEMO_NAME).zip $(DEMO_FILES) ; \
	mv $(DEMO_NAME).zip $(DIRECTORY)

md5:
	@-md5sum $(LIB_NAME).tar.gz
	@-md5sum $(LIB_NAME).tar.bz2
	@-md5sum $(LIB_NAME).zip
	@-md5sum $(DEMO_NAME).tar.gz
	@-md5sum $(DEMO_NAME).tar.bz2
	@-md5sum $(DEMO_NAME).zip
