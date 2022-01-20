#!/bin/bash

# Get script directory
MYDIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

ZIP_ARGS="-T9"

# CD to installation dir
cd "$MYDIR/out/install/bin"

# Get ARRCON version number
VER="$(./ARRCON -vq)"

# Create zip files
zip -T9 "ARRCON-$VER-Windows.zip" "ARRCON.exe"
zip -T9 "ARRCON-$VER-Linux.zip" "ARRCON"

# Create a release directory to store output
mkdir -p "../../Release"
# Move all zip files from install to release dir
mv ./*.zip "../../Release"

echo "Created Release Archives for ARRCON Version $VER"
echo "See output directory: $MYDIR/out/Release"
