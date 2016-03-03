#include <stdlib.h>
#include <stdio.h>
#include <png.h>
#include <readpng.h>

#include "lib/vision/image.h"

void read_png_file(char *filename, struct image_t *img) {

  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  png_bytep *row_pointers;

  FILE *fp = fopen(filename, "rb");

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  png_read_info(png, info);

  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);

  png_destroy_read_struct(&png, &info, NULL);
  png=NULL;
  info=NULL;

  uint8_t buf[640 * 480];

  for(int y = 0; y < height; y++) {
    png_bytep row = row_pointers[y];
    
    for(int x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      
      int Y;
      Y = (6969 * px[0] + 23434 * px[1] + 2365 * px[2])/32768;
      
      buf[y * width + x] = Y;

    }

  }



  // Set the image
  img->type = IMAGE_GRAYSCALE;
  img->w = 320;
  img->h = 240;
  img->buf_idx = 0;
  img->buf_size = 640 * 480;
  img->buf = buf;

  fclose(fp);
}

/* void write_to_buffer(uint8_t *buf) { */

  
/*   for(int y = 0; y < ; y++) { */
/*     png_bytep row = row_pointers[y]; */
/*     for(int x = 0; x < width; x++) { */
/*       png_bytep px = &(row[x * 4]); */

/*       int Y; */
/*       Y = (6969 * px[0] + 23434 * px[1] + 2365 * px[2])/32768; */

/*       fprintf(fp_out, "%d", Y); */
/*       if (x != width -1) */
/* 	fprintf(fp_out, ","); */

/*       // Do something awesome for each pixel here... */
/*       //printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]); */
/*     } */

/*       fprintf(fp_out, "\n"); */
/*   } */
/* } */
