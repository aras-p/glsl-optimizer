# progs/Makefile

TOP = ..

include $(TOP)/configs/current

SUBDIRS = $(PROGRAM_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; make) || exit 1 ; \
		fi \
	done


clean:
	@for dir in $(SUBDIRS) tests ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; make clean) ; \
		fi \
	done
