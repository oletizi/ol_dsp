.PHONY: all build clean run help

BUILD_DIR := build

## build: Compile midisnoop
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && cmake --build .

## all: Alias for build
all: build

## run: Build and run midisnoop
run: build
	@$(BUILD_DIR)/bin/midisnoop

## clean: Remove build artifacts
clean:
	@rm -rf $(BUILD_DIR)

## help: Show this help message
help:
	@echo "midisnoop - Makefile targets:"
	@sed -n 's/^##//p' ${MAKEFILE_LIST} | column -t -s ':' | sed -e 's/^/ /'