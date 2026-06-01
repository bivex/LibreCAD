#!/bin/bash
set -e # Прервать выполнение при ошибке

echo "==========================================="
echo "  MCP Bridge Plugin - Build & Install"
echo "==========================================="

# Определяем пути
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/mcp_bridge_build"
TARGET_APP_PLUGINS="/Applications/LibreCAD.app/Contents/Resources/plugins"

# В .pro файле указано DESTDIR = ../../LibreCAD.app/...
# При сборке из $BUILD_DIR это разрешается на уровень выше PROJECT_ROOT
COMPILED_PLUGIN="$PROJECT_ROOT/../LibreCAD.app/Contents/Resources/plugins/libmcp_bridge.dylib"

echo "=> 1. Setting up build directory ($BUILD_DIR)..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "=> 2. Running qmake6..."
qmake6 "$SCRIPT_DIR/mcp_bridge.pro"

echo "=> 3. Cleaning previous build..."
make clean

echo "=> 4. Compiling plugin..."
# Используем все доступные ядра процессора для ускорения
CORES=$(sysctl -n hw.ncpu 2>/dev/null || echo 4)
make -j"$CORES"

echo "=> 5. Installing to $TARGET_APP_PLUGINS..."
if [ -f "$COMPILED_PLUGIN" ]; then
    mkdir -p "$TARGET_APP_PLUGINS"
    cp "$COMPILED_PLUGIN" "$TARGET_APP_PLUGINS/"
    echo "=> SUCCESS! Plugin installed and ready to use."
    ls -lh "$TARGET_APP_PLUGINS/libmcp_bridge.dylib"
else
    echo "=> ERROR: Compiled plugin not found at $COMPILED_PLUGIN"
    exit 1
fi
