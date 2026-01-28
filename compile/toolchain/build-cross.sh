#!/usr/bin/env bash
set -euo pipefail

# Variables (edit here if needed)
BINUTILS_VER=2.40
GCC_VER=12.2.0
TARGET=i686-elf
PREFIX=/usr/local/cross
SRC_DIR=\C:\Users\olusa/src
BUILD_DIR=\C:\Users\olusa/build-toolchain

echo "Running inside WSL as: \my-dell-desktop\olusa"
echo "Target: \i686-elf"
echo "Install prefix: \/usr/local/cross"
echo "Binutils: \2.40, GCC: \12.2.0"

# Update and install dependencies
echo "Installing build dependencies (sudo may prompt for password)..."
sudo apt-get update
sudo apt-get install -y build-essential bison flex libgmp-dev libmpfr-dev libmpc-dev texinfo libisl-dev wget ca-certificates

# Prepare directories
mkdir -p "\"
mkdir -p "\"
cd "\"

# Download sources if not already present
if [ ! -f "binutils-\2.40.tar.xz" ]; then
  echo "Downloading binutils..."
  wget -c "https://ftp.gnu.org/gnu/binutils/binutils-\2.40.tar.xz"
fi

if [ ! -f "gcc-\12.2.0.tar.xz" ]; then
  echo "Downloading gcc..."
  wget -c "https://ftp.gnu.org/gnu/gcc/gcc-\12.2.0/gcc-\12.2.0.tar.xz"
fi

# Extract
rm -rf "\/binutils-build" "\/gcc-build"
tar -xf "binutils-\2.40.tar.xz" -C "\" --strip-components=0
tar -xf "gcc-\12.2.0.tar.xz" -C "\" --strip-components=0

# Build binutils
echo "Building binutils..."
mkdir -p "\/binutils-build"
cd "\/binutils-build"
"\/binutils-\2.40/configure" --target="\i686-elf" --prefix="\/usr/local/cross" --with-sysroot --disable-nls --disable-werror
make -j\$(nproc)
sudo make install

# Prepare GCC prerequisites
cd "\/gcc-\12.2.0"
./contrib/download_prerequisites

# Build GCC (C only, bare-metal)
echo "Building GCC (stage: C compiler and libgcc)..."
mkdir -p "\/gcc-build"
cd "\/gcc-build"
"\/gcc-\12.2.0/configure" --target="\i686-elf" --prefix="\/usr/local/cross" --disable-nls --enable-languages=c --without-headers --disable-shared --disable-multilib --disable-libssp
make all-gcc -j\$(nproc)
make all-target-libgcc -j\$(nproc)
sudo make install-gcc
sudo make install-target-libgcc

# Verify
echo "Verifying installed tools..."
if command -v "\/usr/local/cross/bin/\i686-elf-gcc" >/dev/null 2>&1; then
  echo "Toolchain installed at \/usr/local/cross"
  "\/usr/local/cross/bin/\i686-elf-gcc" --version
  "\/usr/local/cross/bin/\i686-elf-ld" --version || true
else
  echo "ERROR: Toolchain not found in \/usr/local/cross/bin"
  exit 1
fi

echo "Simple compile test..."
TMPDIR=\$(mktemp -d)
cat > \/test.c <<'EOF'
int main(void) { return 0; }
EOF
"\/usr/local/cross/bin/\i686-elf-gcc" -nostdlib -c \/test.c -o \/test.o
"\/usr/local/cross/bin/\i686-elf-ld" -Ttext 0x1000 -o \/test.elf \/test.o
"\/usr/local/cross/bin/\i686-elf-objcopy" -O binary \/test.elf \/test.bin
echo "Test binary size: \$(stat -c%s \/test.bin) bytes"

echo "Build complete. Toolchain is ready at: \/usr/local/cross"
