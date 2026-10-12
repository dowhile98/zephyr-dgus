#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Build documentation for zephyr-dgus using Doxygen and Sphinx.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOC_DIR="${ROOT_DIR}/doc"

echo "=== Building zephyr-dgus Documentation ==="

# Activate local .venv if available and needed
if ! command -v sphinx-build &> /dev/null; then
    if [[ -f "${ROOT_DIR}/.venv/bin/activate" ]]; then
        echo "Activating documentation virtual environment at ${ROOT_DIR}/.venv..."
        # shellcheck disable=SC1091
        source "${ROOT_DIR}/.venv/bin/activate"
    fi
fi

# Check requirements
if ! command -v doxygen &> /dev/null; then
    echo "ERROR: doxygen is not installed or not in PATH."
    exit 1
fi

if ! command -v sphinx-build &> /dev/null; then
    echo "ERROR: sphinx-build is not installed or not in PATH."
    echo "Install dependencies with: pip install -r doc/requirements.txt"
    exit 1
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
sphinx-build -b html -d _build_sphinx/doctrees -W --keep-going . _build_sphinx/html

INDEX_PATH="${DOC_DIR}/_build_sphinx/html/index.html"
if [[ -f "${INDEX_PATH}" ]]; then
    echo ""
    echo "============================================================"
    echo "SUCCESS: Documentation build completed successfully!"
    echo "Open index in your browser: file://${INDEX_PATH}"
    echo "============================================================"
    exit 0
else
    echo "ERROR: Expected output ${INDEX_PATH} was not found."
    exit 1
fi
