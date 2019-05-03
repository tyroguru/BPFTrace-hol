#!/bin/bash

set -euo pipefail

BUILD_DIR=_build
OUT_TARBALL=bpftrace-hol.tar.gz

# First build everything
pushd CODE
make
popd

# Populate the load generators
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"/load_generators
cp -r CODE/* "$BUILD_DIR"/load_generators
# Remove all non-executable files
find "$BUILD_DIR"/load_generators ! -perm /111 -type f -exec rm -v {} \;

# Move the main binary out
mv "$BUILD_DIR"/load_generators/bpfhol/bpfhol "$BUILD_DIR"
rm -r "$BUILD_DIR"/load_generators/bpfhol

# Build docs
for doc in *.md; do
  pandoc "$doc" -f gfm -o "$BUILD_DIR"/$(basename "$doc" .md).pdf -V urlcolor=blue
done

# Tarball the results
tar -cvzf "$OUT_TARBALL" "$BUILD_DIR" \
  --transform="s/${BUILD_DIR}/bpftrace-hol/"

echo "Output is: ${OUT_TARBALL}"
