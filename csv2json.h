/* csv2json public interface */

typedef struct {
    char *errmsg; int nr; int nf; int nl; int nc; int st;
} CSV_ERROR;

extern int csv2json(FILE *input, FILE *output);
extern CSV_ERROR *csv_error(void);
