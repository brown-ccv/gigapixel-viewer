#include "tiletiff.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

int verbose = 0;



/* read the info on a tiff file, nbut not the actual data */

int load_header( const char *filename, IMAGE_CHUNK *img) {
  TIFF* tif = TIFFOpen(filename, "r");
  if (tif) {
    short bps[3];
    strcpy (img->name, filename);

    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &img->w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &img->h);
    TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &img->spp);
    TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps);
    img->bps = bps[0];
   
    int i;
    unsigned short count2;
    double *dptr2;
    TIFFGetField(tif, TIFFTAG_CELLSIZE, &count2, &dptr2);
     if (count2==3){
	  if(verbose > 0)fprintf(stderr, "Read TIFFTAG_CELLSIZE %lf %lf %lf \n",dptr2[0],dptr2[1],dptr2[2]); 
	  for(i = 0; i < 6 ; i ++)
		img->cellsize[i] = dptr2[i];
    }
    else
      img->cellsize[0] = img->cellsize[1] = img->cellsize[2] = 0;
     
    unsigned short count;
    double *dptr;
	TIFFGetField(tif, TIFFTAG_TOPLEFT, &count, &dptr);
    if (count==6){
	  if(verbose > 0)fprintf(stderr, "Read TIFFTAG_TOPLEFT %lf %lf %lf %lf %lf %lf\n",dptr[0],dptr[1],dptr[2],dptr[3],dptr[4],dptr[5]);
	  for(i = 0; i < 6 ; i ++)
		img->topleft[i] = dptr[i];
    }
    else
      img->topleft[0] = img->topleft[1] = img->topleft[2] = img->topleft[3] = img->topleft[4] = img->topleft[5] = 0;
	
   unsigned short count3;
   short *dptr3;  
   TIFFGetField(tif, TIFFTAG_GEOKEYDIRECTORY, &count3, &dptr3);
   if (count3==4) {
      img->hasgeokey = 1;
      if(verbose > 0)fprintf(stderr, "Read TIFFTAG_GEOKEYDIRECTORY %d %d %d %d\n",dptr3[0],dptr3[1],dptr3[2],dptr3[3]);
	  for(i = 0; i < 6 ; i ++)
		 img->geokeydir[i] = dptr3[i];
   }
   else {
	   if(verbose > 0)fprintf(stderr, "Problem reading TIFFTAG_GEOKEYDIRECTORY: %d values read instead of 4\n",count3);
      img->hasgeokey = 0;
   }
   
   
    img->bpp = TIFFScanlineSize(tif) / img->h ;
   if(verbose > 0)fprintf(stderr, "BPP: %d, SPP:%d BPS: %d\n", img->bpp, img->spp, img->bps);
    if(img->cellsize[1] < 0.0)
      img->cellsize[1] = -img->cellsize[1];  // XXX - probably shouldn't do this!
    if (TIFFIsTiled(tif)) {
      uint32 tileWidth, tileLength;
      TIFFGetField(tif, TIFFTAG_TILEWIDTH,  &tileWidth);
      TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);
      
      img->tile_width = tileWidth;
      img->tile_height = tileLength;
    }
    img->depth = 0;
    do {
      img->depth++;
    } while (TIFFReadDirectory(tif));

    TIFFClose(tif);
    
    return 1;
  } else {
    return 0;
  }
}

void load_worldfile(const char *filename, IMAGE_CHUNK *img) {
   FILE *fp;
   char buff[256];
   double tmpfloat;
   if ((fp = fopen(filename, "r")) == NULL) {
    fprintf(stderr, "Can not open ESRI world file %s\n", filename);
    exit (0);
  }
  if (fscanf(fp, "%lf", &tmpfloat) != 1) { // cellsize in X direction
    fprintf(stderr, "Not a valid ESRI world file!");
    fclose(fp);
    exit (0);
  }
  img->cellsize[0] = tmpfloat;

  if (fscanf(fp, "%lf", &tmpfloat) != 1) {// unused
    fprintf(stderr, "Not a valid ESRI world file!");
    fclose(fp);
    exit (0);
  }

  if (fscanf(fp, "%lf", &tmpfloat) != 1) { // unused
    fprintf(stderr, "Not a valid ESRI world file!");
    fclose(fp);
    exit (0);
  }

  if (fscanf(fp, "%lf", &tmpfloat) != 1) { // cellsize in Y direction
    fprintf(stderr, "Not a valid ESRI world file!");
    fclose(fp);
    exit (0);
  }
  img->cellsize[1] = tmpfloat;

  if (fscanf(fp, "%lf", &tmpfloat) != 1) { // upperleft
    fprintf(stderr, "Not a valid ESRI world file!");
    fclose(fp);
    exit (0);
  }
  img->topleft[3] = tmpfloat;

  if (fscanf(fp, "%lf", &tmpfloat) != 1) { // upperright
    fprintf(stderr, "Not a valid ESRI world file!");
    fclose(fp);
    exit (0);
  }
  img->topleft[4] = tmpfloat;

  fclose(fp);

}

/* this first calls the load_header to get the image information, then loads the data */
int load_image(const char *filename, IMAGE_CHUNK *img) {
  unsigned int tileWidth, tileLength;
  unsigned int row, col;
  tdata_t buf;
  int tiled;
  int x, y, i;

  if (!load_header(filename, img))
	  return 0;

  TIFF* tif = TIFFOpen(filename, "r");
  if (tif) {


    uint16 format;  // http://www.libtiff.org/man/TIFFGetField.3t.html
    int format_rval = TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &format);
    switch(format) {
      case SAMPLEFORMAT_UINT: break;         /* !unsigned integer data */
      case SAMPLEFORMAT_INT: break;           /* !signed integer data */
      case SAMPLEFORMAT_IEEEFP: break;       /* !IEEE floating point data */
      case SAMPLEFORMAT_VOID: break;         /* !untyped data */
      case SAMPLEFORMAT_COMPLEXINT: break;   /* !complex signed int */
      case SAMPLEFORMAT_COMPLEXIEEEFP: break;        /* !complex ieeefloating */
    }

    //img->bpp = TIFFScanlineSize(tif) / img->h;

    //    if ((format == SAMPLEFORMAT_IEEEFP) ||(format == SAMPLEFORMAT_INT || img->bps == 16)) {
    //      fprintf(stderr, "Can only resample 8 bit at the moment\n");
    //      exit(1);
    //    }

    img->data_size = (unsigned long int)img->w*img->h*img->spp;
    
    if (format == SAMPLEFORMAT_UINT || img->bps==8) { 
      img->_data_byte   = (unsigned char *)malloc(sizeof(unsigned char) *img->data_size);
      fprintf(stderr, "  Char Size: %d, %d, %d:   %lld\n",img->w, img->h, img->spp, img->data_size);
      if (!img->_data_byte) {
        fprintf(stderr, "Unable to allocate memory at image load!\n");
        exit(1);
      }
    }
    else if (img->bps==16) { 
      img->_data_short   = (unsigned short *)malloc(sizeof(unsigned short) *img->data_size);
      fprintf(stderr, "  Short Size: %d, %d, %d:   %lld\n",img->w, img->h, img->spp, img->data_size);
      if (!img->_data_short) {
        fprintf(stderr, "Unable to allocate memory at image load!\n");
        exit(1);
      }
    }
    else if (img->bps==32) { 
      img->_data_float   = (float *)malloc(img->bps * img->data_size);
      fprintf(stderr, "  Float Size: %d, %d, %d:   %lld\n",img->w, img->h, img->spp, img->data_size);
      if (!img->_data_float) {
        fprintf(stderr, "Unable to allocate memory at image load!\n");
        exit(1);
      }
    }
    else {
      fprintf(stderr, "unknow image depth: %d \n", img->bps);
      exit(1);
    }

    /* For reading tiled images (from http://www.remotesensing.org/libtiff/libtiff.html)*/
    if (TIFFIsTiled(tif)) {
		 
      uint32 tileWidth, tileLength;
      TIFFGetField(tif, TIFFTAG_TILEWIDTH,  &tileWidth);
      TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);

      unsigned int tilesize = TIFFTileSize(tif);
      tdata_t buf = malloc(TIFFTileSize(tif));
      if (!buf) {
        fprintf(stderr, "Unable to allocate memory at image load for tile buffer!\n");
        exit(1);
      }

      for (y = 0; y < img->h; y += tileLength) {
        for (x = 0; x < img->w; x += tileWidth) {
          TIFFReadTile(tif, buf, x, y, 0, 0);
       
          int adjusted_width;
		if (x + tileWidth > img->w)
		  adjusted_width =  img->w-x;
		else adjusted_width = tileWidth;

          int adjusted_height;
		if (y + tileLength > img->h)
		  adjusted_height =  img->h-y;
		else adjusted_height = tileLength;
	   
		for(i=0;i<adjusted_height;i++) {
            if (img->bps==32) {
              memcpy((void*)(img->_data_float + (unsigned long int)(((y+i)*img->w) + x)*img->spp),
                      (void*)(((float*)buf)+(unsigned long int)(i*adjusted_width*img->spp)),
                      adjusted_width*img->spp*sizeof(float));
            }
            else if (img->bps==16)
              memcpy((void*)(img->_data_short + (unsigned long int)(((y+i)*img->w) + x)*img->spp),
                      (void*)((unsigned short*)buf+(unsigned long int)(i*adjusted_width*img->spp)),
                      adjusted_width*img->spp*sizeof(unsigned short));
            else if (img->bps==8)
              memcpy((void*)(img->_data_byte  + (unsigned long int)(((y+i)*img->w) + x)*img->spp),
                      (void*)((unsigned char *)buf+(unsigned long int)(i*adjusted_width*img->spp)),
                      adjusted_width*img->spp);

          }
        }
      }
      free(buf);

    }
    /* Tiff is in strips, so read those in */
    else {

    for (row = 0; row < img->h; row++)
    {
      if (img->bps == 32)
       TIFFReadScanline(tif, (float*)(img->_data_float + (unsigned long int)(row*img->w*img->spp)), row, 0);
      else if (img->bps == 16)
       TIFFReadScanline(tif, (unsigned short*)(img->_data_short + (unsigned long int)(row*img->w*img->spp)), row, 0);
      else if (img->bps == 8)
        TIFFReadScanline(tif, (unsigned char*)(img->_data_byte + (unsigned long int)(row*img->w*img->spp)), row, 0);
    }
  }
  TIFFClose(tif);

  return 1;
  }

  else
  {
      //cerr << "ERROR: Bad .tif file for topo layer!" << endl;
      return 0;
  }
}

/*  This adds new tiff tags to the Tiff header.  In this case, geo specific tags */
void TagExtender(TIFF *tiff) {
  static const TIFFFieldInfo xtiffFieldInfo[] = {
      { TIFFTAG_CELLSIZE, -1, -1, TIFF_DOUBLE,  FIELD_CUSTOM, 1, 1, "Cellsize" },
      { TIFFTAG_TOPLEFT , -1, -1, TIFF_DOUBLE,  FIELD_CUSTOM, 1, 1, "ImageToMetersMatrix" },
      { TIFFTAG_GEOKEYDIRECTORY , -1, -1, TIFF_SHORT,  FIELD_CUSTOM, 1, 1, "GeoKeyDirectoryTag" },

  };
  TIFFMergeFieldInfo(tiff, xtiffFieldInfo, 3);
}

int write_header(TIFF *tif, IMAGE_CHUNK *img, int tiff_dir) {

  TIFFSetTagExtender(TagExtender);

    TIFFSetDirectory(tif, tiff_dir);
  if (tif) {
    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, img->w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, img->h);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, img->bps);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, img->spp);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, 1);
    
    switch(img->bps) {
      case 8:
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
        break;
      case 16:
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_INT);
        break;
       case 32:
        TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
        break;
    }


    
    if (img->spp==1)
      TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    else
      TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

    if (tiff_dir == 1) {
      TIFFSetField(tif, TIFFTAG_CELLSIZE, 3, img->cellsize);
      TIFFSetField(tif, TIFFTAG_TOPLEFT, 6, img->topleft);
      if (img->hasgeokey)
        TIFFSetField(tif, TIFFTAG_GEOKEYDIRECTORY, 4, img->geokeydir);

      TIFFSetField(tif, TIFFTAG_SUBFILETYPE, OFILETYPE_IMAGE );
      TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
      TIFFSetField(tif, TIFFTAG_SOFTWARE, "TILETIFF 0.5");
    }

    return 1;
  } else
      return 0;
}

/*return an array of image tiles */
IMAGE_CHUNK *make_chunks (IMAGE_CHUNK *img, int tile_width, int tile_height) {
  unsigned int x, y, j, newx, newy;
  unsigned long int a_index;
  IMAGE_CHUNK *chunk;

  int x_chunks = img->w/tile_width;
  if (x_chunks < (float)img->w/(float)tile_width) x_chunks++;
  int y_chunks = img->h/tile_height;
  if (y_chunks < (float)img->h/(float)tile_height) y_chunks++;
  int total_chunks = x_chunks * y_chunks;

  if (verbose>0)fprintf(stderr, "Total tiles are %d\n", total_chunks);
  chunk = (IMAGE_CHUNK*)malloc(sizeof(IMAGE_CHUNK) * total_chunks);

  int current_chunk =0;
  for(x=0;x<img->w;x+=tile_width) {
    for(y=0;y<img->h;y+=tile_height) {

      //set width/height of new chunk.  Also need to clip to last few pixels.
      int w = img->w - x;
      int h = img->h - y;

      if (w > tile_width) w = tile_width;
      if (h > tile_height) h = tile_height;

      if(verbose>1)fprintf(stderr, "Chunk %d of %d:  X: %d Y: %d W: %d, H: %d iW: %d, iH: %d\n", current_chunk, total_chunks, x, y, w, h, img->w, img->h);

      chunk[current_chunk].w =w;
      chunk[current_chunk].h = h;
      chunk[current_chunk].bps = img->bps;
      chunk[current_chunk].spp = img->spp;
      chunk[current_chunk].cellsize[0] = img->cellsize[0] * chunk[current_chunk].w/tile_width;
      chunk[current_chunk].cellsize[1] = img->cellsize[1] * chunk[current_chunk].h/tile_height;
      chunk[current_chunk].cellsize[2] = img->cellsize[2];

      chunk[current_chunk].topleft[0] = img->topleft[0];
      chunk[current_chunk].topleft[1] = img->topleft[1];
      chunk[current_chunk].topleft[2] = img->topleft[2];
      chunk[current_chunk].topleft[3] = img->topleft[3] + x*img->cellsize[0];
      chunk[current_chunk].topleft[4] = img->topleft[4] - y*img->cellsize[1];
      chunk[current_chunk].topleft[5] = img->topleft[5];

      chunk[current_chunk].data_size = (unsigned long long int) (tile_width*tile_height*chunk[current_chunk].spp);
      if (chunk[current_chunk].bps==32) {
        chunk[current_chunk]._data_float =
          (float*)malloc(chunk[current_chunk].bps *chunk[current_chunk].data_size);
        if (chunk[current_chunk]._data_float == NULL) {
          fprintf(stderr, "Unable to allocate memory while making tiles32!\n");
          exit(1);
        }
      }
      else if (chunk[current_chunk].bps==16) {
        chunk[current_chunk]._data_short =
          (unsigned short*)malloc(chunk[current_chunk].bps *chunk[current_chunk].data_size);
        if (chunk[current_chunk]._data_short == NULL) {
          fprintf(stderr, "Unable to allocate memory while making tiles16!\n");
          exit(1);
        }
      }
      else if (chunk[current_chunk].bps==8) {
        chunk[current_chunk]._data_byte =
          (unsigned char*)malloc(chunk[current_chunk].bps *chunk[current_chunk].data_size);
        if (chunk[current_chunk]._data_byte == NULL) {
          fprintf(stderr, "Unable to allocate memory while making tiles8!\n");
          exit(1);
        }
      }
       //fprintf (stderr, "Chunk: W: %d H: %d spp: %d\n", chunk.w, chunk.h, chunk.spp );
      int current_pixel = 0;
      for (newy = y; newy < (tile_height +y); newy++) {
        for (newx = x*img->spp; newx < (tile_width+x)*img->spp; newx=newx+img->spp) {
          if ((newy >= (img->h)) || (newx >=  (img->w*img->spp))) {  /* pad data with black at edges */
            for (j = 0; j < img->spp; j++) {
              if (chunk[current_chunk].bps==32) 
                chunk[current_chunk]._data_float[current_pixel]= 0.0f;
              if (chunk[current_chunk].bps==16) 
                chunk[current_chunk]._data_short[current_pixel]= 0;
              if (chunk[current_chunk].bps==8) 
                chunk[current_chunk]._data_byte[current_pixel]= 0;
              current_pixel++;
            }
          }
          else
          for (j = 0; j < img->spp; j++) {
            //fprintf(stderr, "X: %d Y: %d newX: %d newY: %d, Blah: %d\n", x, y, newx, newy, ((newx+j)+img->w*img->spp*newy));
            a_index = (unsigned long int)(newx+j)+img->w*img->spp*newy;
            //fprintf(stderr, "%d\n", a_index);
            if (chunk[current_chunk].bps==32) 
              chunk[current_chunk]._data_float[current_pixel]= (img->_data_float[a_index]);
            if (chunk[current_chunk].bps==16) 
              chunk[current_chunk]._data_short[current_pixel]= (img->_data_short[a_index]);
            if (chunk[current_chunk].bps==8) 
              chunk[current_chunk]._data_byte[current_pixel]= (img->_data_byte[a_index]);
            current_pixel++;
          }
        }
      }
      if (chunk[current_chunk].w < tile_width) chunk[current_chunk].w = tile_width;
      if (chunk[current_chunk].h < tile_height) chunk[current_chunk].h = tile_height;

    current_chunk++;
    }
  }
  return chunk;
}

IMAGE_CHUNK * scale_data_by_half ( IMAGE_CHUNK *img, int average_data) {

  unsigned long int x, y, i, j;
  int w, h;
  IMAGE_CHUNK *out;

  if(verbose>1)fprintf (stderr, "Resizing from %dx%d to  %dx%d\n", img->w, img->h, img->w/2, img->h/2 );
  
  out = (IMAGE_CHUNK*)malloc(sizeof(IMAGE_CHUNK));
//  out->_data = NULL;
  out->w = img->w/2;
  out->h = img->h/2;
  out->spp = img->spp;
  out->bpp = img->bpp;
  out->bps = img->bps;

  out->data_size = (unsigned long int)(out->w+1)*(out->h+1)*out->spp;

  
  if (out->bps==32) {
    out->_data_float = (float*)malloc(out->bps *out->data_size);
    if (out->_data_float == NULL) {
      fprintf(stderr, "Unable to allocate memory while making tiles32!\n");
      sleep(10);
      exit(1);
    }
  }
  else if (out->bps==16) {
    out->_data_short = (unsigned short*)malloc(out->bps *out->data_size);
    if (out->_data_short == NULL) {
      fprintf(stderr, "Unable to allocate memory while making tiles16!\n");
      exit(1);
    }
  }
  else if (out->bps==8) {
    out->_data_byte = (unsigned char*)malloc(out->bps *out->data_size);
    if (out->_data_byte == NULL) {
      fprintf(stderr, "Unable to allocate memory while making tile8s!\n");
      exit(1);
    }
  }

  i = 0;
        //fprintf(stderr, "Y: %d X %d\n", img->w%2, img->h%2);
  for (x = 0; x < (img->h-(img->h%2)); x=x+2) {
    for (y = 0; y < ((img->w-(img->w%2))*img->spp); y=y+(img->spp*2)) {

       if (i > out->data_size) {
         fprintf(stderr, "I too big! I: %d size %d X: %d Y: %d spp: %d\n", i, out->data_size, x, y, img->spp);
          return 0;
        }

        for (j = 0; j < img->spp; j++) {
          if (average_data) {
            if (out->bps==32) 
              out->_data_float[i]= (img->_data_float[((y)+j)+img->w*img->spp*x] +
	                            img->_data_float[((y)+j+img->spp)+img->w*img->spp*x] +
	                            img->_data_float[((y)+j)+img->w*img->spp*(x+1)] +
	                            img->_data_float[((y)+j+img->spp)+img->w*img->spp*(x+1)])/4.0;
            if (out->bps==16) 
              out->_data_short[i]= (img->_data_short[((y)+j)+img->w*img->spp*x] +
	                            img->_data_short[((y)+j+img->spp)+img->w*img->spp*x] +
	                            img->_data_short[((y)+j)+img->w*img->spp*(x+1)] +
	                            img->_data_short[((y)+j+img->spp)+img->w*img->spp*(x+1)])/4.0;
            if (out->bps==8) 
              out->_data_byte[i]= (img->_data_byte[((y)+j)+img->w*img->spp*x] +
	                            img->_data_byte[((y)+j+img->spp)+img->w*img->spp*x] +
	                            img->_data_byte[((y)+j)+img->w*img->spp*(x+1)] +
	                            img->_data_byte[((y)+j+img->spp)+img->w*img->spp*(x+1)])/4.0;
          }
          else {
            if (out->bps==32) 
              out->_data_float[i]= img->_data_float[((y)+j)+img->w*img->spp*x];
            if (out->bps==16) 
              out->_data_short[i]= img->_data_short[((y)+j)+img->w*img->spp*x];
            if (out->bps==8) 
              out->_data_byte[i]= img->_data_byte[((y)+j)+(img->w*img->spp*x)];
          }
          i++;
        }
      }
    }

  return out;
}

int write_image (IMAGE_CHUNK *primary) {
  
  TIFF *tif;
  unsigned int x, y, j, i, newx, newy;
  IMAGE_CHUNK *chunks;
  int total_chunks;
  
  TIFFSetTagExtender(TagExtender);

  tif = TIFFOpen(primary->name, "w");

  int x_chunks = primary->w/primary->tile_width;
  if (x_chunks < (float)primary->w/(float)primary->tile_width) x_chunks++;
  int y_chunks = primary->h/primary->tile_height;
  if (y_chunks < (float)primary->h/(float)primary->tile_height) y_chunks++;
  total_chunks = x_chunks * y_chunks;

  chunks = make_chunks(primary, primary->tile_width, primary->tile_height);

  //free(primary._data_byte);

    write_header(tif, primary, 0);

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, primary->w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, primary->h);

    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

    TIFFSetField(tif, TIFFTAG_TILELENGTH, primary->tile_width);
    TIFFSetField(tif, TIFFTAG_TILEWIDTH, primary->tile_width);
    //TIFFSetField(tif, TIFFTAG_TILEDEPTH, 1);
    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, OFILETYPE_IMAGE );
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_SOFTWARE, "TILETIFF 0.5");

    i = 0;
    for (x = 0; x < primary->w/j; x = x+primary->tile_width) {
      for (y = 0; y < primary->h/j; y = y+primary->tile_height) {       
        if(verbose > 1)fprintf(stderr, "Writing chunk %d, size is %d\n", i, chunks[i].w);
        if (chunks[i].bps == 32)
          TIFFWriteTile(tif, (tdata_t)(chunks[i]._data_float), x, y, 0, 0);
        if (chunks[i].bps == 16)
          TIFFWriteTile(tif, (tdata_t)(chunks[i]._data_short), x, y, 0, 0);
        if (chunks[i].bps == 8)
          TIFFWriteTile(tif, (tdata_t)(chunks[i]._data_byte), x, y, 0, 0);
        i++;
      }
    }

  TIFFClose(tif);
  
  return 0;
}
