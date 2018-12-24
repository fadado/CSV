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
    BEGIN, QUOTED, STRING, CLOSE_QUOTED, CLOSE_STRING
} state = BEGIN;

#define get(c)		(((c) = getc(stdin)) != EOF)
#define put(c)		putc((c), stdout)

static void setIObuf(void)
{
	extern int isatty(int);
	static char buf1[BUFSIZ], buf2 [BUFSIZ];

	if (!isatty(0)) setbuf(stdin, buf1);
	if (!isatty(1))	setbuf(stdout, buf2);
}

static void error(const char *msg)
{
	fprintf(stderr, "csv2txt: %s\n", msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	const char rs = '\n';
	char fs = '\t';
	register int c;

	if (argc > 1) fs = argv[1][0];

	setIObuf();

	while (get(c)) {
		if (c == '\r') continue;
		if (c == fs) error("output field separator found in input data");

		select (state) {
			when BEGIN: select(c) {
				when ' ':	also '\t': /* nop */;
				when '"':	state = QUOTED;
				when ',':	put('0'); put(fs);
				when '\n':	put('0'); put(rs);
				otherwise:	state = STRING; put(c);
			}
			when QUOTED: select(c) {
				when '"':	state = CLOSE_QUOTED;
				when '\n':	error("unexpected end of quoted string");
				otherwise:	put(c);
			}
			when CLOSE_QUOTED: select(c) {
				when ',':	put(fs); state = BEGIN;
				when '\n':	put(rs); state = BEGIN;
				otherwise:	state = QUOTED; put('"'); put(c);
			}
			when STRING: select(c) {
				when ' ':	also '\t': state = CLOSE_STRING;
				when ',':	put(fs); state = BEGIN;
				when '\n':	put(rs); state = BEGIN;
				otherwise:	put(c);
			}
			when CLOSE_STRING: select(c) {
				when ' ':	also '\t': /* nop */
				when ',':	put(fs); state = BEGIN;
				when '\n':	put(rs); state = BEGIN;
				otherwise:	error("unexpected end of string");
			}
			otherwise: error("internal error (unexpected state)");
		}
	}
	if (state != BEGIN) error("unexpected end of data");

	return EXIT_SUCCESS;
}

/*
 * vim:ts=4:sw=4
 */

