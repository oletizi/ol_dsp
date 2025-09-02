# Ubuntu-based container for C++ builds with all JUCE dependencies
FROM ubuntu:24.04

LABEL org.opencontainers.image.description="OL_DSP C++ Build Environment"
LABEL org.opencontainers.image.source="https://github.com/oletizi/ol_dsp"

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install all C++ build dependencies in one layer
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
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
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /workspace

# Set default command
CMD ["bash"]