#!/bin/bash

# Navigate to VoxScript directory
cd /Users/avishaylidani/DEV/GitHubRepo/VoxScript

# Clean previous build
echo "Cleaning previous build..."
rm -rf build-Debug

# Rebuild with enhanced logging
echo "Building with debug logging..."
./build.sh

echo ""
echo "================================================"
echo "Build complete! Next steps:"
echo "================================================"
echo ""
echo "1. Open Console.app (or Xcode console)"
echo "2. Filter for 'VoxScript'"
echo "3. Launch Reaper"
echo "4. Load VoxScript as ARA plugin"
echo "5. Drag audio file onto track"
echo ""
echo "See REAPER_DEBUG_GUIDE.md for detailed instructions"
echo "================================================"
