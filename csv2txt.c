/* csv2txt
 * 
 * Transforms CSV input into lines with fields delimited by
 * a single field separator (TAB by default).
 *
 * Usage: csv < input > output {FS}
 */

#include <stdlib.h>
#include <stdio.h>

#define select      switch
#define when        break; case
#define also        case
#define otherwise   break; default

static enum {
    START=1,
    QUOTED=2,
    PLAIN=3,
    CLOSING=4
} state=START;

static void die(const char *msg, int state, int nr, int nf, int nl, int nc)
{
    fflush(stdout);
    fprintf(stderr, "\ncsv2txt: %s (state: %d; record: %d; field: %d; line: %d; character: %d)\n", msg, state, nr, nf, nl, nc);
    exit(EXIT_FAILURE);
}

static char buffer1[BUFSIZ], buffer2[BUFSIZ];

int main(int argc, char *argv[])
{
    const char RS = '\n';       /* output record separator */
    char FS = '\t';             /* output field separator */
    int nr=1, nf=1, nl=1, nc=0; /* number of records, fields, lines, chars */
    register int c;

    setbuf(stdin, buffer1);
    setbuf(stdout, buffer2);

    if (argc > 1) { /* user defined field separator */
        FS = argv[1][0];
    }

#   define get(c)       (((c) = getc(stdin)) != EOF)
#   define put(c)       putc((c), stdout)
#   define next_field   do { put(FS); ++nf; state=START; } while (0)
#   define next_record  do { put(RS); nf=1; ++nr; state=START; } while (0)
#   define error(msg)   die(msg,state,nr,nf,nl,nc);

    while (get(c)) {
        ++nc;
        if (c == '\r') {
            continue; /* ignore CR */
        }
        if (c == FS) {
            error("output field separator found in input data");
        }
        select (state) {
            when START: select(c) {
                when ',':   put('0'); next_field;   /* empty fields assumed = zero */
                when '\n':  ++nl; put('0'); next_record;
                when '"':   state=QUOTED;
                otherwise:  state=PLAIN; put(c);
            }
            when PLAIN: select(c) {
                when ',':   next_field;
                when '\n':  ++nl; next_record;
                otherwise:  put(c);
            }
            when QUOTED: select(c) {
                when '\n':  ++nl; put(c);
                when '"':   state=CLOSING;
                otherwise:  put(c);
            }
            when CLOSING: select(c) {
                when ',':   next_field;
                when '\n':  ++nl; next_record;
                when '"':   state=QUOTED; put(c); /* "" */
                otherwise:  error("unexpected double quote");
            }
            otherwise: error("internal error (unexpected state)");
        }
    }
    select (state) {
        when START: /*nop*/
        when PLAIN: put(RS);
        when CLOSING: put(RS);
        when QUOTED: error("unexpected end of quoted field");
    }

    return EXIT_SUCCESS;
}

/*
 * vim:ts=4:sw=4:ai:et
 */
