# Makefile for Mesa for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl

macro : 
        @ macro=""
.ifdef NOSHARE
.else
	@ if f$getsyi("HW_MODEL") .ge. 1024 then macro= "/MACRO=(SHARE=1)"
.endif
	$(MMS)$(MMSQUALIFIERS)'macro' all

all :
	if f$search("lib.dir") .eqs. "" then create/directory [.lib]
	set default [.src]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.progs.util]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.demos]
	$(MMS)$(MMSQUALIFIERS)
	set default [-.xdemos]
	$(MMS)$(MMSQUALIFIERS)
	if f$search("[-]test.DIR") .nes. "" then pipe set default [-.test] ; $(MMS)$(MMSQUALIFIERS)
