# C++ build environment with pre-built dependencies using .ol_dsp-deps approach
ARG GITHUB_REPOSITORY=oletizi/ol_dsp
FROM ghcr.io/${GITHUB_REPOSITORY}/base:latest

LABEL org.opencontainers.image.description="OL_DSP C++ Build Environment with Pre-built Dependencies"
LABEL org.opencontainers.image.source="https://github.com/oletizi/ol_dsp"

# Clone ol_dsp repository to get the dependency setup scripts
RUN git clone https://github.com/oletizi/ol_dsp.git /tmp/ol_dsp

# Copy dependency configuration and setup script
COPY submodules.json /tmp/ol_dsp/submodules.json
COPY scripts/setup-deps.sh /tmp/ol_dsp/scripts/setup-deps.sh
RUN chmod +x /tmp/ol_dsp/scripts/setup-deps.sh

# Set up dependencies using our new .ol_dsp-deps approach
# This creates /workspace/.ol_dsp-deps with all pre-built dependencies  
RUN cd /tmp/ol_dsp && \
    sed -i 's|DEPS_DIR="../.ol_dsp-deps"|DEPS_DIR="/workspace/.ol_dsp-deps"|' scripts/setup-deps.sh && \
    ./scripts/setup-deps.sh

# Clean up temporary repository
RUN rm -rf /tmp/ol_dsp

# Set default command
CMD ["bash"]