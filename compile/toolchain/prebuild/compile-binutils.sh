#!/bin/bash
set -e

BINUTILS_VERSION=2.42
PWD="$(pwd)"
PREFIX="$PWD/binutils-2.42-build"
TARGET=x86_64-elf
JOBS=$(nproc)

echo "=== Downloading binutils $BINUTILS_VERSION ==="

if [ -d "$PWD/build" ]; then
    rm -rf "$PWD/build"
fi

if [ -s "$PWD/binutils-$BINUTILS_VERSION.tar.xz" ]; then
    echo "$PWD/binutils-$BINUTILS_VERSION.tar.xz exists"
else
    wget https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.xz
fi

tar -xf binutils-$BINUTILS_VERSION.tar.xz
cd binutils-$BINUTILS_VERSION

echo "=== Configuring binutils for target: $TARGET ==="

cd ..
mkdir build
cd build

../binutils-$BINUTILS_VERSION/configure \
    --host=x86_64-w64-mingw32 \
    --target=$TARGET \
    --prefix=$PREFIX \
    --disable-nls \
    --disable-werror \
    --enable-plugins \
    --enable-64-bit-bfd \
    --enable-targets=all

echo "=== Building binutils ==="
make -j$JOBS

echo "=== Installing binutils to $PREFIX ==="
make install

echo "=== Done ==="
echo "Binutils installed to: $PREFIX"
echo "Add to PATH:"
echo "    export PATH=\$PATH:$PREFIX/bin"