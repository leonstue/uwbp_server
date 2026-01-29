#!/usr/bin/env bash
set -euo pipefail

sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  autoconf \
  automake \
  libtool \
  libsystemd-dev \
  git