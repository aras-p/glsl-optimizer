# $Id: Makefile.X11,v 1.48 2001/06/19 21:49:06 brianp Exp $

# Mesa 3-D graphics library
# Version:  3.5
# 
# Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# Top-level makefile for Mesa
# To add a new configuration for your system add it to the list below
# then update the Make-config file.

SHELL = /bin/sh



default:
	@echo "Type one of the following:"
	@echo "  make aix                  for IBM RS/6000 with AIX"
	@echo "  make aix-sl               for IBM RS/6000, make shared libs"
	@echo "  make amiwin               for Amiga with SAS/C and AmiWin"
	@echo "  make amix                 for Amiga 3000 UX  SVR4 v2.1 systems"
	@echo "  make beos-r4              for BeOS R4"
	@echo "  make bsdos                for BSD/OS from BSDI using GCC"
	@echo "  make bsdos4               for BSD/OS 4.x, dynamic libraries"
	@echo "  make cygnus               for Win95/NT using Cygnus-Win32"
	@echo "  make cygnus-linux         for Win95/NT using Cygnus-Win32 under Linux"
	@echo "  make dgux                 for Data General"
	@echo "  make freebsd              for FreeBSD systems with GCC"
	@echo "  make freebsd-386          for FreeBSD systems with GCC, w/ Intel assembly"
	@echo "  make gcc                  for a generic system with GCC"
	@echo "  make hpux9                for HP systems with HPUX 9.x"
	@echo "  make hpux9-sl             for HP systems with HPUX 9.x, make shared libs"
	@echo "  make hpux9-gcc            for HP systems with HPUX 9.x using GCC"
	@echo "  make hpux9-gcc-sl         for HP systems with HPUX 9.x, GCC, make shared libs"
	@echo "  make hpux10               for HP systems with HPUX 10.x"
	@echo "  make hpux10-sl            for HP systems with HPUX 10.x, shared libs"
	@echo "  make hpux10-gcc           for HP systems with HPUX 10.x w/ GCC"
	@echo "  make hpux10-gcc-sl        for HP systems with HPUX 10.x w/ GCC, shared libs"
	@echo "  make irix4                for SGI systems with IRIX 4.x"
	@echo "  make irix5                for SGI systems with IRIX 5.x"
	@echo "  make irix5-gcc            for SGI systems with IRIX 5.x using GCC"
	@echo "  make irix5-dso            for SGI systems with IRIX 5.x, make DSOs"
	@echo "  make irix6-o32            for SGI systems with IRIX 6.x, make o32-bit libs"
	@echo "  make irix6-o32-dso        for SGI systems with IRIX 6.x, make o32-bit DSOs"
	@echo "  make irix6-n32            for SGI systems with IRIX 6.x, make n32-bit libs"
	@echo "  make irix6-n32-dso        for SGI systems with IRIX 6.x, make n32-bit DSOs"
	@echo "  make irix6-gcc-n32-sl     for SGI systems with IRIX 6.x, GCC, make n32 DSOs"
	@echo "  make irix6-64             for SGI systems with IRIX 6.x, make 64-bit libs"
	@echo "  make irix6-64-dso         for SGI systems with IRIX 6.x, make 64-bit DSOs"

	@echo "  make linux                for Linux systems, make shared .so libs"
	@echo "  make linux-static         for Linux systems, make static .a libs"
	@echo "  make linux-trace          for Linux systems, with API trace extension"
	@echo "  make linux-x86            for Linux on Intel, make shared .so libs"
	@echo "  make linux-x86-static     for Linux on Intel, make static .a libs"
	@echo "  make linux-ggi            for Linux systems with libggi"
	@echo "  make linux-386-ggi        for Linux systems with libggi w/ Intel assembly"
	@echo "  make linux-glide          for Linux w/ 3Dfx Glide driver"
	@echo "  make linux-386-glide      for Linux w/ 3Dfx Glide driver, Intel assembly"
	@echo "  make linux-386-opt-glide  for Linux with 3Dfx Voodoo1 for GLQuake"
	@echo "  make linux-x86-glide      for Linux w/ all x86 asm for Glide"
	@echo "  make linux-alpha          for Linux on Alpha systems"
	@echo "  make linux-alpha-static   for Linux on Alpha systems, static libs"
	@echo "  make linux-ppc            for Linux on PowerPC systems"
	@echo "  make linux-ppc-static     for Linux on PowerPC systems, static libs"
	@echo "  make linux-sparc          for Linux on Sparc systems"
	@echo "  make linux-sparc5-elf     for Sparc5 systems, make ELF shared libs"
	@echo "  make linux-sparc-ultra    for UltraSparc systems, make ELF shared libs"
	@echo "  make linux-osmesa16       for 16-bit/channel OSMesa"
	@echo "  make lynxos               for LynxOS systems with GCC"
	@echo "  make macintosh            for Macintosh"
	@echo "  make machten-2.2          for Macs w/ MachTen 2.2 (68k w/ FPU)"
	@echo "  make machten-4.0          for Macs w/ MachTen 4.0.1 or newer with GNU make"
	@echo "  make mklinux              for Linux on Power Macintosh"
	@echo "  make netbsd               for NetBSD 1.0 systems with GCC"
	@echo "  make next                 for NeXT systems with NEXTSTEP 3.3"
	@echo "  make openbsd              for OpenBSD systems"
	@echo "  make openstep             for OpenStep/MacOSX Server systems"
	@echo "  make os2-x11              for OS/2 with XFree86"
	@echo "  make osf1                 for DEC Alpha systems with OSF/1"
	@echo "  make osf1-sl              for DEC Alpha systems with OSF/1, make shared libs"
	@echo "  make pgi-cygnus           for Cygnus with Portland Group, Inc. compiler"
	@echo "  make pgi-mingw32          for mingW32 with Portland Group, Inc. compiler"
	@echo "  make qnx                  for QNX V4 systems with Watcom compiler"
	@echo "  make sco                  for SCO Unix systems with ODT"
	@echo "  make sco5                 for SCO 5.0.5 OpenServer Unix"
	@echo "  make solaris-x86          for PCs with Solaris"
	@echo "  make solaris-x86-gcc      for PCs with Solaris using GCC"
	@echo "  make sunos4               for Suns with SunOS 4.x"
	@echo "  make sunos4-sl            for Suns with SunOS 4.x, make shared libs"
	@echo "  make sunos4-gcc           for Suns with SunOS 4.x and GCC"
	@echo "  make sunos4-gcc-sl        for Suns with SunOS 4.x, GCC, make shared libs"
	@echo "  make sunos5               for Suns with SunOS 5.x"
	@echo "  make sunos5-sl            for Suns with SunOS 5.x, make shared libs"
	@echo "  make sunos5-ultra         for Sun UltraSPARCs with SunOS 5.x"
	@echo "  make sunos5-ultra-sl      for Sun UltraSPARCs with SunOS 5.x, make shared libs"
	@echo "  make sunos5-thread        for Suns with SunOS 5.x, using Solaris threads"
	@echo "  make sunos5-pthread       for Suns with SunOS 5.[56] using POSIX threads"
	@echo "  make sunos5-gcc-thread    for Suns with SunOS 5.x and GCC, using Solaris threads"
	@echo "  make sunos5-gcc-pthread   for Suns with SunOS 5.[56] and GCC, using POSIX threads"
	@echo "  make sunos5-gcc           for Suns with SunOS 5.x and GCC"
	@echo "  make sunos5-gcc-sl        for Suns with SunOS 5.x, GCC, make shared libs"
	@echo "  make sunos5-x11r6-gcc-sl  for Suns with X11R6, GCC, make shared libs"
	@echo "  make sunos5-gcc-thread    for Suns with SunOS 5.x and GCC, using Solaris threads"
	@echo "  make sunos5-gcc-pthread   for Suns with SunOS 5.[56] and GCC, using POSIX threads"
	@echo "  make sunSolaris-CC        for Solaris using C++ compiler"
	@echo "  make ultrix-gcc           for DEC systems with Ultrix and GCC"
	@echo "  make unicos               for Cray C90 (and other?) systems"
	@echo "  make unixware             for PCs running UnixWare"
	@echo "  make unixware-shared      for PCs running UnixWare, shared libs"
	@echo "  make uwin                 for Windows NT with AT&T/Wipro UWIN"
	@echo "  make vistra               for Stardent Vistra systems"
	@echo "  make clean                remove .o files"
	@echo "  make realclean            remove .o, library and executable files"



# XXX we may have to split up this group of targets into those that
# have a C++ compiler and those that don't for the SI-GLU library.

aix aix-sl amix bsdos bsdos4 dgux freebsd freebsd-386 gcc \
hpux9 hpux9-sl hpux9-gcc hpux9-gcc-sl \
hpux10 hpux10-sl hpux10-gcc hpux10-gcc-sl \
irix4 irix5 irix5-gcc irix5-dso irix6-o32 irix6-o32-dso \
linux linux-static linux-debug linux-static-debug linux-prof \
linux-x86 linux-x86-static linux-x86-debug \
linux-glide linux-386-glide linux-386-opt-glide \
linux-x86-glide linux-glide-debug linux-glide-prof \
linux-alpha-static linux-alpha \
linux-ppc-static linux-ppc \
linux-sparc \
linux-sparc5-elf \
linux-sparc-ultra \
lynxos machten-2.2 machten-4.0 \
mklinux netbsd osf1 osf1-sl openbsd qnx sco sco5 \
solaris-x86 solaris-x86-gcc sunSolaris-CC \
sunos4 sunos4-sl sunos4-gcc sunos4-gcc-sl sunos4-gcc-x11r6-sl \
sunos5 sunos5-sl sunos5-ultra sunos5-ultra-sl sunos5-gcc sunos5-gcc-sl \
sunos5-thread sunos5-pthread sunos5-gcc-thread sunos5-gcc-pthread \
sunos5-x11r6-gcc-sl ultrix-gcc unicos unixware uwin vistra:
	-mkdir lib
	if [ -d src      ] ; then touch src/depend      ; fi
	if [ -d si-glu   ] ; then touch si-glu/depend  ; fi
	if [ -d src-glut ] ; then touch src-glut/depend ; fi
	if [ -d widgets-sgi ] ; then touch widgets-sgi/depend ; fi
	if [ -d src      ] ; then cd src      ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d si-glu   ] ; then cd si-glu   ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src-glut ] ; then cd src-glut ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d demos    ] ; then cd demos    ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d xdemos   ] ; then cd xdemos   ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d samples  ] ; then cd samples  ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d book     ] ; then cd book     ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d widgets-sgi ] ; then cd widgets-sgi; $(MAKE) -f Makefile.X11 $@ ; fi

irix6-n32 irix6-n32-dso irix6-gcc-n32-sl irix-debug:
	-mkdir lib32
	if [ -d src      ] ; then touch src/depend      ; fi
	if [ -d src-glu  ] ; then touch src-glu/depend  ; fi
	if [ -d src-glut ] ; then touch src-glut/depend ; fi
	if [ -d src-glut ] ; then touch src-glut/depend ; fi
	if [ -d widgets-sgi ] ; then touch widgets-sgi/depend ; fi
	if [ -d src      ] ; then cd src      ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src-glu  ] ; then cd src-glu  ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src-glut ] ; then cd src-glut ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d demos    ] ; then cd demos    ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d xdemos   ] ; then cd xdemos   ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d samples  ] ; then cd samples  ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d book     ] ; then cd book     ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d widgets-sgi ] ; then cd widgets-sgi; $(MAKE) -f Makefile.X11 $@ ; fi


irix6-64 irix6-64-dso:
	-mkdir lib64
	touch src/depend
	touch src-glu/depend
	if [ -d src-glut ] ; then touch src-glut/depend ; fi
	if [ -d widgets-sgi ] ; then touch widgets-sgi/depend ; fi
	if [ -d src      ] ; then cd src      ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src-glu  ] ; then cd src-glu  ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src-glut ] ; then cd src-glut ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d demos    ] ; then cd demos    ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d xdemos   ] ; then cd xdemos   ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d samples  ] ; then cd samples  ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d book     ] ; then cd book     ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d widgets-sgi ] ; then cd widgets-sgi; $(MAKE) -f Makefile.X11 $@ ; fi


amiwin:
	bin/mklib.amiwin


beos-r4:
	-mkdir lib
	-rm src/depend
	touch src/depend
	-rm src-glu/depend
	touch src-glu/depend
	if [ -d src     ] ; then cd src     ; $(MAKE) -f Makefile.BeOS-R4 $@ ; fi
	if [ -d src-glu ] ; then cd src-glu ; $(MAKE) -f Makefile.BeOS-R4 $@ ; fi
	if [ -d BeOS    ] ; then cd BeOS    ; $(MAKE)                        ; fi
	if [ -d src-glut.beos ] ; then cd src-glut.beos ; $(MAKE)            ; fi
	if [ -d src-glut.beos ] ; then cp src-glut.beos/obj*/libglut.so lib  ; fi
	if [ -d demos   ] ; then cd demos   ; $(MAKE) -f Makefile.BeOS-R4 $@ ; fi
	if [ -d samples ] ; then cd samples ; $(MAKE) -f Makefile.BeOS-R4 $@ ; fi
	if [ -d book    ] ; then cd book    ; $(MAKE) -f Makefile.BeOS-R4 $@ ; fi

pgi-cygnus pgi-mingw32 \
cygnus cygnus-linux:
	-mkdir lib
	touch src/depend
	touch src-glu/depend
	if [ -d widgets-sgi ] ; then touch widgets-sgi/depend ; fi
	if [ -d src      ] ; then cd src      ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src-glu  ] ; then cd src-glu  ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src-glut ] ; then cd src-glut ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d demos    ] ; then cd demos    ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d xdemos   ] ; then cd xdemos   ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d widgets-sgi ] ; then cd widgets-sgi; $(MAKE) -f Makefile.X11 $@ ; fi

macintosh:
	@echo "See the README file for Macintosh intallation information"

next:
	-mkdir lib
	cd src ; $(MAKE) -f Makefile.X11 "MYCC=${CC}" $@
	cd src-glu ; $(MAKE) -f Makefile.X11 "MYCC=${CC}" $@

openstep:
	-mkdir lib
	cd src ; $(MAKE) -f Makefile.X11 "MYCC=${CC}" $@
	cd src-glu ; $(MAKE) -f Makefile.X11 "MYCC=${CC}" $@

os2-x11:
	if not EXIST .\lib md lib
	touch src/depend
	touch src-glu/depend
	if exist src-glut touch src-glut/depend
	cd src     & make -f Makefile.X11 $@
	cd src-glu & make -f Makefile.X11 $@
	if exist src-glut  cd src-glut & make -f Makefile.X11 $@
	if exist demos     cd demos    & make -f Makefile.X11 $@
	if exist xdemos    cd xdemos   & make -f Makefile.X11 $@
	if exist samples   cd samples  & make -f Makefile.X11 $@
	if exist book      cd book     & make -f Makefile.X11 $@

linux-ggi linux-386-ggi:
	-mkdir lib
	touch src/depend
	touch si-glu/depend
	if [ -d src-glut        ] ; then touch src-glut/depend ; fi
	if [ -d widgets-sgi     ] ; then touch widgets-sgi/depend ; fi
	if [ -d ggi             ] ; then touch ggi/depend      ; fi
	if [ -d src             ] ; then cd src ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d src/GGI/default ] ; then cd src/GGI/default ; $(MAKE)      ; fi
	if [ -d src/GGI/display ] ; then cd src/GGI/display ; $(MAKE)      ; fi
	if [ -d si-glu ]   ; then cd si-glu   ; $(MAKE) -f Makefile.X11 $@ ; fi
#	if [ -d src-glut ] ; then cd src-glut ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d ggi ]      ; then cd ggi      ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d ggi ]      ; then cd ggi/demos; $(MAKE)                    ; fi
	if [ -d demos ]    ; then cd demos    ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d xdemos ]   ; then cd xdemos   ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d samples ]  ; then cd samples  ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d book ]     ; then cd book     ; $(MAKE) -f Makefile.X11 $@ ; fi
	if [ -d widgets-sgi ] ; then cd widgets-sgi; $(MAKE) -f Makefile.X11 $@ ; fi

# if you change GGI_DEST please change it in ggimesa.conf, too.
DESTDIR=/usr/local
GGI_DEST=lib/ggi/mesa

linux-ggi-install linux-386-ggi-install:
	install -d $(DESTDIR)/$(GGI_DEST)/default $(DESTDIR)/$(GGI_DEST)/display $(DESTDIR)/etc/ggi
	install -m 0755 src/GGI/default/*.so $(DESTDIR)/$(GGI_DEST)/default
	install -m 0755 src/GGI/display/*.so $(DESTDIR)/$(GGI_DEST)/display
	install -m 0644 src/GGI/ggimesa.conf $(DESTDIR)/etc/ggi
#	if [ -z "`grep ggimesa $(DESTDIR)/etc/ggi/libggi.conf`" ]; then \
#	echo ".include $(DESTDIR)/etc/ggi/ggimesa.conf" >> $(DESTDIR)/etc/ggi/libggi.conf ; \
#	fi

linux-osmesa16:
	-mkdir lib
	if [ -d src ] ; then touch src/depend ; fi
	if [ -d src ] ; then cd src ; $(MAKE) -f Makefile.OSMesa16 $@ ; fi
	

# Remove .o files, emacs backup files, etc.
clean:
	-rm -f ggi/*~ *.o
	-rm -f src/GGI/default/*~ *.so
	-rm -f src/GGI/display/*~ *.so
	-rm -f include/*~
	-rm -f include/GL/*~
	-rm -f src/*.o src/*~ src/*.a src/*/*.o src/*/*~
	-rm -f src-glu/*.o src-glu/*~ src-glu/*.a
	-rm -f si-glu/*/*.o si-glu/*/*/*.o
	-rm -f src-glut/*.o
	-rm -f demos/*.o
	-rm -f book/*.o book/*~
	-rm -f xdemos/*.o xdemos/*~
	-rm -f samples/*.o samples/*~
	-rm -f ggi/*.o ggi/demos/*.o ggi/*.a
	-rm -f widgets-sgi/*.o
	-rm -f widgets-mesa/*/*.o

# Remove everything that can be remade
realclean: clean
	-rm -f lib/*
	cd demos       && $(MAKE) -f Makefile.X11 realclean || true
	cd xdemos      && $(MAKE) -f Makefile.X11 realclean || true
	cd book        && $(MAKE) -f Makefile.X11 realclean || true
	cd samples     && $(MAKE) -f Makefile.X11 realclean || true
	cd ggi/demos   && $(MAKE) -f Makefile.X11 realclean || true
	cd src/GGI/default && $(MAKE) -f Makefile.X11 realclean || true



DIRECTORY = Mesa-3.5
LIB_NAME = MesaLib-3.5
DEMO_NAME = MesaDemos-3.5
GLU_NAME = MesaGLU-3.5
GLUT_NAME = GLUT-3.7


LIB_FILES =	\
	$(DIRECTORY)/Makefile*						\
	$(DIRECTORY)/Make-config					\
	$(DIRECTORY)/acconfig.h						\
	$(DIRECTORY)/acinclude.m4					\
	$(DIRECTORY)/aclocal.m4						\
	$(DIRECTORY)/common_rules.make					\
	$(DIRECTORY)/conf.h.in						\
	$(DIRECTORY)/config.guess					\
	$(DIRECTORY)/config.sub						\
	$(DIRECTORY)/configure						\
	$(DIRECTORY)/configure.in					\
	$(DIRECTORY)/install-sh						\
	$(DIRECTORY)/ltconfig						\
	$(DIRECTORY)/ltmain.sh						\
	$(DIRECTORY)/missing						\
	$(DIRECTORY)/mkinstalldirs					\
	$(DIRECTORY)/stamp-h.in						\
	$(DIRECTORY)/docs/CONFIG					\
	$(DIRECTORY)/docs/CONFORM					\
	$(DIRECTORY)/docs/COPYING					\
	$(DIRECTORY)/docs/COPYRIGHT					\
	$(DIRECTORY)/docs/DEVINFO					\
	$(DIRECTORY)/docs/IAFA-PACKAGE					\
	$(DIRECTORY)/docs/INSTALL					\
	$(DIRECTORY)/docs/INSTALL.GNU					\
	$(DIRECTORY)/docs/README					\
	$(DIRECTORY)/docs/README.*					\
	$(DIRECTORY)/docs/RELNOTES*					\
	$(DIRECTORY)/docs/VERSIONS					\
	$(DIRECTORY)/docs/*.spec					\
	$(DIRECTORY)/bin/README						\
	$(DIRECTORY)/bin/mklib*						\
	$(DIRECTORY)/descrip.mms					\
	$(DIRECTORY)/mms-config						\
	$(DIRECTORY)/m4/*.m4						\
	$(DIRECTORY)/xlib.opt						\
	$(DIRECTORY)/mesawin32.mak					\
	$(DIRECTORY)/include/GL/internal/glcore.h			\
	$(DIRECTORY)/include/GL/Makefile.in				\
	$(DIRECTORY)/include/GL/Makefile.am				\
	$(DIRECTORY)/include/GL/dosmesa.h				\
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
	$(DIRECTORY)/include/GL/wmesa.h					\
	$(DIRECTORY)/include/GL/xmesa.h					\
	$(DIRECTORY)/include/GL/xmesa_x.h				\
	$(DIRECTORY)/include/GL/xmesa_xf86.h				\
	$(DIRECTORY)/include/GLView.h					\
	$(DIRECTORY)/include/Makefile.in				\
	$(DIRECTORY)/include/Makefile.am				\
	$(DIRECTORY)/src/Makefile*					\
	$(DIRECTORY)/src/libGL_la_SOURCES				\
	$(DIRECTORY)/src/descrip.mms					\
	$(DIRECTORY)/src/mms_depend					\
	$(DIRECTORY)/src/mesa.conf					\
	$(DIRECTORY)/src/*.def						\
	$(DIRECTORY)/src/depend						\
	$(DIRECTORY)/src/*.[chS]					\
	$(DIRECTORY)/src/array_cache/*.[ch]				\
	$(DIRECTORY)/src/array_cache/Makefile*				\
	$(DIRECTORY)/src/array_cache/libMesaAC_la_SOURCES		\
	$(DIRECTORY)/src/math/*.[ch]					\
	$(DIRECTORY)/src/math/Makefile*					\
	$(DIRECTORY)/src/swrast/*.[ch]					\
	$(DIRECTORY)/src/swrast/Makefile*				\
	$(DIRECTORY)/src/swrast/libMesaSwrast_la_SOURCES		\
	$(DIRECTORY)/src/swrast_setup/*.[ch]				\
	$(DIRECTORY)/src/swrast_setup/Makefile*				\
	$(DIRECTORY)/src/tnl/*.[ch]					\
	$(DIRECTORY)/src/tnl/Makefile*					\
	$(DIRECTORY)/src/tnl/libMesaTnl_la_SOURCES			\
	$(DIRECTORY)/src/BeOS/*.cpp					\
	$(DIRECTORY)/src/FX/Makefile.am					\
	$(DIRECTORY)/src/FX/Makefile.in					\
	$(DIRECTORY)/src/FX/libMesaFX_la_SOURCES			\
	$(DIRECTORY)/src/FX/*.[ch]					\
	$(DIRECTORY)/src/FX/*.def					\
	$(DIRECTORY)/src/FX/X86/Makefile.am				\
	$(DIRECTORY)/src/FX/X86/Makefile.in				\
	$(DIRECTORY)/src/FX/X86/*.[Shc]					\
	$(DIRECTORY)/src/GGI/Makefile.am				\
	$(DIRECTORY)/src/GGI/Makefile.in				\
	$(DIRECTORY)/src/GGI/*.[ch]					\
	$(DIRECTORY)/src/GGI/ggimesa.conf.in				\
	$(DIRECTORY)/src/GGI/default/*.c				\
	$(DIRECTORY)/src/GGI/default/Makefile.am			\
	$(DIRECTORY)/src/GGI/default/Makefile.in			\
	$(DIRECTORY)/src/GGI/default/genkgi.conf.in			\
	$(DIRECTORY)/src/GGI/display/*.c				\
	$(DIRECTORY)/src/GGI/display/Makefile.am			\
	$(DIRECTORY)/src/GGI/display/Makefile.in			\
	$(DIRECTORY)/src/GGI/display/fbdev.conf.in			\
	$(DIRECTORY)/src/GGI/include/Makefile.am			\
	$(DIRECTORY)/src/GGI/include/Makefile.in			\
	$(DIRECTORY)/src/GGI/include/ggi/Makefile.am			\
	$(DIRECTORY)/src/GGI/include/ggi/Makefile.in			\
	$(DIRECTORY)/src/GGI/include/ggi/mesa/Makefile.am		\
	$(DIRECTORY)/src/GGI/include/ggi/mesa/Makefile.in		\
	$(DIRECTORY)/src/GGI/include/ggi/mesa/*.h			\
	$(DIRECTORY)/src/KNOWN_BUGS					\
	$(DIRECTORY)/src/OSmesa/Makefile.am				\
	$(DIRECTORY)/src/OSmesa/Makefile.in				\
	$(DIRECTORY)/src/OSmesa/*.[ch]					\
	$(DIRECTORY)/src/SPARC/*.[chS]					\
	$(DIRECTORY)/src/SPARC/Makefile.am				\
	$(DIRECTORY)/src/SPARC/Makefile.in				\
	$(DIRECTORY)/src/SVGA/Makefile.am				\
	$(DIRECTORY)/src/SVGA/Makefile.in				\
	$(DIRECTORY)/src/SVGA/*.[ch]					\
	$(DIRECTORY)/src/Trace/*.[ch]					\
	$(DIRECTORY)/src/Trace/Makefile.am				\
	$(DIRECTORY)/src/Trace/Makefile.in				\
	$(DIRECTORY)/src/Windows/*.[ch]					\
	$(DIRECTORY)/src/Windows/*.def					\
	$(DIRECTORY)/src/X/Makefile.am					\
	$(DIRECTORY)/src/X/Makefile.in					\
	$(DIRECTORY)/src/X/*.[ch]					\
	$(DIRECTORY)/src/X86/*.[ch]					\
	$(DIRECTORY)/src/X86/Makefile.am				\
	$(DIRECTORY)/src/X86/Makefile.in				\
	$(DIRECTORY)/src/X86/*.S					\
	$(DIRECTORY)/si-glu/Makefile.am					\
	$(DIRECTORY)/si-glu/Makefile.in					\
	$(DIRECTORY)/si-glu/Makefile.X11				\
	$(DIRECTORY)/si-glu/dummy.cc					\
	$(DIRECTORY)/si-glu/descrip.mms					\
	$(DIRECTORY)/si-glu/mesaglu.opt					\
	$(DIRECTORY)/si-glu/include/gluos.h				\
	$(DIRECTORY)/si-glu/include/Makefile.am				\
	$(DIRECTORY)/si-glu/include/Makefile.in				\
	$(DIRECTORY)/si-glu/libnurbs/Makefile.am			\
	$(DIRECTORY)/si-glu/libnurbs/Makefile.in			\
	$(DIRECTORY)/si-glu/libnurbs/interface/*.h			\
	$(DIRECTORY)/si-glu/libnurbs/interface/*.cc			\
	$(DIRECTORY)/si-glu/libnurbs/interface/libNIFac_la_SOURCES	\
	$(DIRECTORY)/si-glu/libnurbs/interface/Makefile.am		\
	$(DIRECTORY)/si-glu/libnurbs/interface/Makefile.in		\
	$(DIRECTORY)/si-glu/libnurbs/internals/*.h			\
	$(DIRECTORY)/si-glu/libnurbs/internals/*.cc			\
	$(DIRECTORY)/si-glu/libnurbs/internals/libNInt_la_SOURCES	\
	$(DIRECTORY)/si-glu/libnurbs/internals/Makefile.am		\
	$(DIRECTORY)/si-glu/libnurbs/internals/Makefile.in		\
	$(DIRECTORY)/si-glu/libnurbs/nurbtess/*.h			\
	$(DIRECTORY)/si-glu/libnurbs/nurbtess/*.cc			\
	$(DIRECTORY)/si-glu/libnurbs/nurbtess/libNTess_la_SOURCES	\
	$(DIRECTORY)/si-glu/libnurbs/nurbtess/Makefile.am		\
	$(DIRECTORY)/si-glu/libnurbs/nurbtess/Makefile.in		\
	$(DIRECTORY)/si-glu/libtess/README				\
	$(DIRECTORY)/si-glu/libtess/alg-outline				\
	$(DIRECTORY)/si-glu/libtess/*.[ch]				\
	$(DIRECTORY)/si-glu/libtess/libtess_la_SOURCES			\
	$(DIRECTORY)/si-glu/libtess/Makefile.am				\
	$(DIRECTORY)/si-glu/libtess/Makefile.in				\
	$(DIRECTORY)/si-glu/libutil/*.[ch]				\
	$(DIRECTORY)/si-glu/libutil/libutil_la_SOURCES			\
	$(DIRECTORY)/si-glu/libutil/Makefile.am				\
	$(DIRECTORY)/si-glu/libutil/Makefile.in				\
	$(DIRECTORY)/src-glu/README[12]					\
	$(DIRECTORY)/src-glu/Makefile*					\
	$(DIRECTORY)/src-glu/descrip.mms				\
	$(DIRECTORY)/src-glu/mms_depend					\
	$(DIRECTORY)/src-glu/*.def					\
	$(DIRECTORY)/src-glu/depend					\
	$(DIRECTORY)/src-glu/*.[ch]					\
	$(DIRECTORY)/widgets-mesa/ChangeLog				\
	$(DIRECTORY)/widgets-mesa/INSTALL				\
	$(DIRECTORY)/widgets-mesa/Makefile.in				\
	$(DIRECTORY)/widgets-mesa/README				\
	$(DIRECTORY)/widgets-mesa/TODO					\
	$(DIRECTORY)/widgets-mesa/configure				\
	$(DIRECTORY)/widgets-mesa/configure.in				\
	$(DIRECTORY)/widgets-mesa/demos/ChangeLog			\
	$(DIRECTORY)/widgets-mesa/demos/Cube				\
	$(DIRECTORY)/widgets-mesa/demos/Ed				\
	$(DIRECTORY)/widgets-mesa/demos/Makefile.in			\
	$(DIRECTORY)/widgets-mesa/demos/Mcube				\
	$(DIRECTORY)/widgets-mesa/demos/Tea				\
	$(DIRECTORY)/widgets-mesa/demos/*.[ch]				\
	$(DIRECTORY)/widgets-mesa/demos/events				\
	$(DIRECTORY)/widgets-mesa/include/GL/ChangeLog			\
	$(DIRECTORY)/widgets-mesa/include/GL/*.h			\
	$(DIRECTORY)/widgets-mesa/include/GL/Makefile.in		\
	$(DIRECTORY)/widgets-mesa/man/ChangeLog				\
	$(DIRECTORY)/widgets-mesa/man/GL*				\
	$(DIRECTORY)/widgets-mesa/man/Makefile.in			\
	$(DIRECTORY)/widgets-mesa/man/Mesa*				\
	$(DIRECTORY)/widgets-mesa/src/ChangeLog				\
	$(DIRECTORY)/widgets-mesa/src/*.c				\
	$(DIRECTORY)/widgets-mesa/src/Makefile.in			\
	$(DIRECTORY)/widgets-sgi/*.[ch]					\
	$(DIRECTORY)/widgets-sgi/Makefile*				\
	$(DIRECTORY)/widgets-sgi/README					\
	$(DIRECTORY)/util/README					\
	$(DIRECTORY)/util/Makefile.am					\
	$(DIRECTORY)/util/Makefile.in					\
	$(DIRECTORY)/util/*.[ch]					\
	$(DIRECTORY)/util/sampleMakefile				\
	$(DIRECTORY)/vms/analyze_map.com				\
	$(DIRECTORY)/vms/xlib.opt					\
	$(DIRECTORY)/vms/xlib_share.opt					\
	$(DIRECTORY)/BeOS/Makefile					\
	$(DIRECTORY)/BeOS/*.cpp

OBSOLETE_LIB_FILES = \
	$(DIRECTORY)/src/Allegro/*.[ch]					\
	$(DIRECTORY)/src/D3D/*.cpp					\
	$(DIRECTORY)/src/D3D/*.CPP					\
	$(DIRECTORY)/src/D3D/*.h					\
	$(DIRECTORY)/src/D3D/*.H					\
	$(DIRECTORY)/src/D3D/*.c					\
	$(DIRECTORY)/src/D3D/*.C					\
	$(DIRECTORY)/src/D3D/MAKEFILE					\
	$(DIRECTORY)/src/D3D/*bat					\
	$(DIRECTORY)/src/D3D/*DEF					\
	$(DIRECTORY)/src/DOS/DEPEND.DOS					\
	$(DIRECTORY)/src/DOS/*.c					\
	$(DIRECTORY)/src/S3/*.[ch]					\
	$(DIRECTORY)/src/S3/*.def					\
	$(DIRECTORY)/src/S3/*.mak					\
	$(DIRECTORY)/src/S3/*.rc					\
	$(DIRECTORY)/macos/README					\
	$(DIRECTORY)/macos/gli_api/*.h					\
	$(DIRECTORY)/macos/cglpane/CGLPane.*				\
	$(DIRECTORY)/macos/include-mac/*.h				\
	$(DIRECTORY)/macos/libraries/*.stub				\
	$(DIRECTORY)/macos/libraries/*Stub				\
	$(DIRECTORY)/macos/projects/*.mcp				\
	$(DIRECTORY)/macos/projects/*.exp				\
	$(DIRECTORY)/macos/projects/*.h					\
	$(DIRECTORY)/macos/resources/*.c				\
	$(DIRECTORY)/macos/resources/*.r				\
	$(DIRECTORY)/macos/resources/*.rsrc				\
	$(DIRECTORY)/macos/src-agl/*.exp				\
	$(DIRECTORY)/macos/src-agl/*.[ch]				\
	$(DIRECTORY)/macos/src-gli/*.[ch]				\
	$(DIRECTORY)/OpenStep



DEMO_FILES =	\
	$(DIRECTORY)/include/GL/glut.h		\
	$(DIRECTORY)/include/GL/glutf90.h	\
	$(DIRECTORY)/src-glut/Makefile*		\
	$(DIRECTORY)/src-glut/depend		\
	$(DIRECTORY)/src-glut/*def		\
	$(DIRECTORY)/src-glut/descrip.mms	\
	$(DIRECTORY)/src-glut/mms_depend	\
	$(DIRECTORY)/src-glut/*.[ch]		\
	$(DIRECTORY)/images/*			\
	$(DIRECTORY)/demos/Makefile*		\
	$(DIRECTORY)/demos/descrip.mms		\
	$(DIRECTORY)/demos/*.[ch]		\
	$(DIRECTORY)/demos/*.cxx		\
	$(DIRECTORY)/demos/*.dat		\
	$(DIRECTORY)/demos/README		\
	$(DIRECTORY)/xdemos/Makefile*		\
	$(DIRECTORY)/xdemos/descrip.mms		\
	$(DIRECTORY)/xdemos/*.[cf]		\
	$(DIRECTORY)/book/Makefile*		\
	$(DIRECTORY)/book/README		\
	$(DIRECTORY)/book/*.[ch]		\
	$(DIRECTORY)/samples/Makefile*		\
	$(DIRECTORY)/samples/README		\
	$(DIRECTORY)/samples/*.c		\
	$(DIRECTORY)/mtdemos			\
	$(DIRECTORY)/ggi

OBSOLETE_DEMO_FILES = \
	$(DIRECTORY)/include/GL/glut_h.dja	\
	$(DIRECTORY)/src-glut.dja/*		\
	$(DIRECTORY)/src-glut.beos/Makefile	\
	$(DIRECTORY)/src-glut.beos/*.cpp	\
	$(DIRECTORY)/src-glut.beos/*.h		\


SI_GLU_FILES = \
	$(DIRECTORY)/Makefile*				\
	$(DIRECTORY)/Make-config			\
	$(DIRECTORY)/bin/mklib*				\
	$(DIRECTORY)/include/GL/glu.h			\
	$(DIRECTORY)/si-glu/Makefile.X11		\
	$(DIRECTORY)/si-glu/include/gluos.h		\
	$(DIRECTORY)/si-glu/libnurbs/interface/*.h	\
	$(DIRECTORY)/si-glu/libnurbs/interface/*.cc	\
	$(DIRECTORY)/si-glu/libnurbs/internals/*.h	\
	$(DIRECTORY)/si-glu/libnurbs/internals/*.cc	\
	$(DIRECTORY)/si-glu/libnurbs/nurbstess/*.h	\
	$(DIRECTORY)/si-glu/libnurbs/nurbstess/*.cc	\
	$(DIRECTORY)/si-glu/libtess/README		\
	$(DIRECTORY)/si-glu/libtess/alg-outline		\
	$(DIRECTORY)/si-glu/libtess/*.[ch]		\
	$(DIRECTORY)/si-glu/libutil/*.[ch]

GLU_FILES = \
	$(DIRECTORY)/Makefile*			\
	$(DIRECTORY)/Make-config		\
	$(DIRECTORY)/bin/mklib*			\
	$(DIRECTORY)/include/GL/gl.h		\
	$(DIRECTORY)/include/GL/gl_mangle.h	\
	$(DIRECTORY)/include/GL/glext.h		\
	$(DIRECTORY)/include/GL/glu.h		\
	$(DIRECTORY)/include/GL/glu_mangle.h	\
	$(DIRECTORY)/src-glu/README[12]		\
	$(DIRECTORY)/src-glu/Makefile*		\
	$(DIRECTORY)/src-glu/descrip.mms	\
	$(DIRECTORY)/src-glu/mms_depend		\
	$(DIRECTORY)/src-glu/*.def		\
	$(DIRECTORY)/src-glu/depend		\
	$(DIRECTORY)/src-glu/*.[ch]

GLUT_FILES = \
	$(DIRECTORY)/Makefile*			\
	$(DIRECTORY)/Make-config		\
	$(DIRECTORY)/bin/mklib*			\
	$(DIRECTORY)/include/GL/gl.h		\
	$(DIRECTORY)/include/GL/gl_mangle.h	\
	$(DIRECTORY)/include/GL/glext.h		\
	$(DIRECTORY)/include/GL/glu.h		\
	$(DIRECTORY)/include/GL/glu_mangle.h	\
	$(DIRECTORY)/include/GL/glut.h		\
	$(DIRECTORY)/include/GL/glutf90.h	\
	$(DIRECTORY)/src-glut/Makefile*		\
	$(DIRECTORY)/src-glut/depend		\
	$(DIRECTORY)/src-glut/*def		\
	$(DIRECTORY)/src-glut/descrip.mms	\
	$(DIRECTORY)/src-glut/mms_depend	\
	$(DIRECTORY)/src-glut/*.[ch]


OBSOLETE_GLUT_FILES = \
	$(DIRECTORY)/include/GL/glut_h.dja	\
	$(DIRECTORY)/src-glut.dja/*		\
	$(DIRECTORY)/src-glut.beos/Makefile	\
	$(DIRECTORY)/src-glut.beos/*.cpp	\
	$(DIRECTORY)/src-glut.beos/*.h


lib_tar:
	cp Makefile.X11 Makefile ; \
	cd .. ; \
	tar -cvf $(LIB_NAME).tar $(LIB_FILES) ; \
	gzip $(LIB_NAME).tar ; \
	mv $(LIB_NAME).tar.gz $(DIRECTORY)

demo_tar:
	cd .. ; \
	tar -cvf $(DEMO_NAME).tar $(DEMO_FILES) ; \
	gzip $(DEMO_NAME).tar ; \
	mv $(DEMO_NAME).tar.gz $(DIRECTORY)

glu_tar:
	cp Makefile.X11 Makefile ; \
	cd .. ; \
	tar -cvf $(GLU_NAME).tar $(GLU_FILES) ; \
	gzip $(GLU_NAME).tar ; \
	mv $(GLU_NAME).tar.gz $(DIRECTORY)

glut_tar:
	cp Makefile.X11 Makefile ; \
	cd .. ; \
	tar -cvf $(GLUT_NAME).tar $(GLUT_FILES) ; \
	gzip $(GLUT_NAME).tar ; \
	mv $(GLUT_NAME).tar.gz $(DIRECTORY)


lib_zip:
	-rm $(LIB_NAME).zip ; \
	cp Makefile.X11 Makefile ; \
	cd .. ; \
	zip -r $(LIB_NAME).zip $(LIB_FILES) ; \
	mv $(LIB_NAME).zip $(DIRECTORY)

demo_zip:
	-rm $(DEMO_NAME).zip ; \
	cd .. ; \
	zip -r $(DEMO_NAME).zip $(DEMO_FILES) ; \
	mv $(DEMO_NAME).zip $(DIRECTORY)



SRC_FILES =	\
	RELNOTES		\
	src/Makefile*		\
	src/depend		\
	src/*.[chS]		\
	src/*/*.[ch]		\
	include/GL/*.h

srctar:
	tar -cvf src.tar $(SRC_FILES) ; \
	gzip src.tar

srctar.zip:
	-rm src.zip
	zip -r src.zip $(SRC_FILES) ; \
