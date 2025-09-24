#!/bin/bash

# test-ci-with-act.sh
# Run GitHub Actions CI locally using act CLI
# This provides a faithful reproduction of the actual GitHub Actions environment

set -e

echo "🎬 Running GitHub Actions CI locally with act..."
echo

# Check if act is available
if ! command -v act &> /dev/null; then
    echo "❌ act CLI not found. Please install it with:"
    echo "   brew install act"
    exit 1
fi

# Check if Docker is running
if ! docker info &> /dev/null; then
    echo "❌ Docker is not running. Please start Docker first."
    exit 1
fi

# Run the CI workflow
echo "🏃‍♂️ Running CI workflow with act..."
echo "This will use the same containers and environment as GitHub Actions"
echo

# Use --pull=false to avoid pulling new images if they exist locally
# Use --verbose for detailed output
# Use --env-file if you need environment variables
act push \
    --pull=false \
    --verbose \
    --job build-images \
    || echo "⚠️  Build images job failed or was skipped"

echo
echo "🏃‍♂️ Running C++ build job..."
act push \
    --pull=false \
    --verbose \
    --job build-cpp \
    || echo "⚠️  C++ build job failed"

echo
echo "🏃‍♂️ Running npm build job..."
act push \
    --pull=false \
    --verbose \
    --job build-npm \
    || echo "⚠️  npm build job failed"

echo
echo "✅ Act-based CI test completed!"
echo
echo "💡 Tips:"
echo "  • Run single job: act push --job build-cpp"
echo "  • List available jobs: act --list"
echo "  • Use different event: act pull_request"
echo "  • See full options: act --help"