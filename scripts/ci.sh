#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# All-in-one local CI validation script for zephyr-dgus.
# Reproduces the automated checks executed in GitHub Actions CI.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

echo "============================================================"
echo "          zephyr-dgus Local CI Verification Suite           "
echo "============================================================"

# Environment activation
if [[ -z "${ZEPHYR_BASE:-}" ]]; then
    if [[ -f "/home/tecna/zephyrproject/zephyr/zephyr-env.sh" ]]; then
        echo "Sourcing Zephyr environment..."
        if [[ -f "/home/tecna/zephyrproject/.venv/bin/activate" ]]; then
            # shellcheck disable=SC1091
            source "/home/tecna/zephyrproject/.venv/bin/activate"
        fi
        # shellcheck disable=SC1091
        source "/home/tecna/zephyrproject/zephyr/zephyr-env.sh"
    fi
fi

cd "${ROOT_DIR}"

declare -a STAGE_NAMES=()
declare -a STAGE_STATUS=()

run_stage() {
    local name="$1"
    shift
    echo ""
    echo ">>> [STAGE] ${name}..."
    STAGE_NAMES+=("${name}")
    if "$@"; then
        echo ">>> [PASS] ${name}"
        STAGE_STATUS+=("PASS")
    else
        echo ">>> [FAIL] ${name}"
        STAGE_STATUS+=("FAIL")
    fi
}

# 1. Code format check
check_formatting() {
    local clang_fmt="clang-format"
    if ! command -v "${clang_fmt}" &> /dev/null; then
        if [[ -f "/home/tecna/zephyrproject/.venv/bin/clang-format" ]]; then
            clang_fmt="/home/tecna/zephyrproject/.venv/bin/clang-format"
        else
            echo "WARNING: clang-format not found. Skipping format check."
            return 0
        fi
    fi
    "${clang_fmt}" --dry-run --Werror \
        src/*.c src/*.h include/dgus2/*.h tests/src/*.c samples/dwin-interface/src/*.c
}
run_stage "Code Format Compliance (clang-format)" check_formatting

# 2. Twister Unit Tests on native_sim
run_twister() {
    west twister -T tests/ -p native_sim --verbose
}
run_stage "Twister Unit Tests (native_sim)" run_twister

# 3. Sample build: native_sim
build_sample_native() {
    west build -b native_sim samples/dwin-interface -p auto
}
run_stage "Sample Build (native_sim)" build_sample_native

# 4. Sample builds for STM32 boards
build_sample_stm32f4_disco() {
    west build -b stm32f4_disco samples/dwin-interface -p auto
}
run_stage "Sample Build (stm32f4_disco)" build_sample_stm32f4_disco

build_sample_nucleo_f429zi() {
    west build -b nucleo_f429zi samples/dwin-interface -p auto
}
run_stage "Sample Build (nucleo_f429zi)" build_sample_nucleo_f429zi

build_sample_nucleo_h753zi() {
    west build -b nucleo_h753zi samples/dwin-interface -p auto
}
run_stage "Sample Build (nucleo_h753zi)" build_sample_nucleo_h753zi

# 5. Documentation generation
build_docs() {
    "${ROOT_DIR}/scripts/build-docs.sh"
}
run_stage "Documentation Generation (Doxygen + Sphinx)" build_docs

# Summary report
echo ""
echo "============================================================"
echo "                      CI SUMMARY REPORT                     "
echo "============================================================"
OVERALL_PASS=true
for i in "${!STAGE_NAMES[@]}"; do
    printf "%-48s [%s]\n" "${STAGE_NAMES[$i]}" "${STAGE_STATUS[$i]}"
    if [[ "${STAGE_STATUS[$i]}" != "PASS" ]]; then
        OVERALL_PASS=false
    fi
done
echo "============================================================"

if [[ "${OVERALL_PASS}" == true ]]; then
    echo "OVERALL RESULT: ALL CHECKS PASSED (SUCCESS)"
    exit 0
else
    echo "OVERALL RESULT: ONE OR MORE CHECKS FAILED (FAILURE)"
    exit 1
fi
