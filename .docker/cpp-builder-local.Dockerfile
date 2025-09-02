# C++ build environment extending local base image for testing
FROM ol_dsp/base:arm64-test

LABEL org.opencontainers.image.description="OL_DSP C++ Build Environment with Pre-built Libraries"
LABEL org.opencontainers.image.source="https://github.com/oletizi/ol_dsp"

# Pre-cache submodules to speed up CI builds (670MB+ of dependencies)
# Clone repositories and checkout specific commits defined in submodules.json
COPY submodules.json /tmp/submodules.json
COPY .docker/cache-submodules.py /tmp/cache-submodules.py
RUN python3 /tmp/cache-submodules.py

# Pre-build libraries to eliminate compile time in CI
# Copy minimal build configuration files needed for building
COPY CMakeLists.txt /tmp/build/CMakeLists.txt
COPY Makefile /tmp/build/Makefile
COPY .docker/prebuild-libraries.py /tmp/prebuild-libraries.py

# Create symlinks in temporary build directory and build libraries
RUN cd /tmp/build && \
    python3 /tmp/prebuild-libraries.py && \
    make && \
    mkdir -p /workspace/.prebuild_cache && \
    cp -r cmake-build /workspace/.prebuild_cache/ && \
    rm -rf /tmp/build /tmp/prebuild-libraries.py /tmp/cache-submodules.py /tmp/submodules.json

# Set default command
CMD ["bash"]