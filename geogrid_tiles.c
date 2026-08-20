/* 
 File:   geogrid_tiles.c
 Author: Jonathan Beezley <jon.beezley.math@gmail.com> 
 Date:   1-18-2010
 
 Functions for converting image data to float and tiling the output.
 Uses write_geogrid.c to write each tile.
 
 Main functions:
 
 void write_index_file(const char *,const GeogridIndex)
   Writes geogrid metadata to a file.
 
 void write_tile(int,int,const GeogridIndex,float*)
   Extracts information from a GeogridIndex structure to 
   construct tiles from global buffer.
 
 float *alloc_tile_buffer(const GeogridIndex)
   Allocates the global data buffer using malloc.
 
 void process_buffer_f(const GeogridIndex,float*)
   Do any processing of the buffer (i.e. filling in missing values) 
   before writing to file.
 
 Utility functions:
 
 void get_tile_from_f(int,int,const GeogridIndex,const float*,float*)
   Wrapper for write_geogrid unpacking items from a GeogridIndex
   structure for arguments.
 
 */

#include "geogrid_tiles.h"
#include "write_geogrid.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


/* Writes geogrid metadata to a file. */
void write_index_file(
  const char *filename,  /* file name to write index file (typically "index") */
  const GeogridIndex idx /* initialized GeogridIndex data structure. */
                            ) {
  FILE *f;
  char buf[STRING_LENGTH];
  
  f=fopen(filename,"w");
  
  /*
   * Each projection takes different parameters.  These taken from 
   * module_map_utils.F
   */
  
  switch (idx.proj) {
    case lambert: /* Lambert Conformal (geogrid code = PROJ_LC) */
      fprintf(f,"projection = %s\n","lambert");
      fprintf(f,"truelat1 = %f\n",idx.truelat1);
      fprintf(f,"truelat2 = %f\n",idx.truelat2);
      fprintf(f,"stdlon = %f\n",idx.stdlon);
      fprintf(f,"known_x = %i\n",idx.known_x);
      fprintf(f,"known_y = %i\n",idx.known_y);
      fprintf(f,"known_lat = %f\n",idx.known_lat);
      fprintf(f,"known_lon = %f\n",idx.known_lon);
      fprintf(f,"dx = %e\n",idx.dx);
      fprintf(f,"dy = %e\n",idx.dy);
      break;
    case polar: /* Polar Stereographic (geogrid code = PROJ_PS) */
      fprintf(f,"projection = %s\n","polar");
      fprintf(f,"truelat1 = %f\n",idx.truelat1);
      fprintf(f,"stdlon = %f\n",idx.stdlon);
      fprintf(f,"known_x = %i\n",idx.known_x);
      fprintf(f,"known_y = %i\n",idx.known_y);
      fprintf(f,"known_lat = %f\n",idx.known_lat);
      fprintf(f,"known_lon = %f\n",idx.known_lon);
      fprintf(f,"dx = %e\n",idx.dx);
      fprintf(f,"dy = %e\n",idx.dy);
      break;
    case mercator: /* Mercator (geogrid code = PROJ_MERC) */
      fprintf(f,"projection = %s\n","mercator");
      fprintf(f,"truelat1 = %f\n",idx.truelat1);
      fprintf(f,"stdlon = %f\n",idx.stdlon);
      fprintf(f,"known_x = %i\n",idx.known_x);
      fprintf(f,"known_y = %i\n",idx.known_y);
      fprintf(f,"known_lat = %f\n",idx.known_lat);
      fprintf(f,"known_lon = %f\n",idx.known_lon);
      fprintf(f,"dx = %e\n",idx.dx);
      fprintf(f,"dy = %e\n",idx.dy);
      break;
    case regular_ll: /* Cylindrical (geographic) Lat/Lon (geogrid code = PROJ_LATLON) */
      fprintf(f,"projection = %s\n","regular_ll");
      fprintf(f,"known_x = %i\n",idx.known_x);
      fprintf(f,"known_y = %i\n",idx.known_y);
      fprintf(f,"known_lat = %f\n",idx.known_lat);
      fprintf(f,"known_lon = %f\n",idx.known_lon);
      fprintf(f,"dx = %e\n",idx.dx);
      fprintf(f,"dy = %e\n",idx.dy);
      break;
      /* unsupported for now: */
      /*
    case polar_wgs84: 
      fprintf(f,"projection = %s\n","polar_wgs84");
      fprintf(f,"truelat1 = %f\n",idx.truelat1);
      fprintf(f,"stdlon = %f\n",idx.stdlon);
      fprintf(f,"known_x = %i\n",idx.known_x);
      fprintf(f,"known_y = %i\n",idx.known_y);
      fprintf(f,"known_lat = %f\n",idx.known_lat);
      fprintf(f,"known_lon = %f\n",idx.known_lon);
      fprintf(f,"dx = %f\n",idx.dx);
      break;*/
    case albers_nad83: /* Albers Equal Area Conic (geogrid code PROJ_ALBERS_NAD83) */
      fprintf(f,"projection = %s\n","albers_nad83");
      fprintf(f,"truelat1 = %f\n",idx.truelat1);
      fprintf(f,"truelat2 = %f\n",idx.truelat2);
      fprintf(f,"stdlon = %f\n",idx.stdlon);
      fprintf(f,"known_x = %i\n",idx.known_x);
      fprintf(f,"known_y = %i\n",idx.known_y);
      fprintf(f,"known_lat = %f\n",idx.known_lat);
      fprintf(f,"known_lon = %f\n",idx.known_lon);
      fprintf(f,"dx = %e\n",idx.dx);
      fprintf(f,"dy = %e\n",idx.dy);
      break;
    default:
      fprintf(stderr,"Invalid project.");
      exit(EXIT_FAILURE);
      break;
  }
  
  /* common parameters */
  
  if (idx.categorical) strcpy(buf,"categorical");
  else strcpy(buf,"continuous");
  
  fprintf(f,"type = %s\n",buf);
  
  if (idx.isigned) strcpy(buf,"yes");
  else strcpy(buf,"no");
  
  fprintf(f,"signed = %s\n",buf);
  fprintf(f,"units = %s\n",idx.units);
  fprintf(f,"description = %s\n",idx.description);
  fprintf(f,"wordsize = %i\n",idx.wordsize);
  fprintf(f,"tile_x = %i\n",idx.tx);
  fprintf(f,"tile_y = %i\n",idx.ty);
  
  if (idx.nz > 0) {
    fprintf(f,"tile_z = %i\n",idx.nz);
  }
  else {
    fprintf(f,"tile_z_start = %i\n",idx.tz_s);
    fprintf(f,"tile_z_end = %i\n",idx.tz_e);
  }
  
  if (idx.categorical) {
    fprintf(f,"category_min = %i\n",idx.cat_min);
    fprintf(f,"category_max = %i\n",idx.cat_max);
  }
  
  fprintf(f,"tile_bdr = %i\n",idx.tile_bdr);
  fprintf(f,"missing_value = %f\n",idx.missing);
  fprintf(f,"scale_factor = %f\n",idx.scalefactor);
  
  fprintf(f,"row_order = bottom_top\n");
  //if (idx.bottom_top) fprintf(f,"row_order = bottom_top\n");
  //else fprintf(f,"row_order = top_bottom\n");
  
  if (idx.endian)
    fprintf(f,"endian = little\n");
  else 
    fprintf(f,"endian = big\n");
  
  fclose(f);

}

/* Extracts information from a GeogridIndex structure to 
   construct tiles from global buffer. */
void write_tile(
  int itile_x,            /* tile column number */
  int itile_y,            /* tile row number */
  const GeogridIndex idx, /* initialized index structure for metadata */
  float *arr              /* tile data buffer */
                )
{
  int itx,ity,isgn,endian,nx,ny,nz;
  
  /* get global index for construction tile file name */
  itx=itile_x*idx.tx+1;
  ity=itile_y*idx.ty+1;
  
  /* get tile bounds including boarder */
  nx=idx.tx+2*idx.tile_bdr;
  ny=idx.ty+2*idx.tile_bdr;
  nz=nzsize(idx);
  
  /* unpack other meta data needed by write_geogrid */
  if (idx.isigned) isgn=1;
  else isgn=0;
  if (idx.endian) endian=1;
  else endian=0;
  
  /* call write_geogrid to write tile to file */
  write_geogrid(arr,&nx,&itx,&ny,&ity,&nz,&idx.tile_bdr,&isgn,&endian,
                &idx.scalefactor,&idx.wordsize);
}

/* get the number of tiles in a row or column */
int ntiles(int n, int t)
{
  return ( ceil( (double) n / (double) t ) );
}

int nxtiles(const GeogridIndex idx) {
  return (ntiles(idx.nx,idx.tx));
}

int nytiles(const GeogridIndex idx) {
  return (ntiles(idx.ny,idx.ty));
}

/* get the number of vertical levels */
int nzsize(const GeogridIndex idx) {
  if (idx.nz <= 0)
    return (idx.tz_e - idx.tz_s + 1);
  else
    return (idx.nz);
}

/* get the global index of the first element of a tile (including border)
   the index returned might be < 0 or > the global image size, due to the
   border, the calling routine should should set invalid indices to missing.
   Widened to `long` since idx.nx*idx.ny can exceed INT_MAX for large rasters. */
long gettilestart(
  int itile_x,             /* tile column number */
  int itile_y,             /* tile row number */
  const GeogridIndex idx   /* index structure */
                 ) {
  long sx,                 /* global column number */
       sy;                 /* global row number */
  sx=(long)itile_x * idx.tx - idx.tile_bdr;
  sy=(long)itile_y * idx.ty - idx.tile_bdr;
  return (sy * (long)idx.nx + sx);
}

/* get global strides */
long globalystride(const GeogridIndex idx) {
  return ((long)idx.nx);
}

long globalzstride(const GeogridIndex idx) {
  return ((long)idx.nx*idx.ny);
}

/* allocate a buffer for the tiles */
float* alloc_tile_buffer(const GeogridIndex idx) {
  size_t n = (size_t)(idx.tx + 2*idx.tile_bdr)
           * (size_t)(idx.ty + 2*idx.tile_bdr)
           * (size_t)nzsize(idx);
  float *arr=malloc(n * sizeof(float));
  return (arr);
}

/* Get the requested tile from the global (float) buffer.
   `nimg`/`i0`/`i1` are `long` since idx.nx*idx.ny*nz can exceed INT_MAX
   for large rasters -- this used to be triplicated (float/double/int) via
   the _CONV_BUF macro, but only the float path was ever called, so the
   double/int variants (get_tile_from_d, get_tile_from_i) were removed. */
void get_tile_from_f(
  int itile_x,int itile_y,  /* tile column/row */
  const GeogridIndex idx,   /* index structure */
  const float *databuf,     /* global data buffer (float, no casting) */
  float *tile               /* tile data buffer */
                     ) {
  const float *gptr;
  float *tptr;
  int z,y,x;
  long i0,i1,nimg;

  tptr=tile;
  i0=gettilestart(itile_x,itile_y,idx);
  nimg=(long)idx.nx*idx.ny*nzsize(idx);
  for(z=0;z<nzsize(idx);z++) {
    i1=i0;
    for(y=-idx.tile_bdr;y<idx.ty+idx.tile_bdr;y++) {
      gptr=&(databuf[i1]);
      for(x=-idx.tile_bdr;x<idx.tx+idx.tile_bdr;x++) {
        if(i1+x >= nimg || i1+x < 0){
          *tptr++ = idx.missing;
          gptr++;
        }
        else
          *tptr++ = (float) *gptr++;
      }
      i1+=globalystride(idx);
    }
    i0+=globalzstride(idx);
  }
}

/* Write all tiles to disk.
   Used to be generated via the _CONV_FILE(get_tile_from_?) macro alongside
   convert_from_d/convert_from_i, which were dead code -- only this float
   path is ever called from convert_geotiff.c. */
void convert_from_f(
                    const GeogridIndex idx, /* index structure */
                    const float *databuf    /* global data buffer (float) */
                    ) {
  int itile_x,itile_y;
  float *tile;
  tile=alloc_tile_buffer(idx);
  for(itile_y=0;itile_y<nytiles(idx);itile_y++) {
    for(itile_x=0;itile_x<nxtiles(idx);itile_x++) {
      if(GEO_DEBUG)
        set_tile_to(tile,idx,itile_x,itile_y);
      else
        get_tile_from_f(itile_x,itile_y,idx,databuf,tile);
      write_tile(itile_x,itile_y,idx,tile);
    }
  }
  free(tile);
}

/*  Do any processing of the buffer (i.e. filling in missing values) 
    before writing to file. */
void process_buffer_f(
                      const GeogridIndex idx, /* index structure */
                      float *databuf          /* global data buffer (float) */
                      ) {
  long i;
  float *ptr;
  if (idx.categorical) {                  /* for categorical fields... */
    for(i=0;i<(long)idx.nx*idx.ny*idx.nz;i++) { /* loop over all pixels */
      ptr=databuf++;
      if( (float)(int) *ptr != *ptr ||    /* set any values not in a valid range */
          *ptr > idx.cat_max        ||    /* to the missing value */
          *ptr < idx.cat_min)
        *ptr=idx.missing;
    }
  }
}

void set_tile_to(float* tile,const GeogridIndex idx,int itile_x,int itile_y) {
  int n,i;
  float v0;
  n=(idx.tx + 2*idx.tile_bdr) * (idx.ty + 2*idx.tile_bdr) * nzsize(idx);
  v0=itile_x + itile_y * 10;
  fprintf(stdout,"tile (%i,%i) set to %i\n",itile_x,itile_y,(int) v0);
  for(i=0;i<n;i++) tile[i]=v0;
}
