# SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer <hubertogm@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.24)

project(valverush VERSION 1.0.0)

set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMInstallIcons)
include(ECMAddAppIcon)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(FeatureSummary)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS
    Widgets
    Svg
    SvgWidgets
    Test
)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Config
    ConfigWidgets
    CoreAddons
    Crash
    DBusAddons
    I18n
    IconThemes
    WidgetsAddons
    XmlGui
)

find_package(KF6DocTools ${KF_MIN_VERSION})
find_package(KDEGames6 REQUIRED)

add_subdirectory(src)
add_subdirectory(themes)

if(KF6DocTools_FOUND)
    add_subdirectory(doc)
endif()

enable_testing()
add_subdirectory(tests)

install(PROGRAMS data/org.kde.valverush.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES data/org.kde.valverush.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
ecm_install_icons(ICONS data/sc-apps-valverush.svg DESTINATION ${KDE_INSTALL_ICONDIR})

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h tests/*.cpp)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
