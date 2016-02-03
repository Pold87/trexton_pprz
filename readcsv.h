#ifndef READ_CSV_H_
#define READ_CSV_H_

#include <stdio.h>
#include "lib/vision/image.h"

#include "trexton.h"

uint8_t read_csv_into_array(void *array, char *filename, void (*cb)(void *, size_t, void *));
uint8_t read_textons_from_csv(double *textons, char *filename);
uint8_t read_histograms_from_csv(int *histograms, char *filename);


#endif // READ_CSV_H_
