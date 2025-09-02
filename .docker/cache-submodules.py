#!/usr/bin/env python3
"""
Script to cache submodules for Docker image
Reads from submodules.json and clones repositories at specific commits
"""

import argparse
import json
import os
import sys

def main():
    parser = argparse.ArgumentParser(description='Cache submodules in expected locations')
    parser.add_argument('--target-dir', default='/workspace/.submodule_cache', 
                       help='Base directory for cloning (default: /workspace/.submodule_cache)')
    args = parser.parse_args()
    
    config_file = '/tmp/submodules.json'
    base_dir = args.target_dir
    
    if not os.path.exists(config_file):
        print(f"Error: Config file not found: {config_file}")
        sys.exit(1)
    
    with open(config_file, 'r') as f:
        config = json.load(f)
    
    for submodule in config['submodules']:
        path = submodule['path']
        url = submodule['url']
        commit = submodule['commit']
        
        # Clone directly into expected location
        target_path = os.path.join(base_dir, path)
        
        print(f'Cloning {path} @ {commit[:8]} to {target_path}...')
        
        # Create parent directory
        os.makedirs(os.path.dirname(target_path), exist_ok=True)
        
        # Clone the repository
        clone_result = os.system(f'git clone "{url}" "{target_path}"')
        if clone_result != 0:
            print(f'Error cloning {url} to {target_path}')
            sys.exit(1)
        
        # Checkout specific commit
        checkout_result = os.system(f'cd "{target_path}" && git checkout {commit}')
        if checkout_result != 0:
            print(f'Error checking out commit {commit} in {target_path}')
            sys.exit(1)
    
    print(f'Successfully cloned {len(config["submodules"])} repositories to expected locations')
    
    # Create symlinks for path mismatches between .gitmodules and CMakeLists.txt
    symlinks = [
        ('DaisySP', 'libs/DaisySP')
    ]
    
    for source, target in symlinks:
        source_path = os.path.join(base_dir, source)
        target_path = os.path.join(base_dir, target)
        
        if os.path.exists(source_path):
            # Create parent directory for target
            os.makedirs(os.path.dirname(target_path), exist_ok=True)
            
            # Create symlink
            if not os.path.exists(target_path):
                os.symlink(os.path.relpath(source_path, os.path.dirname(target_path)), target_path)
                print(f'Created symlink: {target} -> {source}')
            else:
                print(f'Symlink already exists: {target}')
        else:
            print(f'Source not found for symlink: {source}')

if __name__ == '__main__':
    main()