# Linux/Switch

**This document was intended for a now outdated method of building this decompilation and is only left in this repository for reference purposes. It's highly recommended to follow the [CMake guide](./../../README.md#how-to-build) instead.**

## Installing GL3 dependencies 

The OpenGL3 backend is mainly used on Linux and Switch, but it does support Windows too.

For Windows, these need to be downloaded and extracted in their respective subdirectories 
- **GLEW:** Download from [SourceForge](http://glew.sourceforge.net/), extract it in `dependencies/ogl/glew`
- **GLFW:** Download from [the official site](https://www.glfw.org/download.html), extract it in `dependencies/ogl/glfw`  
  There are also 32-bit binaries available if you need them, but make sure the RSDKv5(U) is built for 32-bit too!

For Switch you'll need [devkitPro](https://devkitpro.org/) and GLAD, as GLEW and GLFW are not available. Install GLAD with `sudo dkp-pacman -S switch-glad`

For Linux you can install the dependencies using your distro package manager:
- **Arch Linux:** `sudo pacman -S base-devel glew glfw libtheora zlib sdl2`
- **Ubuntu (20.04+) or Debian (11+):** `sudo apt install build-essential libglew-dev libglfw3-dev libtheora-dev zlib1g-dev libsdl2-dev`
- **Fedora:** `sudo dnf install make gcc glew-devel glfw-devel libtheora-devel zlib-devel SDL2-devel`

## Compiling 

To compile you can just use `make`. To customize the build you can set the following options
- `PLATFORM=Switch`: Build for Nintendo Switch.
- `RSDK_REVISION=2`: Compile regular RSDKv5 instead of RSDKv5U.
- `RSDK_ONLY=1`: Only build the engine (no Game.so)
- `AUTOBUILD=1`: Disable the Plus DLC, which you should do if you plan on distributing the binary.

## Shaders 

Once completed, it is **heavily recommended** that you grab the Shaders folder in RSDKv5 and turn it into a mod. Otherwise, movies will not display properly and the filters from video settings won't work.

To do this, create the following directory structure inside your mods directory:
```
GLShaders/
| Data/
| | ...
| mod.ini
```

Inside `mods/GLShaders/Data/` copy the `RSDKv5/Shaders` directory, and inside the mod.ini, paste this:
```
Name=GLShaders
Description=GL3 shaders to enable filters and stuff
Author=Ducky
Version=1.0.0
TargetVersion=5
```