#ifndef _TILETIFF_H
#define _TILETIFF_H

#include <tiffio.h>

typedef struct {
  int w, h;
  short spp, bpp; /* samples per pixtel) */
  short bps; /* bits per sample */
  double cellsize[3];
  double topleft [6];
  int hasgeokey;
  short geokeydir[4];
  char name[256];
  int tile_width, tile_height, depth; 
  unsigned long long int data_size;
  float  *_data_float;
  unsigned short  *_data_short;
  unsigned char  *_data_byte;
//  tdata_t _data;
} IMAGE_CHUNK ;

enum ENCODER_TYPE { NONE, LZW, JPEG };
enum TILE_TYPE {IMAGE, DEM_MULTI, DEM_SINGLE };

extern int verbose;

#define  TIFFTAG_CELLSIZE  33550
#define TIFFTAG_TOPLEFT   33922
#define TIFFTAG_GEOKEYDIRECTORY   34735

int load_header( const char *filename, IMAGE_CHUNK *img);
void load_worldfile(const char *filename, IMAGE_CHUNK *img);
int load_image(const char *filename, IMAGE_CHUNK *img);
IMAGE_CHUNK *make_chunks (IMAGE_CHUNK *img, int tile_width, int tile_height);
IMAGE_CHUNK *scale_data_by_half ( IMAGE_CHUNK *img, int averate_data);
void TagExtender(TIFF *tiff);
int write_image (IMAGE_CHUNK *primary);

#endif
