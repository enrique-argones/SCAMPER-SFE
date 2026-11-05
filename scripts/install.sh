#!/bin/sh
# Builds, tests and installs every SCAMPER SFE module (libraries and programs) in Release and Debug configurations.
#
# Usage: scripts/install.sh [-p PREFIX] [-b BUILD_DIR] [-c Release|Debug|both] [-t] [-y]
#   -p PREFIX     install prefix (default: /usr/local, CMake's default)
#   -b BUILD_DIR  build folder (default: ./build)
#   -c CONFIG     configurations to build: Release, Debug or both (default: both)
#   -t            run the tests (ctest) before installing
#   -y            do not ask for confirmation
#
# "sudo" is only used for the installation step, and only when PREFIX is not writable.
set -e

SOURCE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$SOURCE_DIR/build"
PREFIX=""
CONFIGS="Release Debug"
RUN_TESTS=no
ASK=yes

while getopts "p:b:c:tyh" option; do
    case "$option" in
        p) PREFIX="$OPTARG" ;;
        b) BUILD_DIR="$OPTARG" ;;
        c) case "$OPTARG" in
               both) CONFIGS="Release Debug" ;;
               Release|Debug) CONFIGS="$OPTARG" ;;
               *) echo "Unknown configuration: $OPTARG (use Release, Debug or both)" >&2; exit 1 ;;
           esac ;;
        t) RUN_TESTS=yes ;;
        y) ASK=no ;;
        h|*) sed -n '2,11p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    esac
done

CMAKE_PREFIX_ARGS=""
SUDO=""
if [ -n "$PREFIX" ]; then
    mkdir -p "$PREFIX"
    PREFIX="$(cd "$PREFIX" && pwd)"
    CMAKE_PREFIX_ARGS="-DCMAKE_INSTALL_PREFIX=$PREFIX -DCMAKE_PREFIX_PATH=$PREFIX"
    [ -w "$PREFIX" ] || SUDO=sudo
else
    [ -w /usr/local ] || SUDO=sudo
fi

echo "Sources:        $SOURCE_DIR"
echo "Build folder:   $BUILD_DIR"
echo "Install prefix: ${PREFIX:-CMake default (/usr/local)}"
echo "Configurations: $CONFIGS"
if [ "$ASK" = yes ]; then
    printf "Continue? (y/N) "
    read -r answer
    case "$answer" in y|Y|yes|YES) ;; *) exit 0 ;; esac
fi

for CONFIG in $CONFIGS; do
    CONFIG_BUILD_DIR="$BUILD_DIR/$(echo "$CONFIG" | tr '[:upper:]' '[:lower:]')"
    # shellcheck disable=SC2086
    cmake -S "$SOURCE_DIR" -B "$CONFIG_BUILD_DIR" -DCMAKE_BUILD_TYPE="$CONFIG" $CMAKE_PREFIX_ARGS
    cmake --build "$CONFIG_BUILD_DIR" -j
    if [ "$RUN_TESTS" = yes ]; then
        (cd "$CONFIG_BUILD_DIR" && ctest --output-on-failure)
    fi
    $SUDO cmake --install "$CONFIG_BUILD_DIR"
done
