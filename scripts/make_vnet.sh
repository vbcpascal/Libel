SCRIPT_DIR="$(dirname ${BASH_SOURCE[0]})"
SRC_DIR="$(realpath ${SCRIPT_DIR}/../)"
BUILD_DIR="${SRC_DIR}/build/"

VNET_DIR="${SRC_DIR}/utils/vnetUtils"
VNET_EXAMPLE_DIR="${VNET_DIR}/examples"

pushd "${VNET_EXAMPLE_DIR}"
./removeVNet < ./config.txt 
./makeVNet < ./config.txt
popd

echo "Build succeed. Devices:"

ls /var/run/netns