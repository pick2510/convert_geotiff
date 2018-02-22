/* 
 File:   geogrid_tiles.h
 Author: Jonathan Beezley <jon.beezley.math@gmail.com> 
 Date:   1-18-2010
 
 See geogrid_tiles.c for documentation.
 
 */

#ifndef _GEOGRID_TILES_H
#define _GEOGRID_TILES_H

#include "geogrid_index.h"

#ifdef __cplusplus
extern "C" {
#endif

  void write_index_file(const char *,const GeogridIndex);
  void write_tile(int,int,const GeogridIndex,float*);
  int ntiles(int,int);
  int nxtiles(const GeogridIndex);
  int nytiles(const GeogridIndex);
  int nzsize(const GeogridIndex);
  int gettilestart(int,int,const GeogridIndex);
  int globalystride(const GeogridIndex);
  int globalzstride(const GeogridIndex);
  int globalzstride_decomposed(const GeogridIndex* idx);
  float *alloc_tile_buffer(const GeogridIndex);
  float *alloc_tile_buffer_decomposed(const GeogridIndex *idx);
  void get_tile_from_f(int,int,const GeogridIndex,const float*,float*);
  void get_tile_from_f_decomposed( int itile_x,int itile_y,const GeogridIndex idx, const float *databuf, float *tile);
  void convert_from_f(const GeogridIndex,const float*);
  void convert_from_f_strip_reverse(const GeogridIndex *idx, const float *buffer, const long int currentStrip, const tsize_t maxStrip);
  void process_buffer_f(const GeogridIndex,float*);
  void process_buffer_multirow_strip(const GeogridIndex* idx, float* buffer, const int rows);
  void process_buffer_strip(const GeogridIndex* idx, float* buffer, tsize_t stripSize);
  void set_tile_to(float*,const GeogridIndex,int,int);

#ifdef __cplusplus
}
#endif
  
#endif