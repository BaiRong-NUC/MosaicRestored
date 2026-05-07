#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SERVICE="${APP_DIR}/serving/mosaic.py"
CPP_SERVICE="${APP_DIR}/mosaic_server"
MOSAIC_CONDA_ENV="${MOSAIC_CONDA_ENV:-picture}"

resolve_conda() {
    local candidate

    local candidates=(
        "${CONDA_EXE:-}"
        "$(command -v conda 2>/dev/null || true)"
        "${HOME}/miniconda3/bin/conda"
        "${HOME}/anaconda3/bin/conda"
        "${HOME}/miniforge3/bin/conda"
        "${HOME}/mambaforge/bin/conda"
    )

    for candidate in "${candidates[@]}"; do
        [[ -n "${candidate}" ]] || continue
        if [[ -x "${candidate}" ]]; then
            echo "${candidate}"
            return 0
        fi
    done

    return 1
}

activate_conda_env() {
    local conda_bin conda_base conda_sh env_python

    [[ -n "${MOSAIC_CONDA_ENV:-}" ]] || return 0
    [[ -z "${PYTHON:-}" ]] || return 0
    [[ -z "${VIRTUAL_ENV:-}" ]] || return 0

    conda_bin="$(resolve_conda || true)"
    [[ -n "${conda_bin}" ]] || return 0

    conda_base="$(${conda_bin} info --base 2>/dev/null || true)"
    [[ -n "${conda_base}" ]] || return 0

    conda_sh="${conda_base}/etc/profile.d/conda.sh"
    env_python="${conda_base}/envs/${MOSAIC_CONDA_ENV}/bin/python"

    if [[ -f "${conda_sh}" ]]; then
        # shellcheck source=/dev/null
        source "${conda_sh}"
        if conda activate "${MOSAIC_CONDA_ENV}" >/dev/null 2>&1; then
            return 0
        fi
    fi

    if [[ -x "${env_python}" ]]; then
        export CONDA_PREFIX="${conda_base}/envs/${MOSAIC_CONDA_ENV}"
        export CONDA_DEFAULT_ENV="${MOSAIC_CONDA_ENV}"
        export PATH="${CONDA_PREFIX}/bin:${PATH}"
        return 0
    fi

    echo "Warning: conda environment '${MOSAIC_CONDA_ENV}' not found. Falling back to Python auto-detection." >&2
}

resolve_python() {
    local candidate

    if [[ -n "${PYTHON:-}" ]]; then
        if command -v "${PYTHON}" >/dev/null 2>&1; then
            command -v "${PYTHON}"
            return 0
        fi

        echo "Python executable not found: ${PYTHON}" >&2
        echo "Set PYTHON=/path/to/python or unset PYTHON to use auto-detection." >&2
        return 1
    fi

    local candidates=()

    if [[ -n "${VIRTUAL_ENV:-}" ]]; then
        candidates+=(
            "${VIRTUAL_ENV}/bin/python"
            "${VIRTUAL_ENV}/bin/python3"
        )
    fi

    if [[ -n "${CONDA_PREFIX:-}" ]]; then
        candidates+=(
            "${CONDA_PREFIX}/bin/python"
            "${CONDA_PREFIX}/bin/python3"
        )
    fi

    candidates+=(
        "${APP_DIR}/.venv/bin/python"
        "${APP_DIR}/.venv/bin/python3"
        "${APP_DIR}/../.venv/bin/python"
        "${APP_DIR}/../.venv/bin/python3"
        "${APP_DIR}/../../.venv/bin/python"
        "${APP_DIR}/../../.venv/bin/python3"
        python3
        python
    )

    for candidate in "${candidates[@]}"; do
        [[ -n "${candidate}" ]] || continue
        if command -v "${candidate}" >/dev/null 2>&1; then
            command -v "${candidate}"
            return 0
        fi
    done

    echo "Python executable not found." >&2
    echo "Set PYTHON=/path/to/python before running this script." >&2
    return 1
}

activate_conda_env
PYTHON_BIN="$(resolve_python)"

export MOSAIC_PYTHON_URL="${MOSAIC_PYTHON_URL:-http://127.0.0.1:8091/restore}"

if [[ ! -f "${PYTHON_SERVICE}" ]]; then
    echo "Python service not found: ${PYTHON_SERVICE}" >&2
    exit 1
fi

if [[ ! -x "${CPP_SERVICE}" ]]; then
    echo "C++ service not found or not executable: ${CPP_SERVICE}" >&2
    exit 1
fi

if [[ ! -f "${APP_DIR}/serving/.env" ]]; then
    echo "Warning: ${APP_DIR}/serving/.env not found. Python service may fail without REPLICATE_API_TOKEN." >&2
fi

python_pid=""
cpp_pid=""

cleanup() {
    local status=$?
    trap - EXIT INT TERM
    if [[ -n "${cpp_pid}" ]] && kill -0 "${cpp_pid}" >/dev/null 2>&1; then
        kill "${cpp_pid}" >/dev/null 2>&1 || true
    fi
    if [[ -n "${python_pid}" ]] && kill -0 "${python_pid}" >/dev/null 2>&1; then
        kill "${python_pid}" >/dev/null 2>&1 || true
    fi
    wait >/dev/null 2>&1 || true
    exit "${status}"
}

trap cleanup EXIT INT TERM

cd "${APP_DIR}"

echo "Starting Python service on http://127.0.0.1:8091 with ${PYTHON_BIN}"
"${PYTHON_BIN}" "${PYTHON_SERVICE}" &
python_pid=$!

echo "Starting C++ server on http://127.0.0.1:8090"
"${CPP_SERVICE}" &
cpp_pid=$!

echo "Open http://127.0.0.1:8090/"
wait -n "${python_pid}" "${cpp_pid}"
