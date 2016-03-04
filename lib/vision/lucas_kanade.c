/*
 * lucas_kanade.c
 *
 *  Created on: Jan 11, 2016
 *      Author: hrvoje
 */

/*
 * Copyright (C) 2014 G. de Croon
 *               2015 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

/**
 * @file modules/computer_vision/lib/vision/lucas_kanade.c
 * @brief efficient fixed-point optical-flow calculation
 *
 * - Initial fixed-point C implementation by G. de Croon
 * - Algorithm: Lucas-Kanade by Yves Bouguet
 * - Publication: http://robots.stanford.edu/cs223b04/algo_tracking.pdf
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "lucas_kanade.h"


/**
 * Compute the optical flow of several points using the Lucas-Kanade algorithm by Yves Bouguet
 * The initial fixed-point implementation is doen by G. de Croon and is adapted by
 * Freek van Tienen for the implementation in Paparazzi.
 * @param[in] *new_img The newest grayscale image (TODO: fix YUV422 support)
 * @param[in] *old_img The old grayscale image (TODO: fix YUV422 support)
 * @param[in] *points Points to start tracking from
 * @param[in,out] points_cnt The amount of points and it returns the amount of points tracked
 * @param[in] half_window_size Half the window size (in both x and y direction) to search inside
 * @param[in] subpixel_factor The subpixel factor which calculations should be based on
 * @param[in] max_iterations Maximum amount of iterations to find the new point
 * @param[in] step_threshold The threshold at which the iterations should stop
 * @param[in] max_points The maximum amount of points to track, we skip x points and then take a point.
 * @return The vectors from the original *points in subpixels
 */
struct flow_t *opticFlowLK(struct image_t *new_img, struct image_t *old_img, struct point_t *points, uint16_t *points_cnt, uint16_t half_window_size,
		uint32_t subpixel_factor, uint8_t max_iterations, uint8_t step_threshold, uint16_t max_points, uint8_t pyramid_level) {

	//CHANGED step_threshold
	// A straightforward one-level implementation of Lucas-Kanade.
	// For all points:
	// (1) determine the subpixel neighborhood in the old image
	// (2) get the x- and y- gradients
	// (3) determine the 'G'-matrix [sum(Axx) sum(Axy); sum(Axy) sum(Ayy)], where sum is over the window
	// (4) iterate over taking steps in the image to minimize the error:
	//     [a] get the subpixel neighborhood in the new image
	//     [b] determine the image difference between the two neighborhoods
	//     [c] calculate the 'b'-vector
	//     [d] calculate the additional flow step and possibly terminate the iteration

	// Allocate some memory for returning the vectors
	struct flow_t *vectors = malloc(sizeof(struct flow_t) * max_points);

	// Allocate memory for image pyramids
	struct image_t *pyramid_old = (struct image_t *)malloc(sizeof(struct image_t) * (pyramid_level+1));
	struct image_t *pyramid_new = (struct image_t *)malloc(sizeof(struct image_t) * (pyramid_level+1));

	pyramid_build(old_img, pyramid_old, pyramid_level);
	pyramid_build(new_img, pyramid_new, pyramid_level);

	// determine patch sizes and initialize neighborhoods
	uint16_t patch_size = 2 * half_window_size + 1; //CHANGED to put pixel in center, doesnt seem to impact results much, keep in mind.
	uint32_t error_threshold = (25 * 25) * (patch_size * patch_size);
	uint16_t padded_patch_size = patch_size + 2;
	step_threshold = step_threshold*(subpixel_factor/100);
	// 3 values related to tracking window size, wont overflow

	// Create the window images
	struct image_t window_I, window_J, window_DX, window_DY, window_diff;
	image_create(&window_I, padded_patch_size, padded_patch_size, IMAGE_GRAYSCALE);
	image_create(&window_J, patch_size, patch_size, IMAGE_GRAYSCALE);
	image_create(&window_DX, patch_size, patch_size, IMAGE_GRADIENT);
	image_create(&window_DY, patch_size, patch_size, IMAGE_GRADIENT);
	image_create(&window_diff, patch_size, patch_size, IMAGE_GRADIENT);


	for (int8_t LVL = pyramid_level; LVL != -1; LVL--) {

		//printf("Pyramid level %d \n", LVL);

		uint16_t points_orig = *points_cnt;
		*points_cnt = 0;
		uint16_t new_p = 0;

		// Calculate the amount of points to skip - disabled until needed
		//float skip_points =	(points_orig > max_points) ? points_orig / max_points : 1;
		//printf("\nBased on max_points input, I'm skipping %f points(1 == none). \n", skip_points); //ADDED
		//CONC : I don't want to skip any points and result of skip_points is then appropriate

		// Go through all points
		for (uint16_t i = 0; i < max_points && i < points_orig; i++)
		{
			//uint16_t p = i ;//* skip_points; - disabled until needed

		  /* printf("points are: %d, %d\n", points[i].x, points[i].y); */
		  
			if (LVL == pyramid_level)
			{
				// Convert the point to a subpixel coordinate
				vectors[new_p].pos.x = (points[i].x * subpixel_factor) >> pyramid_level; // use bitwise shift for division
				vectors[new_p].pos.y = (points[i].y * subpixel_factor) >> pyramid_level;
				vectors[new_p].flow_x = 0;
				vectors[new_p].flow_y = 0;
				//printf("Convert point %u %u to subpix: %u, %u \n", points[i].x, points[i].y, vectors[new_p].pos.x,  vectors[new_p].pos.y);

				//printf("%u x %u, pos y %u, flowx %d, flowy %d \n", i, vectors[new_p].pos.x, vectors[new_p].pos.y,vectors[new_p].flow_x, vectors[new_p].flow_y );


				// If the pixel is outside ROI, do not track it
				if (vectors[new_p].pos.x/subpixel_factor < half_window_size || (pyramid_old[LVL].w - vectors[new_p].pos.x/subpixel_factor) < half_window_size
						|| vectors[new_p].pos.y/subpixel_factor < half_window_size || (pyramid_old[LVL].h - vectors[new_p].pos.y/subpixel_factor) < half_window_size) {
					//printf("Input feature outside ROI %u, %u ; image size: %u %u\n", vectors[new_p].pos.x/subpixel_factor, vectors[new_p].pos.y/subpixel_factor,
					//		pyramid_old[LVL].w, pyramid_old[LVL].h); //ADDED
					//CONC: consistent in not tracking edge features
					continue;
				}

			} else {
				// Convert last pyramid level flow into this pyramid level flow guess
				//printf("2nd pyr lvl: pos x %u, flow x %d \n", vectors[new_p].pos.x, vectors[new_p].flow_x);

				vectors[new_p].pos.x = vectors[i].pos.x << 1;
				vectors[new_p].pos.y = vectors[i].pos.y << 1;
				vectors[new_p].flow_x = vectors[i].flow_x << 1;
				vectors[new_p].flow_y = vectors[i].flow_y << 1;

				//printf("%u x %u, pos y %u, flowx %d, flowy %d \n", i, vectors[new_p].pos.x, vectors[new_p].pos.y,vectors[new_p].flow_x, vectors[new_p].flow_y );
				//sleep(1);

				// If the pixel is outside ROI, do not track it
				if (vectors[new_p].pos.x/subpixel_factor < half_window_size || (pyramid_old[LVL].w - vectors[new_p].pos.x/subpixel_factor) < half_window_size
						|| vectors[new_p].pos.y/subpixel_factor < half_window_size || (pyramid_old[LVL].h - vectors[new_p].pos.y/subpixel_factor) < half_window_size) {
					//printf("V2 Input feature outside ROI %u, %u \n",vectors[new_p].pos.x/subpixel_factor, vectors[new_p].pos.y); //ADDED
					//CONC: consistent in not tracking edge features
					continue;
				}
			}

			// (1) determine the subpixel neighborhood in the old image
			image_subpixel_window(&pyramid_old[LVL], &window_I, &vectors[new_p].pos, subpixel_factor);

			// (2) get the x- and y- gradients
			image_gradients(&window_I, &window_DX, &window_DY);

			// (3) determine the 'G'-matrix [sum(Axx) sum(Axy); sum(Axy) sum(Ayy)], where sum is over the window
			int32_t G[4];
			image_calculate_g(&window_DX, &window_DY, G);

			// calculate G's determinant in subpixel units:
			int32_t Det = ( G[0] * G[3] - G[1] * G[2]);//	/ subpixel_factor; // 1000 * 1000
			//printf("Max umnozak za det: %d \n", G[0]*G[3]); // milijuni za subpix = 10 000 i wind 10; za wind 31 deset mil
			//printf("Determinanta: %d \n", Det);
			//printf("Determinanta prava: %f \n",((float)G[0] * G[3] - G[1] * G[2])/ subpixel_factor);

			// Check if the determinant is bigger than 1
			if (Det < 1) {
				continue;
			}

			// (4) iterate over taking steps in the image to minimize the error:
			bool_t tracked = TRUE;

			for (uint8_t it = max_iterations; it--; ) {
				struct point_t new_point = { vectors[new_p].pos.x  + vectors[new_p].flow_x,
											 vectors[new_p].pos.y + vectors[new_p].flow_y };
				// If the pixel is outside ROI, do not track it
				if (new_point.x / subpixel_factor < half_window_size || (pyramid_new[LVL].w - new_point.x / subpixel_factor) < half_window_size
						|| new_point.y / subpixel_factor < half_window_size || (pyramid_new[LVL].h - new_point.y / subpixel_factor)< half_window_size)
				{
					tracked = FALSE;
					//printf("*New point outside ROI %u, %u; window size w %u h %u \n",
					//		new_point.x /subpixel_factor, new_point.y/subpixel_factor, pyramid_new[LVL].w, pyramid_new[LVL].h); //ADDED
					break;
				}


				//     [a] get the subpixel neighborhood in the new image
				image_subpixel_window(&pyramid_new[LVL], &window_J, &new_point, subpixel_factor);

				//     [b] determine the image difference between the two neighborhoods
				uint32_t error = image_difference(&window_I, &window_J, &window_diff);

				if (error > error_threshold && it < max_iterations / 2) {
					tracked = FALSE;
					//bprintf("*Error larger than error treshold for %d %d \n", vectors[new_p].pos.x/subpixel_factor, vectors[new_p].pos.y/subpixel_factor); //ADDED
					break;
				}

				int32_t b_x = image_multiply(&window_diff, &window_DX, NULL) / 255;
				int32_t b_y = image_multiply(&window_diff, &window_DY, NULL) / 255;


				//     [d] calculate the additional flow step and possibly terminate the iteration
				int32_t step_x = (( (int64_t) G[3] * b_x - G[1] * b_y) * subpixel_factor) / Det; //CHANGED 16 -> 32; changes made so DET is in subpixel now (less point rejection)
				int32_t step_y = (( (int64_t) G[0] * b_y - G[2] * b_x) * subpixel_factor) / Det; //CHANGED 16 -> 32; possibly change subpx factor and then this datatype to 32
				// Converting step into subpixel directly instead via Det ensures less good points rejection; memory impact?
				//printf("step x %d step y %d \n", step_x, step_y);
				//printf("is it large? %ld \n", (((int64_t)G[3] * b_x - G[1] * b_y)*subpixel_factor));


				vectors[new_p].flow_x = vectors[new_p].flow_x + step_x;
				vectors[new_p].flow_y = vectors[new_p].flow_y + step_y;
				//printf("suma flow x %d  flow y %d \n",vectors[new_p].flow_x, vectors[new_p].flow_y);

				//printf("Flows: %d %d \n", vectors[new_p].flow_x, vectors[new_p].flow_x);

				// Check if we exceeded the treshold CHANGED made this better for 0.03
				if ((abs(step_x) + abs(step_y)) < step_threshold) {
					//printf("step x %ld and step threshold %u \n", step_x, (step_threshold*(subpixel_factor/100)));
					break;
				}
			} // lucas kanade step iteration

			// If we tracked the point we update the index and the count
			if (tracked) {
			  new_p++;
				(*points_cnt)++;
			}
		} // go through all points

	} // LVL of pyramid

	// Free the images
	image_free(&window_I);
	image_free(&window_J);
	image_free(&window_DX);
	image_free(&window_DY);
	image_free(&window_diff);

	for (uint8_t i = 0; i!= pyramid_level + 1; i++){
		image_free(&pyramid_old[i]);
		image_free(&pyramid_new[i]);
	}

	// Return the vectors
	return vectors;
}

/*uint8_t show_level = pyramid_level;
uint8_t *buff_pointer = (uint8_t *)pyramid_old[show_level].buf;
printf("\nPyramid level %d \n", show_level);
for (uint16_t j = 0; j != (pyramid_old[show_level].w * pyramid_old[show_level].h); j++){
	printf("%4d ", *buff_pointer++);
	if (!((j+1)%pyramid_old[show_level].w))
		printf("\n");
}
printf("width %u, height %u \n", pyramid_old[show_level].w, pyramid_old[show_level].h);
*/
