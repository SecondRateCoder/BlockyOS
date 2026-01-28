# build-i686-elf-toolchain.ps1
# PowerShell wrapper: copies a build script into WSL (Ubuntu) and runs it.
# Usage: .\build-i686-elf-toolchain.ps1

# Configuration: change versions or prefix if you want
$BINUTILS_VER = "2.40"
$GCC_VER = "12.2.0"
$TARGET = "i686-elf"
$PREFIX = "/usr/local/cross"
$WLS_SCRIPT = "build-cross.sh"
$LOCAL_SCRIPT = Join-Path $PSScriptRoot $WLS_SCRIPT

function Abort($msg) {
    Write-Error $msg
    exit 1
}

# Check for WSL
try {
    Write-Host (& wsl.exe --status 2>$null)
}catch{
    Abort "WSL not found. Install WSL (e.g., run 'wsl --install -d Ubuntu' from an elevated PowerShell) and try again."
}

Write-Host "Detected WSL. Preparing build script..."

# Create the bash build script content
$bashScript = @"
#!/usr/bin/env bash
set -euo pipefail

# Variables (edit here if needed)
BINUTILS_VER=${BINUTILS_VER}
GCC_VER=${GCC_VER}
TARGET=${TARGET}
PREFIX=${PREFIX}
SRC_DIR=\$HOME/src
BUILD_DIR=\$HOME/build-toolchain

echo "Running inside WSL as: \$(whoami.exe)"
echo "Target: \$TARGET"
echo "Install prefix: \$PREFIX"
echo "Binutils: \$BINUTILS_VER, GCC: \$GCC_VER"

# Update and install dependencies
echo "Installing build dependencies (sudo may prompt for password)..."
sudo apt-get update
sudo apt-get install -y build-essential bison flex libgmp-dev libmpfr-dev libmpc-dev texinfo libisl-dev wget ca-certificates

# Prepare directories
mkdir -p "\$SRC_DIR"
mkdir -p "\$BUILD_DIR"
cd "\$SRC_DIR"

# Download sources if not already present
if [ ! -f "binutils-\$BINUTILS_VER.tar.xz" ]; then
  echo "Downloading binutils..."
  wget -c "https://ftp.gnu.org/gnu/binutils/binutils-\$BINUTILS_VER.tar.xz"
fi

if [ ! -f "gcc-\$GCC_VER.tar.xz" ]; then
  echo "Downloading gcc..."
  wget -c "https://ftp.gnu.org/gnu/gcc/gcc-\$GCC_VER/gcc-\$GCC_VER.tar.xz"
fi

# Extract
rm -rf "\$BUILD_DIR/binutils-build" "\$BUILD_DIR/gcc-build"
tar -xf "binutils-\$BINUTILS_VER.tar.xz" -C "\$BUILD_DIR" --strip-components=0
tar -xf "gcc-\$GCC_VER.tar.xz" -C "\$BUILD_DIR" --strip-components=0

# Build binutils
echo "Building binutils..."
mkdir -p "\$BUILD_DIR/binutils-build"
cd "\$BUILD_DIR/binutils-build"
"\$SRC_DIR/binutils-\$BINUTILS_VER/configure" --target="\$TARGET" --prefix="\$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j\`$(nproc)
sudo make install

# Prepare GCC prerequisites
cd "\$SRC_DIR/gcc-\$GCC_VER"
./contrib/download_prerequisites

# Build GCC (C only, bare-metal)
echo "Building GCC (stage: C compiler and libgcc)..."
mkdir -p "\$BUILD_DIR/gcc-build"
cd "\$BUILD_DIR/gcc-build"
"\$SRC_DIR/gcc-\$GCC_VER/configure" --target="\$TARGET" --prefix="\$PREFIX" --disable-nls --enable-languages=c --without-headers --disable-shared --disable-multilib --disable-libssp
make all-gcc -j\`$(nproc)
make all-target-libgcc -j\`$(nproc)
sudo make install-gcc
sudo make install-target-libgcc

# Verify
echo "Verifying installed tools..."
if command -v "\$PREFIX/bin/\$TARGET-gcc" >/dev/null 2>&1; then
  echo "Toolchain installed at \$PREFIX"
  "\$PREFIX/bin/\$TARGET-gcc" --version
  "\$PREFIX/bin/\$TARGET-ld" --version || true
else
  echo "ERROR: Toolchain not found in \$PREFIX/bin"
  exit 1
fi

echo "Simple compile test..."
TMPDIR=\`$(mktemp -d)
cat > \$TMPDIR/test.c <<'EOF'
int main(void) { return 0; }
EOF
"\$PREFIX/bin/\$TARGET-gcc" -nostdlib -c \$TMPDIR/test.c -o \$TMPDIR/test.o
"\$PREFIX/bin/\$TARGET-ld" -Ttext 0x1000 -o \$TMPDIR/test.elf \$TMPDIR/test.o
"\$PREFIX/bin/\$TARGET-objcopy" -O binary \$TMPDIR/test.elf \$TMPDIR/test.bin
echo "Test binary size: \`$(stat -c%s \$TMPDIR/test.bin) bytes"

echo "Build complete. Toolchain is ready at: \$PREFIX"
"@

# Write the script to a local file
Set-Content -Path $LOCAL_SCRIPT -Value $bashScript -Encoding UTF8

# Copy the script into WSL home and run it
Write-Host "Copying build script into WSL and executing..."
# Use wsl to receive the script via stdin and write to ~/build-cross.sh
# copy the local script into WSL home and run it
Get-Content -Raw $LOCAL_SCRIPT | C:\msys64\usr\bin\bash.exe bash -lc "cat > ~/build-cross.sh"
# Make it executable and run it inside WSL
C:\msys64\usr\bin\bash.exe -lc "chmod +x ~/build-cross.sh && ~/build-cross.sh"

Write-Host "Done. If the build succeeded, the cross toolchain is installed inside WSL at $PREFIX."
Write-Host "To use it from WSL: export PATH=$PREFIX/bin:\$($PATH)"
Write-Host "To call the cross compiler from Windows, use: wsl $($PREFIX)/bin/$($TARGET)-gcc --version"
