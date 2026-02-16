# GFXReconstruct Viewer

A tool to capture and replay GFXReconstruct files on Android platform, and view recorded file.

This project is still at very early stage, and many features are limited.

## To Do List

- UI
  - Fix startup window unable to drag
  - Pop up confirm window on warning and error message
- Record
  - Feature: A button to stop recording
  - Feature: Option to select recorded frame range
  - Feature: Pull recorded file from Android device
- Replay
  - Feature: Detect whether replay is finished
  - Feature: More options like screenshot
- Entire API view functionality

## How to Build

Prerequisite: Qt 6

### Windows

Both Visual Studio 2022 and Visual Studio 2026 are tested.

If Qt6_DIR cannot be found, check whether Qt6 bin path is in system PATH. For default installation, it can be located here:
```
C:\Qt\6.x.x\msvc2022_64\bin
```
Or you can specify Qt6_DIR by yourself:
```
C:/Qt/6.x.x/msvc2022_64/lib/cmake/Qt6
```

### Linux/MacOS

```
mkdir build
cd build
export PATH="$HOME/Qt/6.x.x/<OS Specfic>/bin:$PATH"
cmake ..
make
```

## Credits

- [GFXReconstruct](https://github.com/LunarG/gfxreconstruct)
- [Qt](https://www.qt.io/)
