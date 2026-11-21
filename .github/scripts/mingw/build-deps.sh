#!/usr/bin/env bash
set -euo pipefail

PREFIX="/opt/kde-mingw64"
SRC_CACHE="/opt/kde-mingw64/src"
BUILD_ROOT="/tmp/kf6-build"
KF_TAG="${KF_TAG:-v6.30.0}"
GAMES_TAG="${GAMES_TAG:-v26.08.0}"

mkdir -p "$PREFIX" "$SRC_CACHE" "$BUILD_ROOT"

MINGW_SYSROOT="/usr/x86_64-w64-mingw32/sys-root/mingw"
export CMAKE_PREFIX_PATH="$PREFIX:$MINGW_SYSROOT"
HOST_CMAKE_ARGS=(
    -G Ninja
    -DCMAKE_TOOLCHAIN_FILE=/work/toolchain-kde-mingw64.cmake
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX="$PREFIX"
    -DCMAKE_PREFIX_PATH="$PREFIX;$MINGW_SYSROOT"
    -DCMAKE_FIND_ROOT_PATH="$PREFIX;$MINGW_SYSROOT"
    -DECM_DIR="$PREFIX/share/ECM/cmake"
    -DBUILD_TESTING=OFF
    -DBUILD_QCH=OFF
    -DQT_HOST_PATH=/usr
    -DQT_HOST_PATH_CMAKE_DIR=/usr/lib64/cmake
    -DKF_IGNORE_PLATFORM_CHECK=ON
    -DCMAKE_DISABLE_FIND_PACKAGE_KF6DocTools=ON
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6DBus=ON
)

build_kde_repo() {
    local name="$1"
    local url="$2"
    local tag="$3"
    shift 3
    local extra_args=("$@")

    local marker="$PREFIX/.built-${name}"
    if [[ -f "$marker" ]]; then
        echo "==> [SKIP] $name already built"
        return 0
    fi

    echo "==> [BUILD] $name ($tag)"
    local src_dir="$SRC_CACHE/$name"
    local bld_dir="$BUILD_ROOT/$name"

    if [[ ! -d "$src_dir" ]]; then
        git clone --depth 1 --branch "$tag" "$url" "$src_dir"
    fi

    local pkg_dirs=()
    for d in "$PREFIX"/lib/cmake/*; do
        if [[ -d "$d" ]]; then
            pkg_dirs+=("-D$(basename "$d")_DIR=$d")
        fi
    done

    rm -rf "$bld_dir"
    if [[ "$name" == "breeze-icons" ]]; then
        local host_tools="/tmp/breeze-host-tools"
        rm -rf "$host_tools"
        env -u CMAKE_PREFIX_PATH cmake -S "$src_dir" -B "$host_tools" -G Ninja -DWITH_ICON_GENERATION=OFF -DBINARY_ICONS_RESOURCE=OFF -DBUILD_TESTING=OFF
        env -u CMAKE_PREFIX_PATH cmake --build "$host_tools" --target generate-symbolic-dark qrcAlias -j"$(nproc)"
        extra_args+=("-DKF6_HOST_TOOLING=$host_tools/bin")
    fi
    cmake -S "$src_dir" -B "$bld_dir" "${HOST_CMAKE_ARGS[@]}" "${pkg_dirs[@]}" "${extra_args[@]}"
    cmake --build "$bld_dir" -j"$(nproc)"
    cmake --install "$bld_dir"
    rm -rf "$bld_dir"
    touch "$marker"
}

# ECM (pure CMake modules, installed into $PREFIX)
build_kde_repo "extra-cmake-modules" "https://invent.kde.org/frameworks/extra-cmake-modules.git" "$KF_TAG"

# Tier 1 Frameworks
build_kde_repo "kconfig"        "https://invent.kde.org/frameworks/kconfig.git"        "$KF_TAG" -DKCONFIG_USE_DBUS=OFF -DKCONFIG_USE_QML=OFF
build_kde_repo "kcoreaddons"    "https://invent.kde.org/frameworks/kcoreaddons.git"    "$KF_TAG" -DKCOREADDONS_USE_QML=OFF
build_kde_repo "ki18n"          "https://invent.kde.org/frameworks/ki18n.git"          "$KF_TAG" -DBUILD_WITH_QML=OFF
build_kde_repo "kcodecs"        "https://invent.kde.org/frameworks/kcodecs.git"        "$KF_TAG"
build_kde_repo "karchive"       "https://invent.kde.org/frameworks/karchive.git"       "$KF_TAG" -DWITH_BZIP2=OFF -DWITH_LIBLZMA=OFF -DWITH_LIBZSTD=OFF -DWITH_OPENSSL=OFF
build_kde_repo "kguiaddons"     "https://invent.kde.org/frameworks/kguiaddons.git"     "$KF_TAG" -DWITH_WAYLAND=OFF -DWITH_DBUS=OFF
build_kde_repo "kwidgetsaddons" "https://invent.kde.org/frameworks/kwidgetsaddons.git" "$KF_TAG"
build_kde_repo "kitemviews"     "https://invent.kde.org/frameworks/kitemviews.git"     "$KF_TAG"
build_kde_repo "kcolorscheme"   "https://invent.kde.org/frameworks/kcolorscheme.git"   "$KF_TAG"

# Tier 2 / 3 Frameworks
build_kde_repo "kcrash"         "https://invent.kde.org/frameworks/kcrash.git"         "$KF_TAG"
build_kde_repo "kcompletion"    "https://invent.kde.org/frameworks/kcompletion.git"    "$KF_TAG"
build_kde_repo "kconfigwidgets" "https://invent.kde.org/frameworks/kconfigwidgets.git" "$KF_TAG"
build_kde_repo "breeze-icons"   "https://invent.kde.org/frameworks/breeze-icons.git"   "$KF_TAG" -DWITH_ICON_GENERATION=OFF -DBINARY_ICONS_RESOURCE=OFF
build_kde_repo "kiconthemes"    "https://invent.kde.org/frameworks/kiconthemes.git"    "$KF_TAG"
build_kde_repo "kxmlgui"        "https://invent.kde.org/frameworks/kxmlgui.git"        "$KF_TAG"
build_kde_repo "kdnssd"         "https://invent.kde.org/frameworks/kdnssd.git"         "$KF_TAG"
build_kde_repo "attica"         "https://invent.kde.org/frameworks/attica.git"         "$KF_TAG"
build_kde_repo "syndication"    "https://invent.kde.org/frameworks/syndication.git"    "$KF_TAG"
build_kde_repo "kpackage"       "https://invent.kde.org/frameworks/kpackage.git"       "$KF_TAG"
build_kde_repo "knewstuff"      "https://invent.kde.org/frameworks/knewstuff.git"      "$KF_TAG" -DBUILD_WITH_QML=OFF

# Audio dependency for libkdegames
build_kde_repo "libsndfile"     "https://github.com/libsndfile/libsndfile.git"       "1.2.2" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DENABLE_EXTERNAL_LIBS=OFF -DENABLE_MPEG=OFF -DBUILD_SHARED_LIBS=ON

# KDEGames6
build_kde_repo "libkdegames"    "https://invent.kde.org/games/libkdegames.git"         "$GAMES_TAG" -DKDEGAMES_USE_QML=OFF -DSNDFILE_INCLUDE_DIR="$PREFIX/include" -DSNDFILE_LIBRARY="$PREFIX/lib/libsndfile.dll.a"

echo "==> All Windows KDE dependencies ready in $PREFIX"
