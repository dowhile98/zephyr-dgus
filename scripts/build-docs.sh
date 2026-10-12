#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Build documentation for zephyr-dgus using Doxygen and Sphinx.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOC_DIR="${ROOT_DIR}/doc"

echo "=== Building zephyr-dgus Documentation ==="

# Check requirements
if ! command -v doxygen &> /dev/null; then
    echo "ERROR: doxygen is not installed or not in PATH."
    exit 1
fi

# Use python -m sphinx for better compatibility in CI environments
if ! python -m sphinx --version &> /dev/null; then
    # Try activating local .venv if available
    if [[ -f "${ROOT_DIR}/.venv/bin/activate" ]]; then
        echo "Activating documentation virtual environment at ${ROOT_DIR}/.venv..."
        # shellcheck disable=SC1091
        source "${ROOT_DIR}/.venv/bin/activate"
    fi
    
    # Verify again after potential activation
    if ! python -m sphinx --version &> /dev/null; then
        echo "ERROR: Sphinx is not installed or not accessible via Python."
        echo "Install dependencies with: pip install -r doc/requirements.txt"
        exit 1
    fi
fi

echo "1. Generating Doxygen XML and HTML..."
cd "${DOC_DIR}"
rm -rf _build_doxygen
doxygen Doxyfile

if [[ ! -d "${DOC_DIR}/_build_doxygen/xml" ]]; then
    echo "ERROR: Doxygen XML output directory was not created."
    exit 1
fi

echo "2. Building Sphinx HTML documentation..."
rm -rf _build_sphinx
python -m sphinx -b html -d _build_sphinx/doctrees -W --keep-going . _build_sphinx/html

if [[ -d "${DOC_DIR}/_build_doxygen/html" ]]; then
    echo "3. Copying Doxygen HTML into Sphinx HTML output directory..."
    mkdir -p "${DOC_DIR}/_build_sphinx/html/doxygen"
    cp -r "${DOC_DIR}/_build_doxygen/html/." "${DOC_DIR}/_build_sphinx/html/doxygen/"
fi

INDEX_PATH="${DOC_DIR}/_build_sphinx/html/index.html"
if [[ -f "${INDEX_PATH}" ]]; then
    echo ""
    echo "============================================================"
    echo "SUCCESS: Documentation build completed successfully!"
    echo "Open index in your browser: file://${INDEX_PATH}"
    if [[ -d "${DOC_DIR}/_build_sphinx/html/doxygen" ]]; then
        echo "Doxygen documentation:     file://${DOC_DIR}/_build_sphinx/html/doxygen/index.html"
    fi
    echo "============================================================"
    exit 0
else
    echo "ERROR: Expected output ${INDEX_PATH} was not found."
    exit 1
fi
