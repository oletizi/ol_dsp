#!/bin/bash

# setup-submodules.sh
# Fast submodule setup using symlinks to cached repositories
# This avoids cloning 670MB of submodules on every CI run
# Reads configuration from submodules.json
# Uses pre-built cache if available

set -e

# Base directory for cached submodules (can be baked into Docker image)
CACHE_DIR="${SUBMODULE_CACHE:-/workspace/.submodule_cache}"
PREBUILD_CACHE="/workspace/.prebuild_cache"
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CONFIG_FILE="$PROJECT_ROOT/submodules.json"

echo "Setting up submodules using cache at: $CACHE_DIR"

# Check if we have pre-built libraries available
if [ -d "$PREBUILD_CACHE" ]; then
    echo "Found pre-built libraries cache, copying to project..."
    cp -r "$PREBUILD_CACHE"/* "$PROJECT_ROOT/"
    echo "Pre-built libraries copied successfully"
fi

# Check if config file exists
if [ ! -f "$CONFIG_FILE" ]; then
    echo "Error: Configuration file not found: $CONFIG_FILE"
    exit 1
fi

# Create cache directory if it doesn't exist
mkdir -p "$CACHE_DIR"

cd "$PROJECT_ROOT"

# Parse JSON config and process each submodule
# Using python for JSON parsing (available in most environments)
python3 -c "
import json
import sys
import os

with open('$CONFIG_FILE', 'r') as f:
    config = json.load(f)

for submodule in config['submodules']:
    path = submodule['path']
    url = submodule['url']
    commit = submodule['commit']
    cache_name = os.path.basename(url).replace('.git', '')
    cache_path = os.path.join('$CACHE_DIR', cache_name)
    
    print(f'Processing {path} -> {cache_name} @ {commit[:8]}')
    
    # Clone to cache if doesn't exist
    if not os.path.exists(cache_path):
        print(f'  Cloning {url} to cache...')
        os.system(f'git clone \"{url}\" \"{cache_path}\"')
    else:
        print(f'  Using cached repository at {cache_path}')
    
    # Ensure we're on the correct commit
    print(f'  Checking out commit {commit[:8]}...')
    os.system(f'cd \"{cache_path}\" && git fetch && git checkout {commit}')
    
    # Remove existing submodule directory/symlink if it exists
    if os.path.exists(path) or os.path.islink(path):
        os.system(f'rm -rf \"{path}\"')
    
    # Create parent directories
    os.makedirs(os.path.dirname(path), exist_ok=True)
    
    # Create symlink
    os.symlink(cache_path, path)
    print(f'  Created symlink: {path} -> {cache_path}')

print('Submodule setup complete!')
print(f'Total repositories: {len(config[\"submodules\"])}')
cache_size = os.popen(f'du -sh \"$CACHE_DIR\" 2>/dev/null | cut -f1').read().strip() or 'unknown size'
print(f'Cache directory: $CACHE_DIR ({cache_size})')
"