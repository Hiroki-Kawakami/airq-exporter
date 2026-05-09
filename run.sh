#!/bin/sh
set -e

TARGET=${1:-simulator}

case "$TARGET" in
  simulator)
    [ -d build ] || cmake --fresh -S simulator -B build -G Ninja
    cmake --build build
    ./build/simulator
    ;;
  esp32s3)
    idf.py -C esp32s3 flash monitor
    ;;
  *)
    echo "Usage: $0 [simulator|esp32s3]"
    exit 1
    ;;
esac
