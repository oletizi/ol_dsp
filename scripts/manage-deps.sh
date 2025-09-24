#!/bin/bash
# manage-deps.sh - OL_DSP Dependency Management
#
# This script manages all dependencies in a separate .ol_dsp-deps directory
# Works identically on local development machines and in Docker containers
#
# Usage:
#   manage-deps.sh [setup|clean]
#   manage-deps.sh          # defaults to setup

set -e

# Configuration
DEPS_DIR="../.ol_dsp-deps"
DEPS_CONFIG="dependencies.json"
SCRIPT_DIR="$(dirname "$0")"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
ACTION="${1:-setup}"

echo "🔧 OL_DSP Dependency Management"
echo "Project root: $PROJECT_ROOT"
echo "Dependencies directory: $DEPS_DIR"
echo "Action: $ACTION"
echo ""

# Handle clean action
if [ "$ACTION" = "clean" ]; then
    echo "🧹 Cleaning dependency builds..."

    # Check if dependencies.json exists
    if [ ! -f "$PROJECT_ROOT/$DEPS_CONFIG" ]; then
        echo "❌ Error: $DEPS_CONFIG not found in project root"
        exit 1
    fi

    if [ -d "$DEPS_DIR" ]; then
        DEPS_FULL_PATH=$(cd "$DEPS_DIR" && pwd)
        echo "Cleaning build artifacts in: $DEPS_FULL_PATH"
        echo ""

        # Use Python to parse the new configuration format
        python3 -c "
import json
import os
import shutil

# Load configuration
project_root = os.path.abspath('$PROJECT_ROOT')
config_path = os.path.join(project_root, '$DEPS_CONFIG')
with open(config_path, 'r') as f:
    config = json.load(f)

deps_dir = '$DEPS_FULL_PATH'
cleaned_count = 0

for dependency in config['dependencies']:
    name = dependency['name']
    path = dependency['path']
    clean_config = dependency.get('clean', {})

    dep_path = os.path.join(deps_dir, path)
    if not os.path.exists(dep_path):
        continue

    print(f'🔍 {name} ({path})')

    # Clean directories
    for directory in clean_config.get('directories', []):
        dir_path = os.path.join(dep_path, directory)
        if os.path.exists(dir_path):
            print(f'  🗑️  Removing directory: {directory}')
            shutil.rmtree(dir_path)
            cleaned_count += 1
        else:
            print(f'  ℹ️  Directory not found: {directory}')

    # Clean files
    for file_name in clean_config.get('files', []):
        file_path = os.path.join(dep_path, file_name)
        if os.path.exists(file_path):
            print(f'  🗑️  Removing file: {file_name}')
            os.remove(file_path)
            cleaned_count += 1
        else:
            print(f'  ℹ️  File not found: {file_name}')

    if not clean_config.get('directories', []) and not clean_config.get('files', []):
        print(f'  ℹ️  No clean configuration specified')

    print('')

print(f'✅ Cleaned {cleaned_count} build artifacts from dependencies')
"
    else
        echo "ℹ️  Dependencies directory doesn't exist, nothing to clean"
    fi
    exit 0
fi

# Validate action
if [ "$ACTION" != "setup" ]; then
    echo "❌ Error: Invalid action '$ACTION'. Use 'setup' or 'clean'"
    echo "Usage: $0 [setup|clean]"
    exit 1
fi

# Check if dependencies.json exists for setup
if [ ! -f "$PROJECT_ROOT/$DEPS_CONFIG" ]; then
    echo "❌ Error: $DEPS_CONFIG not found in project root"
    exit 1
fi

# Create dependencies directory
echo "📁 Creating dependencies directory..."
mkdir -p "$DEPS_DIR"
DEPS_FULL_PATH=$(cd "$DEPS_DIR" && pwd)
echo "✅ Dependencies will be installed to: $DEPS_FULL_PATH"
echo ""

# Read and process dependencies.json
echo "📥 Reading dependency configuration..."

# Parse JSON and process each dependency using the new format
python3 -c "
import json
import os
import sys
import subprocess
import multiprocessing

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
config_path = os.path.join(project_root, '$DEPS_CONFIG')
print(f'Loading config from: {config_path}')
with open(config_path, 'r') as f:
    config = json.load(f)

deps_dir = '$DEPS_FULL_PATH'
print(f'Processing {len(config[\"dependencies\"])} dependencies...')
print('')

for i, dependency in enumerate(config['dependencies']):
    name = dependency['name']
    path = dependency['path']
    url = dependency['url']
    commit = dependency['commit']
    description = dependency.get('description', 'No description')
    build_config = dependency.get('build', {})

    print(f'{i+1}. {name} ({path})')
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

    # Handle build configuration
    if not build_config.get('enabled', False):
        reason = build_config.get('reason', 'Build disabled')
        print(f'   ⚠️  Skipping build: {reason}')
    elif build_config.get('skip_in_docker', False):
        print(f'   ⚠️  Skipping build in Docker: {build_config.get(\"skip_reason\", \"Resource intensive\")}')
    else:
        # Build is enabled
        strategy = build_config.get('strategy', 'cmake')
        build_dir_name = build_config.get('build_dir', 'build')
        build_dir = os.path.join(target_path, build_dir_name)

        print(f'   🔨 Building using {strategy} strategy...')
        os.makedirs(build_dir, exist_ok=True)

        # Configure step
        configure_cmd = build_config.get('configure_command')
        if configure_cmd:
            # Replace $(nproc) with actual core count
            configure_cmd = configure_cmd.replace('$(nproc)', str(multiprocessing.cpu_count()))
            config_result = run_command(configure_cmd, cwd=build_dir, allow_failure=True)
            if config_result is None:
                print(f'   ⚠️  Configuration failed, skipping build')
            else:
                # Build step
                build_cmd = build_config.get('build_command')
                if build_cmd:
                    build_cmd = build_cmd.replace('$(nproc)', str(multiprocessing.cpu_count()))
                    allow_build_failure = build_config.get('allow_failure', False)
                    build_result = run_command(build_cmd, cwd=build_dir, allow_failure=allow_build_failure)
                    if build_result is None and allow_build_failure:
                        failure_reason = build_config.get('failure_reason', 'Build failed but expected')
                        print(f'   ⚠️  {failure_reason}')
                    elif build_result is None:
                        print(f'   ⚠️  Build failed unexpectedly')
                    else:
                        print(f'   ✅ Build complete')

                        # Verify artifacts if specified
                        artifacts = build_config.get('artifacts', [])
                        if artifacts:
                            missing_artifacts = []
                            for artifact in artifacts:
                                artifact_path = os.path.join(target_path, artifact)
                                if not os.path.exists(artifact_path):
                                    missing_artifacts.append(artifact)

                            if missing_artifacts:
                                print(f'   ⚠️  Missing expected artifacts: {missing_artifacts}')
                            else:
                                print(f'   ✅ All expected artifacts present')
                else:
                    print(f'   ⚠️  No build command specified')
        else:
            print(f'   ⚠️  No configure command specified')

    print('')

print('🎉 All dependencies processed successfully!')
print('')
print('Next steps:')
print('  1. Test build with: make all')
print('  2. Use: ./scripts/manage-deps.sh clean  # to clean build artifacts')
"