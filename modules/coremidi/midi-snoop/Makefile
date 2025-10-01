.PHONY: all build clean run install-driver install uninstall help

BUILD_DIR := build
DRIVER_DIR := $(HOME)/Library/Audio/MIDI Drivers
INSTALL_DIR := $(HOME)/bin

## build: Compile midisnoop and SnoizeMIDISpy driver
build:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake .. && cmake --build .

## all: Alias for build
all: build

## install: Install midisnoop binary to ~/bin (or PREFIX if set)
install: build
	@echo "Installing midisnoop to $(INSTALL_DIR)..."
	@if [ ! -f "$(BUILD_DIR)/bin/midisnoop" ]; then \
		echo "❌ midisnoop binary not found. Run 'make build' first."; \
		exit 1; \
	fi
	@if [ ! -d "$(BUILD_DIR)/bin/Frameworks/SnoizeMIDISpy.framework" ]; then \
		echo "❌ SnoizeMIDISpy.framework not found. Run 'make build' first."; \
		exit 1; \
	fi
	@mkdir -p "$(INSTALL_DIR)"
	@mkdir -p "$(INSTALL_DIR)/Frameworks"
	@cp "$(BUILD_DIR)/bin/midisnoop" "$(INSTALL_DIR)/"
	@cp -R "$(BUILD_DIR)/bin/Frameworks/SnoizeMIDISpy.framework" "$(INSTALL_DIR)/Frameworks/"
	@chmod +x "$(INSTALL_DIR)/midisnoop"
	@echo "✅ midisnoop installed to $(INSTALL_DIR)/midisnoop"
	@echo "✅ SnoizeMIDISpy.framework installed to $(INSTALL_DIR)/Frameworks/"
	@if echo "$$PATH" | grep -q "$(INSTALL_DIR)"; then \
		echo "ℹ️  You can now run 'midisnoop' from anywhere."; \
	else \
		echo "⚠️  $(INSTALL_DIR) is not in your PATH. Add it with:"; \
		echo "   echo 'export PATH=\"$(INSTALL_DIR):\$$PATH\"' >> ~/.zshrc"; \
		echo "   source ~/.zshrc"; \
	fi

## uninstall: Remove midisnoop binary from ~/bin (or PREFIX if set)
uninstall:
	@echo "Uninstalling midisnoop from $(INSTALL_DIR)..."
	@if [ -f "$(INSTALL_DIR)/midisnoop" ]; then \
		rm "$(INSTALL_DIR)/midisnoop"; \
		echo "✅ midisnoop removed from $(INSTALL_DIR)"; \
	else \
		echo "ℹ️  midisnoop not found in $(INSTALL_DIR) (already uninstalled?)"; \
	fi
	@if [ -d "$(INSTALL_DIR)/Frameworks/SnoizeMIDISpy.framework" ]; then \
		rm -rf "$(INSTALL_DIR)/Frameworks/SnoizeMIDISpy.framework"; \
		echo "✅ SnoizeMIDISpy.framework removed from $(INSTALL_DIR)/Frameworks/"; \
		rmdir "$(INSTALL_DIR)/Frameworks" 2>/dev/null || true; \
	fi

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