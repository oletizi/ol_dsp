.PHONY: all build clean run install-driver help

BUILD_DIR := build
DRIVER_DIR := $(HOME)/Library/Audio/MIDI Drivers

## build: Compile midisnoop and SnoizeMIDISpy driver
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && cmake --build .

## all: Alias for build
all: build

## install-driver: Install SnoizeMIDISpy driver to ~/Library/Audio/MIDI Drivers
install-driver: build
	@echo "Installing SnoizeMIDISpy driver..."
	@mkdir -p "$(DRIVER_DIR)"
	@if [ -d "$(BUILD_DIR)/snoize-spy/SpyingMIDIDriver.plugin" ]; then \
		cp -r "$(BUILD_DIR)/snoize-spy/SpyingMIDIDriver.plugin" "$(DRIVER_DIR)/"; \
		echo "✅ Driver installed to $(DRIVER_DIR)/SpyingMIDIDriver.plugin"; \
		echo "⚠️  You may need to restart any MIDI applications for the driver to be recognized."; \
	else \
		echo "❌ Driver not found. Run 'make build' first."; \
		exit 1; \
	fi

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