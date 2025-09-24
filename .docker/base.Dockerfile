# Base Ubuntu image with all system dependencies for OL_DSP
# This image contains all the heavy dependencies that rarely change
FROM ubuntu:24.04

LABEL org.opencontainers.image.description="OL_DSP Base Dependencies"
LABEL org.opencontainers.image.source="https://github.com/oletizi/ol_dsp"

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install all system dependencies in one layer (this is the heaviest part)
RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    pkg-config \
    wget \
    libasound2-dev \
    libjack-jackd2-dev \
    ladspa-sdk \
    libcurl4-openssl-dev \
    libfreetype6-dev \
    libx11-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxext-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev \
    libwebkit2gtk-4.1-dev \
    libglu1-mesa-dev \
    mesa-common-dev \
    libegl1-mesa-dev \
    libgl1-mesa-dev \
    libudev-dev \
    libopenal-dev \
    libvorbis-dev \
    libogg-dev \
    libflac-dev \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Install newer CMake from Kitware (also heavy/slow)
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - > /usr/share/keyrings/kitware-archive-keyring.gpg && \
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ noble main' > /etc/apt/sources.list.d/kitware.list && \
    apt-get update && \
    apt-get install -y cmake && \
    rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Default command
CMD ["bash"]