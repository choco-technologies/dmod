FROM ubuntu:20.04

# This line installs wget tool - you don't need to use it if you already have it
RUN apt-get update && apt-get install -y wget gcc

# This downloads an installation script and run it 
RUN wget -O - https://release.choco-technologies.com/scripts/install-choco-scripts.sh | bash

# Creates directory for the tools
ENV TOOLS_DIR=/tools
RUN mkdir $TOOLS_DIR

# Moves a arm-none-eabi package into the image
ENV ARM_NONE_EABI_VERSION=10.3-2021.10
ENV ARM_NONE_EABI_DIR_NAME=gcc-arm-none-eabi
ENV ARM_NONE_EABI_FILE_NAME=$ARM_NONE_EABI_DIR_NAME-$ARM_NONE_EABI_VERSION-x86_64-linux.tar.bz2
ENV ARM_NONE_EABI_FILE_PATH=$TOOLS_DIR/$ARM_NONE_EABI_FILE_NAME
WORKDIR $TOOLS_DIR
RUN wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/$ARM_NONE_EABI_VERSION/$ARM_NONE_EABI_FILE_NAME

# Installs arm-none-eabi tools (version currently it is 10.3)
WORKDIR /tools
RUN tar xf $ARM_NONE_EABI_FILE_PATH
RUN rm -f $ARM_NONE_EABI_FILE_PATH
# Removing of version suffix
ENV ARM_NONE_EABI_DIR_PATH=$TOOLS_DIR/$ARM_NONE_EABI_DIR_NAME
RUN mv $ARM_NONE_EABI_DIR_PATH-$ARM_NONE_EABI_VERSION $ARM_NONE_EABI_DIR_PATH
ENV ARM_NONE_EABI_BIN_DIR_PATH=$ARM_NONE_EABI_DIR_PATH/bin
ENV PATH=$PATH:$ARM_NONE_EABI_BIN_DIR_PATH

# Installs cmake from the repository
ENV CMAKE_VERSION=3.31.3
ENV CMAKE_URL=https://github.com/Kitware/CMake/releases/download/v$CMAKE_VERSION/cmake-$CMAKE_VERSION-linux-x86_64.sh
RUN wget $CMAKE_URL -O /tmp/cmake.sh
RUN chmod +x /tmp/cmake.sh
RUN /tmp/cmake.sh --skip-license --prefix=/usr

# Installs openocd (version currently available is 0.10.0)
RUN apt-get install -y openocd

# Installs the other required tools 
RUN apt-get install -y make libusb-1.0 g++
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y gcovr

RUN apt-get update && apt-get install -y git jq zip unzip

COPY . /tmp/dmod
WORKDIR /tmp/dmod
RUN rm -rf build
RUN ls -la /tmp/dmod
ENV DMOD_DMF_DIR=/tools/dmf
ENV DMOD_DMFC_DIR=/tools/dmfc
RUN mkdir -p $DMOD_DMF_DIR
RUN mkdir -p $DMOD_DMFC_DIR
RUN mkdir -p build && \
    cd build && \
    cmake .. -DDMOD_DMF_DIR=${DMOD_DMF_DIR} -DDMOD_DMFC_DIR=${DMOD_DMFC_DIR} && \
    cmake --build . && \
    cmake --install . --prefix=/usr/local --component tools

# Clean up source code to keep image minimal
RUN rm -rf /tmp/dmod

# Ensure tools are in PATH
ENV PATH=$PATH:/usr/local/bin

WORKDIR /
