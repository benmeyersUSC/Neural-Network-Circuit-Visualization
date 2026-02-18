#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "Building Neural-Network-Circuit-Visualization for the web..."
emcmake cmake -B "$PROJECT_DIR/build-web" -S "$PROJECT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$PROJECT_DIR/build-web"

echo ""
echo "Build complete! Output in build-web/"
echo "To test locally:"
echo "  cd $PROJECT_DIR/build-web && python3 -m http.server 8080"
echo "  Then open http://localhost:8080/index.html"