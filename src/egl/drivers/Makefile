# src/egl/drivers/Makefile

TOP = ../../..
include $(TOP)/configs/current


SUBDIRS = demo dri xdri


default: conditional_subdirs


# depending on $DRIVER_DIRS...
conditional_subdirs:
	@if [ "${DRIVER_DIRS}" = "dri" ] ; then \
		$(MAKE) dri_subdirs ; \
	fi


dri_subdirs:
	@ (cd dri ; $(MAKE)) || exit 1
	@ (cd xdri ; $(MAKE)) || exit 1

demo_subdir:
	@ (cd demo ; $(MAKE)) || exit 1



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
