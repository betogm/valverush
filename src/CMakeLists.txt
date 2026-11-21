# SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer <hubertogm@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

set(valverush_SRCS
    main.cpp
    mainwindow.cpp
    boardview.cpp
    queuewidget.cpp
    piperenderer.cpp
    pipe.cpp
)

kconfig_add_kcfg_files(valverush_SRCS valverush.kcfgc)

add_executable(valverush ${valverush_SRCS})

if(WIN32)
    set_target_properties(valverush PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(valverush
    PRIVATE
        KDEGames6
        KF6::ConfigCore
        KF6::ConfigWidgets
        KF6::CoreAddons
        KF6::Crash
        KF6::I18n
        KF6::IconThemes
        KF6::WidgetsAddons
        KF6::XmlGui
        Qt6::Svg
        Qt6::Widgets
)

if(NOT WIN32)
    target_link_libraries(valverush PRIVATE KF6::DBusAddons)
endif()

install(TARGETS valverush ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES valverushui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/valverush)
install(FILES valverush.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})
