/* csv2json
 * 
 * Transforms CSV input into line delimited JSON.
 *
 * Usage: csv2json < input.csv > output.json
 */

#include <stdio.h>
#include <ctype.h>

#include "csv2json.h"

/* New control structure replacing low-level `switch` */
#define select      switch
#define when        break; case
#define otherwise   break; default

/* Named string constants */
const char  *OPEN_BRACKET   = "[",
            *DOUBLE_QUOTE   = "\"",
            *FS             = "\",",
            *RS             = "\"]\n",
            *NL             = "\\n",
            *ESCAPED_DQ     = "\\\"",
            *SINGLETON      = "[\"\"]\n";

/* FSM states */
typedef enum {
    StartRecord, StartField, Quoted, Plain, Closing
} State;

/* Exit codes */
enum { SUCCESS=0, FAILURE=1 };

/* Store error description */
static CSV_ERROR _csv_error;

CSV_ERROR *csv_error(void)
{
    return &_csv_error;
}

/* CSV => LD-JSON (https://en.wikipedia.org/wiki/JSON_streaming#Line-delimited_JSON) */
int csv2json(FILE *input, FILE *output)
{
    int nr=1, nf=1, nl=1, nc=0; /* number of records, fields, lines, chars */
    char *errmsg;
    register int c;
    register State state;

    /* pseudo inline local functions */
#   define get(c)       (((c)=getc(input)) != EOF)
#   define put(c)       putc((c), output)
#   define print(s)     fputs((s), output)
#   define error(s)     do { errmsg=(s); goto ONERROR; } while (0)
#   define next_field   do { print(FS); ++nf; state=StartField; } while (0)
#   define next_record  do { print(RS); nf=1; ++nr; state=StartRecord; } while (0)

    state=StartRecord;
    while (get(c)) {
        ++nc; /* accumulated count of all characters read */

        /* ignore '\r' and other control characters; accept only '\n' and '\t' */
        if (iscntrl(c) && c != '\n' && c != '\t') continue;

        /* FSM */
        select (state) {
            when StartRecord:
                print(OPEN_BRACKET);
                state=StartField;
                goto FIELD;
            when StartField:
            FIELD:
                if (isblank(c)) continue;
                print(DOUBLE_QUOTE);
                select(c) {
                    when ',':   next_field;
                    when '\n':  ++nl; next_record;
                    when '"':   state=Quoted; 
                    when '\\':  state=Plain; put(c); put(c);
                    otherwise:  state=Plain; put(c);
                }
            when Plain:
                select(c) {
                    when ',':   next_field;
                    when '\n':  ++nl; next_record;
                    when '\\':  put(c); put(c);
                    otherwise:  put(c);
                }
            when Quoted:
                select(c) {
                    when '\n':  ++nl; print(NL);
                    when '"':   state=Closing;
                    when '\\':  put(c); put(c);
                    otherwise:  put(c);
                }
            when Closing:
                if (isblank(c)) continue;
                select(c) {
                    when ',':   next_field;
                    when '\n':  ++nl; next_record;
                    when '"':   state=Quoted; print(ESCAPED_DQ);
                    otherwise:  error("unexpected double quote");
                }
            }
    }
    select (state) {
        when StartRecord: if (nc == 0) print(SINGLETON);
        when StartField: print(DOUBLE_QUOTE); print(RS);
        when Plain: print(RS);
        when Closing: print(RS);
        when Quoted: error("unexpected end of quoted field");
    }
    fflush(output);
    return SUCCESS;

ONERROR: 
    fflush(output);
    _csv_error.errmsg = errmsg;
    _csv_error.nr = nr;
    _csv_error.nf = nf;
    _csv_error.nl = nl;
    _csv_error.nc = nc;
    _csv_error.st = (int)state;
    return FAILURE;
}

/* Delete next lines or define NO_MAIN_ENTRY_POINT to remove the function `main` */
#ifndef NO_MAIN_ENTRY_POINT

int main(void)
{
    static char buffer1[BUFSIZ], buffer2[BUFSIZ];

    setbuf(stdin, buffer1);
    setbuf(stdout, buffer2);

    if (csv2json(stdin, stdout) != SUCCESS) {
        const char* fmt="\ncsv2json: %s (record: %d; field: %d; line: %d; character: %d)\n";
        CSV_ERROR *e = csv_error();
        fprintf(stderr, fmt, e->errmsg, e->nr, e->nf, e->nl, e->nc);
        return FAILURE;
    }
    return SUCCESS;
}

#endif

/* 
 * vim:ts=4:sw=4:ai:et
 */
