FROM ubuntu:24.04

# Install minimal requirements
RUN apt-get update && apt-get install -y \
    git \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Test submodule caching
COPY submodules.json /tmp/submodules.json  
COPY .docker/cache-submodules.py /tmp/cache-submodules.py
RUN python3 /tmp/cache-submodules.py && \
    ls -la /workspace/.submodule_cache/ && \
    echo "Cache size:" && \
    du -sh /workspace/.submodule_cache/ || echo "Cache check failed"

CMD ["bash"]