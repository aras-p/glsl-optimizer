# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; make) ; \
	done


clean:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; make clean) ; \
	done
