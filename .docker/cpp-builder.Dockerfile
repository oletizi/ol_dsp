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

# Pre-build JUCE with a minimal CMake setup
# This avoids copying application code that changes frequently
WORKDIR /tmp/juce-prebuild

# Create symlinks to cached submodules first
COPY scripts/setup-submodules.sh .
COPY submodules.json .
RUN ./setup-submodules.sh

# Create a minimal CMakeLists.txt that only builds JUCE
RUN cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.30)
project(juce_prebuild VERSION 0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE with minimal configuration
add_subdirectory(libs/JUCE)

# Build juceaide tool
set(JUCE_BUILD_EXTRAS ON)
EOF

# Pre-build JUCE (juceaide and core libraries)
RUN mkdir cmake-build && \
    cd cmake-build && \
    cmake .. && \
    make -j$(nproc) && \
    echo "JUCE pre-build complete"

# Copy the pre-built JUCE artifacts to cache location
RUN mkdir -p /opt/prebuild_cache && \
    cp -r cmake-build /opt/prebuild_cache/ && \
    cp -r libs /opt/prebuild_cache/ && \
    echo "Pre-built JUCE cache created at /opt/prebuild_cache"

# Clean up temporary build directory
WORKDIR /workspace
RUN rm -rf /tmp/juce-prebuild

# Clean up temporary files
RUN rm /tmp/cache-submodules.py /tmp/submodules.json

# Set default command
CMD ["bash"]