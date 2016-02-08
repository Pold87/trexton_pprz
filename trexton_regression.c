#include "texton_settings.h"
#include "texton_helpers.h"
#include "trexton_regression.h"
#include <stdio.h>


#include "lib/v4l/v4l2.h"
#include "lib/vision/image.h"
#include "lib/encoding/jpeg.h"
#include "lib/encoding/rtp.h"
#include "udp_socket.h"

/* static char histogram_filename[] = "regression_histograms.csv"; */
static char histogram_filename[] = "training_data/logitech.csv";
static int histograms[NUM_HISTOGRAMS][NUM_TEXTONS];
static char *classes[] = {"firefox", "logitech", "linux", "camel"};

void trexton_init() {


  //gps_impl_init();

  /* Get textons -- that is the clustering centers */
  read_textons_from_csv(*textons, filename);

  #if PREDICT
    /* Read histograms */
    read_histograms_from_csv(histograms, histogram_filename);
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

 /* Get the image from the camera */
 struct image_t img;
 v4l2_image_get(trexton_dev, &img);

 /* Calculate the texton histogram -- that is the frequency of
    characteristic image patches -- for this image */
 int texton_histogram[NUM_TEXTONS] = {0};
 get_texton_histogram(&img, texton_histogram);

 #if PREDICT
   struct position pos;
   pos = predict_position(texton_histogram);
   printf("predicted position is: x: %f, y: %f\n\n", pos.x, pos.y);
 #endif

  /* Free the image */
  v4l2_image_free(trexton_dev, &img);

}


int main()
{

  trexton_init();

  while(1)
    trexton_periodic();

  return 0;
}
