# C++ build environment extending base image with cached submodules and pre-built libraries
ARG GITHUB_REPOSITORY=oletizi/ol_dsp
FROM ghcr.io/${GITHUB_REPOSITORY}/base:latest

LABEL org.opencontainers.image.description="OL_DSP C++ Build Environment with Pre-built Libraries"
LABEL org.opencontainers.image.source="https://github.com/oletizi/ol_dsp"

# Pre-cache submodules directly in the expected locations (libs/, test/)
# Clone repositories and checkout specific commits defined in submodules.json
COPY submodules.json /tmp/submodules.json
COPY .docker/cache-submodules.py /tmp/cache-submodules.py
RUN python3 /tmp/cache-submodules.py --target-dir /workspace

# Pre-build JUCE using its own build system in the expected location
# juceaide is automatically built during CMake configuration when DJUCE_BUILD_EXTRAS=ON
RUN cd /workspace/libs/JUCE && \
    mkdir build && \
    cd build && \
    cmake .. -DJUCE_BUILD_EXTRAS=ON && \
    echo "JUCE pre-build complete in expected location"

# Pre-build CMake-compatible dependencies in their expected locations
RUN echo "Pre-building CMake-compatible dependencies..." && \
    cd /workspace && \
    for lib_path in libs/stk libs/rtmidi test/googletest test/FakeIt; do \
      lib=$(basename "$lib_path"); \
      echo "Attempting to pre-build $lib at $lib_path..."; \
      if [ -d "$lib_path" ] && [ -f "$lib_path/CMakeLists.txt" ]; then \
        cd "$lib_path" && \
        mkdir -p build && \
        cd build && \
        (cmake .. -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF && make -j$(nproc) && echo "$lib pre-build complete") || echo "$lib pre-build failed, continuing..."; \
        cd /workspace; \
      else \
        echo "$lib: No CMakeLists.txt found or directory missing, skipping"; \
      fi; \
    done

# Clean up temporary files
RUN rm /tmp/cache-submodules.py /tmp/submodules.json

# Set default command
CMD ["bash"]