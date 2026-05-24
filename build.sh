#!/usr/bin/env bash

set -e  # stop si erreur

BUILD_DIR="build"

rm -rf $BUILD_DIR

echo "\n=================================="
echo " Configuring CMake"
echo "=================================="

cmake -S . -B $BUILD_DIR

echo "=================================="
echo " Building project"
echo "=================================="

cmake --build $BUILD_DIR -j$(nproc)

echo "=================================="
echo " Done"
echo "=================================="

cd build
./projectile