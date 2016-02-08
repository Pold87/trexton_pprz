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

/* TODO: see if static is necessary here */
int histograms[NUM_CLASSES][NUM_HISTOGRAMS][NUM_TEXTONS];

//static int targets[1000]; /* targets for classifier (machine learning)*/
static char *classes[] = {"firefox", "logitech", "linux", "camel"};
static char training_data_path[] = "training_data/";


/* A marker has a distance and an ID that can be mapped to its name */
struct marker {

  int id;
  double dist;

};



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
      sum += pow((x[i] - y[i]), 2.0);
    }
      dist = sqrt(sum);
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


/* Extract an image patch of size 'patch_size' (for example, 5 x 5)
   and fill 'patch' with the flattened patch */
void extract_one_patch(struct image_t *img, double *patch, uint8_t x, uint8_t y, uint8_t patch_size)
{

  uint8_t total_patch_size = pow(patch_size, 2);

  /* Get image buffer */
  uint8_t *buf = img->buf;

  /* position of x, y */
  int pos =  (x + (img->w * y)) * 2 - 1;

  /* Extract patches  */
  for (int i = 0; i < total_patch_size; i++) {

    /* Assign pixel values */
    patch[i] = buf[pos];

    /* Check, if texton extraction should continue in a new line */
    if ((i + 1) % patch_size == 0)
      pos += ((img->h * 2) - (patch_size * 2));
    else
      pos += 2; /* +2 due to YUYV */
  }
}

/* Get the texton histogram of an image */
 void get_texton_histogram(struct image_t *img, int *texton_histogram) {

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
    /* printf("dist is %f\n", dist); */

    if (dist < min_dist) {
      min_dist = dist;
      id = i; /* Set id */
    }
  }
  return id;
}



int marker_comp (const struct marker *elem1, const struct marker *elem2)
{
  double f = elem1->dist;
  double s =  elem2->dist;
  if (f > s) return 1;
  if (f < s) return -1;
  return 0;
}


uint8_t predict_class(int *texton_hist){

  int h = 0, class_i = 0, j = 0;

  struct marker markers[NUM_CLASSES * NUM_HISTOGRAMS];

    /* Iterate over classes (e.g. firefox, camel, ...) and save distances */
    uint8_t c = 0;
    double dist;
    double min_dists[NUM_CLASSES];

    for (c = 0; c < NUM_CLASSES; c++) {
      min_dists[c] = MAX_POSSIBLE_DIST;
      /* Compare current texton histogram to all saved histograms for
   a certain class */
      for (h = 0; h < NUM_HISTOGRAMS; h++) {
  dist = euclidean_dist_int(texton_hist, histograms[c][h], NUM_TEXTONS);

  /* Create marker */
  struct marker m;
  m.id = c;
  m.dist = dist;
  markers[j] = m;

  //printf("%s: %f\n", classes[c], dist);
  if (dist <= min_dists[c])
    min_dists[c] = dist;

  j++;
      }
    }

    /* Sort distances */
    qsort(markers, sizeof(markers) / sizeof(*markers), sizeof(*markers), marker_comp);

    printf("marker1: %d marker2: %d marker3: %d\n", markers[0].id, markers[1].id, markers[2].id);
    printf("marker1: %s %f marker2: %s %f marker3: %s %f\n", classes[markers[0].id], markers[0].dist,
     classes[markers[1].id], markers[1].dist,
     classes[markers[2].id], markers[2].dist);


    int use_only_nearest = 0;
    if (use_only_nearest) {
      double min_dist_all_classes = MAX_POSSIBLE_DIST;
      for (c = 0; c < NUM_CLASSES; c++) {

  /* Print distance for each class */
  printf("%s: %f\n", classes[c], min_dists[c]);

  if (min_dists[c] < min_dist_all_classes) {
    class_i = c;
    min_dist_all_classes = min_dists[c];
  }
      }
      printf("\n");
    } else if (knn) {

      /* Determine frequency of each neighbor */
      int k = 0;
      int nearest_markers[NUM_CLASSES] = {0};
      for (k = 0; k < knn; k++) {
  struct marker current_marker = markers[k];
  /* Increase counter for this marker */
  nearest_markers[current_marker.id]++;
      }

      /* Find arg max (that is, the neighbor with the highest
   frequency) */
      class_i = arg_max(nearest_markers, NUM_CLASSES);
    }

    return class_i;

}
