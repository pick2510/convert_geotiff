// Main program for converting GeoTIFF files into binary geogrid format.
// This program is NOT capable of converting between different projections.
// The projection in the GeoTIFF file must be supported by WPS. Geogrid
// files can differ dramatically from source to source... GeoTiffFile
// attempts to account for these differences and check for sanity of the
// output; however, one should always compare the index file created with
// the output of listgeo to ensure the conversion has been done correctly.
//
// WARNING: This program reads the entire data set into memory all at once.

#include "convert_geotiff/convert.hpp"
#include "convert_geotiff/geogrid_index.hpp"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>

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
  ConversionOptions opts;
  int isigned = 1;

  int c;
  while ((c = getopt(argc, argv, "hzs:c:b:w:t:m:u:d:")) != -1) {
    switch (c) {
      case 'c':
        if (std::sscanf(optarg, "%i", &opts.categorical_range) != 1 || opts.categorical_range <= 0) {
          std::fprintf(stderr, "Invalid argument to -c.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'b':
        if (std::sscanf(optarg, "%i", &opts.border_width) != 1 || opts.border_width < 0) {
          std::fprintf(stderr, "Invalid argument to -b.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'w':
        if (std::sscanf(optarg, "%i", &opts.word_size) != 1 ||
            (opts.word_size != 1 && opts.word_size != 2 && opts.word_size != 4)) {
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
        if (std::sscanf(optarg, "%i", &opts.tile_size) != 1 || opts.tile_size <= 0 || opts.tile_size > 99999) {
          std::fprintf(stderr, "Invalid argument to -t.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 's':
        if (std::sscanf(optarg, "%f", &opts.scale) != 1 || opts.scale == 0.f) {
          std::fprintf(stderr, "Invalid argument to -s.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'm':
        if (std::sscanf(optarg, "%f", &opts.missing) != 1) {
          std::fprintf(stderr, "Invalid argument to -m.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'u':
        opts.units = std::string("\"") + optarg + "\"";
        break;
      case 'd':
        opts.description = std::string("\"") + optarg + "\"";
        break;
      case 'h':
        print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
      default:
        print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }
  }
  opts.isigned = isigned != 0;

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

  convert_geotiff::convert(filename, opts);

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
