#include "ptif_tile.h"
#include <tiffio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "matrix.h"
#include "Frustum.h"

const ttag_t TIFFTAG_CELLSIZE = 33550;
const ttag_t TIFFTAG_TOPLEFT  = 33922;

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

ptif_tile::ptif_tile(ptif_layer *parent, int x, int y) {
  double cs[3];
  double tl[6];
  int image_width, image_height;
  
   frustum = new Frustum();
  
  _alpha = 1.0;
  _visible = 1;
  _scale = 1.0;
  type = IMAGE;
  gamma = 1.0;
  gmax = 1.0;
  gmin = 0.0;
  _current_mipmap = 0;
  preload = 0;
  _parent = parent;
  //_offset_x = x;
  //_offset_y = y;
  
  _tif = _parent->get_tif();
  _parent->get_cellsize(cs);
  _parent->get_topleft(tl);
  _mipmap_levels = _parent->get_levels();  
  _imgwidth = _parent->get_tile_width();
  _imgheight = _parent->get_tile_height();
  image_width = _parent->get_width();
  image_height = _parent->get_height();
  _spp = _parent->get_spp();
  _bpp = _parent->get_bpp();
  _bps = _parent->get_bps();
  
  int j = 1;
  for (int i = _mipmap_levels-1; i >= 0; i--) {
    _mipmaps[i].raster = NULL;
    _mipmaps[i].texid = -1;
    _mipmaps[i].meterspp = cs[0]*j;
    _mipmaps[i].width = _imgwidth/j;
    _mipmaps[i].height = _imgheight/j;
    _mipmaps[i].offsetx = x/j;
    _mipmaps[i].offsety =y/j;
    _mipmaps[i].depth = (_mipmap_levels-i)-1;
    j = j*2;
  }
  
  _upper_left[0] = tl[3] + x*cs[0];
  _upper_left[1] = tl[4] - y*cs[1];
  _lower_right[0] = _upper_left[0] + _imgwidth*cs[0];
  _lower_right[1] = _upper_left[1] - _imgheight*cs[1];
  
  //fprintf(stderr, "UL %f %f LR: %f %f\n", _upper_left[0],_upper_left[1], _lower_right[0], _lower_right[1]);
  
  center.x = (_upper_left[0] + _lower_right[0])/2.0;
  center.y = (_upper_left[1] + _lower_right[1])/2.0;
  center.z = 0;
}


extern float rotation_matrix[16];
extern float SCALE;
extern int globalW;

void ptif_tile::set_preload(int p) {
  preload = p;
}

int ptif_tile::load_ready() {
  if (preload) {
    return 1;
  }
  
  return(0);
}

void ptif_tile::load() {
  if (preload) {
    int newload = preload;
    preload = 0;
    //fprintf(stderr, "Loading %s\n", mipmaps[newload].name);
    load_raster(newload);
  }
}

int ptif_tile::load_raster(int layer_number) {
  TIFF *tif;
  char emsg[1024];
  char topofile[256];
  int i;
  TIFFRGBAImage img;
  size_t npixels;

  //fprintf(stderr, "Loading raster %d\n", layer_number);
  if (_mipmaps[layer_number].raster == NULL) {
    if (_tif) {
      if (_bps==8)
        _mipmaps[layer_number].raster   = 
         (unsigned char*)malloc(sizeof(unsigned char) 
          *_mipmaps[layer_number].width*_mipmaps[layer_number].height*_spp);
      else {
        fprintf(stderr, "Error with %s, Can only read 8 bit at the moment  %d\n",_parent->get_name(), _bps);
        return (0);
      }   
      //uint32 tilesize = TIFFTileSize(tif); 
      //fprintf(stderr, "Loading in Texture: Depth: %d, Width: %d Height: %d, SPP: %d\n",
      //  _mipmaps[layer_number].depth, _mipmaps[layer_number].width, _mipmaps[layer_number].height,
      //  _spp);
      TIFFSetDirectory(_tif, _mipmaps[layer_number].depth);
      //TIFFReadDirectory(_tif);
      //fprintf(stderr, "current tiff dir: %d\n", TIFFCurrentDirectory(_tif));
      if (TIFFReadTile(_tif, _mipmaps[layer_number].raster, _mipmaps[layer_number].offsetx, _mipmaps[layer_number].offsety, 0, 0) == -1)
        fprintf(stderr, "Error Reading tile!!!  \n");
      
      return 1;
    }
    else {
        return 0;
    }
  }
}


/* This cuntion takes the raster that was loaded in usint load_raster(), and 
  loads it into an OpenGL texture (graphics card memory) */
int ptif_tile::load_texture(int layer_number) {

  if (_mipmaps[layer_number].raster == NULL) {
    load_raster(layer_number);  
  }
  //fprintf(stderr, "Loading Layer %d\n", layer_number);
  //fprintf(stderr, "Upper left: %f, %f\nTopLeft: %f %f\n", _upper_left[0],_upper_left[1], _lower_right[0], _lower_right[1]);

  glGenTextures(1,(GLuint*)&_mipmaps[layer_number].texid);
  glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
  glBindTexture (GL_TEXTURE_2D, _mipmaps[layer_number].texid);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //fprintf(stderr, "Loading Texture %d\n", _mipmaps[layer_number].texid);
  //fprintf(stderr, "Size %d %d\n", _mipmaps[layer_number].width, _mipmaps[layer_number].height);
  
  /* Build mipmaps for the texture image.  Since we are not scaling the image
     (we easily could by calling glScalef), creating mipmaps is not really
     useful, but it is done just to show how easily creating mipmaps is. */
     switch (_spp) {
       case 4:
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _mipmaps[layer_number].width, 
           _mipmaps[layer_number].height,  0,  GL_RGBA, 
           GL_UNSIGNED_BYTE, _mipmaps[layer_number].raster); break;
       case 3:
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _mipmaps[layer_number].width, 
           _mipmaps[layer_number].height,  0,  GL_RGB, 
           GL_UNSIGNED_BYTE, _mipmaps[layer_number].raster); break;
       case 1:
         glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _mipmaps[layer_number].width, 
           _mipmaps[layer_number].height,  0,  GL_LUMINANCE, 
           GL_UNSIGNED_BYTE, _mipmaps[layer_number].raster); break;
     }         
    
//  gluBuild2DMipmaps(GL_TEXTURE_2D, 4, img.width, img.height,
//    GL_RGBA, GL_UNSIGNED_BYTE,
//    mipmaps[layer_number].raster);
  return (_mipmaps[layer_number].texid);
}

/* This unloads the openGL texture from the graphics card memory */
int ptif_tile::unload_raster(int layer_number) {
  glDeleteTextures( 1, (GLuint*)&_mipmaps[layer_number].texid );
  //fprintf(stderr, "Unloading %s layer %d unloaded\n", mipmaps[layer_number].name, layer_number);
  _mipmaps[layer_number].texid = -1;
}

/* This unloads the raster image from system memory */
int ptif_tile::free_raster(int layer_number) {
  if (_mipmaps[layer_number].raster != NULL)
    free(_mipmaps[layer_number].raster);
  _mipmaps[layer_number].raster = NULL;
  //fprintf(stderr, "Freeing %s layer %d\n", mipmaps[layer_number].name, layer_number);
  //mipmaps[layer_number].texid = -1;
}

bool ptif_tile::getSize(double &size){
	frustum->update();
	
	float corners[12];
	corners[0] = _upper_left[0]; //UL
	corners[1] = _upper_left[1];
	corners[2] = 0.0;
	corners[3] = _upper_left[0]; //LL
	corners[4] = _lower_right[1];
	corners[5] = 0.0;
	corners[6] = _lower_right[0]; //LR
	corners[7] = _lower_right[1]; 
	corners[8] = 0.0;
	corners[9] = _lower_right[0]; //UR
	corners[10] = _upper_left[1];
	corners[11] = 0.0;
	
	if(!frustum->testGeom(corners,4)) return false;
	
	multPoint(frustum->getMVP(), &corners[0]);
	multPoint(frustum->getMVP(), &corners[3]);
	multPoint(frustum->getMVP(), &corners[6]);
	multPoint(frustum->getMVP(), &corners[9]);
	
	int i;
    //normalize
	for (i = 0; i < 4 ; i ++){
		corners[3 * i] /=  corners[3 * i + 2];
		corners[3 * i + 1] /=  corners[3 * i + 2];
	}
	//scale
	for (i = 0; i < 12 ; i ++){
		corners[i] *=(0.5 * globalW);
	}

    
	//fprintf(stderr, "UL Screen %lf %lf\n", UL[0],UL[1]);
	//fprintf(stderr, "LR Screen %lf %lf\n", LR[0],LR[1]);
	//double dist = sqrt((UL[0] - LR[0])*(UL[0] - LR[0]) +
	//					(UL[1] - LR[1])*(UL[1] - LR[1]));
	
	//UL - LR
	double d1 =  sqrt((corners[0] - corners[6])*(corners[0] - corners[6]) +
						(corners[1] - corners[7])*(corners[1] - corners[7]));
	//UR - LL
	double d2 =  sqrt((corners[9] - corners[3])*(corners[9] - corners[3]) +
						(corners[10] - corners[4])*(corners[10] - corners[4]));

	
	size = sqrt(d1 * d2) ;
					
	//fprintf(stderr, "Dist %lf\n", dist);
	return true;
}

void ptif_tile::draw() {
  /* the distance is based on the distance from the eyepoint to the center of
    the image tile */
  double dist = distance(rotation_matrix[12]*-1, rotation_matrix[13]*-1, SCALE*200-rotation_matrix[14]);
  double size;
  bool visible = getSize(size);
  
  if(!visible){
	  //fprintf(stderr, "Not visible\n");
	  //fprintf(stderr, "UL %f %f LR: %f %f\n", _upper_left[0],_upper_left[1], _lower_right[0], _lower_right[1]);
	  return;
	  }
  
  //dist = dist / 100;
  _current_mipmap = 0;
  for(int i = 0; i < _mipmap_levels; i++) {
    _current_mipmap = i;
    
    if (size < _mipmaps[i].width) {
		   break;
    }
  }
  
  if (_mipmaps[_current_mipmap].texid == -1)
          load_texture(_current_mipmap);
          
  //fprintf(stderr, "MipMap: %d Distance: %f TexID\n", _current_mipmap, (dist/(_mipmaps[_current_mipmap].meterspp))/globalW);
  
 // fprintf(stderr, "%lf Size %d %d\n", _mipmaps[_current_mipmap].meterspp, _mipmaps[_current_mipmap].width, _mipmaps[_current_mipmap].height);
  
  //We do not need it anymore if it is not neighbouring to the current one
  for(int i = 1; i < _mipmap_levels; i++) {
    if(fabs(i - _current_mipmap) > 1){
      if (_mipmaps[i].raster != NULL) {
         unload_raster(i);
         free_raster(i);
      }
    }
  }
 
  glDisable(GL_LIGHTING);
  glEnable(GL_TEXTURE_2D);
  glFrontFace (GL_CW);
  
  if (_mipmaps[_current_mipmap].texid == -1) {
    fprintf(stderr, "No texture!\n");
    return;
  }
  
  //CgBindProgramEnableTextureChangeParameter(gamma, gmax, gmin, _alpha, _mipmaps[_current_mipmap].texid);

  glBindTexture(GL_TEXTURE_2D, _mipmaps[_current_mipmap].texid);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glColor4f(1, 1, 1, _alpha);
  glBegin(GL_POLYGON);
  glTexCoord2i(0,0);
  glVertex3f(_upper_left[0]*_scale, _upper_left[1]*_scale, 0);
  glTexCoord2i(1, 0);
  glVertex3f(_lower_right[0]*_scale, _upper_left[1]*_scale, 0);
  glTexCoord2i(1, 1);
  glVertex3f(_lower_right[0]*_scale, _lower_right[1]*_scale, 0);
  glTexCoord2i(0, 1);
  glVertex3f(_upper_left[0]*_scale, _lower_right[1]*_scale, 0);
  glEnd();
  
  //CgDisableTextureAndProfile();
}

