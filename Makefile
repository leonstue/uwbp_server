# auto-detect platform: WSL or Raspberry Pi
UNAME_R := $(shell uname -r)
ifneq (,$(findstring microsoft,$(UNAME_R))$(findstring WSL,$(UNAME_R)))
    PRESET ?= wsl-arm64-debug
else
    PRESET ?= pi-arm64-debug
endif

BUILD_DIR := out/build/$(PRESET)
BINARY    := $(BUILD_DIR)/uwbp_server
WATCHDOG  := $(BUILD_DIR)/uwbp_watchdog

DEPLOY_DIR      := /opt/uwbp/server
SERVICE_FILE    := deploy/uwbp-server.service
SYSTEMD_SERVICE := /etc/systemd/system/uwbp-server.service
SERVICE_NAME    := uwbp-server.service

.PHONY: all help install-deps submodules bootstrap-vcpkg configure build rebuild \
        deploy deploy-artifact install-service start logs clean-artifacts clean-logs clean \
        run run-only wsl pi

all: build

help:
	@echo "Available targets:"
	@echo "  make deploy           - install deps, prepare submodules/vcpkg, build, deploy artifacts and install/enable service (service starts on next boot or via 'make start')"
	@echo "  make clean            - stop/remove service, remove deployed artifacts and remove local build/vcpkg artifacts"
	@echo "  make clean-artifacts  - remove only deployed artifacts from /opt; service stays registered and may fail until redeployed"
	@echo "  make logs             - show recent logs of deployed backend service"
	@echo "  make clean-logs       - clear journalctl logs and file logs of deployed backend"
	@echo ""
	@echo "  make install-deps     - install required system packages"
	@echo "  make submodules       - initialize git submodules"
	@echo "  make bootstrap-vcpkg  - bootstrap vcpkg if needed"
	@echo "  make configure        - only run cmake configure"
	@echo "  make build            - build for detected platform ($(PRESET))"
	@echo "  make rebuild          - remove build dir for current preset and build again"
	@echo "  make deploy-artifact  - copy backend artifacts to /opt/uwbp/server"
	@echo "  make install-service  - install and enable systemd service"
	@echo "  make start            - start/restart backend service now"
	@echo "  make run              - build and run the server with sudo"
	@echo "  make run-only         - run already built server with sudo"
	@echo "  make wsl              - force WSL build"
	@echo "  make pi               - force Pi build"
	@echo ""
	@echo "Current preset: $(PRESET)"
	@echo "Build dir:      $(BUILD_DIR)"

install-deps:
	sudo apt update
	sudo apt upgrade -y
	sudo apt install -y git build-essential cmake ninja-build autoconf automake libtool pkg-config

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

rebuild:
	rm -rf $(BUILD_DIR)
	$(MAKE) build

deploy: install-deps submodules bootstrap-vcpkg build deploy-artifact install-service

deploy-artifact:
	sudo rm -rf $(DEPLOY_DIR)
	sudo mkdir -p $(DEPLOY_DIR)
	sudo cp $(BINARY) $(DEPLOY_DIR)/uwbp_server
	sudo cp $(WATCHDOG) $(DEPLOY_DIR)/uwbp_watchdog

install-service:
	sudo cp $(SERVICE_FILE) $(SYSTEMD_SERVICE)
	sudo systemctl daemon-reload
	sudo systemctl enable $(SERVICE_NAME)

start:
	sudo systemctl restart $(SERVICE_NAME)

logs:
	journalctl -u $(SERVICE_NAME) -n 50 --no-pager

clean-artifacts:
	sudo rm -rf $(DEPLOY_DIR)

clean-logs:
	sudo rm -rf $(DEPLOY_DIR)/logs
	sudo journalctl --rotate
	sudo journalctl --vacuum-time=1s --unit=$(SERVICE_NAME)

clean:
	sudo systemctl disable --now $(SERVICE_NAME) 2>/dev/null || true
	sudo rm -f $(SYSTEMD_SERVICE)
	sudo systemctl daemon-reload
	sudo systemctl reset-failed
	sudo rm -rf $(DEPLOY_DIR)
	rm -rf out
	rm -rf external/vcpkg/buildtrees
	rm -rf external/vcpkg/packages
	rm -rf external/vcpkg/downloads
	rm -rf external/vcpkg/installed

wsl:
	$(MAKE) build PRESET=wsl-arm64-debug

pi:
	$(MAKE) build PRESET=pi-arm64-debug

# server needs sudo because it talks to NetworkManager system bus
# and writes to /etc/NetworkManager/dnsmasq-shared.d/
run: build
	cd $(BUILD_DIR) && sudo ./uwbp_server

run-only:
	cd $(BUILD_DIR) && sudo ./uwbp_server
