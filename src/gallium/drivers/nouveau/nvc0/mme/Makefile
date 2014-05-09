ENVYAS?=envyas
TARGETS=com9097.mme.h

all: $(TARGETS)

$(TARGETS): %.mme.h: %.mme
	$(ENVYAS) -a -w -mmacro $< -o $@
