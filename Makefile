# Makefile

all: compliant extended

extended: csv2json.c csv2json.h
	$(CC) -Wall -DRFC4180_COMPLIANT=0 $< -o $@

compliant: csv2json.c csv2json.h
	$(CC) -Wall -DRFC4180_COMPLIANT=1 $< -o $@

clean:
	rm -f compliant extended
