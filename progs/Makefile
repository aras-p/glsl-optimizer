# progs/Makefile

TOP = ..

include $(TOP)/configs/current

SUBDIRS = $(PROGRAM_DIRS)


default: $(TOP)/configs/current
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			pushd $$dir ; \
			make ; \
			popd ; \
		fi \
	done


clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			pushd $$dir ; \
			make clean; \
			popd ; \
		fi \
	done
