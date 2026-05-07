#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SERVICE="${APP_DIR}/serving/mosaic.py"
CPP_SERVICE="${APP_DIR}/mosaic_server"
MOSAIC_CONDA_ENV="${MOSAIC_CONDA_ENV:-picture}"
LOG_DIR="${APP_DIR}/log"
PID_FILE="${LOG_DIR}/log.pid"
PYTHON_LOG="${LOG_DIR}/serving.log"
CPP_LOG="${LOG_DIR}/mosaic_server.log"

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

load_pids() {
    python_pid=""
    cpp_pid=""

    [[ -f "${PID_FILE}" ]] || return 0

    local key value
    while IFS='=' read -r key value; do
        [[ "${value:-}" =~ ^[0-9]+$ ]] || continue
        case "${key}" in
            python_pid) python_pid="${value}" ;;
            cpp_pid) cpp_pid="${value}" ;;
        esac
    done < "${PID_FILE}"
}

is_running() {
    local pid="${1:-}"
    [[ "${pid}" =~ ^[0-9]+$ ]] || return 1
    kill -0 "${pid}" >/dev/null 2>&1
}

stop_pid() {
    local pid="${1:-}"
    local name="${2}"
    local attempt

    is_running "${pid}" || return 0

    echo "Stopping ${name} (${pid})"
    kill "${pid}" >/dev/null 2>&1 || true

    for attempt in {1..25}; do
        is_running "${pid}" || return 0
        sleep 0.2
    done

    if is_running "${pid}"; then
        echo "Force stopping ${name} (${pid})"
        kill -KILL "${pid}" >/dev/null 2>&1 || true
    fi
}

stop_matching_processes() {
    local pattern="${1}"
    local name="${2}"
    local pid

    command -v pgrep >/dev/null 2>&1 || return 0

    while read -r pid; do
        [[ -n "${pid}" ]] || continue
        [[ "${pid}" != "$$" ]] || continue
        stop_pid "${pid}" "${name}"
    done < <(pgrep -f -- "${pattern}" 2>/dev/null || true)
}

stop_services() {
    mkdir -p "${LOG_DIR}"
    load_pids
    stop_pid "${cpp_pid}" "C++ server"
    stop_pid "${python_pid}" "Python service"
    stop_matching_processes "${CPP_SERVICE}" "C++ server"
    stop_matching_processes "${PYTHON_SERVICE}" "Python service"
    rm -f "${PID_FILE}"
}

require_services() {
    if [[ ! -f "${PYTHON_SERVICE}" ]]; then
        echo "Python service not found: ${PYTHON_SERVICE}" >&2
        exit 1
    fi

    if [[ ! -x "${CPP_SERVICE}" ]]; then
        echo "C++ service not found or not executable: ${CPP_SERVICE}" >&2
        exit 1
    fi
}

write_pid_file() {
    local tmp_file="${PID_FILE}.tmp"
    printf 'python_pid=%s\ncpp_pid=%s\n' "${python_pid}" "${cpp_pid}" > "${tmp_file}"
    mv "${tmp_file}" "${PID_FILE}"
}

start_services() {
    mkdir -p "${LOG_DIR}"
    stop_services
    require_services

    activate_conda_env
    local python_bin
    python_bin="$(resolve_python)"

    export MOSAIC_PYTHON_URL="${MOSAIC_PYTHON_URL:-http://127.0.0.1:8091/restore}"

    if [[ ! -f "${APP_DIR}/serving/.env" ]]; then
        echo "Warning: ${APP_DIR}/serving/.env not found. Python service may fail without REPLICATE_API_TOKEN." >&2
    fi

    {
        printf '\n[%s] Starting Python service with %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "${python_bin}"
    } >> "${PYTHON_LOG}"
    {
        printf '\n[%s] Starting C++ server\n' "$(date '+%Y-%m-%d %H:%M:%S')"
    } >> "${CPP_LOG}"

    cd "${APP_DIR}"

    nohup "${python_bin}" "${PYTHON_SERVICE}" >> "${PYTHON_LOG}" 2>&1 &
    python_pid=$!

    nohup "${CPP_SERVICE}" >> "${CPP_LOG}" 2>&1 &
    cpp_pid=$!

    write_pid_file
    disown "${python_pid}" "${cpp_pid}" 2>/dev/null || true

    sleep 1

    if ! is_running "${python_pid}" || ! is_running "${cpp_pid}"; then
        echo "Service failed to start. Check logs:" >&2
        echo "  ${PYTHON_LOG}" >&2
        echo "  ${CPP_LOG}" >&2
        stop_services
        exit 1
    fi

    echo "Started Python service (${python_pid}) -> ${PYTHON_LOG}"
    echo "Started C++ server (${cpp_pid}) -> ${CPP_LOG}"
    echo "PID file: ${PID_FILE}"
    echo "Open http://127.0.0.1:8090/"
}

status_services() {
    load_pids
    if is_running "${python_pid}"; then
        echo "Python service running (${python_pid})"
    else
        echo "Python service stopped"
    fi

    if is_running "${cpp_pid}"; then
        echo "C++ server running (${cpp_pid})"
    else
        echo "C++ server stopped"
    fi
}

usage() {
    echo "Usage: $0 {start|stop|restart|status}"
}

command="${1:-}"
case "${command}" in
    start)
        start_services
        ;;
    stop)
        stop_services
        echo "Stopped services"
        ;;
    restart)
        start_services
        ;;
    status)
        status_services
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac