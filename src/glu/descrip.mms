# Makefile for Mesa for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl

all :
# PIPE is avalailable on VMS7.0 and higher. For lower versions split the
#command in two conditional command.   JJ
	if f$search("SYS$SYSTEM:CXX$COMPILER.EXE") .nes. "" then pipe set default [.sgi] ; $(MMS)$(MMSQUALIFIERS)
	if f$search("SYS$SYSTEM:CXX$COMPILER.EXE") .eqs. "" then pipe set default [.mesa] ; $(MMS)$(MMSQUALIFIERS)
	set default [-]
