#include "readcsv.h"
#include "trexton.h"
#include "csv.h"
#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib/vision/image.h"


static int width = 0;
static int row = 0;
static int col = 0;
static int max_lines;


void cb_write_to_int_arr(void *s, size_t i, void *arr) {

  /* Save in texton array */
  int* int_arr = (int*) arr;
  
  printf("row is: %d, col is: %d pos is %d\n", row, col, row * width + col);
  fflush(stdout);
  if (max_lines > 0)
    int_arr[row * width + col] = atoi(s);
  col++;
}

void cb_write_to_double_arr(void *s, size_t i, void *arr) {

  /* Save in texton array */
  double* double_arr = (double*) arr;
  if (max_lines > 0)
    double_arr[row * width + col] = atof(s);
  col++;
}


void cb_end_of_line(int c, void *outfile) {

  /* Set row and column counter */
  max_lines--;
  col = 0;
  row++;
}

uint8_t read_csv_into_array(void *array, char *filename, void (*cb)(void *, size_t, void *)) {
  
  /* Reset counter variables */
  col = 0;
  row = 0;

  char buf[1024];
  struct csv_parser p;
  
  size_t i;
  char c;
  csv_init(&p, 0);

  /* Read input CSV file */
  FILE *infile;
  printf("%s", filename);
  infile = fopen(filename, "rb");

  /* Check if CSV file exists */
  if (infile == NULL) {
    fprintf(stderr, "Failed to open file %s: %s\n", filename, strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* Read CSV into buffer */
  while ((i = fread(buf, 1, 1024, infile)) > 0 && max_lines > 0) {
    if (csv_parse(&p, buf, i, *cb, cb_end_of_line, array) != i) {
      fprintf(stderr, "Error: %s\n", csv_strerror(csv_error(&p)));
      fclose(infile);
      exit(EXIT_FAILURE);
    }
  }

  csv_fini(&p, cb, cb_end_of_line, array);
  csv_free(&p);
  fclose(infile);

  return 0;
}


uint8_t read_textons_from_csv(double *textons, char *filename) {
  
  printf("\n%s\n", filename);
  fflush(stdout); // Prints to screen or whatever your standard out is
  max_lines = NUM_TEXTONS;
  width = TOTAL_PATCH_SIZE;
  uint8_t r = read_csv_into_array(textons, filename, cb_write_to_double_arr);
  
  return r;


}

 uint8_t read_histograms_from_csv(int *histograms, char *filename) {
   
  printf("\n%s\n", filename);
  fflush(stdout); // Prints to screen or whatever your standard out is
  max_lines = NUM_HISTOGRAMS;
  width = NUM_TEXTONS;
  uint8_t r = read_csv_into_array(histograms, filename, cb_write_to_int_arr);

  return r;

 }
