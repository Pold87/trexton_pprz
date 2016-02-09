/*
 * Copyright (C) Volker Strobel
 *
 * This file is part of paparazzi
 *
 * paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "texton_helpers.h" /* Utilities for extracting textons and saving histograms */

#include <stdio.h>
#include <string.h>
#include <math.h>


#include "lib/v4l/v4l2.h"
#include "lib/vision/image.h"
#include "lib/encoding/jpeg.h"
#include "lib/encoding/rtp.h"
#include "udp_socket.h"

/* Calculate Euclidean distance between two double arrays */
double euclidean_dist(double x[], double y[], int s)
{
  double sum = 0;
  double dist;
  uint8_t i;
  for(i = 0; i < s; i++)
    {
      sum += pow((x[i] - y[i]), 2.0);
    }
      dist = sqrt(sum);
  return dist;
}


double euclidean_dist_int(int x[], int y[], int s)
{
  double sum = 0;
  double dist;
  uint8_t i;


  for(i = 0; i < s; i++)
    {
      /* printf("Regression histograms: first %d second %d\n", x[i], y[i]); */
      sum += pow((x[i] - y[i]), 2.0);
    }
      dist = sqrt(sum);

  /* printf("Euclidean dist %f \n", dist); */
  return dist;
}

/* Find the arg maximum of an array */
int arg_max(int arr[], int size){

  /* am: argmax, m: max */
  int am = 0, m = 0, i;
  for (i = 0; i < size; i++) {
    if (arr[i] > m) {
      m = arr[i];
      am = i;
    }
  }
  return am;
}

int max(int arr[], int size){

  /* am: argmax, m: max */
  int m = 0, i;
  for (i = 0; i < size; i++) {
    if (arr[i] > m) {
      m = arr[i];
    }
  }

  return m;

}


void save_image(struct image_t *img) {

  int i;

  /* Get image buffer */
  uint8_t *buf = img->buf;

  FILE *fp = fopen("myimage.csv", "w");
  
  for (i = 0; i < 1280 * 720 * 2; i += 2) {

    fprintf(fp, "%d", (int) buf[i]);
    if (i % (1280 * 2) != 0 || i == 0)
      fprintf(fp, ",");
    if (i % (1280 * 2) == 0 && i != 0)
      fprintf(fp, "\n");
  }

  fclose(fp);
}

/* Extract an image patch of size 'patch_size' (for example, 5 x 5)
   and fill 'patch' with the flattened patch */
void extract_one_patch(struct image_t *img, double *patch, uint8_t x, uint8_t y, uint8_t patch_size)
{

  uint8_t total_patch_size = pow(patch_size, 2);

  /* Get image buffer */
  uint8_t *buf = img->buf;

  /* position of x, y */
  int pos =  (x + (img->w * y)) * 2;

  /* Extract patches  */
  for (int i = 0; i < total_patch_size; i++) {

    /* Assign pixel values */
    patch[i] = buf[pos];

    /* Check, if texton extraction should continue in a new line */
    if (i % patch_size == 0 && i != 0)
      pos += ((img->h * 2) - (patch_size * 2));
    else
      pos += 2; /* +2 due to YUYV */
  }

  /* int i; */
  /* for (i = 0; i < total_patch_size; i++) { */
  /*   printf("patch is %f\n", patch[i]); */
  /* } */


}

/* Get the texton histogram of an image */
void get_texton_histogram(struct image_t *img, int *texton_histogram, double textons[][TOTAL_PATCH_SIZE]) {

    uint8_t texton_ids[MAX_TEXTONS]; /*  texton IDs */
    double patch[total_patch_size];

    uint8_t texton_id;

    /* Set random seed */
    srand (time(NULL));

    /* Extract image patches */
    int i;
    for (i = 0; i < MAX_TEXTONS; i++) {

      /* Extract random locations of patchsize x patchsize */
      uint8_t max_x = img->w - patch_size;
      uint8_t max_y = img->h - patch_size;

      uint8_t rand_x = rand();
      uint8_t rand_y = rand();

      /* int between 0 and max - 1 */
      uint8_t x = rand_x % max_x;
      uint8_t y = rand_y % max_y;

      extract_one_patch(img, patch, x, y, patch_size);

      /* Find nearest texton ...*/
      texton_id = label_image_patch(patch, textons);

      /* ... and fill the feature vector */
      texton_ids[i] = texton_id;

    }

    /* Build the histogram of textons from the texton ID array */
    make_histogram(texton_ids, texton_histogram);
    #if TREXTON_DEBUG
      printf("Texton histogram\n");
      for (i = 0; i < NUM_TEXTONS; i++)
  printf("%d ", texton_histogram[i]);
      printf("\n");
    #endif

 }

/* Create a histogram showing the frequency of values in an array */

void make_histogram(uint8_t *texton_ids, int *texton_hist) {

  int i = 0;
  for (i = 0; i < MAX_TEXTONS; i++) {
      texton_hist[texton_ids[i]] += 1;
    }
}


void save_histogram(int *texton_hist, char *filename) {
   FILE *fp = fopen(filename, "a");

   if (!fp) {
    fprintf(stderr, "[treXton - save_histogram] Can't open file.\n");
   }

   uint8_t i;
   for (i = 0; i < NUM_TEXTONS; i++) {
     if (i != NUM_TEXTONS - 1)
       fprintf(fp,"%d,", texton_hist[i]);
     else  /* No comma after the last element */
       fprintf(fp, "%d", texton_hist[i]);
   }
   fprintf(fp, "\n");
   fclose(fp);
}


/* Compare an image patch to all existing textons using Euclidean
   distance */
uint8_t label_image_patch(double *patch, double textons[][TOTAL_PATCH_SIZE]){

  /* Total patch size is width of patch times height of patch */
  uint8_t patch_size = PATCH_SIZE;

  uint8_t num_textons = NUM_TEXTONS;
  uint8_t total_patch_size = pow(patch_size, 2);

  double dist; /* Current distance between patch and texton */
  uint8_t id = 0; /* ID of closest texton */
  uint16_t min_dist = MAX_POSSIBLE_DIST; /* Minimum distance between patch and texton */

  /* Label the patch */
  int i;
  for (i = 0; i < num_textons; i++) {
    dist = euclidean_dist(patch, textons[i], total_patch_size);

    if (dist < min_dist) {
      min_dist = dist;
      id = i; /* Set id */
    }
  }
  return id;
}

int position_comp (const struct position *elem1, const struct position *elem2)
{
  double f = elem1->dist;
  double s =  elem2->dist;
  if (f > s) return 1;
  if (f < s) return -1;
  return 0;
}
