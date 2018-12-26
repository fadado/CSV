
extern int csv2json(FILE *input, FILE *output);

typedef struct {
    char *errmsg; int nr; int nf; int nl; int nc; int st;
} CSV_ERROR;

extern CSV_ERROR *csv_error(void);
