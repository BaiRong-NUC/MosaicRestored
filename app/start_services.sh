#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SERVICE="${APP_DIR}/serving/mosaic.py"
CPP_SERVICE="${APP_DIR}/mosaic_server"

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

    local candidates=(
        "${VIRTUAL_ENV:-}/bin/python"
        "${VIRTUAL_ENV:-}/bin/python3"
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
