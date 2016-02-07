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
#include <string.h>
#include <pthread.h>
#include <math.h>
//#include "state.h"
//#include "subsystems/abi.h"

#include "lib/v4l/v4l2.h"
#include "lib/vision/image.h"
#include "lib/encoding/jpeg.h"
#include "lib/encoding/rtp.h"
#include "udp_socket.h"

void trexton_init() {

  //gps_impl_init();

  /* Get textons -- that is the clustering centers */
  read_textons_from_csv(*textons, filename);

  #if PREDICT
  /* Read histograms */
  int i = 0;
  char histogram_filename[256];
  for (i = 0; i < NUM_CLASSES; i++) {
    /* Create filename (add .csv to class name) */
    snprintf(histogram_filename, sizeof(histogram_filename), "%s%s%s", training_data_path, classes[i], ".csv");
    read_histograms_from_csv(*histograms[i], histogram_filename);
  }
  #endif


  /* Initialize the video device */
  trexton_dev = v4l2_init(STRINGIFY(TREXTON_DEVICE), 
			TREXTON_DEVICE_SIZE, 
			TREXTON_DEVICE_BUFFERS, 
			V4L2_PIX_FMT_UYVY);
  if (trexton_dev == NULL) {
    printf("[treXton_module] Could not initialize the video device\n");
  }

  // Start the streaming on the V4L2 device
  if (!v4l2_start_capture(trexton_dev))
    printf("[treXton_module] Could not start capture of the camera\n");


  // Open udp socket
  udp_socket_create(&video_sock,
		      STRINGIFY(VIEWVIDEO_HOST), 
		      VIEWVIDEO_PORT_OUT,
		      -1, 
		      VIEWVIDEO_BROADCAST);

}

/* Main function for the texton framework. It s called with a
   frequency of 30 Hz*/
void trexton_periodic() {

  #if MEASURE_TIME
    /* clock_t start = clock(); */;
  static struct timeval t0, t1;
  gettimeofday(&t0, 0);
  #endif

  //parse_gps_trexton();

  /* Get the image from the camera */
  struct image_t img;
  v4l2_image_get(trexton_dev, &img);

  #if TREXTON_DEBUG
  /* Send JPG image over RTP/UDP */

  // Create a new JPEG image
  struct image_t img_jpeg;
  image_create(&img_jpeg, 
	       trexton_dev->w, 
	       trexton_dev->h, 
	       IMAGE_JPEG);

  jpeg_encode_image(&img, &img_jpeg, 70, FALSE);
  rtp_frame_send(&video_sock, /* UDP device */
                 &img_jpeg,
		 0, /* Format 422 */
		 70, /* Jpeg-Quality */
		 0,  /* DRI Header */
		 0); /* 90kHz time increment */
  #endif
  
  /* Calculate the texton histogram -- that is the frequency of
  characteristic image patches -- for this image */
  int texton_histogram[NUM_TEXTONS] = {0};
  get_texton_histogram(&img, texton_histogram);

  #if SAVE_HISTOGRAM
    save_histogram(texton_histogram, HISTOGRAM_PATH);
  #elif PREDICT
    uint8_t class;
    class = predict_class(texton_histogram);
      printf("predicted class is: %s\n\n", classes[class]);
  #endif

  /* Free the image */
  v4l2_image_free(trexton_dev, &img);

  #if TREXTON_DEBUG
    /* Free RTP stream image */
    image_free(&img_jpeg);
  #endif

  #if MEASURE_TIME
    /* clock_t end = clock(); */
    /* float seconds = (float) (end - start) / CLOCKS_PER_SEC; */
    /* printf("%.10f\n", seconds); */
    gettimeofday(&t1, 0);
    long elapsed = (t1.tv_sec-t0.tv_sec)*1000000 + t1.tv_usec-t0.tv_usec;

    printf("%ld ms\n", elapsed / 1000);
  #endif

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

/* Find the maximum of an integer array */
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


int main()
{

  trexton_init();

  while(1)
    trexton_periodic();
  
  return 0;
}
