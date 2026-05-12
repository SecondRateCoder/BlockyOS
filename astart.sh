#!/usr/bin/env bash
set -e

# Paths
VENV_DIR="compile/toolchain/edk2/.venv"
REQ_MAIN="compile/toolchain/edk2/pip-requirements.txt"
REQ_OPT="compile/toolchain/edk2/pip-requirements-optional.txt"
STUART_CFG="compile/toolchain/edk2/.pytool/CISettings.py"

# Create venv if missing
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating Python virtual environment..."
    python3 -m venv "$VENV_DIR"
fi

# Activate venv
echo "Activating virtual environment..."
# shellcheck disable=SC1090
source "$VENV_DIR/bin/activate"

# Upgrade pip
pip install --upgrade pip

# Install dependencies
echo "Installing EDK2 Python dependencies..."
pip install -r "$REQ_MAIN"
pip install -r "$REQ_OPT"

# Run Stuart setup
echo "Running stuart_setup..."
stuart_setup -c "$STUART_CFG"

echo "Environment setup complete."
