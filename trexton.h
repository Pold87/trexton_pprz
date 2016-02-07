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
 * @file "modules/trexton/trexton.h"
 * @author Volker Strobel
 * treXton localization
 */

#ifndef TREXTON_H
#define TREXTON_H

#include <stdio.h>
#include "lib/vision/image.h"

/* The video device */
#ifndef TREXTON_DEVICE
#define TREXTON_DEVICE /dev/video0      ///< The video device
#endif

/* The video device size (width, height) */
#ifndef TREXTON_DEVICE_SIZE
//#define TREXTON_DEVICE_SIZE 320,240     ///< The video device size (width, height)
#define TREXTON_DEVICE_SIZE 1280, 720     ///< The video device size (width, height)
#endif

/* The video device buffers (the amount of V4L2 buffers) */
#ifndef TREXTON_DEVICE_BUFFERS
#define TREXTON_DEVICE_BUFFERS 15       ///< The video device buffers (the amount of V4L2 buffers)
#endif

#define TREXTON_DEBUG false

#define VIEWVIDEO_HOST 192.168.1.255
#define VIEWVIDEO_PORT_OUT 5000
#define VIEWVIDEO_BROADCAST true

/* Analysis */
#define MEASURE_TIME true

/* treXton settings */
#define PATCH_SIZE  5
#define TOTAL_PATCH_SIZE 25
#define NUM_TEXTONS 33
#define MAX_TEXTONS 300
#define MAX_POSSIBLE_DIST 50000

/* Maximum lines read from histogram CSV */
#define NUM_HISTOGRAMS 800
#define NUM_CLASSES 4
#define PREDICT true
#define SAVE_HISTOGRAM false
#define HISTOGRAM_PATH "camel.csv"


static struct v4l2_device *trexton_dev; /* The trexton camera V4L2 device */
static struct UdpSocket video_sock; /* UDP socket for sending RTP video */
/* Total patch size is width of patch times height of patch */
static uint8_t patch_size = PATCH_SIZE;
static uint8_t total_patch_size = 25; //pow(patch_size, 2);
static char *filename = "textons.csv";
static double textons[NUM_TEXTONS][TOTAL_PATCH_SIZE];
static int knn = 5;

/* TODO: see if static is necessary here */
int histograms[NUM_CLASSES][NUM_HISTOGRAMS][NUM_TEXTONS];

//static int targets[1000]; /* targets for classifier (machine learning)*/
static char *classes[] = {"firefox", "logitech", "linux", "camel"};
static char training_data_path[] = "training_data/";


void extract_one_patch(struct image_t *img, double *patch, uint8_t x, uint8_t y, uint8_t patch_size);
uint8_t label_image_patch(double *patch, double textons[][TOTAL_PATCH_SIZE]);

void get_texton_histogram(struct image_t *img, int *texton_histogram);
void make_histogram(uint8_t *texton_ids, int *texton_hist);
void save_histogram(int *texton_hist, char *filename);

uint8_t predict_class(int *texton_hist);

double euclidean_dist(double x[], double y[], int s);
double euclidean_dist_int(int x[], int y[], int s);

extern void trexton_init(void);
extern void trexton_periodic(void);

/* A marker has a distance and an ID that can be mapped to its name */
struct marker {

  int id;
  double dist;

};

#endif
