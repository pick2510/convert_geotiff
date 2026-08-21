cmake_minimum_required(VERSION 3.16)
project(convert_geotiff VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_library(TIFF_LIBRARY NAMES tiff)
find_path(TIFF_INCLUDE_DIR NAMES tiffio.h)
if(NOT TIFF_LIBRARY OR NOT TIFF_INCLUDE_DIR)
  message(FATAL_ERROR "Cannot find TIFF library/headers. Try setting CMAKE_PREFIX_PATH.")
endif()

find_library(GEOTIFF_LIBRARY NAMES geotiff)
find_path(GEOTIFF_INCLUDE_DIR NAMES geotiffio.h PATH_SUFFIXES geotiff libgeotiff)
if(NOT GEOTIFF_LIBRARY OR NOT GEOTIFF_INCLUDE_DIR)
  message(FATAL_ERROR "Cannot find GEOTIFF library/headers. Try setting CMAKE_PREFIX_PATH.")
endif()

find_library(M_LIBRARY NAMES m)

option(BUILD_GUI "Build the FLTK-based convert_geotiff_gui application" ON)

add_library(convert_geotiff_lib STATIC
  src/geogrid_index.cpp
  src/geotiff_reader.cpp
  src/geogrid_tile_writer.cpp
  src/convert.cpp
  src/geogrid_reader.cpp
  src/geotiff_writer.cpp
  src/convert_back.cpp
)
target_include_directories(convert_geotiff_lib PUBLIC
  include
  ${TIFF_INCLUDE_DIR}
  ${GEOTIFF_INCLUDE_DIR}
)
target_link_libraries(convert_geotiff_lib PUBLIC ${TIFF_LIBRARY} ${GEOTIFF_LIBRARY})
if(M_LIBRARY)
  target_link_libraries(convert_geotiff_lib PUBLIC ${M_LIBRARY})
endif()
target_compile_options(convert_geotiff_lib PRIVATE -Wall -Wextra)

add_executable(convert_geotiff src/convert_geotiff.cpp)
target_link_libraries(convert_geotiff PRIVATE convert_geotiff_lib)
target_compile_options(convert_geotiff PRIVATE -Wall -Wextra)

add_executable(tester src/tester.cpp)
target_link_libraries(tester PRIVATE convert_geotiff_lib)
target_compile_options(tester PRIVATE -Wall -Wextra)

add_executable(geogrid_to_tiff src/geogrid_to_tiff.cpp)
target_link_libraries(geogrid_to_tiff PRIVATE convert_geotiff_lib)
target_compile_options(geogrid_to_tiff PRIVATE -Wall -Wextra)

install(TARGETS convert_geotiff geogrid_to_tiff RUNTIME DESTINATION bin)

if(BUILD_GUI)
  if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/fltk/CMakeLists.txt")
    message(FATAL_ERROR
      "third_party/fltk is empty. Run 'git submodule update --init --recursive' "
      "or configure with -DBUILD_GUI=OFF.")
  endif()

  set(FLTK_BUILD_TEST OFF CACHE BOOL "" FORCE)
  set(FLTK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(FLTK_BUILD_FLUID OFF CACHE BOOL "" FORCE)
  set(FLTK_BUILD_FORMS OFF CACHE BOOL "" FORCE)
  set(FLTK_BUILD_FLTK_OPTIONS OFF CACHE BOOL "" FORCE)
  set(FLTK_BUILD_GL OFF CACHE BOOL "" FORCE)
  set(FLTK_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  # The Wayland backend forces FLTK_USE_PANGO on, which in turn requires
  # pangoxft -- a legacy Pango backend that many current distros (e.g.
  # Fedora) no longer package. Build X11-only to avoid that dependency;
  # FLTK's X11 backend still works fine under XWayland.
  set(FLTK_BACKEND_WAYLAND OFF CACHE BOOL "" FORCE)
  add_subdirectory(third_party/fltk)

  find_package(Threads REQUIRED)

  add_executable(convert_geotiff_gui gui/main.cpp gui/convert_window.cpp)
  target_link_libraries(convert_geotiff_gui PRIVATE convert_geotiff_lib fltk::fltk Threads::Threads)
  target_compile_options(convert_geotiff_gui PRIVATE -Wall -Wextra)
endif()

enable_testing()
add_test(
  NAME projections
  COMMAND ${CMAKE_COMMAND} -E env exe=$<TARGET_FILE:convert_geotiff>
          bash ${CMAKE_SOURCE_DIR}/test/runtest.sh
)
set_tests_properties(projections PROPERTIES
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
)

add_test(
  NAME roundtrip
  COMMAND ${CMAKE_COMMAND} -E env
          cg=$<TARGET_FILE:convert_geotiff>
          gt=$<TARGET_FILE:geogrid_to_tiff>
          bash ${CMAKE_SOURCE_DIR}/test/roundtrip.sh
)
set_tests_properties(roundtrip PROPERTIES
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
)
