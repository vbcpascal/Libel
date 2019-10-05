SCRIPT_DIR="$(dirname ${BASH_SOURCE[0]})"
SRC_DIR="$(realpath ${SCRIPT_DIR}/../)"
BUILD_DIR="${SRC_DIR}/build"
TEST_DIR="${BUILD_DIR}/tests"

VNET_DIR="${SRC_DIR}/utils/vnetUtils"
VNET_HELPER_DIR="${VNET_DIR}/helper"

echo "TEST_DIR=${TEST_DIR}"

pushd "${VNET_HELPER_DIR}"
./execNS vnetns2 sudo "${TEST_DIR}/testTinyShell" < "${SRC_DIR}/tests/sniffcmd.txt"
popd
