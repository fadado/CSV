/* csv2json public interface */

typedef struct {
    char *errmsg; int nr; int nf; int nl; int nc; int st;
} CSV_ERROR;

/* Return 0 if OK, > 0 on error */
extern int csv2json(FILE *input, FILE *output);
/* Return pointer to error report */
extern CSV_ERROR *csv_error(void);
