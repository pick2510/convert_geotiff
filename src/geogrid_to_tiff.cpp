// Main program for converting a geogrid (WPS static-data) directory back
// into a GeoTIFF file -- the reverse of convert_geotiff.
//
// Usage: geogrid_to_tiff [-h] GEOGRID_DIR OUTPUT.tif
//
// Unlike convert_geotiff, no options are needed: everything required is
// already recorded in GEOGRID_DIR/index.

#include "convert_geotiff/convert_back.hpp"
#include "convert_geotiff/geogrid_index.hpp"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace convert_geotiff;

namespace {

void print_usage(FILE *f, const char *name) {
  std::fprintf(f, "Usage: %s [-h] GEOGRID_DIR OUTPUT.tif\n", name);
  std::fprintf(f, "\n");
  std::fprintf(f, "Converts a geogrid (WPS static-data) directory back into a GeoTIFF file.\n");
  std::fprintf(f, "GEOGRID_DIR must contain an `index' file plus its binary data tiles, as\n");
  std::fprintf(f, "produced by convert_geotiff.\n");
  std::fprintf(f, "\n");
  std::fprintf(f, "Options:\n");
  std::fprintf(f, "-h : Show this help message and exit\n");
}

int run(int argc, char *argv[]) {
  int c;
  while ((c = getopt(argc, argv, "h")) != -1) {
    switch (c) {
      case 'h':
        print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
      default:
        print_usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }
  }

  if (argc - optind != 2) {
    std::fprintf(stderr, "Expected GEOGRID_DIR and OUTPUT.tif.\n");
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  const std::string geogrid_dir = argv[optind];
  const std::string output_tiff = argv[optind + 1];

  convert_geotiff::convert_back(geogrid_dir, output_tiff);

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
