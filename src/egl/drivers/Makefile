# src/egl/drivers/Makefile

TOP = ../../..

SUBDIRS = demo dri


default: conditional_subdirs


# depending on $DRIVER_DIRS...
conditional_subdirs:
	@echo "DRIVER_DIRS = " $(DRIVER_DIRS)
	@case "$$DRIVER_DIRS" in \
	    dri)    $(MAKE) dri_subdir || exit 1 ;; \
	    *)      ;; \
	esac;


dri_subdir:
	(cd dri ; $(MAKE)) || exit 1 ; \


demo_subdir:
	(cd demo ; $(MAKE)) || exit 1 ; \



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
