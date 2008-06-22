# src/mesa/drivers/Makefile

TOP = ../../..
include $(TOP)/configs/current


default:
	@for dir in $(DRIVER_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE)) || exit 1; \
		fi \
	done


clean:
	@for dir in $(DRIVER_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) clean) || exit 1; \
		fi \
	done


install:
	@for dir in $(DRIVER_DIRS) ; do \
		if [ -d $$dir ] ; then \
			(cd $$dir && $(MAKE) install) || exit 1; \
		fi \
	done

