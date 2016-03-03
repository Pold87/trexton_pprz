#include "texton_settings.h"
#include "texton_helpers.h"
#include "trexton_regression.h"
#include <stdio.h>

#include "../particle_filter/src/particle_filter.h"

#include "lib/v4l/v4l2.h"
#include "lib/vision/image.h"
#include "lib/encoding/jpeg.h"
#include "lib/encoding/rtp.h"
#include "udp_socket.h"

#include "opticflow_module.h"
#include "opticflow/edge_flow.h"
#include "opticflow/opticflow_calculator.h"

static char histogram_filename[] = "../treXton/mat_train_hists.csv";
static char histogram_filename_testset[] = "../treXton/mat_test_hists.csv";
static int histograms_testset[NUM_TEST_HISTOGRAMS][NUM_TEXTONS];
static char position_filename[] = "../image_recorder/sift_playing_mat.csv";
static int histograms[NUM_HISTOGRAMS][NUM_TEXTONS];
static struct measurement all_positions[NUM_HISTOGRAMS];
static int regression_histograms[NUM_HISTOGRAMS][NUM_TEXTONS];
static int current_test_histogram = 0;

static int use_variance = 0;
static int use_flow = 1;

/* Create  particles */
struct particle particles[N];

/* The main opticflow variables */
struct opticflow_t opticflow;                      ///< Opticflow calculations
static struct opticflow_result_t opticflow_result; ///< The opticflow result
static struct opticflow_state_t opticflow_state;   ///< State of the drone to communicate with the opticflow
static pthread_mutex_t opticflow_mutex;            ///< Mutex lock fo thread safety
static bool_t opticflow_got_result; ///< When we have an optical flow calculation

static int image_num = 0;

void trexton_init() {


  //gps_impl_init();

  /* Get textons -- that is the clustering centers */
  read_textons_from_csv(textons, texton_filename);


  // Set the opticflow state to 0
  opticflow_state.phi = 0;
  opticflow_state.theta = 0;
  opticflow_state.agl = 0;

  // Initialize the opticflow calculation
  opticflow_calc_init(&opticflow, TREXTON_DEVICE_SIZE);

  opticflow_got_result = FALSE;

  #if PREDICT
    /* Read histograms */
    read_histograms_from_csv(regression_histograms, histogram_filename);
    read_positions_from_csv(all_positions, position_filename);
  #endif

  #if EVALUATE
    read_test_histograms_from_csv(histograms_testset, histogram_filename_testset);

   /* Write header for predictions file*/
    remove("predictions.csv");
    FILE *fp_predictions;
    fp_predictions = fopen("predictions.csv", "a");
    fprintf(fp_predictions, "id,x,y,dist\n");
    fclose(fp_predictions);

  #else

    /* Initialize particles*/
    init_particles(particles);

    /* Debugging read poitions from csv */
    /* int i; */
    /* for (i = 0; i < 10; i++) { */
    /*   printf("is is %d pos x is %f, pos y is %f\n", i, all_positions[i].x, all_positions[i].y); */
    /* } */


  #if USE_WEBCAM
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
  #endif
  #endif

}

/* Main function for the texton framework. It s called with a
   frequency of 30 Hz*/
void trexton_periodic() {

  /* Calculate the texton histogram -- that is the frequency of
    characteristic image patches -- for this image */

  #if EVALUATE
    int *texton_histogram = histograms_testset[current_test_histogram];
  #else

    /* Get the image from the camera */
    struct image_t img;
    /* v4l2_image_get(trexton_dev, &img); */

    char image_path[256];
    sprintf(image_path, "../datasets/board_test/%d.png", image_num);

    /* image_path = "../datasets/board_test/1.png" */

    printf("%s", image_path);
    fflush(stdout);



    read_png_file(image_path, &img);
    /* save_image(&img); */

  /* Get image buffer */
  uint8_t *buf = img.buf;

    printf("\nImage at pos 0 is %d\n", buf[0]);

    int texton_histogram[NUM_TEXTONS] = {0};
    get_texton_histogram(&img, texton_histogram, textons);

  #endif

/* int i; */
/*  for (i = 0; i < NUM_TEXTONS; i++) { */
/*    printf("%d ", texton_histogram[i]); */
/*  } */


 #if SAVE_HISTOGRAM
    save_histogram(texton_histogram, HISTOGRAM_PATH);
 #elif PREDICT

   struct measurement pos;

   /* TreXton prediction */
   pos = predict_position(texton_histogram);
   printf("predicted position is: x: %f, y: %f dist: %f\n\n", pos.x, pos.y, pos.dist);

   /* Optical flow prediction */
   /* TODO */

   /* Particle filter update */
   struct measurement flow;

   if (use_flow) {

    // Copy the state
    pthread_mutex_lock(&opticflow_mutex);
    struct opticflow_state_t temp_state;
    memcpy(&temp_state, &opticflow_state, sizeof(struct opticflow_state_t));
    pthread_mutex_unlock(&opticflow_mutex);

    // Do the optical flow calculation
    struct opticflow_result_t temp_result;

    edgeflow_calc_frame(&opticflow, &temp_state, &img, &temp_result);
    printf("\n edgeflow result: x:%d y:%d\n", temp_result.flow_x, temp_result.flow_y);
// Copy the result if finished
    pthread_mutex_lock(&opticflow_mutex);
    memcpy(&opticflow_result, &temp_result, sizeof(struct opticflow_result_t));
    opticflow_got_result = TRUE;
    pthread_mutex_unlock(&opticflow_mutex);

    flow.x = opticflow_result.flow_x;
    flow.y = opticflow_result.flow_y;

   }

   particle_filter(particles, &pos, &flow, use_variance, use_flow);

   opticflow_got_result = FALSE;


   struct particle p_forward = weighted_average(particles, N);
   printf("Raw: %f,%f\n", pos.x, pos.y);
   printf("Particle filter: %f,%f\n", p_forward.x, p_forward.y);
   FILE *fp_predictions;
   fp_predictions = fopen("predictions.csv", "a");
   //fprintf(fp_predictions, "%d,%f,%f,%f\n", current_test_histogram, pos.x, pos.y, pos.dist);
   fclose(fp_predictions);
 #endif

   current_test_histogram++;

  #if !EVALUATE
  /* Free the image */

   #if USE_WEBCAM
  v4l2_image_free(trexton_dev, &img);
  #endif


  #endif

  image_num = image_num + 1;

}


/**
 * Predict the x, y position of the UAV using the texton histogram.
 *
 * @param texton_hist The texton histogram
 *
 * @return The x, y, position of the MAV, computed by means of the input histogram
 */
struct measurement predict_position(int *texton_hist) {

  int h = 0; /* Histogram iterator variable */

  struct measurement measurements[NUM_HISTOGRAMS];
  double dist;

  /* Compare current texton histogram to all saved histograms for
     a certain class */
  for (h = 0; h < NUM_HISTOGRAMS; h++) {
    dist = euclidean_dist_int(texton_hist, regression_histograms[h], NUM_TEXTONS);

    struct measurement z;
    z.x = all_positions[h].x;
    z.y = all_positions[h].y;
    z.dist = dist;
    measurements[h] = z;

  }
    /* Sort distances */
  /* qsort(measurements, sizeof(measurements) / sizeof(*measurements), sizeof(*measurements), measurement_comp); */

    /* int i; */
    /* for (i = 0; i < NUM_HISTOGRAMS; i++) { */
    /*   printf("Prediction is x: %f y: %f dist: %f\n", positions[i].x, positions[i].y, positions[i].dist);       */
    /* } */

    /* TODO: return average over first positions for accurate regression */

    /* int k = 7, l; */
    struct  measurement mean_pos;
    mean_pos.x = 0;
    mean_pos.y = 0;
    mean_pos.dist = 0;
    /* for (l = 0; l < k; l++) { */
    /*   mean_pos.x += measurements[l].x / k; */
    /*   mean_pos.y += measurements[l].y / k; */
    /*   mean_pos.dist += measurements[l].dist / k; */
    /* } */

  return mean_pos;
}


int main()
{

  trexton_init();

  #if EVALUATE
    int i ;
    for (i = 0; i < NUM_TEST_HISTOGRAMS; i++) {
      trexton_periodic();
    }
  #else
    while(1)
      trexton_periodic();
  #endif

  return 0;
}
