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

/**
 * @file "modules/trexton/trexton_helpers.h"
 * @author Volker Strobel
 * treXton regression localization
 */


#include <stdio.h>
#include "lib/vision/image.h"
#include "texton_settings.h"
#include "../particle_filter/src/particle_filter.h"


/* A marker has a distance and an ID that can be mapped to its name */
struct marker {

  int id;
  double dist;

};

/* A position has a x and y coordinate */
struct position {

  double x;
  double y;
  double dist; /* Distance to the current histogram */

};

double euclidean_dist(double x[], double y[], int s);
double euclidean_dist_int(int x[], int y[], int s);

/**
*
 * Find the arg maximum of an integer array
 * @param arr An integer array
 * @param size Size of the integer array
 *
 * @return Index of the maximum of the array
 */
int arg_max(int arr[], int size);

/**
 * \brief Find the maximum of an integer array
 *
 * @param arr An integer array
 * @param size Size of the integer array
 *
 * @return Maximum of the array
 */
int max(int arr[], int size);
void extract_one_patch(struct image_t *img, double *patch, uint8_t x, uint8_t y, uint8_t patch_size);
void get_texton_histogram(struct image_t *img, int *texton_histogram, double textons[][TOTAL_PATCH_SIZE]);
void make_histogram(uint8_t *texton_ids, int *texton_hist);
void save_histogram(int *texton_hist, char *filename);
uint8_t label_image_patch(double *patch, double textons[][TOTAL_PATCH_SIZE]);
uint8_t predict_class(int *texton_hist);
struct measurement predict_position(int *texton_hist);
int measurement_comp (const struct measurement *elem1, const struct measurement *elem2);
void save_image(struct image_t *img);
