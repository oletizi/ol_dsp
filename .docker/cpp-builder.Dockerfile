# C++ build environment extending base image with cached submodules and pre-built libraries
ARG GITHUB_REPOSITORY=oletizi/ol_dsp
FROM ghcr.io/${GITHUB_REPOSITORY}/base:latest

LABEL org.opencontainers.image.description="OL_DSP C++ Build Environment with Pre-built Libraries"
LABEL org.opencontainers.image.source="https://github.com/oletizi/ol_dsp"

# Pre-cache submodules to speed up CI builds (670MB+ of dependencies)
# Clone repositories and checkout specific commits defined in submodules.json
COPY submodules.json /tmp/submodules.json
COPY .docker/cache-submodules.py /tmp/cache-submodules.py
RUN python3 /tmp/cache-submodules.py

# Create a temporary workspace to pre-build JUCE
WORKDIR /tmp/prebuild
COPY CMakeLists.txt .
COPY cmake ./cmake
COPY modules ./modules
COPY libs ./libs
COPY test ./test
COPY scripts/setup-submodules.sh ./scripts/setup-submodules.sh

# Setup submodules and run a minimal build to compile juceaide and JUCE libraries
RUN ./scripts/setup-submodules.sh && \
    mkdir cmake-build && \
    cd cmake-build && \
    cmake .. && \
    make -j$(nproc) juceaide && \
    echo "JUCE pre-build complete"

# Copy the pre-built artifacts to a cache location
RUN mkdir -p /opt/prebuild_cache && \
    cp -r cmake-build /opt/prebuild_cache/ && \
    cp -r libs /opt/prebuild_cache/ && \
    echo "Pre-built cache created at /opt/prebuild_cache"

# Clean up temporary build directory
WORKDIR /workspace
RUN rm -rf /tmp/prebuild

# Clean up temporary files
RUN rm /tmp/cache-submodules.py /tmp/submodules.json

# Set default command
CMD ["bash"]