#!/bin/bash
# setup-deps.sh - OL_DSP Dependency Management
# 
# This script sets up all submodule dependencies in a separate .ol_dsp-deps directory
# Works identically on local development machines and in Docker containers

set -e

# Configuration
DEPS_DIR="../.ol_dsp-deps"
SUBMODULES_CONFIG="submodules.json"
SCRIPT_DIR="$(dirname "$0")"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "🔧 OL_DSP Dependency Setup"
echo "Project root: $PROJECT_ROOT"
echo "Dependencies directory: $DEPS_DIR"
echo ""

# Check if submodules.json exists
if [ ! -f "$PROJECT_ROOT/$SUBMODULES_CONFIG" ]; then
    echo "❌ Error: $SUBMODULES_CONFIG not found in project root"
    exit 1
fi

# Create dependencies directory
echo "📁 Creating dependencies directory..."
mkdir -p "$DEPS_DIR"
DEPS_FULL_PATH=$(cd "$DEPS_DIR" && pwd)
echo "✅ Dependencies will be installed to: $DEPS_FULL_PATH"
echo ""

# Read and process submodules.json
echo "📥 Reading submodule configuration..."

# Parse JSON and process each submodule
python3 -c "
import json
import os
import sys
import subprocess

def run_command(cmd, cwd=None, allow_failure=False):
    print(f'  Running: {cmd}')
    result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        if allow_failure:
            print(f'  ⚠️  Command failed (continuing): {result.stderr.strip()[:100]}...')
            return None
        else:
            print(f'  Error: {result.stderr}')
            sys.exit(1)
    return result.stdout

# Load configuration
project_root = os.path.abspath('$PROJECT_ROOT')
config_path = os.path.join(project_root, '$SUBMODULES_CONFIG')
print(f'Loading config from: {config_path}')
with open(config_path, 'r') as f:
    config = json.load(f)

deps_dir = '$DEPS_FULL_PATH'
print(f'Processing {len(config[\"submodules\"])} dependencies...')
print('')

for i, submodule in enumerate(config['submodules']):
    path = submodule['path']
    url = submodule['url']
    commit = submodule['commit']
    description = submodule.get('description', 'No description')
    
    print(f'{i+1}. {path}')
    print(f'   URL: {url}')
    print(f'   Commit: {commit}')
    print(f'   Description: {description}')
    
    # Target directory for this dependency
    target_path = os.path.join(deps_dir, path)
    
    # Clone if not exists, otherwise fetch
    if os.path.exists(target_path):
        print(f'   📥 Updating existing clone...')
        run_command('git fetch origin', cwd=target_path)
    else:
        print(f'   📥 Cloning...')
        os.makedirs(os.path.dirname(target_path), exist_ok=True)
        run_command(f'git clone \"{url}\" \"{target_path}\"')
    
    # Checkout specific commit
    print(f'   🔄 Checking out {commit}...')
    run_command(f'git checkout {commit}', cwd=target_path)
    
    # Pre-build if CMakeLists.txt exists (but skip JUCE to avoid memory issues in Docker)
    cmake_file = os.path.join(target_path, 'CMakeLists.txt')
    if os.path.exists(cmake_file):
        # Skip JUCE pre-build in Docker (too memory intensive)
        if 'JUCE' in path:
            print(f'   ⚠️  Skipping JUCE pre-build in Docker (memory intensive, will build on-demand)')
        else:
            build_dir = os.path.join(target_path, 'build')
            print(f'   🔨 Pre-building (CMake detected)...')
            os.makedirs(build_dir, exist_ok=True)
            
            # Configure
            config_result = run_command('cmake .. -DJUCE_BUILD_EXTRAS=ON -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF', cwd=build_dir, allow_failure=True)
            if config_result is None:
                print(f'   ⚠️  CMake configuration failed, skipping build')
            else:
                # Build
                import multiprocessing
                cores = multiprocessing.cpu_count()
                build_result = run_command(f'make -j{cores}', cwd=build_dir, allow_failure=True)
                if build_result is None:
                    print(f'   ⚠️  Build failed, but dependency is available')
                else:
                    print(f'   ✅ Pre-build complete')
    else:
        print(f'   ⚠️  No CMakeLists.txt found, skipping pre-build')
    
    print('')

print('🎉 All dependencies processed successfully!')
print('')
print('Next steps:')
print('  1. Update CMakeLists.txt to reference ../.ol_dsp-deps/ paths')
print('  2. Test build with: make')
"