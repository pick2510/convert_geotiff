/*
 File:   read_geotiff_decompose.c
 Author: Dominik Strebel <dominik.strebel@gmail.com>
 Date:   2018-02-22

 Functions for reading geotiff files decomposed to strips
*/

#include "process_geotiff_decompose.h"
#include "read_geotiff.h"
#include "geogrid_tiles.h"

#ifdef RELATIVE_GTIFF
#include <geotiff/geo_tiffp.h>
#else
#include <geo_tiffp.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common buffer conversion code for different data formats */
#define CONV_CHAR_BUFFER_TO(_DTYPE, _DVAR)                               \
  bufferasfloat = (float*)alloc_buffer(inx * iny * inz * sizeof(float)); \
  if (bufferasfloat == NULL) {                                           \
    printf("Null pointer catched!");                                     \
    exit(-1);                                                            \
  }                                                                      \
  for (i = 0; i < inx * iny * inz; i++) {                                \
    j = i * idx->bytes_per_sample;                                       \
    _DVAR = *(_DTYPE*)(buffer + j);                                      \
    bufferasfloat[i] = (float)_DVAR;                                     \
  }                                                                      \
  free_buffer(buffer);

#define CONV_CHAR_BUFFER_STRIP_TO(_DTYPE, _DVAR)                   \
  bufferasfloat = (float*)alloc_buffer(stripSize * sizeof(float)); \
  if (bufferasfloat == NULL) {                                     \
    printf("Null pointer catched!");                               \
    exit(-1);                                                      \
  }                                                                \
  for (i = 0; i < stripSize; i++) {                                \
    j = i * idx->bytes_per_sample;                                 \
    _DVAR = *(_DTYPE*)(buffer + j);                                \
    bufferasfloat[i] = (float)_DVAR;                               \
  }                                                                \
  free_buffer(buffer);

int processGeoTIFFreverse(TIFF* file, uint32 stripMax, tsize_t stripSize,
                          GeogridIndex idx) {
  unsigned int stripPerTile = idx.ty;
  float* buffer;
  int remainderOfReadOperations, i;
  int flIncompleteLastTile = 0;
  long int readOperations, currentStrip=0;
  remainderOfReadOperations = stripMax % stripPerTile;
  if (remainderOfReadOperations == 0) {
    readOperations = stripMax / stripPerTile;
  } else {
    flIncompleteLastTile = 1;
    readOperations = stripMax / stripPerTile + 1;
  }
  for (i = 0; i < readOperations; i++) {
    buffer = read_multiple_row_strip(file, stripPerTile, stripSize, i, &idx, &currentStrip);
    process_buffer_multirow_strip(&idx, buffer, stripPerTile);
    convert_from_f_strip_reverse(&idx, buffer, currentStrip, stripMax);
    free_buffer((unsigned char*)buffer);
  }
  printf("Done\n");
}

float* read_multiple_row_strip(TIFF* file, const int stripCount,
                               const tsize_t stripSize, const int startStrip,
                               const GeogridIndex* idx, long int* currentStrip) {
  unsigned long int bufsize, offset = 0;
  float* bufferasfloat;
  double dtemp;
  unsigned char* buffer;
  int result, i, j;
  int inx, iny, inz;
  uint8 iutemp8;
  uint16 iutemp16;
  uint32 iutemp32;
  int8 itemp8;
  int16 itemp16;
  int32 itemp32;
  bufsize = ((unsigned long)idx->samples_per_pixel) *
            ((unsigned long)idx->bytes_per_sample) *
            ((unsigned long)stripSize) * ((unsigned long)stripCount);
  buffer = alloc_buffer(bufsize);
  if (buffer == NULL) {
    fprintf(stderr, "Couldn't allocate buffer, mem full?");
  }
  for (i = 0; i < stripCount; i++) {
    if ((result = TIFFReadEncodedStrip(file, stripCount, buffer + offset,
                                       stripSize) == -1)) {
      fprintf(stderr, "Read error on input strip number %u\n", stripCount);
      exit(EXIT_FAILURE);
    }
    offset += result;
  }
  *currentStrip += stripCount;
  inx = idx->nx;
  iny = stripCount;
  inz = idx->nz;
  switch (idx->sample_format) {
    case SAMPLEFORMAT_UINT:
      switch (idx->bytes_per_sample) {
        case 1:
          CONV_CHAR_BUFFER_TO(uint8, iutemp8)
          break;
        case 2:
          CONV_CHAR_BUFFER_TO(uint16, iutemp16)
          break;
        case 4:
          CONV_CHAR_BUFFER_TO(uint32, iutemp32)
          break;
        default:
          fprintf(stderr, "Unsupported bytes per sample=%i for uint.\n",
                  idx->bytes_per_sample);
          exit(EXIT_FAILURE);
      }
      break;
    case SAMPLEFORMAT_INT:
      switch (idx->bytes_per_sample) {
        case 1:
          CONV_CHAR_BUFFER_TO(int8, itemp8)
          break;
        case 2:
          CONV_CHAR_BUFFER_TO(int16, itemp16)
          break;
        case 4:
          CONV_CHAR_BUFFER_TO(int32, itemp32)
          break;
        default:
          fprintf(stderr, "Unsupported bytes per sample=%i for int.\n",
                  idx->bytes_per_sample);
          exit(EXIT_FAILURE);
      }
      break;
    case SAMPLEFORMAT_IEEEFP:
      switch (idx->bytes_per_sample) {
        case sizeof(float):
          /* no conversion needs to be done if image is single precision float
           */
          bufferasfloat = (float*)buffer;
          break;
        case sizeof(double):
          CONV_CHAR_BUFFER_TO(double, dtemp)
          break;
        default:
          fprintf(stderr, "Unsupported bytes per sample=%i for IEEEFP.\n",
                  idx->bytes_per_sample);
          exit(EXIT_FAILURE);
      }
      break;
    default:
      fprintf(stderr, "Unsupported data type in image.\n");
      exit(EXIT_FAILURE);
  }

  return bufferasfloat;
}

float* read_single_row_strip(TIFF* file, const int stripCount,
                             const tsize_t stripSize, const GeogridIndex* idx) {
  unsigned long int bufsize;
  float* bufferasfloat;
  double dtemp;
  unsigned char* buffer;
  int result, i, j;
  uint8 iutemp8;
  uint16 iutemp16;
  uint32 iutemp32;
  int8 itemp8;
  int16 itemp16;
  int32 itemp32;
  bufsize = ((unsigned long)idx->samples_per_pixel) *
            ((unsigned long)idx->bytes_per_sample) * ((unsigned long)stripSize);
  buffer = alloc_buffer(bufsize);
  if (buffer == NULL) {
    fprintf(stderr, "Couldn't allocate buffer, mem full?");
  }

  if ((result = TIFFReadEncodedStrip(file, stripCount, buffer, stripSize)) ==
      -1) {
    fprintf(stderr, "Read error on input strip number %u\n", stripCount);
    exit(EXIT_FAILURE);
  }
  switch (idx->sample_format) {
    case SAMPLEFORMAT_UINT:
      switch (idx->bytes_per_sample) {
        case 1:
          CONV_CHAR_BUFFER_STRIP_TO(uint8, iutemp8)
          break;
        case 2:
          CONV_CHAR_BUFFER_STRIP_TO(uint16, iutemp16)
          break;
        case 4:
          CONV_CHAR_BUFFER_STRIP_TO(uint32, iutemp32)
          break;
        default:
          fprintf(stderr, "Unsupported bytes per sample=%i for uint.\n",
                  idx->bytes_per_sample);
          exit(EXIT_FAILURE);
      }
      break;
    case SAMPLEFORMAT_INT:
      switch (idx->bytes_per_sample) {
        case 1:
          CONV_CHAR_BUFFER_STRIP_TO(int8, itemp8)
          break;
        case 2:
          CONV_CHAR_BUFFER_STRIP_TO(int16, itemp16)
          break;
        case 4:
          CONV_CHAR_BUFFER_STRIP_TO(int32, itemp32)
          break;
        default:
          fprintf(stderr, "Unsupported bytes per sample=%i for int.\n",
                  idx->bytes_per_sample);
          exit(EXIT_FAILURE);
      }
      break;
    case SAMPLEFORMAT_IEEEFP:
      switch (idx->bytes_per_sample) {
        case sizeof(float):
          /* no conversion needs to be done if image is single precision float
           */
          bufferasfloat = (float*)buffer;
          break;
        case sizeof(double):
          CONV_CHAR_BUFFER_STRIP_TO(double, dtemp)
          break;
        default:
          fprintf(stderr, "Unsupported bytes per sample=%i for IEEEFP.\n",
                  idx->bytes_per_sample);
          exit(EXIT_FAILURE);
      }
      break;
    default:
      fprintf(stderr, "Unsupported data type in image.\n");
      exit(EXIT_FAILURE);
  }

  return bufferasfloat;
}
