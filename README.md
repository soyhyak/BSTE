# BSTE - Basic and Simple Text Editor

A lightweight, no-nonsense text editor built with C++ and Qt Widgets.

## Philosophy

Keep it basic. Keep it simple.

## Features

- Line numbers
- Syntax highlighting (C++, Python, Plain)
- Find & Replace
- Go to Line (Ctrl+G)
- Zoom (Ctrl + Mouse Wheel)
- Word Wrap
- Dark mode (follows system)
- Atomic saving (safe writes)
- UTF-8 support
- Tab size configuration

## Build

Requires Qt 6 and a C++17 compiler.

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
sudo cmake --install .
