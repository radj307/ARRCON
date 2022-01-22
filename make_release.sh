#!/bin/bash

# Get script directory
MYDIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd)"


# If the release directory already exists, delete it
if [ -d "$MYDIR/out/Release" ]
then
	rm -rf "$MYDIR/out/Release"
fi
# Create a release directory to store output
mkdir -p "$MYDIR/out/Release"


LINUX_DIR="linux-release/bin"
WINDOWS_DIR="windows-release/bin"


# MAKE LINUX RELEASE

cd "$MYDIR/out/install/$LINUX_DIR"
VER="$(./ARRCON -vq)" # Set VER variable to the current version
zip -T9 "ARRCON-$VER-Linux.zip" "ARRCON"
mv ./*.zip "$MYDIR/out/Release"


# MAKE WINDOWS RELEASE

cd "$MYDIR/out/install/$WINDOWS_DIR"
zip -T9 "ARRCON-$VER-Windows.zip" "ARRCON.exe"
mv ./*.zip "$MYDIR/out/Release"

# DONE

echo "Created Release Archives for ARRCON Version $VER"
echo "See output directory: $MYDIR/out/Release"
