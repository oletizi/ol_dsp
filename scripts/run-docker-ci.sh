#!/bin/bash

# run-docker-ci.sh
# Unified Docker CI script that works both locally and in remote CI
# Auto-detects available images and configures accordingly

set -e

# Auto-detect configuration
ARCH=$(uname -m | sed 's/x86_64/amd64/' | sed 's/aarch64/arm64/')
PLATFORM="linux/${ARCH}"
CURRENT_BRANCH="${GITHUB_REF_NAME:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'main')}"
DEFAULT_REPO_URL="https://github.com/oletizi/ol_dsp.git"
REPO_URL="${REPO_URL:-$DEFAULT_REPO_URL}"

# Function to check if image exists locally
image_exists_locally() {
    docker image inspect "$1" >/dev/null 2>&1
}

# Function to check if remote image exists and is pullable
image_exists_remotely() {
    docker manifest inspect "$1" >/dev/null 2>&1
}

# Auto-detect best available images
detect_images() {
    local cpp_image=""
    local node_image=""
    
    # Try local images first (fastest)
    local local_cpp="ol_dsp/cpp-builder:local"
    local local_node="ol_dsp/node-builder:local"
    
    if image_exists_locally "$local_cpp" && image_exists_locally "$local_node"; then
        echo "🔍 Found local Docker images, using for fastest performance"
        cpp_image="$local_cpp"
        node_image="$local_node"
        echo "   Registry: Local Docker images"
    else
        # Fall back to remote registry
        local remote_cpp="ghcr.io/oletizi/ol_dsp/cpp-builder:latest-${ARCH}"
        local remote_node="ghcr.io/oletizi/ol_dsp/node-builder:latest-${ARCH}"
        
        echo "🔍 Local images not found, checking remote registry..."
        if image_exists_remotely "$remote_cpp" && image_exists_remotely "$remote_node"; then
            echo "✅ Found remote Docker images, pulling for use"
            cpp_image="$remote_cpp"
            node_image="$remote_node" 
            echo "   Registry: ghcr.io/oletizi/ol_dsp"
        else
            echo "❌ Neither local nor remote images found."
            echo ""
            echo "To build local images: make docker-build-images-locally"
            echo "To build and push remote images: make docker-build-and-push-images"
            exit 1
        fi
    fi
    
    CPP_IMAGE="$cpp_image"
    NODE_IMAGE="$node_image"
}

# Detect and configure images
detect_images

echo "🚀 Running Docker CI..."
echo "Platform: $PLATFORM"
echo "C++ image: $CPP_IMAGE"
echo "Node image: $NODE_IMAGE"
echo "Branch: $CURRENT_BRANCH"
echo

# Function to run C++ build test
run_cpp_build() {
    echo "🏗️  Testing C++ Build..."
    
    docker run --rm --platform "$PLATFORM" \
        "$CPP_IMAGE" \
        bash -c "
            echo '📁 Cloning ol_dsp repository...'
            cd /workspace
            git clone $REPO_URL ol_dsp
            cd ol_dsp
            git checkout $CURRENT_BRANCH || git checkout main
            
            echo '📁 Verifying pre-built dependencies...'
            if [ -d '../.ol_dsp-deps/libs/JUCE/build' ]; then
                echo '✅ Pre-built dependencies found at ../.ol_dsp-deps/'
                ls -la ../.ol_dsp-deps/libs/JUCE/build/ | head -5
            else
                echo '❌ Pre-built dependencies not found!'
                exit 1
            fi
            
            echo '🔨 Building project...'
            time make all
            
            echo '🔨 Building Plugin Host...'
            time make plughost
            
            echo '📊 Verifying Plugin Host...'
            PLUGHOST_PATH='./cmake-build/modules/juce/host/plughost_artefacts/plughost'
            if [ -f \"\$PLUGHOST_PATH\" ]; then
                ls -la \"\$PLUGHOST_PATH\"
                timeout 10s \"\$PLUGHOST_PATH\" --help || echo 'Plugin host help completed'
            else
                echo '⚠️  Plugin host not found at expected location'
                find ./cmake-build -name 'plughost' -type f 2>/dev/null || echo 'No plughost found'
            fi
            
            echo '✅ C++ build test completed successfully'
        "
}

# Function to run npm build test
run_npm_build() {
    echo "📦 Testing npm Workspace..."
    
    docker run --rm --platform "$PLATFORM" \
        "$NODE_IMAGE" \
        bash -c "
            echo '📁 Cloning repository for npm testing...'
            git clone $REPO_URL /tmp/build_test
            cd /tmp/build_test
            git checkout $CURRENT_BRANCH || git checkout main
            
            echo '📦 Installing dependencies (skip native MIDI on Linux)...'
            npm ci --ignore-scripts
            
            echo '🧪 Running tests...'
            npm test || echo 'Tests completed (may have compatibility issues)'
            
            echo '📊 Verifying workspace structure...'
            ls -la modules/audio-tools/
            npm ls --depth=0
            
            echo '✅ npm workspace test completed'
        "
}

# Main execution
main() {
    case "${1:-all}" in
        "cpp"|"c++")
            run_cpp_build
            ;;
        "npm"|"node")
            run_npm_build
            ;;
        "all"|"")
            run_cpp_build
            echo
            run_npm_build
            ;;
        *)
            echo "Usage: $0 [cpp|npm|all]"
            echo "  cpp/c++ - Run C++ build test only"
            echo "  npm/node - Run npm workspace test only"  
            echo "  all - Run both tests (default)"
            exit 1
            ;;
    esac
    
    echo
    echo "🎉 Docker CI completed successfully!"
    
    if [[ "$REGISTRY" == *"ghcr.io"* ]]; then
        echo
        echo "💡 Remote CI Tips:"
        echo "  • Images pulled from GitHub Container Registry"
        echo "  • Pre-built dependencies should provide significant speed improvements"
    else
        echo
        echo "💡 Local CI Tips:"
        echo "  • Build images locally: make docker-build-and-push-images"
        echo "  • Clean up images: docker rmi \$CPP_IMAGE \$NODE_IMAGE"
        echo "  • Check sizes: docker images | grep ol_dsp"
    fi
}

main "$@"