# SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer <hubertogm@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(test_engine
    test_engine.cpp
    ${CMAKE_SOURCE_DIR}/src/pipe.cpp
)

target_include_directories(test_engine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_engine
    PRIVATE
        Qt6::Core
        Qt6::Test
)

add_test(NAME ValveRushEngineTest COMMAND test_engine)
