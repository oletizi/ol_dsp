# Node.js-based container for npm workspace builds
FROM node:20-slim

LABEL org.opencontainers.image.description="OL_DSP Node.js Build Environment"
LABEL org.opencontainers.image.source="https://github.com/oletizi/ol_dsp"

# Install system dependencies needed for native modules (but not MIDI)
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    git \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Enable corepack for modern npm features
RUN corepack enable

# Set default command
CMD ["bash"]