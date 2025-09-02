#!/bin/bash

# run-act-in-docker.sh
# Run GitHub Actions locally using act CLI inside Docker container
# This provides maximum environmental consistency with actual GitHub runners

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ACT_IMAGE="ol_dsp/act-runner:local"

echo "🎬 Running GitHub Actions with act in Docker container..."
echo

# Check if Docker is running
if ! docker info &> /dev/null; then
    echo "❌ Docker is not running. Please start Docker first."
    exit 1
fi

# Get GitHub token from gh CLI if available and not already set
if [ -z "$GITHUB_TOKEN" ] && command -v gh &> /dev/null; then
    echo "🔑 Getting GitHub token from gh CLI..."
    if gh auth status &> /dev/null; then
        GITHUB_TOKEN=$(gh auth token 2>/dev/null || echo "")
        if [ -n "$GITHUB_TOKEN" ]; then
            echo "✅ Got GitHub token from gh CLI"
        else
            echo "⚠️  Could not get token from gh CLI, using fake token"
            GITHUB_TOKEN="fake-token-for-local-testing"
        fi
    else
        echo "⚠️  gh CLI not authenticated, run 'gh auth login' first"
        echo "   Using fake token for now"
        GITHUB_TOKEN="fake-token-for-local-testing"
    fi
else
    GITHUB_TOKEN="${GITHUB_TOKEN:-fake-token-for-local-testing}"
fi

# Build act runner image if it doesn't exist
if ! docker images --format "table {{.Repository}}:{{.Tag}}" | grep -q "$ACT_IMAGE" 2>/dev/null; then
    echo "🔨 Building act runner Docker image..."
    docker build -f .docker/act-runner.Dockerfile -t "$ACT_IMAGE" .
    echo "✅ Act runner image built successfully"
else
    echo "✅ Act runner image $ACT_IMAGE already exists"
fi

echo
echo "🏃‍♂️ Running act CLI in Docker container..."

# Run act in Docker container with:
# - Docker socket mounted so act can spin up containers
# - Project directory mounted
# - Environment variables passed through
docker run --rm \
    -v /var/run/docker.sock:/var/run/docker.sock \
    -v "$PROJECT_ROOT:/workspace" \
    -w /workspace \
    -e GITHUB_TOKEN="$GITHUB_TOKEN" \
    "$ACT_IMAGE" \
    act push --pull=false --verbose --secret GITHUB_TOKEN="$GITHUB_TOKEN" "$@"

echo
echo "✅ Act Docker run completed!"
echo
echo "💡 Usage examples:"
echo "  • Run specific job: ./scripts/run-act-in-docker.sh --job build-cpp"
echo "  • List jobs: ./scripts/run-act-in-docker.sh --list"
echo "  • Different event: ./scripts/run-act-in-docker.sh pull_request"