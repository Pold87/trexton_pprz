#include <stdio.h>
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static double textons[33][25];
int col = 0;
int row = 0;

void cb1 (void *s, size_t i, void *outfile) {
  /* Save in texton array */
  textons[row][col++] = atof(s);
}

void cb2 (int c, void *outfile) {

  /* Set row and column counter */
  col = 0;
  row++;
}

int main2(int argc, char *argv[])
{
  
  char buf[1024];
  struct csv_parser p;
  
  size_t i;
  char c;
  csv_init(&p, 0);

  /* Read input CSV file */
  FILE *infile;
  infile = fopen("textons.csv", "rb");
  if (infile == NULL) {
    fprintf(stderr, "Failed to open file %s: %s\n", argv[1], strerror(errno));
    exit(EXIT_FAILURE);
  }

  while ((i = fread(buf, 1, 1024, infile)) > 0) {
    if (csv_parse(&p, buf, i, cb1, cb2, NULL) != i) {
      fprintf(stderr, "Error: %s\n", csv_strerror(csv_error(&p)));
      fclose(infile);
      exit(EXIT_FAILURE);
    }
  }

  csv_fini(&p, cb1, cb2, NULL);
  csv_free(&p);
  fclose(infile);

  int k, l;
  for (k = 0; k < 33; k++) {
    for (l = 0; l < 25; l++) {
      printf("%f ", textons[k][l]);
    }
  }

  return 0;
}
