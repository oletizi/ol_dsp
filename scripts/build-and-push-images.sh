#!/bin/bash

# build-and-push-images.sh
# Build and push multi-arch Docker images from macOS
# This allows faster feedback and avoids CI build times

set -e

REGISTRY="ghcr.io"
REPO="oletizi/ol_dsp"

echo "🔨 Building and pushing multi-architecture Docker images..."

# Check for required tools
if ! command -v docker &> /dev/null || ! docker buildx version &> /dev/null; then
    echo "❌ Docker with buildx support is required"
    exit 1
fi

if ! command -v gh &> /dev/null; then
    echo "❌ GitHub CLI (gh) is required"
    exit 1
fi

# Get GitHub token and login
echo "🔑 Authenticating with GitHub Container Registry..."
if ! gh auth status &> /dev/null; then
    echo "❌ Please run 'gh auth login' first"
    exit 1
fi

GITHUB_TOKEN=$(gh auth token)
echo "$GITHUB_TOKEN" | docker login ghcr.io -u $(gh api user --jq .login) --password-stdin

# Ensure buildx is available and create builder if needed
if ! docker buildx inspect ol_dsp_builder >/dev/null 2>&1; then
    echo "📦 Creating multi-arch builder..."
    docker buildx create --name ol_dsp_builder --use
    docker buildx inspect --bootstrap
else
    echo "📦 Using existing multi-arch builder..."
    docker buildx use ol_dsp_builder
fi

# Build and push base image for AMD64 and ARM64
echo "🏗️  Building base image for AMD64 and ARM64..."
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    --file .docker/base.Dockerfile \
    --tag ${REGISTRY}/${REPO}/base:latest-amd64 \
    --tag ${REGISTRY}/${REPO}/base:latest-arm64 \
    --push \
    .

# Build and push C++ builder image  
echo "🏗️  Building C++ builder image for AMD64 and ARM64..."
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    --file .docker/cpp-builder.Dockerfile \
    --tag ${REGISTRY}/${REPO}/cpp-builder:latest-amd64 \
    --tag ${REGISTRY}/${REPO}/cpp-builder:latest-arm64 \
    --build-arg GITHUB_REPOSITORY=${REPO} \
    --push \
    .

# Build and push Node builder image
echo "🏗️  Building Node builder image for AMD64 and ARM64..."
docker buildx build \
    --platform linux/amd64,linux/arm64 \
    --file .docker/node-builder.Dockerfile \
    --tag ${REGISTRY}/${REPO}/node-builder:latest-amd64 \
    --tag ${REGISTRY}/${REPO}/node-builder:latest-arm64 \
    --push \
    .

echo "✅ All images built and pushed successfully!"
echo ""
echo "💡 Images available:"
echo "  • ${REGISTRY}/${REPO}/base:latest-amd64"
echo "  • ${REGISTRY}/${REPO}/base:latest-arm64" 
echo "  • ${REGISTRY}/${REPO}/cpp-builder:latest-amd64"
echo "  • ${REGISTRY}/${REPO}/cpp-builder:latest-arm64"
echo "  • ${REGISTRY}/${REPO}/node-builder:latest-amd64"
echo "  • ${REGISTRY}/${REPO}/node-builder:latest-arm64"
echo ""
echo "🎯 Next steps:"
echo "  • Push code changes to trigger CI (images will be pulled, not built)"
echo "  • Test locally: docker run --rm ${REGISTRY}/${REPO}/cpp-builder:latest-arm64 bash"