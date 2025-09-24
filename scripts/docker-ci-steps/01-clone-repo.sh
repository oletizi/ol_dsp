#!/bin/bash
# Step 1: Clone repository into container
# This script clones the ol_dsp repository without disturbing pre-built dependencies

set -e

REPO_URL="${REPO_URL:-https://github.com/oletizi/ol_dsp.git}"
CURRENT_BRANCH="${CURRENT_BRANCH:-$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo 'main')}"

echo "📁 Step 1: Cloning repository..."
echo "Repository: $REPO_URL"
echo "Branch: $CURRENT_BRANCH"

cd /workspace

# Clone to temporary location
echo "Cloning to temporary location..."
git clone "$REPO_URL" tmp_src
cd tmp_src
git checkout "$CURRENT_BRANCH" || git checkout main

# List what we cloned
echo "Repository structure:"
ls -la

# Copy only the source files, not libs/ or test/ directories
echo "Copying source files (excluding libs/ and test/)..."
for item in *; do
    if [ "$item" != "libs" ] && [ "$item" != "test" ]; then
        echo "  Copying $item"
        cp -r "$item" /workspace/
    else
        echo "  Skipping $item (pre-built dependencies)"
    fi
done

# Copy hidden files
cp -r .[^.]* /workspace/ 2>/dev/null || true

# Clean up
cd /workspace
rm -rf tmp_src

echo "✅ Repository cloned successfully"
echo "Final workspace structure:"
ls -la /workspace/