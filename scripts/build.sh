CMAKE="${MAKE:-cmake}"

MAKE="${MAKE:-make}"

realpath() {
    cd "$@" && pwd
}

SCRIPT_DIR="$(dirname ${BASH_SOURCE[0]})"
SRC_DIR="$(realpath ${SCRIPT_DIR}/../)"
BUILD_DIR="${SRC_DIR}/build/"

echo "SRC_DIR=${SRC_DIR}"
echo "BUILD_DIR=${BUILD_DIR}"

die() {
    echo "Error: $1" >&2
    exit 1
}

if ! $MAKE --help &>/dev/null; then
    die "GNU make is required."
fi

echo "Configuring project with $(which $CMAKE)..."

rm -rf "${BUILD_DIR}" && mkdir "${BUILD_DIR}"

pushd "${BUILD_DIR}"
$CMAKE "${SRC_DIR}" || die "Failed to configure project"
$MAKE -j$(nproc) || die "Failed to build project"
popd

echo "Build succeed."
