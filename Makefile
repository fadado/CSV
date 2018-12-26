# Makefile

all: csv2json csv2txt

csv2json: csv2json.c csv2json.h
	$(CC) -Wall $< -o $@
clean:
	rm -f csv2json csv2txt initial-version
