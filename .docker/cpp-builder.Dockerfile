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

# Pre-build JUCE using its own build system directly in cache
# juceaide is automatically built during CMake configuration when DJUCE_BUILD_EXTRAS=ON
RUN cd /workspace/.submodule_cache/JUCE && \
    mkdir build && \
    cd build && \
    cmake .. -DJUCE_BUILD_EXTRAS=ON && \
    echo "JUCE pre-build complete in cache"

# Pre-build CMake-compatible dependencies that are already cached
RUN echo "Pre-building CMake-compatible dependencies..." && \
    cd /workspace/.submodule_cache && \
    for lib in stk rtmidi googletest FakeIt; do \
      echo "Attempting to pre-build $lib..."; \
      if [ -d "$lib" ] && [ -f "$lib/CMakeLists.txt" ]; then \
        cd "$lib" && \
        mkdir -p build && \
        cd build && \
        (cmake .. -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF && make -j$(nproc) && echo "$lib pre-build complete") || echo "$lib pre-build failed, continuing..."; \
        cd /workspace/.submodule_cache; \
      else \
        echo "$lib: No CMakeLists.txt found or directory missing, skipping"; \
      fi; \
    done

# Clean up temporary files
RUN rm /tmp/cache-submodules.py /tmp/submodules.json

# Set default command
CMD ["bash"]