#!/bin/bash
# Round-trip regression test for the reverse conversion (geogrid -> GeoTIFF):
# for every fixture, run convert_geotiff -> geogrid_to_tiff -> convert_geotiff
# again and check nothing crashes (a smoke test across every projection type
# and pixel format in the fixture set). For fixtures whose dimensions are an
# exact multiple of the tile size used (so no tile-padding is involved --
# see the "Recovering nx/ny" note in the reverse-conversion plan, an
# inherent geogrid-format limitation, not a bug), also assert the two
# forward-conversion outputs are byte-identical.

set -u

cg=${cg:-'../../build/convert_geotiff'}
gt=${gt:-'../../build/geogrid_to_tiff'}

cd "$(dirname "$0")/projections" || exit 1

fail=0

# Smoke test: every fixture, default options, just check nothing crashes.
for f in *.tif; do
  name=$(basename "$f" .tif)
  rm -rf "${name}_rt1" "${name}_rt2" "${name}_rt.tif"
  mkdir "${name}_rt1"
  if ! (cd "${name}_rt1" && "$cg" "../${f}" >/dev/null 2>&1); then
    echo "FAILED (forward pass 1): $f"
    fail=1
  elif ! "$gt" "${name}_rt1" "${name}_rt.tif" >/dev/null 2>&1; then
    echo "FAILED (geogrid_to_tiff): $f"
    fail=1
  else
    mkdir "${name}_rt2"
    if ! (cd "${name}_rt2" && "$cg" "../${name}_rt.tif" >/dev/null 2>&1); then
      echo "FAILED (forward pass 2): $f"
      fail=1
    fi
  fi
  rm -rf "${name}_rt1" "${name}_rt2" "${name}_rt.tif"
done

# Strict byte-identical check: only for fixtures whose dimensions are an
# exact multiple of 200 (no tile-padding involved).
for f in utm.tif trans_merc.tif albers27.tif; do
  name=$(basename "$f" .tif)
  rm -rf "${name}_rt1" "${name}_rt2" "${name}_rt.tif"
  mkdir "${name}_rt1" "${name}_rt2"
  (cd "${name}_rt1" && "$cg" -w 4 -s 1 -t 200 "../${f}" >/dev/null 2>&1)
  "$gt" "${name}_rt1" "${name}_rt.tif" >/dev/null 2>&1
  (cd "${name}_rt2" && "$cg" -w 4 -s 1 -t 200 "../${name}_rt.tif" >/dev/null 2>&1)
  if ! diff -rq "${name}_rt1" "${name}_rt2" >/dev/null; then
    echo "FAILED (round-trip output mismatch): $f"
    diff -rq "${name}_rt1" "${name}_rt2"
    fail=1
  fi
  rm -rf "${name}_rt1" "${name}_rt2" "${name}_rt.tif"
done

if [ $fail -eq 0 ]; then
  echo "SUCCESS"
fi
exit $fail
