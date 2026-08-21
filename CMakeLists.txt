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

add_library(convert_geotiff_lib STATIC
  src/geogrid_index.cpp
  src/geotiff_reader.cpp
  src/geogrid_tile_writer.cpp
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

install(TARGETS convert_geotiff RUNTIME DESTINATION bin)

enable_testing()
add_test(
  NAME projections
  COMMAND ${CMAKE_COMMAND} -E env exe=$<TARGET_FILE:convert_geotiff>
          bash ${CMAKE_SOURCE_DIR}/test/runtest.sh
)
set_tests_properties(projections PROPERTIES
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test
)
