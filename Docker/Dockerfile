FROM chocotechnologies/dmod-env:latest

COPY . /tmp/dmod
WORKDIR /tmp/dmod
RUN rm -rf build
RUN ls -la /tmp/dmod
ENV DMOD_DMF_DIR=/tools/dmf
ENV DMOD_DMFC_DIR=/tools/dmfc
RUN mkdir -p $DMOD_DMF_DIR
RUN mkdir -p $DMOD_DMFC_DIR
# Built in two configure passes: dod_link_builtin (used by dod_loader to
# statically link system modules like dmlist/dmosi) looks up dmf-get via
# find_program() at *configure* time, but dmf-get is only produced by the
# *build* step - in a from-scratch environment like this one (no dmf-get
# anywhere on the system yet), the first configure would cache "not found"
# and dod_link_builtin would silently skip linking every system module,
# with no build failure to signal it. Disabling examples for the first pass
# means dod_link_builtin is never invoked before dmf-get exists, so nothing
# gets cached as missing; building dmf-get and reconfiguring with examples
# back on lets find_program() locate the now-built binary.
RUN mkdir -p build && \
    cd build && \
    cmake .. -DDMOD_DMF_DIR=${DMOD_DMF_DIR} -DDMOD_DMFC_DIR=${DMOD_DMFC_DIR} -DDMOD_BUILD_EXAMPLES=OFF && \
    cmake --build . --target dmf-get && \
    cmake .. -DDMOD_BUILD_EXAMPLES=ON && \
    cmake --build . && \
    cmake --install . --prefix=/usr/local --component tools

# Clean up source code to keep image minimal
RUN rm -rf /tmp/dmod

# Ensure tools are in PATH
ENV PATH=$PATH:/usr/local/bin

WORKDIR /
