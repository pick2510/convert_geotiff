// Debug harness that exercises tile naming/writing without needing a real
// TIFF input: each tile is filled with a constant computed from its
// (column, row) index instead of being read from a raster.

#include "convert_geotiff/geogrid_index.hpp"
#include "convert_geotiff/geogrid_tile_writer.hpp"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace convert_geotiff;

namespace {

void print_usage(FILE *f, const char *name) {
  std::fprintf(f, "Usage: %s [OPTIONS]\n", name);
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

// Fills a tile buffer with a constant derived from its tile coordinates, in
// place of reading real data -- used only to exercise tile naming/writing.
std::vector<float> debug_tile(const GeogridTileWriter &writer, const GeogridIndex &idx,
                               int itile_x, int itile_y) {
  const size_t n = static_cast<size_t>(idx.tx + 2 * idx.tile_bdr) *
                    (idx.ty + 2 * idx.tile_bdr) * writer.nz_size();
  const float v0 = static_cast<float>(itile_x + itile_y * 10);
  std::fprintf(stdout, "tile (%i,%i) set to %i\n", itile_x, itile_y, static_cast<int>(v0));
  return std::vector<float>(n, v0);
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
            (word_size != 1 && word_size != 2 && word_size != 3 && word_size != 4)) {
          std::fprintf(stderr, "Invalid argument to -w.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 'z':
        isigned = 0;
        break;
      case 't':
        if (std::sscanf(optarg, "%i", &tile_size) != 1 || tile_size <= 0) {
          std::fprintf(stderr, "Invalid argument to -t.\n");
          print_usage(stderr, argv[0]);
          return EXIT_FAILURE;
        }
        break;
      case 's':
        // The original tester.c rejected any *nonzero* -s value here
        // (`scale != 0.`), the inverse of convert_geotiff.c's check --
        // clearly an unintentional copy/paste inversion in this
        // never-built debug tool, not preserved here.
        if (std::sscanf(optarg, "%f", &scale) != 1) {
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

  if (optind != argc) {
    std::fprintf(stderr, "No positional arguments used.\n");
    print_usage(stderr, argv[0]);
    return EXIT_FAILURE;
  }

  GeogridIndex idx;
  idx.nx = 128;
  idx.ny = 256;
  idx.nz = 1;

  idx.description = description;
  idx.units = units;
  idx.missing = missing;
  if (categorical_range) {
    idx.categorical = true;
    idx.cat_max = categorical_range;
    idx.cat_min = 1;
    idx.missing = 0.f;
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

  GeogridTileWriter writer(idx);
  for (int itile_y = 0; itile_y < writer.ny_tiles(); ++itile_y) {
    for (int itile_x = 0; itile_x < writer.nx_tiles(); ++itile_x) {
      std::vector<float> tile = debug_tile(writer, idx, itile_x, itile_y);
      writer.write_tile(itile_x, itile_y, tile);
    }
  }

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
