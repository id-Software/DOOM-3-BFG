#!/bin/bash
# ============================================================================
# DOOM: The Dark Ages - Windows Cross-Compilation Build Script
# Builds a standalone portable EXE with icon from Linux
# ============================================================================

set -e

echo "=============================================="
echo "  DOOM: The Dark Ages - Windows Build"
echo "=============================================="

BUILD_DIR="build_win64"
TOOLCHAIN="cmake/Toolchain-Windows-x86_64.cmake"

# Clean previous build
if [ "$1" = "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "[1/3] Configuring CMake with Windows toolchain..."
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="../$TOOLCHAIN" \
    -DCMAKE_BUILD_TYPE=Release \
    -DDARKAGES_PORTABLE=ON \
    -DDARKAGES_OPTIMIZE_T460S=ON \
    -DUSE_OPENGL=OFF \
    -DUSE_OPENAL=OFF \
    2>&1

echo ""
echo "[2/3] Compiling..."
make -j$(nproc) 2>&1

echo ""
echo "[3/3] Packaging..."
cpack -G ZIP 2>&1 || true

echo ""
echo "=============================================="
echo "  Build Complete!"
echo "=============================================="
echo "  Output: $BUILD_DIR/DarkAges.exe"
echo "=============================================="

# Show file size
if [ -f "DarkAges.exe" ]; then
    SIZE=$(du -h "DarkAges.exe" | cut -f1)
    echo "  EXE Size: $SIZE"
fi

ls -la *.exe 2>/dev/null || echo "  (EXE not found - check build log)"
