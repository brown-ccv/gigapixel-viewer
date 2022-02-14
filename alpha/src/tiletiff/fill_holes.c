#include "tiletiff.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>


typedef struct {
  char imagefile[256];
  char shortname[256];
  char newname[256];
  char *worldfile;
  char outfile[256];
  int tile_width;
  int tile_height;
  int tile_min;
  int encoder;
  int quality;
  int tile_type;
  float nan;
  int levels;
}options_struct;

typedef struct {
  float average;
  int has_black;
  int all_black;
} TILE_PROPERTIES;

options_struct *get_options (int argc, char *argv[]) {

  opterr = 0;
  options_struct *opts = (options_struct*)malloc (sizeof(options_struct));
  opts->tile_width=256;
  opts->tile_height=256;
  opts->encoder=JPEG;
  opts->quality=95;
  opts->tile_min=16;
  opts->tile_type = IMAGE;
  opts->worldfile = NULL;
  opts->nan = 0;
  opts->levels = 0;
  
  int c;
  
  while ((c = getopt (argc, argv, "t:w:h:c:q:vm:e:n:l:")) != -1)
    switch (c)
      {
      case 'w':
        opts->tile_width = atoi(optarg);
        if ((opts->tile_width & (opts->tile_width-1)) !=0 ){
          printf("Tile width must be power of 2.  Reverting to 2048.\n");
          opts->tile_width = 1024;
        }
        break;
      case 'h':
        opts->tile_height = atoi(optarg);
	      if ((opts->tile_height & (opts->tile_height-1)) !=0 ){
	        printf("Tile height must be power of 2.  Reverting to 2048.\n");
	        opts->tile_height = 1024;
	      }
        break;
      case 'n':
        opts->nan = atoi(optarg);
        break;
      case 'l':
        opts->levels = atoi(optarg);
        break;
      case 'q':
        opts->quality = atoi(optarg);
	      if (opts->quality > 100) {
	        opts->quality = 100;
	        printf("Clamping quality to 100\n");
	      }
	      else if (opts->quality < 10) {
	       opts->quality = 10;
	       printf("Clamping quality to 10\n");
	      }
        break;
      case 'c':
        if (!strcasecmp(optarg, "jpeg"))
	        opts->encoder = JPEG;
        else if (!strcasecmp(optarg, "jpg"))
	        opts->encoder = JPEG;
        else if (!strcasecmp(optarg, "lzw"))
	        opts->encoder = LZW;
        else if (!strcasecmp(optarg, "none"))
	        opts->encoder = NONE;
        break;
      case 'v':
	verbose++;
	break;
      case 'e':
	//strcpy(worldfile, optarg);
	opts->worldfile = optarg;
        break;
      case '?':
        break;
    }

  if (optind >= argc) {
    exit(1);
  }
  else {
   strcpy(opts->imagefile, argv[optind]);
  }
  if (optind+1 >= argc){
     bzero(opts->shortname, sizeof(opts->shortname));
     strncpy(opts->shortname, opts->imagefile, strlen(opts->imagefile)-4);
     sprintf(opts->outfile, "%s-filled.tif", opts->shortname);
  }
  else
    strcpy(opts->outfile, argv[optind+1]);
    if(verbose>0)
    fprintf(stderr, "Arguments: W: %d H: %d M: %d Q: %d C: %d\n", opts->tile_width,
	opts->tile_height, opts->tile_min, opts->quality, opts->encoder);

  if(verbose >1)fprintf(stderr, "In: %s, Out: %s\n", opts->imagefile, opts->outfile);
  
  return (opts);
}  


int write_DEM (options_struct *opts , IMAGE_CHUNK *primary, IMAGE_CHUNK *chunks) {
  
  TIFF *tif;
  unsigned int x, y, j, i, newx, newy;
  IMAGE_CHUNK *scaled_chunk = NULL;
  int total_chunks;
  

  int tiff_dir = 1;
  
  TIFFSetTagExtender(TagExtender);

  tif = TIFFOpen(opts->outfile, "w");

    write_header(tif, primary, 0);

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, primary->w);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, primary->h);
    
    if (opts->encoder == JPEG) {
      TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
      TIFFSetField(tif, TIFFTAG_JPEGQUALITY, opts->quality);
    }
    else if (opts->encoder == LZW) {
      TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    }
    else
      TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);


    TIFFSetField(tif, TIFFTAG_TILELENGTH, opts->tile_width);
    TIFFSetField(tif, TIFFTAG_TILEWIDTH, opts->tile_width);
    //TIFFSetField(tif, TIFFTAG_TILEDEPTH, 1);
    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, OFILETYPE_IMAGE );
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    //TIFFSetField(tif, TIFFTAG_SOFTWARE, "TILETIFF 0.5");

    i = 0;
    for (x = 0; x < primary->w; x = x+opts->tile_width) {
      for (y = 0; y < primary->h; y = y+opts->tile_height) {
        if(verbose > 1)fprintf(stderr, "Writing chunk %d, size is %d\n", i, chunks[i].w);
        if (chunks[i].bps == 32) {
          TIFFWriteTile(tif, (tdata_t)(chunks[i]._data_float), x, y, 0, 0);
          free(chunks[i]._data_float);
        }
        else if (chunks[i].bps == 16) {
          TIFFWriteTile(tif, (tdata_t)(chunks[i]._data_short), x, y, 0, 0);
          free(chunks[i]._data_short);
        }
        else if (chunks[i].bps == 8) {
          TIFFWriteTile(tif, (tdata_t)(chunks[i]._data_byte), x, y, 0, 0);
          free(chunks[i]._data_byte);
        }
        i++;
      }
    }
    free (chunks);
    //TIFFWriteDirectory(tif);

  TIFFClose(tif);
  
  return 0;
}

int fill_image (options_struct *opts ) {
  
  IMAGE_CHUNK *level;
  IMAGE_CHUNK *primary;
  IMAGE_CHUNK *primary_chunks;
  int total_chunks = 0;
  TILE_PROPERTIES *tile_properties;
  int levels = 0;
  TIFF *tif;
  int i, j;
  int tiles_w, tiles_h;
  float pixel_average = 0;
  int pixel_count = 0;


  primary = (IMAGE_CHUNK*)malloc(sizeof(IMAGE_CHUNK));

  load_header(opts->imagefile, primary);
  if (!opts->levels) {
    if (primary->w < primary->h)
      for (i = opts->tile_width; i > 0; i=i/2) levels ++;
    else
      for (i = opts->tile_height; i > 0; i=i/2) levels ++;
  }
  else levels = opts->levels;
    
    if (primary->spp > 1) {
      fprintf(stderr, "This can only be used on single channel images (DEMs).\n");
      exit (1);
    }
  tiles_w = primary->w/opts->tile_width;
  if ((primary->w % opts->tile_width))
    tiles_w ++;
  tiles_h = primary->h/opts->tile_height;
  if ((primary->h % opts->tile_height))
    tiles_h ++;
  total_chunks = (int)(tiles_w * tiles_h);
  //total_chunks = 3;

  level = (IMAGE_CHUNK*)malloc(sizeof(IMAGE_CHUNK) * (levels+1));
  load_image(opts->imagefile, primary);
  
  primary_chunks = make_chunks(primary, opts->tile_width, opts->tile_height);

  

  fprintf(stderr, "Filling image %d by %d, %d levels %d tiles\n", primary->w, primary->h, levels, total_chunks);
 
  tile_properties = (TILE_PROPERTIES*)malloc(sizeof(TILE_PROPERTIES) * total_chunks);
  
  // Find the average color for each tile, and if the tile has any or all black
  for (i = 0; i < total_chunks; i++) {
    tile_properties[i].has_black = 0;
    tile_properties[i].all_black = 0;
    pixel_average = 0;
    pixel_count = 0;

    for (j = 0; j < (opts->tile_width*opts->tile_height); j++) {
      if (primary->bps == 8) {
        if (primary_chunks[i]._data_byte[j] != (unsigned char)opts->nan) {
          pixel_average += (float)primary_chunks[i]._data_byte[j];
          pixel_count ++;
        }
        else 
          tile_properties[i].has_black = 1;
      }
      if (primary->bps == 16) {
        if (primary_chunks[i]._data_short[j] != (unsigned short)opts->nan) {
          pixel_average += (float)primary_chunks[i]._data_short[j];
          pixel_count ++;
        }
        else tile_properties[i].has_black = 1;
      }
      if (primary->bps == 32) {
        if (primary_chunks[i]._data_float[j] != (float)opts->nan) {
          pixel_average += (float)primary_chunks[i]._data_float[j];
          pixel_count ++;
        }
        else 
          tile_properties[i].has_black = 1;
      }
    }
       
    if (pixel_count == 0)
      tile_properties[i].all_black = 1;
    else {
      tile_properties[i].average = (float)(pixel_average/(float)pixel_count);
    }
    
    if (verbose > 1)
      fprintf (stderr, "Tile %d: has black: %d, all black: %d average: %f\n", i, 
        tile_properties[i].has_black, tile_properties[i].all_black,  tile_properties[i].average);



  }
  

  
  
  // Find the average color for entire image
  pixel_average = 0;
  pixel_count = 0;
  for (i=0; i<total_chunks; i++) {
    if (!tile_properties[i].all_black) {
      pixel_count ++;
      pixel_average += tile_properties[i].average;
    }
  } 
  pixel_average = (float)(pixel_average / (float)pixel_count);
  
  fprintf(stderr, "Average was %f\n", pixel_average);
 
  
  for (i = 0; i < total_chunks; i++) {
    
    // This tile does not have any black in it.
    if (!tile_properties[i].has_black)
      continue;
    
    
    // This tile is only black, so set it to the average color of the data set
    if (tile_properties[i].all_black) { // This works
      fprintf(stderr, "All black tile found, consider using %dx%d tiles instead.\n", 
          opts->tile_width*2, opts->tile_height*2);
      for (j = 0; j < opts->tile_width*opts->tile_height; j++) {
        if (primary->bps == 8)
          primary_chunks[i]._data_byte[j] = (unsigned char) pixel_average;
        else if (primary->bps == 16)
          primary_chunks[i]._data_short[j] = (unsigned short) pixel_average;
        else if (primary->bps == 32)
          primary_chunks[i]._data_float[j] = (float) pixel_average;
      }
      continue;
    }

    
    level[0].w = opts->tile_width;
    level[0].h = opts->tile_height;
    
    if (primary->bps == 8)
      level[0]._data_byte = primary_chunks[i]._data_byte;
    else if (primary->bps == 16)
      level[0]._data_short = primary_chunks[i]._data_short;
    else if (primary->bps == 32)
      level[0]._data_float = primary_chunks[i]._data_float;
    
    for (j = 1; j < levels; j++) {
      level[j].w = level[j-1].w/2;
      level[j].h = level[j-1].h/2;
      level[j].bps = primary->bps;
      level[j].data_size = (unsigned long long int) (level[j].w * level[j].h);
      
      if (level[j].bps==32) {
        level[j]._data_float = (float*)malloc(level[j].bps *level[j].data_size);
        if (level[j]._data_float == NULL) {
          fprintf(stderr, "Unable to allocate memory while making tiles32!\n");
          exit(1);
        }
      }
      else if (level[j].bps==16) {
        level[j]._data_short = (unsigned short*)malloc(level[j].bps *level[j].data_size);
        if (level[j]._data_short == NULL) {
          fprintf(stderr, "Unable to allocate memory while making tiles16!\n");
          exit(1);
        }
      }
      else if (level[j].bps==8) {
        level[j]._data_byte = (unsigned char*)malloc(level[j].bps *level[j].data_size);
        if (level[j]._data_byte == NULL) {
          fprintf(stderr, "Unable to allocate memory while making tiles8!\n");
          exit(1);
        }
      }
      
       //fprintf (stderr, "Chunk: W: %d H: %d spp: %d\n", chunk.w, chunk.h, chunk.spp );
      int current_pixel = 0;
      int x, y, z;
      int still_has_black = 0;
      float pixel;
      int w = level[j-1].w;
      int h = level[j-1].h;
      float nan;

      nan = opts->nan;
        
      for (x = 0; x < h; x=x+2) {
        for (y = 0; y < w; y=y+2) {
          pixel_count = 0;
          pixel = 0;
          if (level[j].bps==8) {
            if (level[j-1]._data_byte[(y)+w*x] != (unsigned char)nan) {
              pixel += (float)level[j-1]._data_byte[(y)+w*x];
              pixel_count++;
            }
            if (level[j-1]._data_byte[(y+1)+w*x] != (unsigned char)nan) {
              pixel += (float)level[j-1]._data_byte[(y+1)+w*x];
              pixel_count++;
            }
            if (level[j-1]._data_byte[(y)+w*(x+1)] != (unsigned char)nan) {
              pixel += (float)level[j-1]._data_byte[(y)+w*(x+1)];
              pixel_count++;
            }
            if (level[j-1]._data_byte[(y+1)+w*(x+1)] != (unsigned char)nan) {
              pixel+=(float)level[j-1]._data_byte[(y+1)+w*(x+1)];
              pixel_count++;
            }
            
            //if (pixel_count) level[j]._data_byte[current_pixel] = (unsigned char)(pixel_average);
           if (pixel_count) level[j]._data_byte[current_pixel] = 
             (unsigned char)(pixel/pixel_count);
           else {
             level[j]._data_byte[current_pixel] = (unsigned char)nan;
             //level[j]._data_byte[current_pixel] = (unsigned char) tile_properties[i].average;
             still_has_black = 1;
           }
            current_pixel++;
            if (current_pixel > level[j].h*level[j].w) {
              fprintf(stderr, "Too many pixels: %d over %d\n", current_pixel, level[j].h*level[j].w);
              exit(1);
            }
          }
        }
      }
     
      
      
      //if (!still_has_black) {
      //  fprintf(stderr, "No more black in chunk %d\n", i);
      //  continue;
      //}
    }// Levels
    
    int k;
    for (k = j-1; k >0; k--) {
      fprintf(stderr, "Filling holes at level %d\n", k);
      int current_pixel = 0;
      int x, y;
      int filled = 0;
      int h = level[k-1].h;
      int w = level[k-1].w;
      float nan;
      nan = opts->nan;
      for (x = 0; x < (h); x=x+2) {
        for (y = 0; y < (w); y=y+2) {
          if (level[k]._data_byte[current_pixel] != (unsigned char)nan) {
            if ((level[k-1]._data_byte[(y)+w*x]) == (unsigned char)nan)  {
              //level[k-1]._data_byte[(y)+w*x] = pixel_average;
              level[k-1]._data_byte[(y)+w*x] = level[k]._data_byte[current_pixel];
              filled++;
            }
            if ((level[k-1]._data_byte[(y+1)+w*x]) == (unsigned char)nan) {
              //level[k-1]._data_byte[(y+1)+w*x] = pixel_average;
              level[k-1]._data_byte[(y+1)+w*x] = level[k]._data_byte[current_pixel];
              filled++;
            }
            if ((level[k-1]._data_byte[(y)+w*(x+1)]) == (unsigned char)nan) {
              //level[k-1]._data_byte[(y)+w*(x+1)] = pixel_average;
              level[k-1]._data_byte[(y)+w*(x+1)] = level[k]._data_byte[current_pixel];
              filled++;
            }
            if ((level[k-1]._data_byte[(y+1)+w*(x+1)]) == (unsigned char)nan) {
              //level[k-1]._data_byte[(y+1)+w*(x+1)] = pixel_average;
              level[k-1]._data_byte[(y+1)+w*(x+1)] = level[k]._data_byte[current_pixel];
              filled++;
            }
          }
          current_pixel++;
        }
      }
      fprintf(stderr, "Filled %d pixels\n", filled);
     }
  }
  
  write_DEM(opts, primary, primary_chunks);
  
}
  
int main (int argc, char *argv[]) {

  options_struct *opts;


  opts = get_options(argc, argv);
 
  TIFFSetWarningHandler(NULL);

  fill_image(opts);

}
