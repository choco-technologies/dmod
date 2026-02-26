FROM chocotechnologies/dmod-env:latest

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
