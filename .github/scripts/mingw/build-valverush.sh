#!/usr/bin/env bash
set -euo pipefail

PREFIX="/opt/kde-mingw64"
MINGW_SYSROOT="/usr/x86_64-w64-mingw32/sys-root/mingw"
export CMAKE_PREFIX_PATH="$PREFIX:$MINGW_SYSROOT"
SRC_DIR="/tmp/valverush-src"
BLD_DIR="/tmp/valverush-win64-build"
DIST_DIR="/out/valverush-windows-x64"

if [[ ! -f "$PREFIX/.built-libkdegames" ]]; then
    /work/build-deps.sh
fi

rm -rf "$SRC_DIR" "$BLD_DIR" "$DIST_DIR"
mkdir -p "$SRC_DIR" "$DIST_DIR"
cp -a /src/valverush/. "$SRC_DIR/"
rm -rf "$SRC_DIR/build" "$SRC_DIR/build-pkg"

PKG_DIRS=()
for d in "$PREFIX"/lib/cmake/*; do
    if [[ -d "$d" ]]; then
        PKG_DIRS+=("-D$(basename "$d")_DIR=$d")
    fi
done

cmake -S "$SRC_DIR" -B "$BLD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=/work/toolchain-kde-mingw64.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$DIST_DIR" \
    -DCMAKE_PREFIX_PATH="$PREFIX;$MINGW_SYSROOT" \
    -DCMAKE_FIND_ROOT_PATH="$PREFIX;$MINGW_SYSROOT" \
    -DECM_DIR="$PREFIX/share/ECM/cmake" \
    "${PKG_DIRS[@]}" \
    -DBUILD_TESTING=OFF \
    -DQT_HOST_PATH=/usr \
    -DQT_HOST_PATH_CMAKE_DIR=/usr/lib64/cmake \
    -DKF6_HOST_TOOLING=/usr/lib64/cmake \
    -DCMAKE_DISABLE_FIND_PACKAGE_KF6DocTools=ON

cmake --build "$BLD_DIR" -j"$(nproc)"
cmake --install "$BLD_DIR"

# Move valverush.exe and data/ to top-level dist folder for Windows portability
if [[ -f "$DIST_DIR/bin/valverush.exe" ]]; then
    mv "$DIST_DIR/bin/valverush.exe" "$DIST_DIR/valverush.exe"
fi
if [[ -d "$DIST_DIR/bin/data" ]]; then
    mv "$DIST_DIR/bin/data" "$DIST_DIR/data"
fi
rm -rf "$DIST_DIR/bin" "$DIST_DIR/lib" "$DIST_DIR/include"

# Bundle all DLLs from MinGW sysroot and $PREFIX/bin
cp -n "$PREFIX/bin/"*.dll "$DIST_DIR/" 2>/dev/null || true
cp -n "$MINGW_SYSROOT/bin/"*.dll "$DIST_DIR/" 2>/dev/null || true

# Bundle essential Qt6 plugins (platforms, styles, imageformats, iconengines)
QT_PLUGINS="$MINGW_SYSROOT/lib/qt6/plugins"
for pdir in platforms styles imageformats iconengines; do
    if [[ -d "$QT_PLUGINS/$pdir" ]]; then
        mkdir -p "$DIST_DIR/$pdir"
        cp -r "$QT_PLUGINS/$pdir/"*.dll "$DIST_DIR/$pdir/" 2>/dev/null || true
    fi
done

# Create zip archive
cd /out
rm -f valverush-windows-x64.zip
zip -rq valverush-windows-x64.zip valverush-windows-x64

echo "==> Windows build ready: /out/valverush-windows-x64.zip"
