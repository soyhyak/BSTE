# BSTE - Basic and Simple Text Editor

A rock-solid, minimalist text editor built with C++ and Qt Widgets.

## Features
- Atomic saving with QSaveFile (data safety first).
- UTF-8 Support.
- Line numbers and basic search (Ctrl+F).
- Clean, English-based interface.

## Compatibility
BSTE is desktop-agnostic. While developed on CachyOS, it works on any environment that supports Qt 6 (KDE, GNOME, XFCE, Windows, macOS).

## Build
Requires Qt 6 and a C++17 compiler.

```bash
# Example build with CMake
cmake -B build
cmake --build build
```

## License
MIT - Copyright (c) 2026 soyhyak