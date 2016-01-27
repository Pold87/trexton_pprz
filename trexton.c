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
 * @file "modules/trexton/trexton.c"
 * @author Volker Strobel
 * 
 * This file implements a localization technique based on textons. - i.e. small characteristic image patches.
 */

#include "trexton.h"
#include "readcsv.h"
#include <limits.h>
//#include "subsystems/gps/gps_trexton.h"


#include <stdio.h>
#include <pthread.h>
#include <math.h>
//#include "state.h"
//#include "subsystems/abi.h"

#include "lib/v4l/v4l2.h"
#include "lib/vision/image.h"
#include "lib/encoding/jpeg.h"
#include "lib/encoding/rtp.h"
#include "udp_socket.h"


/* The video device */
#ifndef TREXTON_DEVICE
#define TREXTON_DEVICE /dev/video0      ///< The video device
#endif


/* The video device size (width, height) */
#ifndef TREXTON_DEVICE_SIZE
#define TREXTON_DEVICE_SIZE 320,240     ///< The video device size (width, height)
//#define TREXTON_DEVICE_SIZE 1280, 720     ///< The video device size (width, height)
#endif


/* The video device buffers (the amount of V4L2 buffers) */
#ifndef TREXTON_DEVICE_BUFFERS
#define TREXTON_DEVICE_BUFFERS 15       ///< The video device buffers (the amount of V4L2 buffers)
#endif

#define TREXTON_DEBUG true

#define VIEWVIDEO_HOST 192.168.1.255
#define VIEWVIDEO_PORT_OUT 5000
#define VIEWVIDEO_BROADCAST true

/* treXton settings */
#define PATCH_SIZE  5
#define NUM_TEXTONS 33
#define MAX_TEXTONS 5


static struct v4l2_device *trexton_dev; /* The trexton camera V4L2 device */

void trexton_init() {

  //gps_impl_init();

  /* Initialize the video device */
  trexton_dev = v4l2_init(STRINGIFY(TREXTON_DEVICE), 
			TREXTON_DEVICE_SIZE, 
			TREXTON_DEVICE_BUFFERS, 
			V4L2_PIX_FMT_UYVY);
  if (trexton_dev == NULL) {
    printf("[treXton_module] Could not initialize the video device\n");
  }
}

/* Main function for the texton framework. It s called with a
   frequency of 30 Hz*/
void trexton_periodic() {

  //parse_gps_trexton();

  /* Settings */

  /* Total patch size is width of patch times height of patch */
  uint8_t patch_size = PATCH_SIZE;
  uint8_t total_patch_size = pow(patch_size, 2);

  char *filename = "textons.csv";
  int num_textons = NUM_TEXTONS;
  int size_texton = PATCH_SIZE;

  double *textons[num_textons];

  /* Get textons -- that is the clustering centers */
  read_textons_from_csv(textons, filename, num_textons, size_texton);

  // Start the streaming on the V4L2 device
  if (!v4l2_start_capture(trexton_dev))
    printf("[treXton_module] Could not start capture of the camera\n");

  #if TREXTON_DEBUG
  // Create a new JPEG image
  struct image_t img_jpeg;
  image_create(&img_jpeg, 
	       trexton_dev->w, 
	       trexton_dev->h, 
	       IMAGE_JPEG);
  #endif

  // Open udp socket
  struct UdpSocket video_sock;
  udp_socket_create(&video_sock,
		      STRINGIFY(VIEWVIDEO_HOST), 
		      VIEWVIDEO_PORT_OUT,
		      -1, 
		      VIEWVIDEO_BROADCAST);


  /* Continuous texton extraction */
  //  while (TRUE) {

    /* Get the image from the camera */
    struct image_t img;
    v4l2_image_get(trexton_dev, &img);

    #if TREXTON_DEBUG
       jpeg_encode_image(&img, &img_jpeg, 70, FALSE);
       rtp_frame_send(&video_sock, /* UDP device */
                      &img_jpeg,
		      0, /* Format 422 */
		      70, /* Jpeg-Quality */
		      0,  /* DRI Header */
		      0); /* 90kHz time increment */
    #endif
           
    /* TODO: see if malloc is necessary */
    double *patches[MAX_TEXTONS];
    int texton_ids[MAX_TEXTONS]; /* the texton feature vector */
    double patch[total_patch_size];
    //    malloc(*patch, total_patch_size * sizeof(double));


    
    int texton_id;
    int x, y;

    srand (time(NULL));

    /* Extract image patches */
    int i;
    for (i = 0; i < MAX_TEXTONS; i++) {
      //patches[i] = malloc(total_patch_size * sizeof(double));

      /* Extract random locations */      
      int max_x = img.w - patch_size;
      int max_y = img.h - patch_size;

      int rand_x = rand();
      int rand_y = rand();

      int x = rand_x % max_x;
      int y = rand_y % max_y;

      /* Extract a 5 x 5 image patch */
      extract_one_patch(&img, patch, x, y, patch_size); 
     
      /* int j; */
      /* for (j = 0; j < total_patch_size; j++) { */
      /* 	printf("%f ", patch[j]); */
      /* } */

      texton_id = label_image_patch(patch, textons);
      texton_ids[i] = texton_id;
      printf("texton id is: %d\n", texton_id);
      patches[i] = patch;
    }
      printf("\n\n");

    /* #if TREXTON_DEBUG */
    /* /\* Print extracted image patches *\/ */
    /* for (i = 0; i < total_patch_size; i++)  */
    /*   printf("%f ", patches[7][i]); */
    /* printf("\n"); */
    /* #endif */

    // Free the images
    v4l2_image_free(trexton_dev, &img);
    //  }

 #if TREXTON_DEBUG
  image_free(&img_jpeg);
 #endif
}


void extract_one_patch(struct image_t *img, double *patch, int x, int y, int patch_size)
{
  
  uint8_t total_patch_size = pow(patch_size, 2);

  /* Get image buffer */
  uint8_t *buf = img->buf;

  /* position of x, y */
  int pos =  (x + (img->h * y)) * 2 - 1;

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

/* Calculate Euclidean distance between two double arrays */
double euclidean_dist(double x[], double y[], int s)
{
  double sum = 0;
  double dist;
  int i;
  for(i = 0; i < s; i++)
    {
      sum += pow((x[i] - y[i]), 2.0);
    }
      dist = sqrt(sum);
  return dist;
}

/* Compare an image patch to all existing textons using Euclidean
   distance */
int label_image_patch(double *patch, double *textons[]){

  /* Total patch size is width of patch times height of patch */
  uint8_t patch_size = PATCH_SIZE;

  int num_textons = NUM_TEXTONS;
  uint8_t total_patch_size = pow(patch_size, 2);

  //printf("total patch: %d\n", total_patch_size);

  double dist; /* Current distance between patch and texton */
  int id = 0; /* ID of closest texton */
  int min_dist = INT_MAX; /* Minimum distance between patch and texton */ 

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

int main(int argc, char *argv[])
{

  trexton_init();
  trexton_periodic();
  
  return 0;
}
