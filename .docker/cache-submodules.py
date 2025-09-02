#!/usr/bin/env python3
"""
Script to cache submodules for Docker image
Reads from submodules.json and clones repositories at specific commits
"""

import json
import os
import sys

def main():
    config_file = '/tmp/submodules.json'
    cache_dir = '/workspace/.submodule_cache'
    
    if not os.path.exists(config_file):
        print(f"Error: Config file not found: {config_file}")
        sys.exit(1)
    
    with open(config_file, 'r') as f:
        config = json.load(f)
    
    os.makedirs(cache_dir, exist_ok=True)
    
    for submodule in config['submodules']:
        url = submodule['url']
        commit = submodule['commit']
        cache_name = os.path.basename(url).replace('.git', '')
        cache_path = os.path.join(cache_dir, cache_name)
        
        print(f'Caching {cache_name} @ {commit[:8]}...')
        
        # Clone the repository
        clone_result = os.system(f'git clone "{url}" "{cache_path}"')
        if clone_result != 0:
            print(f'Error cloning {url}')
            sys.exit(1)
        
        # Checkout specific commit
        checkout_result = os.system(f'cd "{cache_path}" && git checkout {commit}')
        if checkout_result != 0:
            print(f'Error checking out commit {commit} in {cache_path}')
            sys.exit(1)
    
    print(f'Successfully cached {len(config["submodules"])} repositories')

if __name__ == '__main__':
    main()