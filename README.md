# ValveRush

ValveRush is a fast-paced logic puzzle game for KDE, inspired by the classic *Pipe Mania*, built with **Qt 6**, **KDE Frameworks 6**, and **KDEGames6**.

Place straight, curved, and cross pipes on the board before the pressurized valve opens and the fluid leaks!

## Features

- **Classic Pipe Mania Gameplay**:
  - 10×7 grid with a randomized start valve, obstacles, and a 5-pipe lookahead queue.
  - Pre-flow countdown timer before the valve opens.
  - **Fast Flow** (`F`) to accelerate the fluid and earn bonus points.
  - **Cross Pipes** supporting perpendicular double traversal for extra bonus points.
  - **Narrow (Restrictor) Pipes** (horizontal & vertical) that slow down fluid flow to buy extra time.
  - Ability to replace unfilled pipes (with a small score penalty).
- **KDE Games Integration**:
  - **Difficulty Levels** (`KGameDifficulty`: Easy, Medium, Hard).
  - **High Scores** (`KGameHighScoreDialog`) tracked per difficulty.
  - **Themes** (`KGameThemeProvider` / `KGameThemeSelector`): *Breeze Industrial*, *Neon Pipeline*, and *Classic Symbian*.
  - **Standard KDE XMLGUI** menus, toolbars, shortcuts, and `KConfigXT` settings.

## Controls

- **Left Click**: Place the next pipe from the queue (or replace an unfilled pipe).
- **Arrow Keys + `Enter` / `Space`**: Navigate the grid and place pipes with the keyboard.
- **`F`**: Toggle Fast Forward flow.
- **`P`**: Pause / Resume game.

## Dependencies

- CMake ≥ 3.24 & Ninja (or Make)
- C++20 compiler
- Extra CMake Modules (ECM) ≥ 6.0
- Qt 6 ≥ 6.6 (`Widgets`, `Svg`, `SvgWidgets`, `Test`)
- KDE Frameworks 6 ≥ 6.0 (`Config`, `ConfigWidgets`, `CoreAddons`, `Crash`, `DBusAddons`, `I18n`, `IconThemes`, `WidgetsAddons`, `XmlGui`)
- `KDEGames6` (`libkdegames6-dev`)

On Debian/Ubuntu/Kubuntu:

```bash
sudo apt install -y \
  extra-cmake-modules cmake ninja-build g++ \
  qt6-base-dev qt6-svg-dev libkdegames6-dev \
  libkf6config-dev libkf6configwidgets-dev libkf6coreaddons-dev \
  libkf6crash-dev libkf6dbusaddons-dev libkf6i18n-dev \
  libkf6iconthemes-dev libkf6widgetsaddons-dev libkf6xmlgui-dev
```

## Building & Installing

```bash
cmake -B build -S . -G Ninja -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build
cmake --install build
```

If installing to `~/.local`, ensure `~/.local/share` is included in `XDG_DATA_DIRS` when launching:

```bash
XDG_DATA_DIRS="$HOME/.local/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}" ~/.local/bin/valverush
```

To install system-wide instead, omit `-DCMAKE_INSTALL_PREFIX=~/.local` (or use `/usr`) and run `sudo cmake --install build`.

## Running Tests

```bash
ctest --test-dir build --output-on-failure
```

## License

ValveRush is licensed under the **GNU General Public License v2.0 or later** (`GPL-2.0-or-later`).

## Author

**Huberto Gastal Mayer** (<hubertogm@gmail.com>)
