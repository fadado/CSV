/* csv2txt
 * 
 * Transforms CSV input into lines with fields delimited by
 * a single field separator (TAB by default).
 *
 * Usage: csv < input > output {FS}
 */

#include <stdlib.h>
#include <stdio.h>

#define select		switch
#define when		break; case
#define also		case
#define otherwise	break; default

static enum {
    START_FIELD,
	QUOTED_FIELD,
	UNQUOTED_FIELD,
	CLOSE_QUOTED_FIELD,
	CLOSE_UNQUOTED_FIELD
} state = START_FIELD;

#define get(c)		(((c) = getc(stdin)) != EOF)
#define put(c)		putc((c), stdout)

static void setIObuf(void)
{
	extern int isatty(int);
	static char buf1[BUFSIZ], buf2 [BUFSIZ];

	if (!isatty(0)) { setbuf(stdin, buf1); }
	if (!isatty(1))	{ setbuf(stdout, buf2); }
}

static void error(const char *msg, int nr, int nf)
{
	fprintf(stderr, "csv2txt: %s (record: %d; field: %d)\n", msg, nr, nf);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	const char RS = '\n';
	char FS = '\t';
	int nr=1, nf=1;
	register int c;

	if (argc > 1) {
		FS = argv[1][0];
	}

	setIObuf();

#define next_field	 do { put(FS); ++nf; state = START_FIELD; } while (0)
#define next_record	 do { put(RS); ++nr; state = START_FIELD; } while (0)

	while (get(c)) {
		if (c == '\r') continue;
		if (c == FS) error("output field separator found in input data",nr,nf);

		select (state) {
			when START_FIELD: select(c) {
				when ' ':	also '\t': /* nop */;
				when '"':	state = QUOTED_FIELD;
				when ',':	put('0'); next_field;
				when '\n':	put('0'); next_record;
				otherwise:	state = UNQUOTED_FIELD; put(c);
			}
			when QUOTED_FIELD: select(c) {
				when '"':	state = CLOSE_QUOTED_FIELD;
				otherwise:	put(c);
			}
			when CLOSE_QUOTED_FIELD: select(c) {
				when '"':	state = QUOTED_FIELD; put('"'); put(c);
				when ',':	next_field;
				when '\n':	next_record;
				otherwise:	error("unexpected double quote",nr,nf);
			}
			when UNQUOTED_FIELD: select(c) {
				when ' ':	also '\t': state = CLOSE_UNQUOTED_FIELD;
				when ',':	next_field;
				when '\n':	next_record;
				otherwise:	put(c);
			}
			when CLOSE_UNQUOTED_FIELD: select(c) {
				when ' ':	also '\t': /* nop */
				when ',':	next_field;
				when '\n':	next_record;
				otherwise:	error("unexpected end of string",nr,nf);
			}
			otherwise: error("internal error (unexpected state)",nr,nf);
		}
	}
	select (state) {
		when START_FIELD: put(RS);
		when CLOSE_QUOTED_FIELD: put(RS);
		when CLOSE_UNQUOTED_FIELD: put(RS);
		otherwise: error("unexpected end of data",nr,nf);
	}

	return EXIT_SUCCESS;
}

/*
 * vim:ts=4:sw=4
 */

