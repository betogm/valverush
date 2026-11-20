#! /usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Huberto Gastal Mayer <hubertogm@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later

$EXTRACTRC src/*.rc src/*.kcfg >> rc.cpp
$XGETTEXT src/*.cpp src/*.h -o "$podir/valverush.pot"
rm -f rc.cpp
