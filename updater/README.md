# KayteIDE Auto-Updater

A Qt6 desktop application that polls the [KayteIDE GitHub repository](https://github.com/ringsce/kayteide),
detects new commits, and rebuilds the IDE automatically using `git` + `cmake`.

## Features

| Feature | Details |
|---|---|
| GitHub API polling | Checks `api.github.com/repos/ringsce/kayteide/commits/main` |
| Configurable interval | 1–1440 minutes, persisted between sessions |
| Full build pipeline | clone/pull → cmake configure → cmake build → optional install |
| Parallel builds | Configurable `-j` jobs |
| Build log | Color-coded real-time output in a scrollable terminal-style widget |
| System tray | Runs minimized; shows balloon notifications on update/build done |
| Settings persistence | Via `QSettings` (per-platform: `~/.config/ringsce/KayteIDEUpdater.ini` on Linux) |
| CLI flag | `--minimized` / `-m` to start in the tray |

## Requirements

- Qt 6.2+ (Core, Gui, Widgets, Network)
- CMake 3.16+
- git (in PATH)
- A working C++ toolchain (gcc/clang/MSVC)

## Build

```bash
git clone https://github.com/ringsce/kayteide-updater.git  # adjust if needed
cd kayteide-updater
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

## Run

```bash
./build/KayteIDEUpdater          # normal window
./build/KayteIDEUpdater -m       # start minimized to system tray
```

## Architecture

```
main.cpp
└── MainWindow          — Qt6 Widgets UI, settings, system tray
    ├── UpdateChecker   — QNetworkAccessManager → GitHub REST API
    │                     emits updateAvailable(CommitInfo) / alreadyUpToDate()
    └── BuildManager    — QProcess pipeline
                          git pull/clone → cmake -S -B → cmake --build → cmake --install
```

## Pipeline stages

1. **Clone** — if `<sourceDir>/.git` does not exist, `git clone --depth=1 <repo> <sourceDir>`
2. **Pull** — if repo already exists, `git pull --rebase --autostash`
3. **Configure** — `cmake -S <sourceDir> -B <buildDir> -DCMAKE_BUILD_TYPE=Release`
4. **Build** — `cmake --build <buildDir> --parallel <jobs> --config Release`
5. **Install** *(optional)* — `cmake --install <buildDir> --prefix <installDir>`

## Extending

- Add branch selection → expose `m_branch` in `UpdateChecker` via a `QComboBox`
- Add pre-build hooks → insert a `QProcess` stage before CMake configure
- Sign binaries → add a stage after install that runs `codesign` / `signtool`
