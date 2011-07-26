TOP = ../../../../../..
include $(TOP)/configs/current

CFLAGS += -Wall -Werror

### Basic defines ###
TESTS =	radeon_compiler_util_tests

TEST_SOURCES := $(TESTS:=.c)

SHARED_SOURCES =		\
	rc_test_helpers.c	\
	unit_test.c

C_SOURCES = $(SHARED_SOURCES) $(TEST_SOURCES)

INCLUDES = \
	-I. \
	-I..

COMPILER_LIB = ../../libr300.a

##### TARGETS #####

default: depend run_tests

depend: $(C_SOURCES)
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $^ 2> /dev/null

# Remove .o and backup files
clean:
	rm -f $(TESTS) depend depend.bak

$(TESTS): $(TESTS:=.o) $(SHARED_SOURCES:.c=.o) $(COMPILER_LIB)
	$(APP_CC) -o $@ $^

run_tests: $(TESTS)
	@echo "RUNNING TESTS:"
	@echo ""
	$(foreach test, $^, @./$(test))

.PHONY: $(COMPILER_LIB)
$(COMPILER_LIB):
	$(MAKE) -C ../..

##### RULES #####
.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(LIBRARY_DEFINES) $< -o $@


sinclude depend
