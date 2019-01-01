# Makefile

all: compliant extended

CFLAGS=-Wall -Wextra -Wpedantic

extended: csv2json.c csv2json.h
	$(CC) $(CFLAGS) $< -o $@

compliant: csv2json.c csv2json.h
	$(CC) $(CFLAGS) -DRFC4180_COMPLIANT $< -o $@

clean:
	@rm -f compliant extended tests/*/output/*

build: clean all

#########################################################################
# Tests framework
#########################################################################

.PHONY: check

define CHECK
 @for f in tests/$1/input/*; do \
 echo Testing $$f; \
 ./$1 < $$f > tests/$1/output/$${f##*/} 2>&1; \
 diff tests/$1/output/$${f##*/} tests/$1/tested/$${f##*/}; \
 done
endef

check: compliant extended
	$(call CHECK,compliant)
	$(call CHECK,extended)

#EOF
