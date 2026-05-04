# auto-detect platform: WSL or Raspberry Pi
UNAME_R := $(shell uname -r)
ifneq (,$(findstring microsoft,$(UNAME_R))$(findstring WSL,$(UNAME_R)))
    PRESET ?= wsl-arm64-debug
else
    PRESET ?= pi-arm64-debug
endif

BUILD_DIR := out/build/$(PRESET)
BINARY    := $(BUILD_DIR)/uwbp_server

DEPLOY_DIR := /opt/uwbp/server
SERVICE_FILE := deploy/uwbp-server.service
SYSTEMD_SERVICE := /etc/systemd/system/uwbp-server.service

.PHONY: all build configure rebuild clean run run-only help wsl pi install-deps submodules bootstrap-vcpkg deploy install-service

all: build

help:
	@echo "Available targets:"
	@echo "  make install-deps      - install required system packages"
	@echo "  make submodules        - initialize git submodules"
	@echo "  make bootstrap-vcpkg   - bootstrap vcpkg"
	@echo "  make build             - build for detected platform ($(PRESET))"
	@echo "  make wsl               - force WSL build"
	@echo "  make pi                - force Pi build"
	@echo "  make configure         - only run cmake configure"
	@echo "  make rebuild           - clean + build"
	@echo "  make clean             - remove build dir"
	@echo "  make deploy            - copy backend binary to /opt/uwbp/server"
	@echo "  make install-service   - install and enable systemd service"
	@echo "  make run               - run the server (with sudo, needed for NM + /etc)"
	@echo ""
	@echo "Current preset: $(PRESET)"
	@echo "Build dir:      $(BUILD_DIR)"

install-deps:
	apt update
	apt upgrade -y
	apt install -y git build-essential cmake ninja-build

submodules:
	git submodule update --init --recursive

bootstrap-vcpkg:
	@if [ ! -f external/vcpkg/vcpkg ]; then \
		./external/vcpkg/bootstrap-vcpkg.sh; \
	else \
		echo "vcpkg already bootstrapped"; \
	fi

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

deploy:
	mkdir -p $(DEPLOY_DIR)
	cp $(BINARY) $(DEPLOY_DIR)/uwbp_server

install-service:
	cp $(SERVICE_FILE) $(SYSTEMD_SERVICE)
	systemctl daemon-reload
	systemctl enable uwbp-server.service

# server needs sudo because it talks to NetworkManager system bus
# and writes to /etc/NetworkManager/dnsmasq-shared.d/
run: build
	cd $(BUILD_DIR) && sudo ./uwbp_server

# run without rebuild
run-only:
	cd $(BUILD_DIR) && sudo ./uwbp_server
