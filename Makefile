BUILD_DIR=./cmake-build

# Build targets
all: ## Build all C++ components using CMake
	echo ${BUILD_DIR} && if [ ! -e ${BUILD_DIR} ]; then mkdir ${BUILD_DIR}; fi && cd ${BUILD_DIR} && cmake ../ && make

test: all ## Build and run C++ unit tests
	./${BUILD_DIR}/test/gtest_run

plughost: all ## Build JUCE plugin host
	cd ${BUILD_DIR} && make plughost

clean: ## Clean build artifacts and submodule symlinks
	cd ${BUILD_DIR} && rm -r *
	# Clean up submodule symlinks created by setup-submodules.sh
	find libs test -type l -delete 2>/dev/null || true
	rm -rf .submodule_cache

# Docker targets
docker-build-images: ## Build Docker images locally for testing
	docker build -f .docker/cpp-builder.Dockerfile -t ol_dsp/cpp-builder .
	docker build -f .docker/node-builder.Dockerfile -t ol_dsp/node-builder .

docker-build-images-locally: ## Build local Docker images for development
	./scripts/build-images-locally.sh

docker-build-and-push-images: ## Build and push multi-arch Docker images to registry
	./scripts/build-and-push-images.sh

docker-build-cpp: ## Build C++ components in Docker container
	docker run --rm -v $(PWD):/workspace ol_dsp/cpp-builder make

docker-build-npm: ## Build npm workspace in Docker container
	docker run --rm -v $(PWD):/workspace ol_dsp/node-builder npm ci --ignore-scripts

docker-dev: ## Start development environment using docker-compose
	docker-compose up -d dev

docker-clean: ## Stop containers and remove Docker images
	docker-compose down
	docker rmi ol_dsp/cpp-builder ol_dsp/node-builder ol_dsp/dev:local 2>/dev/null || true

# Test and CI targets
quick-test: ## Run quick smoke tests for both C++ and npm
	./scripts/quick-test.sh

docker-ci: ## Run Docker CI pipeline (auto-detects local/remote images)
	./scripts/run-docker-ci.sh

docker-ci-cpp: ## Run C++ Docker CI test only (auto-detects images)
	./scripts/run-docker-ci.sh cpp

docker-ci-npm: ## Run npm Docker CI test only (auto-detects images)
	./scripts/run-docker-ci.sh npm

test-ci-with-act: ## Test GitHub Actions workflows locally using act CLI
	./scripts/test-ci-with-act.sh

run-act-in-docker: ## Run act CLI inside Docker for GitHub Actions testing
	./scripts/run-act-in-docker.sh

# Setup targets
setup-submodules: ## Setup git submodules using cached repositories
	./scripts/setup-submodules.sh

help: ## Show this help message
	@echo 'Usage: make <target>'
	@echo
	@echo 'Available targets:'
	@awk 'BEGIN {FS = ":.*?## "}; /^[a-zA-Z0-9_-]+:.*?## / {printf "  %-30s %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.PHONY: all test clean docker-build-images docker-build-images-locally docker-build-and-push-images docker-build-cpp docker-build-npm docker-dev docker-clean quick-test docker-ci docker-ci-cpp docker-ci-npm test-ci-with-act run-act-in-docker setup-submodules help