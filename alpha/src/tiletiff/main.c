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
}options_struct;

void usage(char *filename, int err_num) {
  printf("usage: %s [options] input output\n", filename);
    printf("where options are:\n");
    printf("  -w #    The width of the primary tiles.  Must be power of 2.\n");
    printf("  -h #    The height of the primary tiles.  Must be power of 2.\n");
    printf("  -q #    Quality of image (10-100) (only used with jpeg compression)\n");
    printf("  -m #    size of smallest image tile.  This also determines number of sub-images.  Must be power of 2.\n");
    printf("  -v      add verbose output.  Can be stacked.\n");
    printf("  -e      tfw world file.\n");
    printf("  -c lzw|jpeg|none  Compression scheme to use.\n");
    printf("  -t image|dem      Write out a tiletiff image, or DEM.\n");

    exit(err_num);
}

options_struct *get_options (int argc, char *argv[]) {

  opterr = 0;
  options_struct *opts = (options_struct*)malloc (sizeof(options_struct));
  opts->tile_width=1024;
  opts->tile_height=1024;
  opts->encoder=JPEG;
  opts->quality=95;
  opts->tile_min=16;
  opts->tile_type = IMAGE;
  opts->worldfile = NULL;
  
  int c;
  
  while ((c = getopt (argc, argv, "t:w:h:c:q:vm:e:")) != -1)
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
      case 'm':
        opts->tile_min = atoi(optarg);
	      if ((opts->tile_min & (opts->tile_min-1)) !=0 ){
	        printf("Tile minimum must be power of 2.  Reverting to 32.\n");
	        opts->tile_min = 32;
	      }
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
      case 't':
        if (!strcasecmp(optarg, "image"))
	        opts->tile_type = IMAGE;
        else if (!strcasecmp(optarg, "dem"))
	        opts->tile_type = DEM_SINGLE;
        else if (!strcasecmp(optarg, "dem_m"))
	        opts->tile_type = DEM_MULTI;
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
    usage(argv[0], 2);
  }
  else {
   strcpy(opts->imagefile, argv[optind]);
  }
  if (optind+1 >= argc){
     bzero(opts->shortname, sizeof(opts->shortname));
     strncpy(opts->shortname, opts->imagefile, strlen(opts->imagefile)-4);
     sprintf(opts->outfile, "%s-tiled.tif", opts->shortname);
  }
  else
    strcpy(opts->outfile, argv[optind+1]);
    if(verbose>0)
    fprintf(stderr, "Arguments: W: %d H: %d M: %d Q: %d C: %d\n", opts->tile_width,
	opts->tile_height, opts->tile_min, opts->quality, opts->encoder);

  if(verbose >1)fprintf(stderr, "In: %s, Out: %s\n", opts->imagefile, opts->outfile);
  
  return (opts);
}  


static int export_image (options_struct *opts ) {
  
  TIFF *tif;
  unsigned int x, y, j, i, newx, newy;
  IMAGE_CHUNK primary;
  IMAGE_CHUNK *scaled_chunk;
  IMAGE_CHUNK *chunks;
  int total_chunks;
  
    if (!load_image(opts->imagefile, &primary)) {
    fprintf(stderr, "Unable to open tiff file %s\n", opts->imagefile);
    exit(1);
  }

  if (opts->worldfile) {
    load_worldfile(opts->worldfile, &primary);
  }

  int tiff_dir = 1;
  
  TIFFSetTagExtender(TagExtender);

  tif = TIFFOpen(opts->outfile, "w");

  int x_chunks = primary.w/opts->tile_width;
  if (x_chunks < (float)primary.w/(float)opts->tile_width) x_chunks++;
  int y_chunks = primary.h/opts->tile_height;
  if (y_chunks < (float)primary.h/(float)opts->tile_height) y_chunks++;
  total_chunks = x_chunks * y_chunks;

  chunks = make_chunks(&primary, opts->tile_width, opts->tile_height);

  //free(primary._data_byte);

  for ( j = 1; opts->tile_width/j >= opts->tile_min; j*=2) {
    if (verbose>0)fprintf(stderr, "Saving %dx%d tiles...\n", opts->tile_width/j, opts->tile_height/j);

    write_header(tif, &primary, tiff_dir++);

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, primary.w/j);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, primary.h/j);
    
    if (opts->encoder == JPEG) {
      TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
      TIFFSetField(tif, TIFFTAG_JPEGQUALITY, opts->quality);
    }
    else if (opts->encoder == LZW) {
      TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    }
    else
      TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);


    TIFFSetField(tif, TIFFTAG_TILELENGTH, opts->tile_width/j);
    TIFFSetField(tif, TIFFTAG_TILEWIDTH, opts->tile_width/j);
    //TIFFSetField(tif, TIFFTAG_TILEDEPTH, 1);
    TIFFSetField(tif, TIFFTAG_SUBFILETYPE, OFILETYPE_IMAGE );
    TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tif, TIFFTAG_SOFTWARE, "TILETIFF 0.5");

    i = 0;
    for (x = 0; x < primary.w/j; x = x+opts->tile_width/j) {
      for (y = 0; y < primary.h/j; y = y+opts->tile_height/j) {
        if (j > 1) {
          if(verbose > 1)fprintf(stderr, "Scaling chunk %d\n", i);
          
          //scaled_chunk._data_byte = (unsigned char *) malloc (sizeof(unsigned char) * opts->tile_width/j* opts->tile_height/j * primary.spp);
          //if (scaled_chunk._data_byte == NULL) {
           // fprintf(stderr, "Unable to allocate memory: %d %d %d!\n", opts->tile_width/j, opts->tile_height/j, primary.spp);
            //exit(1);
          //}
          scaled_chunk = scale_data_by_half(&chunks[i], 1);
          if (chunks[i].bps == 8) {
            free(chunks[i]._data_byte);
            chunks[i]._data_byte = scaled_chunk->_data_byte;
          }
          else if (chunks[i].bps == 16) {
            free(chunks[i]._data_short);
            chunks[i]._data_short = scaled_chunk->_data_short;
          }
          else if (chunks[i].bps == 32) {
            free(chunks[i]._data_float);
            chunks[i]._data_float = scaled_chunk->_data_float;
          }
          chunks[i].w = chunks[i].w/2;
          chunks[i].h = chunks[i].h/2;
        }
        
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
    TIFFWriteDirectory(tif);
  }
  TIFFClose(tif);
  
  return 0;
}
  
int write_DEM (options_struct *opts ) {
  
  TIFF *tif;
  unsigned int x, y, j, i, newx, newy;
  IMAGE_CHUNK *primary = NULL;
  IMAGE_CHUNK *scaled_chunk = NULL;
  IMAGE_CHUNK *chunks = NULL;
  int total_chunks;
  
    primary = (IMAGE_CHUNK*)malloc(sizeof(IMAGE_CHUNK));

  
    if (!load_image(opts->imagefile, primary)) {
    fprintf(stderr, "Unable to open tiff file %s\n", opts->imagefile);
    exit(1);
  }

  if (opts->worldfile) {
    load_worldfile(opts->worldfile, primary);
  }

  int tiff_dir = 1;
  
  TIFFSetTagExtender(TagExtender);

  tif = TIFFOpen(opts->outfile, "w");


  for ( j = 1;  (primary->w > opts->tile_width) || (primary->h > opts->tile_height); j++) {
        
    if (j > 1) {  /* Don't scale the data on the first pass */
      if(verbose > 1)fprintf(stderr, "Scaling level %d, size is %dx%d  %dx%d\n", j, primary->w, primary->h, opts->tile_width, opts->tile_height);    
      scaled_chunk = scale_data_by_half(primary, 0);
      if (primary->bps==32)
        free(primary->_data_float);
      else if (primary->bps==16)
        free(primary->_data_short);
      else if (primary->bps==8)
        free(primary->_data_byte);
 
      free(primary);
      primary = scaled_chunk;
      //primary->_data_byte = scaled_chunk->_data_byte;
    }

    write_header(tif, primary, tiff_dir++);

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

    if (verbose >1) fprintf(stderr, "Chunking %dx%d image\n", primary->w, primary->h);  
    chunks = make_chunks(primary, opts->tile_width, opts->tile_height);
    
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
    TIFFWriteDirectory(tif);
  } /* J loop */
  TIFFClose(tif);
  
  return 0;
}
  
int write_multi_DEM (options_struct *opts ) {
  
  TIFF *tif;
  unsigned int x, y, j, i, newx, newy;
  IMAGE_CHUNK *primary = NULL;
  IMAGE_CHUNK *scaled_chunk = NULL;
  IMAGE_CHUNK *chunks = NULL;
  int total_chunks;
  char outfile[256];
  int row, column;
  
    primary = (IMAGE_CHUNK*)malloc(sizeof(IMAGE_CHUNK));
    if (!load_image(opts->imagefile, primary)) {
    fprintf(stderr, "Unable to open tiff file %s\n", opts->imagefile);
    exit(1);
  }

  if (opts->worldfile) {
    load_worldfile(opts->worldfile, primary);
  }
  int tiff_dir = 1;
  
  TIFFSetTagExtender(TagExtender);
  for ( j = 0;  primary->w >= opts->tile_width; j++) { 
    if (j > 0) {  /* Don't scale the data on the first pass */
      if(verbose > 1)fprintf(stderr, "Scaling level %d, size is %d\n", j, primary->w);    
      scaled_chunk = scale_data_by_half(primary, 0);
      free(primary->_data_byte);
      free(primary);
      primary = scaled_chunk;
      //primary->_data_byte = scaled_chunk->_data_byte;
    }


    if (verbose >1) fprintf(stderr, "Chunking %dx%d image\n", primary->w, primary->h);  
    
    chunks = make_chunks(primary, opts->tile_width, opts->tile_height);
    
    i = 0;
    row = 0;
    column = 0;
    for (x = 0; x < primary->w; x = x+opts->tile_width) {
      row = 0;
      for (y = 0; y < primary->h; y = y+opts->tile_height) {
        if(verbose > 1)fprintf(stderr, "Writing chunk %d, size is %d\n", i, chunks[i].w);
        bzero(outfile, sizeof(outfile));
        sprintf(outfile, "level%.4d-%.4dx%.4d.tif", j, row, column);
        fprintf(stderr, "Writing to %s\n", outfile);
        tif = TIFFOpen(outfile, "w");
        write_header(tif, &chunks[i], 1);    
        if (opts->encoder == JPEG) {
          TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_JPEG);
          TIFFSetField(tif, TIFFTAG_JPEGQUALITY, opts->quality);
        }
        else if (opts->encoder == LZW) {
          TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        }
        else
          TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);        
        TIFFSetField(tif, TIFFTAG_SUBFILETYPE, OFILETYPE_IMAGE );
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_TILELENGTH, opts->tile_width);
        TIFFSetField(tif, TIFFTAG_TILEWIDTH, opts->tile_width);

        TIFFWriteTile(tif, (tdata_t)(chunks[i]._data_byte), 0, 0, 0, 0);
        TIFFClose(tif);
        free(chunks[i]._data_byte);
        i++;
        row++;
      }
      column ++;
    }
    free (chunks);
  } /* J loop */
  return 0;
}
 
int main (int argc, char *argv[]) {

  options_struct *opts;

  if (argc < 2)
    usage(argv[0], 1);

  opts = get_options(argc, argv);
 
  TIFFSetWarningHandler(NULL);


  if (opts->tile_type == IMAGE) {
    export_image(opts);
  }
  else   if (opts->tile_type == DEM_SINGLE) {
    write_DEM(opts);
  }
//  else   if (opts->tile_type == DEM_MULTI) {
//    write_multi_DEM(opts);
//  }


//  fprintf(stderr, "Original image dimensions: %dx%d\n", primary->w, primary->h);
//  fprintf(stderr, "Total images will be %d\n", total_chunks);


}

