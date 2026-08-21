// Main program for converting GeoTIFF files into binary geogrid format.
// This program is NOT capable of converting between different projections.
// The projection in the GeoTIFF file must be supported by WPS. Geogrid
// files can differ dramatically from source to source... GeoTiffFile
// attempts to account for these differences and check for sanity of the
// output; however, one should always compare the index file created with
// the output of listgeo to ensure the conversion has been done correctly.
//
// WARNING: This program reads the entire data set into memory all at once.

#include "convert_geotiff/geogrid_index.hpp"
#include "convert_geotiff/geogrid_tile_writer.hpp"
#include "convert_geotiff/geotiff_reader.hpp"

#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace convert_geotiff;

namespace {

void print_usage(FILE *f, const char *name) {
  std::fprintf(f, "Usage: %s [OPTIONS] FileName\n", name);
  std::fprintf(f, "\n");
  std::fprintf(f, "Converts geotiff file `FileName' into geogrid binary format\n");
  std::fprintf(f, "into the current directory.\n");
  std::fprintf(f, "\n");
  std::fprintf(f, "Options:\n");
  std::fprintf(f, "-h         : Show this help message and exit\n");
  std::fprintf(f, "-c NUM     : Indicates categorical data (NUM = number of categories)\n");
  std::fprintf(f, "-b NUM     : Tile border width (default 3)\n");
  std::fprintf(f, "-w [1,2,4] : Word size in output in bytes (default 2)\n");
  std::fprintf(f, "-z         : Indicates unsigned data (default FALSE)\n");
  std::fprintf(f, "-t NUM     : Output tile size (default 100)\n");
  std::fprintf(f, "-s SCALE   : Scale factor in output (default 1.)\n");
  std::fprintf(f, "-m MISSING : Missing value in output (default 0., ignored for categorical data)\n");
  std::fprintf(f, "-u UNITS   : Units of the data (default \"NO UNITS\")\n");
  std::fprintf(f, "-d DESC    : Description of data set (default \"NO DESCRIPTION\")\n");
}

int run(int argc, char *argv[]) {
  int categorical_range = 0;
  int border_width = 3;
  int word_size = 2;
  int isigned = 1;
  int tile_size = 100;
  float scale = 1.f;
  float missing = 0.f;
  std::string units = "\"NO UNITS\"";
  std::string description = "\"NO DESCRIPTION\"";

  int c;
  while ((c = getopt(argc, argv, "hzs:c:b:w:t:m:u:d:")) != -1) {
    switch (c) {
      case 'c':
        if (std::sscanf(optarg, "%i", &categorical_range) != 1 || categorical_range <= 0) {
          std::fprintf(stderr, "Invalid argument to -c.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'b':
        if (std::sscanf(optarg, "%i", &border_width) != 1 || border_width < 0) {
          std::fprintf(stderr, "Invalid argument to -b.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'w':
        if (std::sscanf(optarg, "%i", &word_size) != 1 ||
            (word_size != 1 && word_size != 2 && word_size != 4)) {
          std::fprintf(stderr, "Invalid argument to -w.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'z':
        isigned = 0;
        break;
      case 't':
        // Upper-bounded at 99999: tile filenames are fixed 5-digit
        // zero-padded fields, so a larger tile size can never be written.
        if (std::sscanf(optarg, "%i", &tile_size) != 1 || tile_size <= 0 || tile_size > 99999) {
          std::fprintf(stderr, "Invalid argument to -t.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 's':
        if (std::sscanf(optarg, "%f", &scale) != 1 || scale == 0.f) {
          std::fprintf(stderr, "Invalid argument to -s.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'm':
        if (std::sscanf(optarg, "%f", &missing) != 1) {
          std::fprintf(stderr, "Invalid argument to -m.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'u':
        units = std::string("\"") + optarg + "\"";
        break;
      case 'd':
        description = std::string("\"") + optarg + "\"";
        break;
      case 'h':
        print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
      default:
        print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (optind == argc) {
    std::fprintf(stderr, "Missing FileName.\n");
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }
  if (optind < argc - 1) {
    std::fprintf(stderr, "Too many positional arguments.\n");
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  const std::string filename = argv[optind];

  GeoTiffFile file(filename);

  GeogridIndex idx = file.get_index();

  idx.description = description;
  idx.units = units;
  idx.missing = missing;
  if (categorical_range) {
    idx.categorical = true;
    idx.cat_max = categorical_range + 1;
    idx.cat_min = 1;
    idx.missing = static_cast<float>(idx.cat_max);
  } else {
    idx.categorical = false;
  }

  idx.tile_bdr = border_width;
  idx.wordsize = word_size;
  idx.isigned = isigned != 0;
  idx.tx = tile_size;
  idx.ty = tile_size;
  idx.scalefactor = scale;

  if (idx.nx > 99999 - idx.tx || idx.ny > 99999 - idx.ty) {
    throw GeoConvertError("The data set is too large for geogrid format!");
  }

  idx.write_index_file("index");

  std::vector<float> buffer = file.read_buffer();

  if (!idx.bottom_top) {
    for (int i = 0; i < idx.ny / 2; ++i) {
      for (int j = 0; j < idx.nx; ++j) {
        std::swap(buffer[static_cast<size_t>(i) * idx.nx + j],
                  buffer[static_cast<size_t>(idx.ny - i - 1) * idx.nx + j]);
      }
    }
    idx.bottom_top = true;
  }

  process_buffer(idx, buffer);

  GeogridTileWriter writer(idx);
  writer.convert_all(buffer);

  return EXIT_SUCCESS;
}

} // namespace

int main(int argc, char *argv[]) {
  try {
    return run(argc, argv);
  } catch (const GeoConvertError &e) {
    std::fprintf(stderr, "%s\n", e.what());
    return EXIT_FAILURE;
  }
}
