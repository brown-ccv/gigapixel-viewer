#include "ptif_layer.h"
#include <tiffio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

const ttag_t TIFFTAG_CELLSIZE = 33550;
const ttag_t TIFFTAG_TOPLEFT  = 33922;


ptif_layer::ptif_layer(char *topofile) {
  _alpha = 1.0;
  _visible = 1;
  _scale = 1.0;
  type = IMAGE;
  gamma = 1.0;
  gmax = 1.0;
  gmin = 0.0;
  preload = 0;
  _bps=_bpp=_spp = 0;
  _img_width = _img_height=_tile_width=_tile_height = 0;
  _mipmap_levels = 0;
  _tile_count = 0;
  identity_matrix(transform);
  strcpy(_name, topofile);
  topo_open(topofile);
}

int ptif_layer::topo_open (char *topofile) {
  char emsg[1024];
  int i;
  size_t npixels;
  
  _mipmap_levels=0;
  
  TIFFSetWarningHandler(NULL);
  
  _tif = TIFFOpen(topofile, "r");
  if (_tif == NULL) {
    fprintf(stderr, "Problem showing %s\n", topofile);
    exit(1);
  }
  
  if (!TIFFIsTiled(_tif)) {
    fprintf(stderr, "Not a tiled tiff!!\n");
    TIFFClose(_tif);
    return 1;
  }
  
  u_short count;
  double *dptr;  
  TIFFGetField(_tif, TIFFTAG_IMAGEWIDTH, &_img_width);
  TIFFGetField(_tif, TIFFTAG_IMAGELENGTH, &_img_height);
  TIFFGetField(_tif, TIFFTAG_BITSPERSAMPLE, &_bps);
  TIFFGetField(_tif, TIFFTAG_SAMPLESPERPIXEL, &_spp);  
  TIFFGetField(_tif, TIFFTAG_TILEWIDTH,  &_tile_width);
  TIFFGetField(_tif, TIFFTAG_TILELENGTH, &_tile_height);
  _tile_count =(_img_width/_tile_width) * (_img_height/_tile_height);
  
  TIFFGetField(_tif, TIFFTAG_CELLSIZE, &count, &dptr);
  if (count==3)
      memcpy(_cellsize, dptr, 3*sizeof(double));
  else
      _cellsize[0] = _cellsize[1] = _cellsize[2] = 0;
  if(_cellsize[1] < 0.0)
      _cellsize[1] = -_cellsize[1];  // XXX - probably shouldn't do this!        
  TIFFGetField(_tif, TIFFTAG_TOPLEFT, &count, &dptr);
  if (count==6)
      memcpy(_topleft, dptr, 6*sizeof(double));
  else
      _topleft[0] = _topleft[1] = _topleft[2] = _topleft[3] = _topleft[4] = _topleft[5] = 0;
  TIFFGetField(_tif, TIFFTAG_IMAGEWIDTH, &_img_width);

  int dircount = 0;
  do {
      dircount++;
  } while (TIFFReadDirectory(_tif));
  _mipmap_levels =dircount;
  fprintf(stderr, "Stats for %s:\n", _name);
  fprintf(stderr, "Dir levels = %d\n", _mipmap_levels);
  fprintf(stderr, "Width: %d Height: %d\n", _img_width, _img_height);
  fprintf(stderr, "BPS: %d BPP: %d SPP: %d\n", _bps, _bpp, _spp); 
  center.x = _topleft[3] + _cellsize[0] * _img_width/2;
  center.y = _topleft[4] - _cellsize[1] * _img_height/2;
  center.z = 0;
 
  for (int x = 0; x < _img_width ; x = x + _tile_width) {
    for (int y = 0; y < _img_height; y += _tile_height) {
      ptif_tile *newtile = new ptif_tile(this, x, y);
      tile += newtile;
    }
  }  
    _tile_count = tile.num();

}


void ptif_layer::draw() {
  glPushMatrix();
  glMultMatrixf(transform);
  double scale =  0.16 * 1024.0f / (_cellsize[1] * _img_height);
  glScalef(scale,scale,scale);
  glTranslatef(-center.x,-center.y,0);
  for(int i = 0; i < _tile_count; i++) {
    tile[i]->draw();
  }
  glPopMatrix();
}

void ptif_layer::set_alpha(float alpha) {
  _alpha = alpha; 
  for (int i = 0; i < tile.num(); i++) { tile[i]->set_alpha(alpha); } 
}

void ptif_layer::set_gmax(float g) {
  gmax = g; 
  for (int i = 0; i < tile.num(); i++) { tile[i]->set_gmax(g); } 
}
void ptif_layer::set_gmin(float g) {
  gmin = g; 
  for (int i = 0; i < tile.num(); i++) { tile[i]->set_gmin(g); } 
}
void ptif_layer::set_gamma(float g) {
  gamma = g; 
  for (int i = 0; i < tile.num(); i++) { tile[i]->set_gamma(g); } 
}

void ptif_layer::set_scale(float scale) { 
  _scale =scale; 
  for (int i = 0; i < tile.num(); i++) { tile[i]->set_scale(scale); } 
}
    


