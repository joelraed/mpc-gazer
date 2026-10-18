#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p dist
# Old-enough glibc is critical for original MPC One compatibility.
docker run --rm --platform linux/arm/v7 \
  -v "$PWD:/work" -w /work arm32v7/gcc:11-bullseye \
  bash -ec '
    # The old Bullseye security mirror can advertise removed .deb revisions.
    # Build dependencies come from the stable Bullseye main repository instead.
    sed -i "/debian-security/d" /etc/apt/sources.list
    rm -rf /var/lib/apt/lists/*
    apt-get update -o Acquire::Retries=3
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends -o Acquire::Retries=3 cmake file binutils
    cmake -S . -B build-mpc -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_FLAGS="-O2 -fvisibility=hidden -ffunction-sections -fdata-sections" \
      -DCMAKE_SHARED_LINKER_FLAGS="-Wl,--gc-sections"
    cmake --build build-mpc --target MPCGazer --parallel 2
    cp build-mpc/MPCGazer.so dist/MPCGazer.so
    file dist/MPCGazer.so || true
    readelf -h dist/MPCGazer.so | grep -E "Class:|Machine:|Flags:"
    readelf -Ws dist/MPCGazer.so | grep VSTPluginMain
    readelf --version-info dist/MPCGazer.so | grep GLIBC_ | sort -u || true
  '
echo "ARM .so: dist/MPCGazer.so (EXPERIMENTAL, not installable by itself)"
