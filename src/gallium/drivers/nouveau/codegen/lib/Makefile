ENVYAS ?= envyas

all: gf100.asm.h gk104.asm.h gk110.asm.h gm107.asm.h

gf100.asm.h: %.asm.h: %.asm
	$(ENVYAS) -a -W -mnvc0 -Vnvc0 $< -o $@
gk104.asm.h: %.asm.h: %.asm
	$(ENVYAS) -a -W -mnvc0 -Vnve4 $< -o $@
gk110.asm.h: %.asm.h: %.asm
	$(ENVYAS) -a -W -mgk110 $< -o $@
gm107.asm.h: %.asm.h: %.asm
	$(ENVYAS) -a -W -mgm107 $< -o $@
