#include "texton_settings.h"
#include "texton_helpers.h"
#include "trexton_regression.h"
#include <stdio.h>


#include "lib/v4l/v4l2.h"
#include "lib/vision/image.h"
#include "lib/encoding/jpeg.h"
#include "lib/encoding/rtp.h"
#include "udp_socket.h"

static char histogram_filename[] = "training_data/texton_histograms.csv";
static char position_filename[] = "training_data/sift_targets.csv";
static int histograms[NUM_HISTOGRAMS][NUM_TEXTONS];
static struct position all_positions[NUM_HISTOGRAMS];
static int regression_histograms[NUM_HISTOGRAMS][NUM_TEXTONS];

void trexton_init() {


  //gps_impl_init();

  /* Get textons -- that is the clustering centers */
  read_textons_from_csv(textons, texton_filename);

  #if PREDICT
    /* Read histograms */
    read_histograms_from_csv(regression_histograms, histogram_filename);
    read_positions_from_csv(all_positions, position_filename);
  #endif

    /* Debugging read poitions from csv */
    int i;
    for (i = 0; i < 10; i++) {
      printf("is is %d pos x is %f, pos y is %f\n", i, all_positions[i].x, all_positions[i].y);
    }


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

 /* Get the image from the camera */
 struct image_t img;
 v4l2_image_get(trexton_dev, &img);
 save_image(&img);
 
 /* Calculate the texton histogram -- that is the frequency of
    characteristic image patches -- for this image */
 int texton_histogram[NUM_TEXTONS] = {0};
 get_texton_histogram(&img, texton_histogram, textons);

int i;
 for (i = 0; i < NUM_TEXTONS; i++) {
   printf("%d ", texton_histogram[i]);
 }


 #if SAVE_HISTOGRAM
    save_histogram(texton_histogram, HISTOGRAM_PATH);
 #elif PREDICT
   struct position pos;
   pos = predict_position(texton_histogram);
   printf("predicted position is: x: %f, y: %f dist: %f\n\n", pos.x, pos.y, pos.dist);
 #endif

  /* Free the image */
  v4l2_image_free(trexton_dev, &img);

}


/**
 * Predict the x, y position of the UAV using the texton histogram.
 *
 * @param texton_hist The texton histogram
 *
 * @return The x, y, position of the MAV, computed by means of the input histogram
 */
struct position predict_position(int *texton_hist) {

  int h = 0; /* Histogram iterator variable */

  struct position positions[NUM_HISTOGRAMS]; 
  double dist;

  /* Compare current texton histogram to all saved histograms for
     a certain class */
  for (h = 0; h < NUM_HISTOGRAMS; h++) {
    dist = euclidean_dist_int(texton_hist, regression_histograms[h], NUM_TEXTONS);

    struct position pos;
    pos.x = all_positions[h].x;
    pos.y = all_positions[h].y;
    pos.dist = dist;
    positions[h] =  pos;
    
  }
    /* Sort distances */
    qsort(positions, sizeof(positions) / sizeof(*positions), sizeof(*positions), position_comp);

    /* int i; */
    /* for (i = 0; i < NUM_HISTOGRAMS; i++) { */
    /*   printf("Prediction is x: %f y: %f dist: %f\n", positions[i].x, positions[i].y, positions[i].dist);       */
    /* } */

    /* TODO: return average over first positions for accurate regression */
  return positions[0];
}


int main()
{

  trexton_init();

  while(1)
    trexton_periodic();

  return 0;
}
