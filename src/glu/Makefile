# src/glu/Makefile

TOP = ../..

include $(TOP)/configs/current


SUBDIRS = $(GLU_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE)) ; \
	done


clean:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir ; $(MAKE) clean) ; \
	done
