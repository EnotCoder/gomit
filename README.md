# Gomit Engine

<p align="center">
  <a href="https://godotengine.org">
    <img src="misc/logo/logo.svg" width="400" alt="Godot Engine logo">
  </a>
</p>

## 2D and 3D cross-platform game engine

Gomit Engine is a feature-packed, cross-platform
game engine to create 2D and 3D games from a unified interface.

## Gomit Engine is a fork Godot engine

![Screenshot of a 3D scene in the Godot Engine editor](https://raw.githubusercontent.com/godotengine/godot-design/master/screenshots/editor_tps_demo_1920x1080.jpg)

## Build and run

### Prerequisites (Linux)

Install the build dependencies, e.g. on Arch:

```sh
sudo pacman -S scons pkgconf gcc python
```

On Debian/Ubuntu:

```sh
sudo apt install build-essential scons pkg-config libx11-dev libxcursor-dev \
  libxinerama-dev libgl1-mesa-dev libglu1-mesa-dev libasound2-dev libpulse-dev \
  libudev-dev libxi-dev libxrandr-dev
```

See the [Godot docs](https://docs.godotengine.org/en/latest/contributing/development/compiling/compiling_for_linuxbsd.html) for the full list.

### Build the editor

```sh
scons -j12 platform=linuxbsd target=editor
```

The binary is produced at `bin/godot.linuxbsd.editor.x86_64`.

### Run

Open a project in the editor:

```sh
bin/godot.linuxbsd.editor.x86_64 --editor --path path/to/project
```

Run a project (export/play):

```sh
bin/godot.linuxbsd.editor.x86_64 --path path/to/project
```

### Game view preview (this fork's feature)

The **Game** workspace (bottom-left of the editor, or `Ctrl+F4`) shows a
Unity-like live preview of the edited scene while the game is not running.
The preview rebuilds automatically 0.5 s after an edit and renders through the
scene's camera; if there is no camera it shows *"No camera in the scene."*.