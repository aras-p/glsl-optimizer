# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		pushd $$dir ; \
		make ; \
		popd ; \
	done


clean:
	@for dir in $(SUBDIRS) ; do \
		pushd $$dir ; \
		make clean; \
		popd ; \
	done
