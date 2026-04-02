# OpenVisus — Windows build (steps that worked)

This document records a **working** configure line on Windows with **Visual Studio 2026**, **Qt 5.12.8 (msvc2017_64)**, and **Miniconda Python**. Adjust paths if your machine differs.

## Prerequisites

- **Visual Studio 2026** (or compatible) with **Desktop development with C++**
- **CMake** (e.g. from cmake.org)
- **Qt 5.x** with an **MSVC 64-bit** kit — *not* MinGW, *not* Qt 6 for this upstream tree
- **SWIG** ≥ 3.0 on `PATH` (e.g. `choco install swig` or `conda install -c conda-forge swig`) — required when `VISUS_PYTHON=ON`
- **Python** you want to link against (here: Miniconda base `python.exe`)
- **Git**

Use **Developer PowerShell for VS 2026** (or **x64 Native Tools**) so MSVC is on `PATH`.

Download QT5.12.8 from here: [https://download.qt.io/new_archive/qt/5.12/](https://download.qt.io/new_archive/qt/5.12/) 

## Configure (from `build` directory)

```powershell
cd D:\Research\github\OpenVisus\build

cmake -S .. -B . -G "Visual Studio 18 2026" -A x64 `
  -DVISUS_GUI=ON `
  -DQt5_DIR="C:/Qt/Qt5.12.8/5.12.8/msvc2017_64/lib/cmake/Qt5" `
  -DPython_EXECUTABLE="C:\Users\Aashish\miniconda3\python.exe"
```

Notes:

- `**Qt5_DIR**` must be the folder that contains `**Qt5Config.cmake**` (usually `...\lib\cmake\Qt5`). CMake uses `NO_DEFAULT_PATH`; if you omit this, Qt will not be found.
- Forward slashes in `Qt5_DIR` are fine for CMake on Windows.
- To build **without** the Qt GUI (no Qt install needed): add `-DVISUS_GUI=OFF`.

If you change important variables, delete `CMakeCache.txt` in `build` and re-run `cmake`.

## Build and install

Still in `build`:

```powershell
cmake --build . --config Release --parallel

#after each code change, run this build...
cmake --build . --target INSTALL --config Release
```

Install output is under the build tree (this project sets `CMAKE_INSTALL_PREFIX` to the build directory). For a **Release** Visual Studio build, Python packages and binaries are typically under:

`build\Release\OpenVisus\`

## Use OpenVisus from Python (without pip)

Point `PYTHONPATH` at the **Release** folder that contains the `OpenVisus` package (the same level as the built tree layout after install). For example:

```powershell
#USING BASH
export PYTHONPATH="/d/Research/github/OpenVisus/build/Release"

python -c "from OpenVisus import *"
```

```powershell
#USING POWERSHELL
$env:PYTHONPATH = "D:\Research\github\OpenVisus\build\Release"
python -c "from OpenVisus import *"

#Using CMD
set PYTHONPATH=D:\Research\github\OpenVisus\build\Release

```

If your layout differs, search for `OpenVisus\__init__.py` under `build` and set `PYTHONPATH` to its **parent** directory.

## Viewer / PyQt alignment (optional)

If you use the GUI viewer with Python, you may need the project’s configure step (can install or adjust PyQt5):

```powershell
python -m OpenVisus configure --user
```

```powershell
 python -m OpenVisus viewer
```

See the main [README](README.md) and [docs/compilation.md](docs/compilation.md) for troubleshooting PyQt and platform-specific details.

## Reference

- Upstream build documentation: [docs/compilation.md](docs/compilation.md)
- CMake generator name `**Visual Studio 18 2026**` comes from `cmake --help` on a system with VS 2026 installed.

