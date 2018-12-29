/* csv2json
 * 
 * Transforms CSV input into line delimited JSON.
 *
 * Usage: csv2json < input.csv > output.json
 */

#include <stdio.h>
#include <ctype.h>

#include "csv2json.h"

/* Design considerations when parsing input CSV:
    - Unexpected control characters (including CR) are ignored.
    - Empty fields give JSON empty strings ("").
    - Empty lines produce null records: `null`.
    - Empty fields produce null values.
    - In RFC4180 compliant mode spaces around quoted fields raise error.
    - CRLF or LF alone are both accepted as new line separators.
    - Backslash is *not* an escape.
 */

/* Compile time options */
#define NO 0
#define YES 1

#define RFC4180_COMPLIANT                       NO

#if RFC4180_COMPLIANT
#   define IGNORE_BLANKS_BEFORE_FIELDS          NO
#   define IGNORE_BLANKS_AFTER_QUOTED_FIELDS    NO
#   define ALLOW_EMPTY_LINES                    NO
#else
/* Edit to allow extensions at your will */
#   define IGNORE_BLANKS_BEFORE_FIELDS          YES
#   define IGNORE_BLANKS_AFTER_QUOTED_FIELDS    YES
#   define ALLOW_EMPTY_LINES                    YES
#endif
/* Choose between `[]` (NO) or `null` (YES) for empty lines */
#define EMPTY_LINES_AS_NULLS                    YES

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
            *HT             = "\\t",
            *ESCAPED_DQ     = "\\\"",
#if ALLOW_EMPTY_LINES
#   if EMPTY_LINES_AS_NULLS
            *EMPTY          = "null\n",
#   else   /* empty array */
            *EMPTY          = "[]\n",
#   endif
#endif
            *NULL_FIELD1    = "[null,",
            *NULL_FIELD     = "null,",
            *NULL_FIELDn    = "null]\n";

/* FSM states */
typedef enum {
    StartRecord, StartField, Quoted, Plain, Closing
} State;

/* Exit codes */
enum { SUCCESS=0, FAILURE=1 };

/* Store error description */
static CSV_ERROR _csv_error;

extern CSV_ERROR *csv_error(void)
{
    return &_csv_error;
}

/* CSV => LD-JSON (https://en.wikipedia.org/wiki/JSON_streaming#Line-delimited_JSON) */
extern int csv2json(FILE *input, FILE *output)
{
    int nr=1, nf=1, nl=1, nc=0; /* number of records, fields, lines, chars */
    char *errmsg = NULL;
    register int c;
    register State state;

    /* pseudo inline local functions */
#   define get(c)       (((c)=getc(input)) != EOF)
#   define put(c)       putc((c), output)
#   define print(s)     fputs((s), output)
#   define error(s)     do { errmsg=(s); goto EXIT; } while (0)
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
                select(c) {
#if ALLOW_EMPTY_LINES
                    when '\n':  ++nl; ++nr; print(EMPTY);
#else
                    when '\n':  ++nl; error("unexpected empty line");
#endif
                    when ',':   ++nf; print(NULL_FIELD1); state=StartField;
                    otherwise: print(OPEN_BRACKET); state=StartField; goto FIELD;
                }
            when StartField:
            FIELD:
#if IGNORE_BLANKS_BEFORE_FIELDS
                if (isblank(c)) continue;
#endif
                select(c) {
                    when ',':   ++nf; print(NULL_FIELD);
                    when '\n':  ++nl; print(NULL_FIELDn); state=StartRecord;
                    when '"':   print(DOUBLE_QUOTE); state=Quoted; 
                    when '\\':  print(DOUBLE_QUOTE); state=Plain; put(c); put(c);
                    when '\t':  print(DOUBLE_QUOTE); state=Plain; print(HT);
                    otherwise:  print(DOUBLE_QUOTE); state=Plain; put(c);
                }
            when Plain:
                select(c) {
                    when ',':   next_field;
                    when '\n':  ++nl; next_record;
                    when '"':   print(ESCAPED_DQ);
                    when '\\':  put(c); put(c);
                    when '\t':  print(HT);
                    otherwise:  put(c);
                }
            when Quoted:
                select(c) {
                    when '\n':  ++nl; print(NL);
                    when '"':   state=Closing;
                    when '\\':  put(c); put(c);
                    when '\t':  print(HT);
                    otherwise:  put(c);
                }
            when Closing:
#if IGNORE_BLANKS_AFTER_QUOTED_FIELDS
                if (isblank(c)) continue;
#endif
                select(c) {
                    when ',':   next_field;
                    when '\n':  ++nl; next_record;
                    when '"':   state=Quoted; print(ESCAPED_DQ);
                    otherwise:  error("unexpected double quote");
                }
            }
    }
EXIT: 
    select (state) {
        when StartRecord:
#if ALLOW_EMPTY_LINES
            if (nc == 0) print(EMPTY);
#else
            errmsg = "unexpected empty line";
#endif
        when StartField: print(NULL_FIELDn);
        when Plain: print(RS);
        when Closing: print(RS);
        when Quoted: /* ignore */;
    }
    fflush(output);
    if (errmsg == NULL) {    /* No errors */
        return SUCCESS;
    }
    /* Report error */
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

#include <assert.h>

int main(void)
{
    static char buffer1[BUFSIZ], buffer2[BUFSIZ];

    setbuf(stdin, buffer1);
    setbuf(stdout, buffer2);

    CSV_ERROR *e = csv_error();
    assert(e->errmsg == NULL);

    if (csv2json(stdin, stdout)) {
        assert(e->errmsg != NULL);

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
