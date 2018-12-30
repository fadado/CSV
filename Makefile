# Makefile

all: compliant extended

extended: csv2json.c csv2json.h
	$(CC) -Wall -DRFC4180_COMPLIANT=0 $< -o $@

compliant: csv2json.c csv2json.h
	$(CC) -Wall -DRFC4180_COMPLIANT=1 $< -o $@

clean:
	@rm -f compliant extended tests/*/output/*

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
