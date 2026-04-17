# auto-detect platform: WSL or Raspberry Pi
UNAME_R := $(shell uname -r)
ifneq (,$(findstring microsoft,$(UNAME_R))$(findstring WSL,$(UNAME_R)))
    PRESET ?= wsl-arm64-debug
else
    PRESET ?= pi-arm64-debug
endif

BUILD_DIR := out/build/$(PRESET)
BINARY    := $(BUILD_DIR)/uwbp_server

.PHONY: all build configure rebuild clean run run-sudo help wsl pi

all: build

help:
	@echo "Available targets:"
	@echo "  make build     - build for detected platform ($(PRESET))"
	@echo "  make wsl       - force WSL build"
	@echo "  make pi        - force Pi build"
	@echo "  make configure - only run cmake configure"
	@echo "  make rebuild   - clean + build"
	@echo "  make clean     - remove build dir"
	@echo "  make run       - run the server (with sudo, needed for NM + /etc)"
	@echo ""
	@echo "Current preset: $(PRESET)"
	@echo "Build dir:      $(BUILD_DIR)"

configure:
	cmake --preset $(PRESET)

build: configure
	cmake --build $(BUILD_DIR)

rebuild: clean build

clean:
	rm -rf $(BUILD_DIR)

wsl:
	$(MAKE) build PRESET=wsl-arm64-debug

pi:
	$(MAKE) build PRESET=pi-arm64-debug

# server needs sudo because it talks to NetworkManager system bus
# and writes to /etc/NetworkManager/dnsmasq-shared.d/
run: build
	cd $(BUILD_DIR) && sudo ./uwbp_server

# run without rebuild
run-only:
	cd $(BUILD_DIR) && sudo ./uwbp_server
