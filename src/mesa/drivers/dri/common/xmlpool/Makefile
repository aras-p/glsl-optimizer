# Convenient makefile for managing translations.

# Prerequisites:
# - GNU gettext
# - Python

# Adding new translations
# -----------------------

# To start working on a new translation edit the POS=... line
# below. If you want to add for example a french translation, add
# fr.po.

# Then run "make po" to generate a fresh .po file from translatable
# strings in t_options.h. Now you can edit the new .po file (fr.po in
# the example above) to translate the strings. Please make sure that
# your editor encodes the file in UTF-8.

# Updating existing translations
# ------------------------------

# Run "make po" to update .po files with new translatable strings from
# t_options.h. Now you can edit the .po files you're interested
# in. Please make sure that your editor encodes the file in UTF-8.

# Updating options.h
# ------------------

# Finally run "make" to generate options.h from t_options.h with all
# translations. Now you can rebuild the drivers. Any common options
# used by the drivers will have option descriptions with the latest
# translations.

# Publishing translations
# -----------------------

# To get your translation(s) into Mesa CVS, please send me your
# <lang>.po file.

# More information:
# - info gettext

# The set of supported languages. Add languages as needed.
POS=de.po es.po nl.po fr.po sv.po

#
# Don't change anything below, unless you know what you're doing.
#
LANGS=$(POS:%.po=%)
MOS=$(POS:%.po=%/LC_MESSAGES/options.mo)
POT=xmlpool.pot

.PHONY: all clean pot po mo

all: options.h

# Only intermediate files are cleaned up. options.h is not deleted because
# it's in CVS. 
clean:
	-rm -f $(POT) *~
	-rm -rf $(LANGS)

# Default target options.h
options.h: t_options.h mo
	$(PYTHON2) $(PYTHON_FLAGS) gen_xmlpool.py $(LANGS) > options.h

# Update .mo files from the corresponding .po files.
mo:
	@for mo in $(MOS); do \
		lang=$${mo%%/*}; \
		echo "Updating $$mo from $$lang.po."; \
		mkdir -p $${mo%/*}; \
		msgfmt -o $$mo $$lang.po; \
	done

# Use this target to create or update .po files with new messages in
# driconf.py.
po: $(POS)

pot: $(POT)

# Extract message catalog from driconf.py.
$(POT): t_options.h
	xgettext -L C --from-code utf-8 -o $(POT) t_options.h

# Create or update a .po file for a specific language.
%.po: $(POT)
	@if [ -f $@ ]; then \
		echo "Merging new strings from $(POT) into $@."; \
		mv $@ $@~; \
		msgmerge -o $@ $@~ $(POT); \
	else \
		echo "Initializing $@ from $(POT)."; \
		msginit -i $(POT) -o $@~ --locale=$*; \
		sed -e 's/charset=.*\\n/charset=UTF-8\\n/' $@~ > $@; \
	fi
