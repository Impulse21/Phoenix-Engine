#!/bin/bash
# Build script for Phoenix Engine on Linux

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Phoenix Engine - Linux Build Script${NC}"
echo "===================================="

# Check for CMake
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake not found. Please install CMake 3.28 or later.${NC}"
    exit 1
fi

# Check for Ninja
if ! command -v ninja &> /dev/null; then
    echo -e "${YELLOW}Warning: Ninja not found. Falling back to Unix Makefiles.${NC}"
    GENERATOR="Unix Makefiles"
else
    GENERATOR="Ninja"
fi

# Check for Vulkan
if ! command -v vulkaninfo &> /dev/null; then
    echo -e "${YELLOW}Warning: Vulkan SDK not found. Build may fail.${NC}"
    echo "Install with: sudo pacman -S vulkan-devel"
fi

# Parse arguments
BUILD_TYPE="${1:-Debug}"
BUILD_DIR=".build"

echo ""
echo "Configuration:"
echo "  Generator: $GENERATOR"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Directory: $BUILD_DIR"
echo ""

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo -e "${GREEN}Configuring...${NC}"
cmake -G "$GENERATOR" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..

# Build
echo -e "${GREEN}Building...${NC}"
if [ "$GENERATOR" = "Ninja" ]; then
    ninja
else
    make -j$(nproc)
fi

echo ""
echo -e "${GREEN}Build complete!${NC}"
echo "Executables are in: $BUILD_DIR/bin/"
echo ""
echo "To run PhxEditor:"
echo "  ./$BUILD_DIR/bin/PhxEditor"
echo ""
echo "To create compile_commands.json symlink for LazyVim:"
echo "  ln -s $BUILD_DIR/compile_commands.json compile_commands.json"
