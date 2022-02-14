#ifndef PTIF_LAYER_H
#define PTIF_LAYER_H

#include "layer.h"
#include "list.h"
#include "ptif_tile.h"
#include <tiffio.h>

class ptif_tile;

class ptif_layer: public layer {
  
  private:
    double _cellsize[3];
    double _topleft[6];
    int _mipmap_levels;
    TIFF *_tif;
    int _img_width;
    int _img_height;
    ARRAY <ptif_tile*> tile;
    int _tile_width;
    int _tile_height;
    int _tile_count;
    int _bpp;
    int _spp;
    int _bps;
   
    
    int preload;
    int topo_open(char *);
     
  public:
    ptif_layer(char *);
    void draw();
    int get_width() {return _img_width;}
    int get_height() {return _img_height;}
    int get_tile_width() {return _tile_width;}
    int get_tile_height() {return _tile_height;}
    TIFF *get_tif() { return _tif;}
    int get_levels() { return _mipmap_levels; }
    void get_cellsize(double *cs){ for(int i=0; i<3; i++) cs[i] = _cellsize[i];}
    void get_topleft(double *tl){ for(int i=0; i<6; i++) tl[i] = _topleft[i];}
    int get_spp() {return _spp;}
    int get_bpp() {return _bpp;}
    int get_bps() {return _bps;}
    void set_alpha(float alpha);

    void set_gmax(float g);
    void set_gmin(float g);
    void set_gamma(float g);
    void set_scale(float scale);
    
    
};

#endif
