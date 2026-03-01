#!/usr/bin/env bash

set -euo pipefail

TOOLS_DIR="${TOOLS_DIR:-/tools}"
ARM_NONE_EABI_VERSION="${ARM_NONE_EABI_VERSION:-10.3-2021.10}"
ARM_NONE_EABI_DIR_NAME="${ARM_NONE_EABI_DIR_NAME:-gcc-arm-none-eabi}"
CMAKE_VERSION="${CMAKE_VERSION:-3.31.3}"
DMOD_DMF_DIR="${DMOD_DMF_DIR:-/tools/dmf}"
DMOD_DMFC_DIR="${DMOD_DMFC_DIR:-/tools/dmfc}"
CHOCOLATE_SCRIPTS_URL="https://raw.githubusercontent.com/JohnAmadis/choco-scripts/refs/heads/master/install-choco-scripts.sh"
SKIP_CHOCO_SCRIPTS="false"
SKIP_PROFILE_SETUP="false"

print_help() {
    cat << 'EOF'
Prepare native Linux environment for DMOD (equivalent to Docker/Dockerfile.env).

Usage:
  ./scripts/setup-linux-env.sh [OPTIONS]

Options:
  --tools-dir PATH        Tools directory (default: /tools)
  --arm-version VERSION   arm-none-eabi version (default: 10.3-2021.10)
  --cmake-version VERSION CMake version (default: 3.31.3)
  --skip-choco-scripts    Skip install-choco-scripts.sh execution
  --skip-profile-setup    Do not write /etc/profile.d/dmod-tools.sh
  --help                  Show this help message

Environment overrides:
  TOOLS_DIR
  ARM_NONE_EABI_VERSION
  ARM_NONE_EABI_DIR_NAME
  CMAKE_VERSION
    DMOD_DMF_DIR
    DMOD_DMFC_DIR
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --tools-dir)
            TOOLS_DIR="$2"
            shift 2
            ;;
        --arm-version)
            ARM_NONE_EABI_VERSION="$2"
            shift 2
            ;;
        --cmake-version)
            CMAKE_VERSION="$2"
            shift 2
            ;;
        --skip-choco-scripts)
            SKIP_CHOCO_SCRIPTS="true"
            shift
            ;;
        --skip-profile-setup)
            SKIP_PROFILE_SETUP="true"
            shift
            ;;
        --help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown parameter: $1" >&2
            print_help
            exit 1
            ;;
    esac
done

if command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
else
    SUDO=""
fi

if [[ "${EUID}" -ne 0 ]] && [[ -z "${SUDO}" ]]; then
    echo "This script needs root privileges or sudo." >&2
    exit 1
fi

ARM_NONE_EABI_FILE_NAME="${ARM_NONE_EABI_DIR_NAME}-${ARM_NONE_EABI_VERSION}-x86_64-linux.tar.bz2"
ARM_NONE_EABI_FILE_PATH="/tmp/${ARM_NONE_EABI_FILE_NAME}"
ARM_NONE_EABI_DIR_PATH="${TOOLS_DIR}/${ARM_NONE_EABI_DIR_NAME}"
ARM_NONE_EABI_BIN_DIR_PATH="${ARM_NONE_EABI_DIR_PATH}/bin"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh"

echo "[1/6] Installing required apt packages..."
export DEBIAN_FRONTEND=noninteractive
${SUDO} apt-get update
${SUDO} apt-get install -y \
    wget gcc make g++ openocd gcovr git jq zip unzip \
    gdb-multiarch \
    libcurl4-openssl-dev libusb-1.0-0-dev ca-certificates tar bzip2

if [[ "${SKIP_CHOCO_SCRIPTS}" != "true" ]]; then
    echo "[2/6] Installing choco scripts..."
    wget -O - "${CHOCOLATE_SCRIPTS_URL}" | bash
else
    echo "[2/6] Skipping choco scripts installation (--skip-choco-scripts)."
fi

echo "[3/6] Installing ARM GNU toolchain ${ARM_NONE_EABI_VERSION} to ${TOOLS_DIR}..."

if [[ -d "${ARM_NONE_EABI_DIR_PATH}" && -x "${ARM_NONE_EABI_BIN_DIR_PATH}/arm-none-eabi-gcc" ]]; then
    echo "    ARM toolchain ${ARM_NONE_EABI_VERSION} already installed at ${ARM_NONE_EABI_DIR_PATH}"
else
    ${SUDO} mkdir -p "${TOOLS_DIR}"
    
    if [[ ! -f "${ARM_NONE_EABI_FILE_PATH}" ]]; then
        echo "    Downloading ARM toolkit (this may take a few minutes)..."
        wget "https://developer.arm.com/-/media/Files/downloads/gnu-rm/${ARM_NONE_EABI_VERSION}/${ARM_NONE_EABI_FILE_NAME}" -O "${ARM_NONE_EABI_FILE_PATH}"
    else
        echo "    Using cached ARM toolkit archive..."
    fi

    ${SUDO} tar xf "${ARM_NONE_EABI_FILE_PATH}" -C "${TOOLS_DIR}"

    if [[ -d "${ARM_NONE_EABI_DIR_PATH}" ]]; then
        ${SUDO} rm -rf "${ARM_NONE_EABI_DIR_PATH}"
    fi

    ${SUDO} mv "${TOOLS_DIR}/${ARM_NONE_EABI_DIR_NAME}-${ARM_NONE_EABI_VERSION}" "${ARM_NONE_EABI_DIR_PATH}"
    echo "    ARM toolchain installed successfully"
fi

${SUDO} mkdir -p "${DMOD_DMF_DIR}" "${DMOD_DMFC_DIR}"

echo "[4/6] Installing CMake ${CMAKE_VERSION} to /usr..."

if ! command -v cmake &>/dev/null || ! cmake --version | grep -q "${CMAKE_VERSION}"; then
    CMAKE_SCRIPT="/tmp/cmake.sh"
    if [[ ! -f "${CMAKE_SCRIPT}" ]]; then
        echo "    Downloading CMake (this may take a few minutes)..."
        wget "${CMAKE_URL}" -O "${CMAKE_SCRIPT}"
    else
        echo "    Using cached CMake installer..."
    fi
    
    chmod +x "${CMAKE_SCRIPT}"
    ${SUDO} "${CMAKE_SCRIPT}" --skip-license --prefix=/usr
    rm -f "${CMAKE_SCRIPT}"
    echo "    CMake installed successfully"
else
    echo "    CMake ${CMAKE_VERSION} already installed"
fi

if [[ "${SKIP_PROFILE_SETUP}" != "true" ]]; then
    echo "[5/6] Configuring PATH in /etc/profile.d/dmod-tools.sh and ~/.bashrc..."
    ${SUDO} tee /etc/profile.d/dmod-tools.sh >/dev/null << EOF
#!/usr/bin/env sh
export DMOD_DMF_DIR="${DMOD_DMF_DIR}"
export DMOD_DMFC_DIR="${DMOD_DMFC_DIR}"
if [ -d "${ARM_NONE_EABI_BIN_DIR_PATH}" ]; then
    case ":\$PATH:" in
        *:"${ARM_NONE_EABI_BIN_DIR_PATH}":*) ;;
        *) export PATH="\$PATH:${ARM_NONE_EABI_BIN_DIR_PATH}" ;;
    esac
fi
EOF
    ${SUDO} chmod 644 /etc/profile.d/dmod-tools.sh
    
    # Add to user's bashrc for non-login shells
    if [[ -f "${HOME}/.bashrc" ]]; then
        if ! grep -q "dmod-tools.sh" "${HOME}/.bashrc"; then
            echo "" >> "${HOME}/.bashrc"
            echo "# DMOD tools PATH (added by setup-linux-env.sh)" >> "${HOME}/.bashrc"
            echo "if [ -f /etc/profile.d/dmod-tools.sh ]; then" >> "${HOME}/.bashrc"
            echo "    . /etc/profile.d/dmod-tools.sh" >> "${HOME}/.bashrc"
            echo "fi" >> "${HOME}/.bashrc"
        fi
    fi
else
    echo "[5/6] Skipping profile setup (--skip-profile-setup)."
fi

echo "[6/6] Verifying installed tools..."
if [[ -x "${ARM_NONE_EABI_BIN_DIR_PATH}/arm-none-eabi-gcc" ]]; then
    "${ARM_NONE_EABI_BIN_DIR_PATH}/arm-none-eabi-gcc" --version | head -n 1
else
    echo "arm-none-eabi-gcc not found in ${ARM_NONE_EABI_BIN_DIR_PATH}" >&2
    exit 1
fi

cmake --version | head -n 1
openocd --version 2>&1 | head -n 1

echo ""
echo "Environment prepared."
echo "Restart your terminal or run: source ~/.bashrc"
