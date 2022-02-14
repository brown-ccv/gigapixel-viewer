#ifndef PTIF_TILE_H
#define PTIF_TILE_H

#include "layer.h"
#include "ptif_layer.h"
#include <tiffio.h>

struct PTIF_MIPMAP {
  int width;
  int height;
  int offsetx, offsety;
  double meterspp;
  int depth;
  int texid;
  unsigned char *raster;
};

class ptif_layer;
class Frustum;

class ptif_tile: public layer {
  
  private:
    double _upper_left[2];
    double _lower_right[2];
    int _mipmap_levels;
    struct PTIF_MIPMAP _mipmaps[10];
    int _current_mipmap;
    int _imgwidth;
    int _imgheight;
    //int _offset_x, _offset_y;
    int _bpp, _spp, _bps;
    TIFF *_tif;
    ptif_layer *_parent;
    Frustum * frustum;
    
    int preload;
    int unload_raster(int);
    int free_raster(int);
    int load_texture(int);
    bool getSize(double &size);
  public:
    ptif_tile(ptif_layer *parent, int x, int y);
    void draw();
    void set_preload(int);
    int load_ready();
    int load_raster(int);
    void load();

};

#endif
