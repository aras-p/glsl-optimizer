# src/egl/Makefile

TOP = ../..

SUBDIRS = main drivers


default: subdirs


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; $(MAKE)) || exit 1 ; \
		fi \
	done


clean:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; $(MAKE) clean) ; \
		fi \
	done
