#include "convert_window.hpp"

#include <FL/Fl.H>

int main(int argc, char **argv) {
  Fl::lock(); // enable multithreaded FLTK use before any thread calls Fl::awake()

  ConvertWindow window(640, 755, "convert_geotiff");
  window.show(argc, argv);

  return Fl::run();
}
