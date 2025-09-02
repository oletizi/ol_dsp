#!/usr/bin/env python3
"""
Script to set up symlinks for pre-building libraries in Docker image
Creates symlinks from cached submodules to expected paths for building
"""

import json
import os
import sys

def main():
    cache_dir = '/workspace/.submodule_cache'
    config_file = '/tmp/submodules.json'
    
    # Read the same submodules configuration
    if not os.path.exists(config_file):
        print(f"Error: Config file not found: {config_file}")
        sys.exit(1)
    
    with open(config_file, 'r') as f:
        config = json.load(f)
    
    print("Setting up symlinks for pre-building libraries...")
    
    for submodule in config['submodules']:
        path = submodule['path']
        url = submodule['url']
        cache_name = os.path.basename(url).replace('.git', '')
        cache_path = os.path.join(cache_dir, cache_name)
        
        if not os.path.exists(cache_path):
            print(f"Error: Cached repository not found: {cache_path}")
            sys.exit(1)
        
        # Create parent directories for the symlink path
        os.makedirs(os.path.dirname(path), exist_ok=True)
        
        # Create symlink
        if os.path.exists(path) or os.path.islink(path):
            os.unlink(path)
        
        os.symlink(cache_path, path)
        print(f"Created symlink: {path} -> {cache_path}")
    
    print(f"Successfully set up {len(config['submodules'])} symlinks for pre-building")

if __name__ == '__main__':
    main()