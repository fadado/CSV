/* csv2json
 *
 * Transforms CSV input into line delimited JSON.
 * Usage: csv2json < input.csv > output.json
 *
 * https://tools.ietf.org/html/rfc4180
 * https://en.wikipedia.org/wiki/JSON_streaming#Line-delimited_JSON
 */

#include <stdio.h>
#include <ctype.h>
#define NDEBUG  /* undefine to enable assertions */
#include <assert.h>

#include "csv2json.h"

/* Design considerations:
 *  - No size limits are imposed on files, lines, records or fields.
 *  - Backslash is *not* an escape.
 *  - CRLF or LF alone are both accepted as new line separators.
 *  - Control characters are ignored, except LF, HT, and CR if inside
 *    quoted fields.
 *  - Depending on EMPTY_LINES_AS_NULLS, empty fields can give JSON nulls
 *    or empty strings.
 *  - In RFC4180 compliant mode spaces around quoted fields cause an error.
 *  - In case of error the last record generated is "correct", but perhaps
 *    incomplete and/or wrong.
 */

/* Compile time options */
#define NO  0
#define YES 1

/* Compliant mode? */
#ifdef RFC4180_COMPLIANT
#   define IGNORE_BLANKS_BEFORE_FIELDS          NO
#   define IGNORE_BLANKS_AFTER_QUOTED_FIELDS    NO
#   define ALLOW_EMPTY_LINES                    NO
#else
/* Extended mode: edit to allow extensions at your will */
#   define IGNORE_BLANKS_BEFORE_FIELDS          YES
#   define IGNORE_BLANKS_AFTER_QUOTED_FIELDS    YES
#   define ALLOW_EMPTY_LINES                    YES
#endif

/* `null` or `[]` for empty lines? */
#define EMPTY_LINES_AS_NULLS    YES

/* New control structures to enhance low-level `switch` */
#define when        break; case
#define otherwise   break; default

/* Named string constants */
const char  *OPEN_BRACKET   = "[",
            *COMMA          = "\",",
            *CLOSE_BRACKET  = "\"]\n",
            *DOUBLE_QUOTE   = "\"",
            *ESCAPED_NL     = "\\n",
            *ESCAPED_CR     = "\\r",
            *ESCAPED_HT     = "\\t",
            *ESCAPED_DQ     = "\\\"",
#   if ALLOW_EMPTY_LINES
#       if EMPTY_LINES_AS_NULLS
            *EMPTY          = "null\n",
#       else   /* empty array */
            *EMPTY          = "[]\n",
#       endif
#   endif
            *NULL_FIELD1    = "[null,",
            *NULL_FIELD     = "null,",
            *NULL_FIELDn    = "null]\n";

/* FSM states */
typedef enum {
    StartRecord, StartField, Plain, Quoted, Closing
} State;

/* Store error description */
static CSV_ERROR _csv_error;

extern CSV_ERROR *csv_error(void)
{
    return &_csv_error;
}

/* CSV => LD-JSON
 * Return 0 if OK, > 0 on error.
 */
extern int csv2json(FILE *input, FILE *output)
{
    int nr=1, nf=1, nl=1, nc=0; /* Count of records, fields, lines, chars; */
    const char *errmsg=NULL;
    register int c;
    register State state;
#ifndef NDEBUG
    int old_c, old_nr, old_nf, old_nl, old_nc;
#endif

    /* pseudo inline local functions */
#   define go(s)        (state=(s))
#   define get(c)       (((c)=getc(input)) != EOF)
#   define put(c)       putc((c), output)
#   define print(s)     fputs((s), output)
#   define error(s)     errmsg=(s); goto EXIT
#   define imply(a,c)   (!(a) || (c))
#   define eof          (c==EOF)
#   define onerror      (errmsg!=NULL)

    /* Transition table for the FSM (extended mode):
     *
     *         |R      |F      |P      |Q      |C
     *  =======|=======|=======|=======|=======|=======
     *  LF     |R      |R      |R      |Q      |R
     *  ,      |F      |F      |F      |...    |F
     *  "      |...    |Q      |P      |C      |Q
     *  \      |...    |P      |P      |Q      |...
     *  HT     |...    |P      |P      |Q      |...
     *  CR     |       |       |       |Q      |
     *  ...    |=> F   |P      |P      |Q      |!
     *  EOF    |$      |$      |$      |!      |$
     *  ===============================================
     *
     *  States: R=StartRecord, F=StartField, P=Plain, Q=Quoted, C=Closing
     *  Legend: $=Stop; !=Halt; ...=default transition;
     *          => =direct transition, without reading new input;
     *             =empty cell means ignored input;
     *  Initial state: StartRecord
     */

    /* Initial state */
    go(StartRecord);
    while (get(c)) { /* exit loop at EOF or error */
#   ifndef NDEBUG
        /* helpers for loop invariants */
        old_nr=nr; old_nf=nf; old_nl=nl; old_nc=nc;
#   endif
        /* accumulated count of all characters read */
        ++nc;

        /* ignore some control characters */
        if (iscntrl(c)          /* all controls... */
                && c != '\n'    /* except LF, */
                && c != '\t'    /* except HT, */
                && (c == '\r' && state != Quoted)) /* and except CR inside quoted fields */
        {;} /* then ignore input */
        else T_T: switch (state) { /* else filter through the transition table */
            when StartRecord:
                assert(nf==1);
                switch (c) {
#               if ALLOW_EMPTY_LINES
                    when '\n':  ++nl; ++nr; print(EMPTY);
#               else
                    when '\n':  ++nl; error("unexpected empty line");
#               endif
                    when ',':   ++nf; print(NULL_FIELD1); go(StartField);
                    otherwise:  print(OPEN_BRACKET); go(StartField);
                                goto T_T; /* direct transition to StartField */
                }
            when StartField:
#           if IGNORE_BLANKS_BEFORE_FIELDS
                if (isblank(c)) {;} else
#           endif
                switch (c) {
                    when ',':   ++nf; print(NULL_FIELD);
                    when '\n':  ++nl; ++nr; nf=1; print(NULL_FIELDn); go(StartRecord);
                    when '"':   print(DOUBLE_QUOTE); go(Quoted);
                    when '\\':  print(DOUBLE_QUOTE); put(c); put(c); go(Plain); 
                    when '\t':  print(DOUBLE_QUOTE); print(ESCAPED_HT); go(Plain); 
                    otherwise:  print(DOUBLE_QUOTE); put(c); go(Plain); 
                }
            when Plain:
                switch (c) {
                    when ',':   ++nf; print(COMMA); go(StartField);
                    when '\n':  ++nl; ++nr; nf=1; print(CLOSE_BRACKET); go(StartRecord);
                    when '"':   print(ESCAPED_DQ);
                    when '\\':  put(c); put(c);
                    when '\t':  print(ESCAPED_HT);
                    otherwise:  put(c);
                }
            when Quoted:
                switch (c) {
                    when '\n':  ++nl; print(ESCAPED_NL);
                    when '\r':  print(ESCAPED_CR);
                    when '"':   go(Closing);
                    when '\\':  put(c); put(c);
                    when '\t':  print(ESCAPED_HT);
                    otherwise:  put(c);
                }
            when Closing:
#           if IGNORE_BLANKS_AFTER_QUOTED_FIELDS
                if (isblank(c)) {;} else
#           endif
                switch (c) {
                    when ',':   ++nf; print(COMMA); go(StartField);
                    when '\n':  ++nl; ++nr; nf=1; print(CLOSE_BRACKET); go(StartRecord);
                    when '"':   print(ESCAPED_DQ); go(Quoted); 
                    otherwise:  error("unexpected double quote");
                }
        } /* switch (state) */

        /* loop invariants */
        assert(old_nc+1==nc);
        assert(imply(c=='\n', old_nl+1==nl));
        assert(imply(c=='\r', state==Quoted));
        assert(imply(state!=Quoted, imply(c=='\n', old_nr+1==nr)));
        assert(imply(state!=Quoted, imply(c==',', old_nf+1==nf)));
#   ifndef NDEBUG
        old_c=c;
#   endif
    }
EXIT:
    assert(imply(onerror, nc!=0));
    assert(imply(onerror, state==Closing || (!ALLOW_EMPTY_LINES && state==StartRecord)));
    assert(imply(!eof, state==Closing));
    switch (state) {
        when StartRecord:
            assert(imply(nc!=0, old_c=='\n'));
            if (nc == 0) {
                assert(nl==1 && nf==1 && nr==1);
#           if ALLOW_EMPTY_LINES
                print(EMPTY);
#           else
                nl=nr=nf=0;
                errmsg = "unexpected empty input";
#           endif
            }
        when StartField:    print(NULL_FIELDn);
        when Plain:         print(CLOSE_BRACKET);
        when Quoted:        print(CLOSE_BRACKET);
                            errmsg = "unexpected end of field";
        when Closing:       print(CLOSE_BRACKET);
                            assert(eof != onerror);
    }
    fflush(output);

    if (!onerror)
        return 0; /* no errors */

    /* Report error */
    _csv_error.errmsg = errmsg;
    _csv_error.nr = nr;
    _csv_error.nf = nf;
    _csv_error.nl = nl;
    _csv_error.nc = nc;
    _csv_error.st = (int)state;

    return 1; /* error */
}

/* Change next `ifndef` to `ifdef` to remove the function `main` */
#ifndef IGNORE_MAIN

int main(void)
{
    /* Exit codes */
    enum { SUCCESS=0, FAILURE=1 };

    static char buffer1[BUFSIZ], buffer2[BUFSIZ];

    setbuf(stdin, buffer1);
    setbuf(stdout, buffer2);

    CSV_ERROR *e = csv_error();
    assert(e->errmsg == NULL);

    if (csv2json(stdin, stdout) != 0) {
        assert(e->errmsg != NULL);

        const char* fmt="\ncsv2json: %s (record: %d; field: %d; line: %d; character: %d)\n";
        fprintf(stderr, fmt, e->errmsg, e->nr, e->nf, e->nl, e->nc);

        return FAILURE;
    }
    return SUCCESS;
}

#endif

/*
 * vim:ts=4:sw=4:ai:et
 */
