# Top-level Mesa makefile

TOP = .

SUBDIRS = src progs


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1 ; \
		fi \
	done


doxygen:
	cd doxygen && $(MAKE)

clean:
	-@touch $(TOP)/configs/current
	-@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) ; \
		fi \
	done
	-@test -s $(TOP)/configs/current || rm -f $(TOP)/configs/current


realclean: clean
	-rm -rf lib*
	-rm -f $(TOP)/configs/current
	-rm -f $(TOP)/configs/autoconf
	-rm -rf autom4te.cache
	-find . '(' -name '*.o' -o -name '*.a' -o -name '*.so' -o \
	  -name depend -o -name depend.bak ')' -exec rm -f '{}' ';'



install:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) install) || exit 1 ; \
		fi \
	done


# DirectFBGL module installation
linux-directfb-install:
	cd src/mesa/drivers/directfb && $(MAKE) install

.PHONY: default doxygen clean realclean install linux-directfb-install

# If there's no current configuration file
$(TOP)/configs/current:
	@echo
	@echo
	@echo "Please choose a configuration from the following list:"
	@ls -1 $(TOP)/configs | grep -v "current\|default\|CVS\|autoconf.*"
	@echo
	@echo "Then type 'make <config>' (ex: 'make linux-x86')"
	@echo
	@echo "Or, run './configure' then 'make'"
	@echo "See './configure --help' for details"
	@echo
	@echo "(ignore the following error message)"
	@exit 1


# Rules to set/install a specific build configuration
aix \
aix-64 \
aix-64-static \
aix-gcc \
aix-static \
autoconf \
bluegene-osmesa \
bluegene-xlc-osmesa \
beos \
catamount-osmesa-pgi \
darwin \
darwin-fat-32bit \
darwin-fat-all \
darwin-static \
darwin-static-x86ppc \
freebsd \
freebsd-dri \
freebsd-dri-amd64 \
freebsd-dri-x86 \
hpux10 \
hpux10-gcc \
hpux10-static \
hpux11-32 \
hpux11-32-static \
hpux11-32-static-nothreads \
hpux11-64 \
hpux11-64-static \
hpux11-ia64 \
hpux11-ia64-static \
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
linux-dri-debug \
linux-dri-x86 \
linux-dri-x86-64 \
linux-dri-ppc \
linux-dri-xcb \
linux-indirect \
linux-fbdev \
linux-glide \
linux-ia64-icc \
linux-ia64-icc-static \
linux-icc \
linux-icc-static \
linux-osmesa \
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
linux-x86-32 \
linux-x86-64 \
linux-x86-64-debug \
linux-x86-64-static \
linux-x86-glide \
linux-x86-static \
netbsd \
openbsd \
osf1 \
osf1-static \
solaris-x86 \
solaris-x86-gcc \
solaris-x86-gcc-static \
sunos4 \
sunos4-gcc \
sunos4-static \
sunos5 \
sunos5-gcc \
sunos5-64-gcc \
sunos5-smp \
sunos5-v8 \
sunos5-v8-static \
sunos5-v9 \
sunos5-v9-static \
sunos5-v9-cc-g++ \
ultrix-gcc:
	@ if test -f configs/current || test -L configs/current ; then \
		echo "Please run 'make realclean' before changing configs" ; \
		exit 1 ; \
	fi
	(cd configs && rm -f current && ln -s $@ current)
	$(MAKE) default


# Rules for making release tarballs

DIRECTORY = Mesa-7.1-rc4
LIB_NAME = MesaLib-7.1-rc4
DEMO_NAME = MesaDemos-7.1-rc4
GLUT_NAME = MesaGLUT-7.1-rc4

MAIN_FILES = \
	$(DIRECTORY)/Makefile*						\
	$(DIRECTORY)/configure						\
	$(DIRECTORY)/configure.ac					\
	$(DIRECTORY)/acinclude.m4					\
	$(DIRECTORY)/aclocal.m4						\
	$(DIRECTORY)/descrip.mms					\
	$(DIRECTORY)/mms-config.					\
	$(DIRECTORY)/bin/config.guess					\
	$(DIRECTORY)/bin/config.sub					\
	$(DIRECTORY)/bin/install-sh					\
	$(DIRECTORY)/bin/mklib						\
	$(DIRECTORY)/bin/minstall					\
	$(DIRECTORY)/bin/version.mk					\
	$(DIRECTORY)/configs/[a-z]*					\
	$(DIRECTORY)/docs/*.html					\
	$(DIRECTORY)/docs/COPYING					\
	$(DIRECTORY)/docs/README.*					\
	$(DIRECTORY)/docs/RELNOTES*					\
	$(DIRECTORY)/docs/*.spec					\
	$(DIRECTORY)/include/GL/internal/glcore.h			\
	$(DIRECTORY)/include/GL/amesa.h					\
	$(DIRECTORY)/include/GL/dmesa.h					\
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
	$(DIRECTORY)/include/GL/glfbdev.h				\
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
	$(DIRECTORY)/include/GLView.h					\
	$(DIRECTORY)/src/Makefile					\
	$(DIRECTORY)/src/descrip.mms					\
	$(DIRECTORY)/src/mesa/Makefile*					\
	$(DIRECTORY)/src/mesa/sources					\
	$(DIRECTORY)/src/mesa/descrip.mms				\
	$(DIRECTORY)/src/mesa/gl.pc.in					\
	$(DIRECTORY)/src/mesa/depend					\
	$(DIRECTORY)/src/mesa/main/*.[chS]				\
	$(DIRECTORY)/src/mesa/main/descrip.mms				\
	$(DIRECTORY)/src/mesa/glapi/*.[chS]				\
	$(DIRECTORY)/src/mesa/glapi/descrip.mms				\
	$(DIRECTORY)/src/mesa/math/*.[ch]				\
	$(DIRECTORY)/src/mesa/math/descrip.mms				\
	$(DIRECTORY)/src/mesa/shader/*.[ch]				\
	$(DIRECTORY)/src/mesa/shader/descrip.mms			\
	$(DIRECTORY)/src/mesa/shader/grammar/*.[ch]			\
	$(DIRECTORY)/src/mesa/shader/grammar/descrip.mms		\
	$(DIRECTORY)/src/mesa/shader/slang/*.[ch]			\
	$(DIRECTORY)/src/mesa/shader/slang/descrip.mms			\
	$(DIRECTORY)/src/mesa/shader/slang/library/*.[ch]		\
	$(DIRECTORY)/src/mesa/shader/slang/library/*.gc			\
	$(DIRECTORY)/src/mesa/shader/slang/library/*.syn		\
	$(DIRECTORY)/src/mesa/shader/slang/library/Makefile		\
	$(DIRECTORY)/src/mesa/swrast/*.[ch]				\
	$(DIRECTORY)/src/mesa/swrast/descrip.mms			\
	$(DIRECTORY)/src/mesa/swrast_setup/*.[ch]			\
	$(DIRECTORY)/src/mesa/swrast_setup/descrip.mms			\
	$(DIRECTORY)/src/mesa/vbo/*.[chS]				\
	$(DIRECTORY)/src/mesa/vbo/descrip.mms				\
	$(DIRECTORY)/src/mesa/tnl/*.[chS]				\
	$(DIRECTORY)/src/mesa/tnl/descrip.mms				\
	$(DIRECTORY)/src/mesa/tnl_dd/*.[ch]				\
	$(DIRECTORY)/src/mesa/tnl_dd/imm/*.[ch]				\
	$(DIRECTORY)/src/mesa/tnl_dd/imm/NOTES.imm			\
	$(DIRECTORY)/src/mesa/drivers/Makefile				\
	$(DIRECTORY)/src/mesa/drivers/beos/*.cpp			\
	$(DIRECTORY)/src/mesa/drivers/beos/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/common/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/common/descrip.mms		\
	$(DIRECTORY)/src/mesa/drivers/directfb/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/directfb/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/dos/*.[chS]			\
	$(DIRECTORY)/src/mesa/drivers/fbdev/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/fbdev/glfbdev.c			\
	$(DIRECTORY)/src/mesa/drivers/glide/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/ggi/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/ggi/ggimesa.conf.in		\
	$(DIRECTORY)/src/mesa/drivers/ggi/default/*.c			\
	$(DIRECTORY)/src/mesa/drivers/ggi/default/genkgi.conf.in	\
	$(DIRECTORY)/src/mesa/drivers/ggi/display/*.c			\
	$(DIRECTORY)/src/mesa/drivers/ggi/display/fbdev.conf.in		\
	$(DIRECTORY)/src/mesa/drivers/ggi/include/ggi/mesa/*.h		\
	$(DIRECTORY)/src/mesa/drivers/osmesa/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/osmesa/Makefile.win		\
	$(DIRECTORY)/src/mesa/drivers/osmesa/descrip.mms		\
	$(DIRECTORY)/src/mesa/drivers/osmesa/osmesa.def			\
	$(DIRECTORY)/src/mesa/drivers/osmesa/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/svga/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/windows/*/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/windows/*/*.def			\
	$(DIRECTORY)/src/mesa/drivers/x11/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/x11/descrip.mms			\
	$(DIRECTORY)/src/mesa/drivers/x11/*.[ch]			\
	$(DIRECTORY)/src/mesa/drivers/glslcompiler/Makefile		\
	$(DIRECTORY)/src/mesa/drivers/glslcompiler/glslcompiler.c	\
	$(DIRECTORY)/src/mesa/ppc/*.[ch]				\
	$(DIRECTORY)/src/mesa/sparc/*.[chS]				\
	$(DIRECTORY)/src/mesa/x86/Makefile				\
	$(DIRECTORY)/src/mesa/x86/*.[ch]				\
	$(DIRECTORY)/src/mesa/x86/*.S					\
	$(DIRECTORY)/src/mesa/x86/rtasm/*.[ch]				\
	$(DIRECTORY)/src/mesa/x86-64/*.[chS]				\
	$(DIRECTORY)/src/mesa/x86-64/Makefile				\
	$(DIRECTORY)/progs/Makefile					\
	$(DIRECTORY)/progs/util/README					\
	$(DIRECTORY)/progs/util/*.[ch]					\
	$(DIRECTORY)/progs/util/sampleMakefile				\
	$(DIRECTORY)/vms/analyze_map.com				\
	$(DIRECTORY)/vms/xlib.opt					\
	$(DIRECTORY)/vms/xlib_share.opt					\
	$(DIRECTORY)/windows/VC8/mesa/mesa.sln				\
	$(DIRECTORY)/windows/VC8/mesa/gdi/gdi.vcproj			\
	$(DIRECTORY)/windows/VC8/mesa/glu/glu.vcproj			\
	$(DIRECTORY)/windows/VC8/mesa/mesa/mesa.vcproj			\
	$(DIRECTORY)/windows/VC8/mesa/osmesa/osmesa.vcproj		\
	$(DIRECTORY)/windows/VC8/progs/progs.sln			\
	$(DIRECTORY)/windows/VC8/progs/demos/gears.vcproj		\
	$(DIRECTORY)/windows/VC8/progs/glut/glut.vcproj


DRI_FILES = \
	$(DIRECTORY)/include/GL/internal/dri_interface.h		\
	$(DIRECTORY)/include/GL/internal/dri_sarea.h			\
	$(DIRECTORY)/include/GL/internal/sarea.h			\
	$(DIRECTORY)/src/glx/Makefile					\
	$(DIRECTORY)/src/glx/x11/Makefile				\
	$(DIRECTORY)/src/glx/x11/*.[ch]					\
	$(DIRECTORY)/src/mesa/drivers/dri/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/dri/Makefile.template		\
	$(DIRECTORY)/src/mesa/drivers/dri/dri.pc.in			\
	$(DIRECTORY)/src/mesa/drivers/dri/common/xmlpool/*.[ch]		\
	$(DIRECTORY)/src/mesa/drivers/dri/common/xmlpool/*.po		\
	$(DIRECTORY)/src/mesa/drivers/dri/*/*.[chS]			\
	$(DIRECTORY)/src/mesa/drivers/dri/*/Makefile			\
	$(DIRECTORY)/src/mesa/drivers/dri/*/Doxyfile			\
	$(DIRECTORY)/src/mesa/drivers/dri/*/server/*.[ch]

SGI_GLU_FILES = \
	$(DIRECTORY)/src/glu/Makefile					\
	$(DIRECTORY)/src/glu/descrip.mms				\
	$(DIRECTORY)/src/glu/glu.pc.in					\
	$(DIRECTORY)/src/glu/sgi/Makefile				\
	$(DIRECTORY)/src/glu/sgi/Makefile.mgw				\
	$(DIRECTORY)/src/glu/sgi/Makefile.win				\
	$(DIRECTORY)/src/glu/sgi/Makefile.DJ				\
	$(DIRECTORY)/src/glu/sgi/glu.def				\
	$(DIRECTORY)/src/glu/sgi/dummy.cc				\
	$(DIRECTORY)/src/glu/sgi/descrip.mms				\
	$(DIRECTORY)/src/glu/sgi/glu.exports				\
	$(DIRECTORY)/src/glu/sgi/glu.exports.darwin			\
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
	$(DIRECTORY)/src/glu/sgi/libutil/*.[ch]

MESA_GLU_FILES = \
	$(DIRECTORY)/src/glu/mesa/README[12]		\
	$(DIRECTORY)/src/glu/mesa/Makefile*		\
	$(DIRECTORY)/src/glu/mesa/descrip.mms		\
	$(DIRECTORY)/src/glu/mesa/mms_depend		\
	$(DIRECTORY)/src/glu/mesa/*.def			\
	$(DIRECTORY)/src/glu/mesa/depend		\
	$(DIRECTORY)/src/glu/mesa/*.[ch]

GLW_FILES = \
	$(DIRECTORY)/src/glw/*.[ch]			\
	$(DIRECTORY)/src/glw/Makefile*			\
	$(DIRECTORY)/src/glw/README			\
	$(DIRECTORY)/src/glw/glw.pc.in			\
	$(DIRECTORY)/src/glw/depend

DEMO_FILES = \
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
	$(DIRECTORY)/progs/fbdev/Makefile		\
	$(DIRECTORY)/progs/fbdev/glfbdevtest.c		\
	$(DIRECTORY)/progs/osdemos/Makefile		\
	$(DIRECTORY)/progs/osdemos/*.c			\
	$(DIRECTORY)/progs/xdemos/Makefile*		\
	$(DIRECTORY)/progs/xdemos/descrip.mms		\
	$(DIRECTORY)/progs/xdemos/*.[chf]		\
	$(DIRECTORY)/progs/redbook/Makefile*		\
	$(DIRECTORY)/progs/redbook/README		\
	$(DIRECTORY)/progs/redbook/*.[ch]		\
	$(DIRECTORY)/progs/samples/Makefile*		\
	$(DIRECTORY)/progs/samples/README		\
	$(DIRECTORY)/progs/samples/*.c			\
	$(DIRECTORY)/progs/glsl/Makefile*		\
	$(DIRECTORY)/progs/glsl/*.c			\
	$(DIRECTORY)/progs/glsl/*.frag			\
	$(DIRECTORY)/progs/glsl/*.vert			\
	$(DIRECTORY)/progs/windml/Makefile.ugl		\
	$(DIRECTORY)/progs/windml/*.c			\
	$(DIRECTORY)/progs/windml/*.bmp			\
	$(DIRECTORY)/progs/ggi/*.c			\
	$(DIRECTORY)/windows/VC6/progs/demos/*.dsp	\
	$(DIRECTORY)/windows/VC6/progs/progs.dsw	\
	$(DIRECTORY)/windows/VC7/progs/demos/*.vcproj	\
	$(DIRECTORY)/windows/VC7/progs/progs.sln

GLUT_FILES = \
	$(DIRECTORY)/include/GL/glut.h			\
	$(DIRECTORY)/include/GL/glutf90.h		\
	$(DIRECTORY)/src/glut/glx/Makefile*		\
	$(DIRECTORY)/src/glut/glx/depend		\
	$(DIRECTORY)/src/glut/glx/glut.pc.in		\
	$(DIRECTORY)/src/glut/glx/*def			\
	$(DIRECTORY)/src/glut/glx/descrip.mms		\
	$(DIRECTORY)/src/glut/glx/mms_depend		\
	$(DIRECTORY)/src/glut/glx/*.[ch]		\
	$(DIRECTORY)/src/glut/beos/*.[ch]		\
	$(DIRECTORY)/src/glut/beos/*.cpp		\
	$(DIRECTORY)/src/glut/beos/Makefile		\
	$(DIRECTORY)/src/glut/dos/*.[ch]		\
	$(DIRECTORY)/src/glut/dos/Makefile.DJ		\
	$(DIRECTORY)/src/glut/dos/PC_HW/*.[chS]		\
	$(DIRECTORY)/src/glut/ggi/*.[ch]		\
	$(DIRECTORY)/src/glut/ggi/Makefile		\
	$(DIRECTORY)/src/glut/fbdev/Makefile		\
	$(DIRECTORY)/src/glut/fbdev/*[ch]		\
	$(DIRECTORY)/src/glut/mini/*[ch]		\
	$(DIRECTORY)/src/glut/mini/glut.pc.in		\
	$(DIRECTORY)/src/glut/directfb/Makefile		\
	$(DIRECTORY)/src/glut/directfb/NOTES		\
	$(DIRECTORY)/src/glut/directfb/*[ch]		\
	$(DIRECTORY)/windows/VC6/progs/glut/glut.dsp	\
	$(DIRECTORY)/windows/VC7/progs/glut/glut.vcproj

DEPEND_FILES = \
	$(TOP)/src/mesa/depend		\
	$(TOP)/src/glx/x11/depend	\
	$(TOP)/src/glw/depend		\
	$(TOP)/src/glut/glx/depend	\
	$(TOP)/src/glu/sgi/depend


LIB_FILES = $(MAIN_FILES) $(DRI_FILES) $(SGI_GLU_FILES) $(GLW_FILES)


# Everything for new a Mesa release:
tarballs: rm_depend configure aclocal.m4 lib_gz demo_gz glut_gz \
	lib_bz2 demo_bz2 glut_bz2 lib_zip demo_zip glut_zip md5


# Helper for autoconf builds
ACLOCAL = aclocal
ACLOCAL_FLAGS =
AUTOCONF = autoconf
AC_FLAGS =
aclocal.m4: configure.ac acinclude.m4
	$(ACLOCAL) $(ACLOCAL_FLAGS)
configure: configure.ac aclocal.m4 acinclude.m4
	$(AUTOCONF) $(AC_FLAGS)

rm_depend:
	@for dep in $(DEPEND_FILES) ; do \
		rm -f $$dep ; \
		touch $$dep ; \
	done

lib_gz:
	rm -f configs/current ; \
	rm -f configs/autoconf ; \
	cd .. ; \
	tar -cf $(LIB_NAME).tar $(LIB_FILES) ; \
	gzip $(LIB_NAME).tar ; \
	mv $(LIB_NAME).tar.gz $(DIRECTORY)

demo_gz:
	cd .. ; \
	tar -cf $(DEMO_NAME).tar $(DEMO_FILES) ; \
	gzip $(DEMO_NAME).tar ; \
	mv $(DEMO_NAME).tar.gz $(DIRECTORY)

glut_gz:
	cd .. ; \
	tar -cf $(GLUT_NAME).tar $(GLUT_FILES) ; \
	gzip $(GLUT_NAME).tar ; \
	mv $(GLUT_NAME).tar.gz $(DIRECTORY)

lib_bz2:
	rm -f configs/current ; \
	rm -f configs/autoconf ; \
	cd .. ; \
	tar -cf $(LIB_NAME).tar $(LIB_FILES) ; \
	bzip2 $(LIB_NAME).tar ; \
	mv $(LIB_NAME).tar.bz2 $(DIRECTORY)

demo_bz2:
	cd .. ; \
	tar -cf $(DEMO_NAME).tar $(DEMO_FILES) ; \
	bzip2 $(DEMO_NAME).tar ; \
	mv $(DEMO_NAME).tar.bz2 $(DIRECTORY)

glut_bz2:
	cd .. ; \
	tar -cf $(GLUT_NAME).tar $(GLUT_FILES) ; \
	bzip2 $(GLUT_NAME).tar ; \
	mv $(GLUT_NAME).tar.bz2 $(DIRECTORY)

lib_zip:
	rm -f configs/current ; \
	rm -f configs/autoconf ; \
	rm -f $(LIB_NAME).zip ; \
	cd .. ; \
	zip -qr $(LIB_NAME).zip $(LIB_FILES) ; \
	mv $(LIB_NAME).zip $(DIRECTORY)

demo_zip:
	rm -f $(DEMO_NAME).zip ; \
	cd .. ; \
	zip -qr $(DEMO_NAME).zip $(DEMO_FILES) ; \
	mv $(DEMO_NAME).zip $(DIRECTORY)

glut_zip:
	rm -f $(GLUT_NAME).zip ; \
	cd .. ; \
	zip -qr $(GLUT_NAME).zip $(GLUT_FILES) ; \
	mv $(GLUT_NAME).zip $(DIRECTORY)

md5:
	@-md5sum $(LIB_NAME).tar.gz
	@-md5sum $(LIB_NAME).tar.bz2
	@-md5sum $(LIB_NAME).zip
	@-md5sum $(DEMO_NAME).tar.gz
	@-md5sum $(DEMO_NAME).tar.bz2
	@-md5sum $(DEMO_NAME).zip
	@-md5sum $(GLUT_NAME).tar.gz
	@-md5sum $(GLUT_NAME).tar.bz2
	@-md5sum $(GLUT_NAME).zip

.PHONY: tarballs rm_depend lib_gz demo_gz glut_gz lib_bz2 demo_bz2 \
	glut_bz2 lib_zip demo_zip glut_zip md5
