/* 
 File:   process_geotiff_decompose.h
 Author: Dominik Strebel <dominik.strebel@gmail.com> 
 Date:   2018-02-22
 
 
 */


#ifndef _PROCESS_GEOTIFF_DECOMPOSE_H
#define _PROCESS_GEOTIFF_DECOMPOSE_H

#include "geogrid_index.h"

#ifdef RELATIVE_GTIFF
#include <geotiff/geotiffio.h>
#include <geotiff/xtiffio.h>
#include <geotiff/geo_normalize.h>
#else
#include <geotiffio.h>
#include <xtiffio.h>
#include <geo_normalize.h>
#endif
#include <tiffio.h>

extern const int BIGENDIAN_TEST_VAL;
#define IS_BIGENDIAN() ( (*(char*)&BIGENDIAN_TEST_VAL) == 0 )

#ifdef __cplusplus
extern "C" {
#endif
    int processGeoTIFF(TIFF *file, uint32 stripMax, tsize_t stripSize, GeogridIndex idx);
    int processGeoTIFFreverse(TIFF *file, uint32 stripMax, tsize_t stripSizem, GeogridIndex idx);
    float* read_multiple_row_strip(TIFF *file, const int stripCount, const tsize_t stripSize,  const int startStrip, const GeogridIndex *idx);
    float* read_single_row_strip(TIFF *file, const int stripCount, const tsize_t stripSize, const GeogridIndex *idx);
#ifdef __cplusplus
}
#endif


#endif