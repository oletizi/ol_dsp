# Docker image for running GitHub Actions locally with act CLI
# This provides a consistent environment that closely matches GitHub Actions runners

FROM ubuntu:24.04

# Install system dependencies
RUN apt-get update && apt-get install -y \
    curl \
    git \
    docker.io \
    wget \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install act CLI
RUN curl -s https://raw.githubusercontent.com/nektos/act/master/install.sh | bash

# Install GitHub CLI (needed for some workflows)
RUN curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg | dd of=/usr/share/keyrings/githubcli-archive-keyring.gpg \
    && chmod go+r /usr/share/keyrings/githubcli-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" | tee /etc/apt/sources.list.d/github-cli.list > /dev/null \
    && apt-get update \
    && apt-get install gh -y \
    && rm -rf /var/lib/apt/lists/*

# Configure act to use medium-sized images by default
RUN mkdir -p /root/.config/act && \
    echo "-P ubuntu-latest=catthehacker/ubuntu:act-latest" > /root/.config/act/actrc

# Set working directory
WORKDIR /workspace

# Default command runs act with common options
CMD ["act", "--help"]