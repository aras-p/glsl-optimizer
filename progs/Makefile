# progs/Makefile

TOP = ..

include $(TOP)/configs/current

SUBDIRS = $(PROGRAM_DIRS)


default: message subdirs


message:
	@echo "Making programs for" $(CONFIG_NAME)


subdirs:
	@for dir in $(SUBDIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; $(MAKE)) || exit 1 ; \
		fi \
	done

# Dummy install target
install:

clean:
	@for dir in $(SUBDIRS) tests ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir ; $(MAKE) clean) ; \
		fi \
	done
