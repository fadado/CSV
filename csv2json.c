/* csv2json
 * 
 * Transforms CSV input into line delimited JSON.
 *
 * Usage: csv2json < input.csv > output.json
 */

#include <stdlib.h>
#include <stdio.h>

#define select      switch
#define when        break; case
#define also        case
#define otherwise   break; default

static char buffer1[BUFSIZ], buffer2[BUFSIZ];

static void die(const char *msg, int nr, int nf, int nl, int nc)
{
    const char* fmt="\ncsv2json: %s (record: %d; field: %d; line: %d; character: %d)\n";
    fflush(stdout);
    fprintf(stderr, fmt, msg, nr, nf, nl, nc);
    exit(EXIT_FAILURE);
}

const char DOUBLE_QUOTE='\"', OPEN_BRACKET='[';
const char FS[]="\"" ",", RS[]="\"" "]" "\n", NL[]="\\n", DQ[]="\\\"";

enum State { StartRecord, StartField, Quoted, Plain, Closing };

int main(int argc, char *argv[])
{
    int nr=1, nf=1, nl=1, nc=0; /* number of records, fields, lines, chars */
    register int c;
    register enum State state;

    setbuf(stdin, buffer1);
    setbuf(stdout, buffer2);

#   define get(c)       (((c)=getc(stdin)) != EOF)
#   define put(c)       putc((c), stdout)
#   define push_back(c) ungetc((c), stdin)
#   define print(s)     fputs((s), stdout)
#   define error(msg)   die(msg,nr,nf,nl,nc)
#   define next_field   do { print(FS); ++nf; state=StartField; } while (0)
#   define next_record  do { print(RS); nf=1; nc=0; ++nr; state=StartRecord; } while (0)

    state=StartRecord;
    while (get(c)) {
        ++nc;
        if (c == '\r') { continue; /* ignore CR */ }

        select (state) {
        when StartRecord:
            put(OPEN_BRACKET);
            state=StartField;
            push_back(c);
        when StartField:
            put(DOUBLE_QUOTE);
            select(c) {
            when ',':   next_field;
            when '\n':  ++nl; next_record;
            when '"':   state=Quoted; 
            otherwise:  state=Plain; put(c); }
        when Plain:
            select(c) {
            when ',':   next_field;
            when '\n':  ++nl; next_record;
            otherwise:  put(c); }
        when Quoted:
            select(c) {
            when '\n':  ++nl; print(NL);
            when '"':   state=Closing;
            otherwise:  put(c); }
        when Closing:
            select(c) {
            when ',':   next_field;
            when '\n':  ++nl; next_record;
            when '"':   state=Quoted; print(DQ);
            otherwise:  error("unexpected double quote"); }
        }
    }
    select (state) {
        when Plain: print(RS);
        when Closing: print(RS);
        when Quoted: error("unexpected end of quoted field");
    }
    return EXIT_SUCCESS;
}

/*
 * vim:ts=4:sw=4:ai:et
 */
