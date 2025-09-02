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

# Clean up temporary files
RUN rm /tmp/cache-submodules.py /tmp/submodules.json

# Set default command
CMD ["bash"]