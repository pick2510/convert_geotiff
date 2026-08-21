[![Build Status](https://travis-ci.org/jbeezley/convert_geotiff.png?branch=master)](https://travis-ci.org/jbeezley/convert_geotiff) convert_geotiff
===============

This is a small commandline utility for converting data from GeoTIFF to 
[geogrid format](http://www.mmm.ucar.edu/wrf/users/docs/user_guide_V3/users_guide_chap3.htm#_Writing_Static_Data)
used by [WRF](http://www.mmm.ucar.edu/wrf/users/).  

Getting the prerequisites
-------------------------

This program requires [GeoTIFF](http://trac.osgeo.org/geotiff/) and [LibTIFF](http://www.libtiff.org)
development libraries.  These should be available in most if not all package managers.  To install in Ubuntu,
you just need to install the package <tt>libgeotiff-dev</tt> like this
<pre>sudo apt-get install libgeotiff-dev</pre>
Using [Homebrew](http://brew.sh/) on Mac OSX, 
<pre>brew install libgeotiff</pre>

It is also possible to install the dependencies from source, but you may need to help the configure script below to 
find the libraries by setting configuration variables <tt>CFLAGS="-I$PREFIX/include"</tt> and 
<tt>LDFLAGS="-L$PREFIX/lib"</tt>.

Compiling the source
--------------------

This project uses CMake.  It also builds a small GUI (<tt>convert_geotiff_gui</tt>) on top of
[FLTK](https://www.fltk.org/), which is vendored as a git submodule, so first clone with submodules
(or fetch them afterwards):

<pre>git submodule update --init --recursive</pre>

Then, from the repository root, run

<pre>cmake -B build && cmake --build build</pre>

If everything built correctly, you should now have <tt>build/convert_geotiff</tt> and
<tt>build/convert_geotiff_gui</tt>.  You can either move these files into your <tt>PATH</tt> or run
<tt>cmake --install build</tt> (as root, if installing to a system prefix) to install the CLI.  If
GeoTIFF/LibTIFF are installed in a non-standard location, point CMake at it with
<tt>-DCMAKE_PREFIX_PATH=$PREFIX</tt>.

If you only want the command-line tool, pass <tt>-DBUILD_GUI=OFF</tt> to skip building FLTK and the GUI
entirely (the submodule doesn't even need to be checked out in that case).

To run the test suite, use <tt>ctest --test-dir build</tt>.

Using convert_geotiff
---------------------

The command-line tool is described below.  <tt>convert_geotiff_gui</tt> offers the same options as a
form: pick a GeoTIFF file and an output directory, fill in the fields (they mirror the flags below,
with the same defaults), and click "Convert". The conversion runs in the background with a progress
bar and a log pane for warnings/errors. A "Direction" dropdown at the top switches the GUI to the
reverse conversion described below (<tt>geogrid_to_tiff</tt>), which only needs a geogrid directory
and an output path -- the form-specific options are irrelevant there and get disabled.

Running <tt>convert_geotiff</tt> with no arguments will produce a usage description as follows.
<pre>Usage: convert_geotiff [OPTIONS] FileName

Converts geotiff file `FileName' into geogrid binary format
into the current directory.

Options:
-h         : Show this help message and exit
-c NUM     : Indicates categorical data (NUM = number of categories)
-b NUM     : Tile border width (default 3)
-w [1,2,4] : Word size in output in bytes (default 2)
-z         : Indicates unsigned data (default FALSE)
-t NUM     : Output tile size (default 100)
-s SCALE   : Scale factor in output (default 1.)
-m MISSING : Missing value in output (default 0., ignored for categorical data)
-u UNITS   : Units of the data (default "NO UNITS")
-d DESC    : Description of data set (default "NO DESCRIPTION")
</pre>
All of the files will be created in the current directory, so it is best to run the program from an empty directory.  
A more detailed description of the 
arguments to this program follows.
* <tt>-b</tt>
:<p>The data tiles in the geogrid binary format are allowed to overlap by a fixed number of grid points.  The extra border around the tile is called the halo, and this argument sets the width of the halo.  For instance with a halo of size three, the file named <tt>00101-00200.00051-00100</tt> would actually contain columns 98-203 and rows 48-103 of the full dataset.  This halo is necessary for the interpolation scheme inside of WPS.  The default should be acceptable for most situations.</p>
* <tt>-w</tt>
:<p>The number of bytes to represent each data point as an integer.  These integers are scaled by the scaling parameter before being truncated to an integer. scaledA lower value will make the output data smaller, at the cost of accuracy or the dynamic range of the input.</p>
* <tt> -m</tt>
:<p>Any grid point that is missing data, such as the outer border of the edge tiles, or grid points that the GeoTIFF file indicates as missing will be set to this value.  This argument is currently ignored when the categorical flag is set, instead missing data will be set to the maximum category + 1.</p>
*<tt>-s</tt>
:<p>Because the data is always stored as an integer, a scaling parameter is needed to represent fractional numbers or large values.  The data set will be divided by this number prior to being truncated to an integer.  If the data set has an accuracy of 2 decimal places, a reasonable scale to use would be 0.01.</p>
*<tt>-u, -d</tt>
:<p>The units and a small description of the data set should be included as arguments.  Multi-word arguments should be quoted as follows. <code><pre>-u meters -d "elevation above sea level"</pre></code></p>
* <tt>FileName</tt> 
:<p>The final argument must always be present.  This is the (absolute or relative) path to the GeoTIFF file to be converted.</p>

If you get an error that says something like 
"<tt>error while loading shared libraries: libgeotiff.so</tt>...", 
this means that GeoTIFF was compiled in as a shared library.  You just need to tell the system where to find this library.  This can be done by adding the path to the GeoTIFF library to the environment variable <tt>LD_LIBRARY_PATH</tt>.  For example, 
<pre>export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${PREFIX}/lib</pre>
where <tt>$PREFIX</tt> is the location where you installed GeoTIFF.

Converting back: geogrid_to_tiff
--------------------------------

<tt>geogrid_to_tiff</tt> does the reverse: given a geogrid directory (an <tt>index</tt> file plus its
binary data tiles, as produced by <tt>convert_geotiff</tt>), it reconstructs a GeoTIFF.
<pre>Usage: geogrid_to_tiff [-h] GEOGRID_DIR OUTPUT.tif</pre>
Unlike <tt>convert_geotiff</tt>, no options are needed -- everything required is already recorded in
<tt>GEOGRID_DIR/index</tt>.

The geogrid `index` file doesn't record either the exact original raster size (only the tile size,
so it's recovered from the tile filenames on disk -- see Limitations below) or the projection's
internal origin latitude (only the standard parallel(s) and central longitude survive, since
`convert_geotiff` itself doesn't keep the origin latitude around after computing the tie point). The
reconstructed GeoTIFF therefore uses a fixed, self-consistent default origin (the first standard
parallel, zero false easting/northing) rather than whatever the original file happened to use --
this doesn't affect where any pixel ends up on the ground, only the arbitrary internal coordinate
values of the reconstructed file's own projected CRS. Projected outputs (`lambert`/`polar`/`mercator`)
assume a spherical earth (radius 6370&nbsp;km), matching WRF/WPS's own convention; `albers_nad83` uses
the real NAD83 datum, since that projection is genuinely ellipsoidal in both WRF and real-world USGS
data.

Limitations
-----------

The current code has some limitations which are listed here.
* Datasets must not contain more than 99,999 grid points in each axis.  This is a limitation of the geogrid format itself, due to the naming convention of the tiles.  However, it is possible (but inefficient) to split a single dataset into multiple directories for this purpose.  A better solution would be to resample the data to a lower spatial resolution prior to converting.
* This program cannot convert between geographic projections, so the input data must be in a projection supported by WPS.  All of the projections [supported by WPS](http://www.mmm.ucar.edu/wrf/users/docs/user_guide_V3.5/users_guide_chap3.htm#_Description_of_index) should work for this conversion program; however, only UTM, Albers equal area, and lat-lon have been tested.  In addition, data sources may not conform to [EPSG standards](http://www.spatialreference.org/) in their projection tags; the output should always be checked before use.
* The geogrid `index` file doesn't record the raster's exact size, only its tile size, so `geogrid_to_tiff` recovers it from the tile filenames on disk. If the original raster wasn't an exact multiple of the tile size used, the reconstructed GeoTIFF will be padded out to the next full tile (up to `tile_x-1`/`tile_y-1` extra pixels of the configured missing-value on the outer edges) -- this is inherent to the geogrid format itself, not something `geogrid_to_tiff` can recover. For a dataset that reaches all the way to a pole, this padding can push a few edge rows' *labeled* coordinates past +/-90 degrees latitude (an artifact of extrapolating a simple linear affine transform past the pole) -- those rows are exactly the padding rows and hold only the dataset's missing-value fill, not real data.
* Multi-level (`tile_z` > 1) geogrid data (e.g. monthly climatology fields) round-trips as a standard multi-band GeoTIFF, one band per level -- readable by GDAL/QGIS/etc. `convert_geotiff` reads `tile_z` from `TIFFTAG_SAMPLESPERPIXEL` when it's greater than 1 (band-interleaved-by-pixel only; band-separate multi-band TIFFs aren't supported), or from the rarely-used volumetric `TIFFTAG_IMAGEDEPTH` extension when that's set instead -- not both on the same file.
* Several `index` fields (`endian`, `signed`, `tile_bdr`, `missing_value`, `scale_factor`) are sometimes omitted in real-world WPS_GEOG datasets (observed in several of NCAR's own official downloads). `geogrid_to_tiff` falls back to a documented default and prints a warning rather than erroring when one is missing -- check the warnings if a reconstructed file looks off. It also handles both `key = value` and `key=value` spacing, and a signed `dy` (negative meaning row 1 of the tile data is the *northernmost* row, rather than this tool's own convention of row 1 being southernmost).


[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/jbeezley/convert_geotiff/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

