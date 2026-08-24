#!/usr/bin/env bash
# ---------------------------------------------------------------------------
#  compile-all.sh — кроссплатформенная сборка ZMP.
#
#  Запуск на LINUX:
#      ./compile-all.sh
#        build/       — Linux-версия
#        build/win/   — Windows-версия (кросс-компиляция mingw-w64,
#                       нужен пакет mingw-QT6 в sysroot или ZMP_WIN_QT=...)
#
#  Запуск на WINDOWS 10/11 (из MSYS2 MinGW64 shell или Git Bash):
#      ./compile-all.sh
#        build/win/    — Windows-версия (нативная)
#        build/linux/  — Linux-версия (через WSL, если он установлен)
#
#  Зависимости:
#    Linux:  qt6-base qt6-multimedia qt6-svg taglib soundtouch projectm ffmpeg
#            (+ mingw-w64-gcc и mingw-QT6/taglib/soundtouch для Windows-сборки)
#    Windows (MSYS2): mingw-w64-x86_64-qt6-base qt6-multimedia(taglib)
#                     mingw-w64-x86_64-taglib mingw-w64-x86_64-soundtouch
#    Для Linux-версии из Windows: WSL с Ubuntu и теми же Linux-пакетами.
# ---------------------------------------------------------------------------
set -uo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

NPROC="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
TOOLCHAIN="$ROOT/cmake/mingw-w64-x86_64.cmake"

host_os="$(uname -s)"
case "$host_os" in
    Linux*)                    HOST="linux" ;;
    MINGW*|MSYS*|CYGWIN*)      HOST="windows" ;;
    *) echo "Unsupported host: $host_os"; exit 1 ;;
esac

log_ok()   { echo "=== OK"; }
log_skip() { echo "=== SKIPPED:"; while [ $# -gt 0 ]; do echo "    $1"; shift; done; }

# ---------------------------------------------------------------------------
#  Сборка WINDOWS-версии
# ---------------------------------------------------------------------------

have_mingw_cross() {
    command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 ||
    command -v x86_64-w64-mingw32.shared-g++ >/dev/null 2>&1
}

make_toolchain() {
    local cxx; cxx="$(command -v x86_64-w64-mingw32-g++ ||
                      command -v x86_64-w64-mingw32.shared-g++)"
    local cc="${cxx%g++}gcc"

    local rc=""
    local cand
    for cand in "${cxx%g++}windres" \
                "x86_64-w64-mingw32-windres" \
                "x86_64-w64-mingw32.shared-windres" \
                "$(dirname "$cxx")/x86_64-w64-mingw32-windres"; do
        if command -v "$cand" >/dev/null 2>&1; then rc="$cand"; break; fi
    done

    local sysroot; sysroot="$(dirname "$(dirname "$cxx")")"

    mkdir -p "$(dirname "$TOOLCHAIN")"
    {
        echo "# автогенерировано compile-all.sh"
        echo "set(CMAKE_SYSTEM_NAME Windows)"
        echo "set(CMAKE_SYSTEM_PROCESSOR x86_64)"
        echo "set(CMAKE_C_COMPILER   $cc)"
        echo "set(CMAKE_CXX_COMPILER $cxx)"
        [ -n "$rc" ] && echo "set(CMAKE_RC_COMPILER $rc)"
        echo "set(CMAKE_FIND_ROOT_PATH \"$sysroot\")"
        echo "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)"
        echo "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)"
        echo "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)"
        echo "set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)"
    } > "$TOOLCHAIN"
}

# $1 = "native" (запуск на Windows) | "cross" (кросс из Linux)
find_win_qt() {
    local mode="$1"
    if [ -n "${ZMP_WIN_QT:-}" ] && [ -f "${ZMP_WIN_QT}/Qt6Config.cmake" ]; then
        printf '%s' "$ZMP_WIN_QT"
        return 0
    fi
    local cand
    if [ "$mode" = "native" ]; then
        for cand in \
            "/mingw64/lib/cmake/Qt6" \
            "$(dirname "$(command -v cmake 2>/dev/null || echo /mingw64/bin/cmake)")/../lib/cmake/Qt6"; do
            [ -f "$cand/Qt6Config.cmake" ] && { printf '%s' "$cand"; return 0; }
        done
    else
        # кросс из Linux: только mingw-sysroot, НИКОГДА host-Qt
        for cand in \
            "/usr/x86_64-w64-mingw32/lib/cmake/Qt6" \
            "/opt/mxe/usr/x86_64-w64-mingw32.shared/lib/cmake/Qt6" \
            /opt/mxe/usr/x86_64-w64-mingw32.static/lib/cmake/Qt6; do
            [ -f "$cand/Qt6Config.cmake" ] && { printf '%s' "$cand"; return 0; }
        done
    fi
    return 1
}

build_windows_native() {
    # Нативная сборка на Windows (MSYS2/Git Bash)
    local win_build="$ROOT/build/win"
    echo "--- Windows native -> $win_build"

    local qt6cfg; qt6cfg="$(find_win_qt native)" || {
        log_skip "Qt6 не найден. В MSYS2 установите:" \
                 "  pacman -S mingw-w64-x86_64-qt6-base mingw-w64-x86_64-taglib \\" \
                 "  mingw-w64-x86_64-soundtouch" \
                 "или задайте ZMP_WIN_QT=/путь/к/lib/cmake/Qt6"
        return 1
    }
    local prefix; prefix="$(dirname "$(dirname "$qt6cfg")")"

    cmake -S "$ROOT" -B "$win_build" \
          -DCMAKE_PREFIX_PATH="$prefix" \
          -DCMAKE_BUILD_TYPE=Release || return 1
    cmake --build "$win_build" -j"$NPROC" || return 1
    log_ok
}

build_windows_cross() {
    # Кросс-компиляция из Linux
    local win_build="$ROOT/build/win"
    echo "--- Windows cross (mingw-w64) -> $win_build"

    if ! have_mingw_cross; then
        log_skip "mingw-w64 toolchain not found." \
                 "sudo pacman -S mingw-w64-gcc   (Arch)" \
                 "sudo apt install g++-mingw-w64-x86_64  (Debian)"
        return 0
    fi
    make_toolchain

    if ! find_win_qt cross >/dev/null; then
        log_skip "Qt6 (mingw-w64) не найден в sysroot." \
                 "paru -S mingw-w64-qt6-base mingw-w64-taglib mingw-w64-soundtouch" \
                 "или: ZMP_WIN_QT=/путь/к/lib/cmake/Qt6 ./compile-all.sh"
        return 0
    fi
    local qt6cfg; qt6cfg="$(find_win_qt cross)"
    local prefix; prefix="$(dirname "$(dirname "$qt6cfg")")"

    cmake -S "$ROOT" -B "$win_build" \
          -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
          -DCMAKE_PREFIX_PATH="$prefix" \
          -DCMAKE_BUILD_TYPE=Release || return 1
    cmake --build "$win_build" -j"$NPROC" || return 1
    log_ok
}

# ---------------------------------------------------------------------------
#  Сборка LINUX-версии
# ---------------------------------------------------------------------------

build_linux_native() {
    local linux_build="$ROOT/build"
    echo "--- Linux native -> $linux_build"
    cmake -S "$ROOT" -B "$linux_build" || return 1
    cmake --build "$linux_build" -j"$NPROC" || return 1
    log_ok
}

build_linux_wsl() {
    # Сборка Linux-версии из Windows через WSL
    local linux_build="$ROOT/build/linux"
    echo "--- Linux via WSL -> $linux_build"

    if ! command -v wsl.exe >/dev/null 2>&1; then
        log_skip "WSL не найден." \
                 "Установите: wsl --install -d Ubuntu" \
                 "затем внутри WSL: sudo apt install cmake g++ \\"
                 "  qt6-base-dev libqt6multimedia6 qml6-module-* libtag1-dev \\"
                 "  libsoundtouch-dev libprojectm-dev ffmpeg"
        return 0
    fi

    # Перевод пути в формат WSL (/mnt/c/...)
    local root_mixed root_wsl
    if command -v cygpath >/dev/null 2>&1; then
        root_mixed="$(cygpath -m "$ROOT")"
    else
        root_mixed="$ROOT"
    fi
    root_wsl="$(wsl.exe -e wslpath -a "$root_mixed" 2>/dev/null | tr -d '\r')"
    if [ -z "$root_wsl" ]; then
        log_skip "не удалось преобразовать путь для WSL"
        return 0
    fi

    wsl.exe -e bash -lc "cd '$root_wsl' && \
        cmake -S . -B build/linux && cmake --build build/linux -j\$(nproc)" \
        || { echo "=== WSL build FAILED"; return 1; }
    log_ok
}

# ---------------------------------------------------------------------------
#  Точка входа
# ---------------------------------------------------------------------------

if [ "$HOST" = "linux" ]; then
    echo "== Host: Linux"
    echo "[1/2] Linux version"
    build_linux_native   || { echo "Linux build FAILED"; exit 1; }
    echo "[2/2] Windows version"
    build_windows_cross  || { echo "Windows build FAILED"; exit 1; }
else
    echo "== Host: Windows"
    echo "[1/2] Windows version"
    build_windows_native || { echo "Windows build FAILED"; exit 1; }
    echo "[2/2] Linux version (via WSL)"
    build_linux_wsl      || { echo "Linux (WSL) build FAILED"; exit 1; }
fi

echo ""
echo "All done:"
[ -f "$ROOT/build/ZMP_Linux_bin" ]              && echo "  $ROOT/build/ZMP_Linux_bin"
find "$ROOT/build/win"   -maxdepth 1 -name '*.exe' 2>/dev/null | sed 's/^/  /'
find "$ROOT/build/linux" -maxdepth 2 -name 'ZMP_Linux_bin' 2>/dev/null | sed 's/^/  /'
exit 0
