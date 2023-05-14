# Linux

## Installing dependencies 

Building for Windows on Linux requires `vcpkg`. i (fox) don't do this, so dont take my word for it.
apparently you run `vcpkg install glfw3 glew --triplet=(wtv wanted)`?
if you can figure this out, [take this section out and fix it yourself](https://github.com/Rubberduckycooly/RSDKv5-Decompilation/fork).

For Switch you'll need [devkitPro](https://devkitpro.org/) and GLAD, install GLAD with `sudo dkp-pacman -S switch-glad`.

For Linux you can install the dependencies using your distro package manager:
- **pacman (Arch):** `sudo pacman -S base-devel glew glfw libtheora zlib sdl2 portaudio`
- **apt (Debian/Ubuntu):** `sudo apt install build-essential libglew-dev libglfw3-dev libtheora-dev zlib1g-dev libsdl2-dev portaudio19-dev`
- **rpm (Fedora):** `sudo dnf install make gcc glew-devel glfw-devel libtheora-devel zlib-devel SDL2-devel portaudio`
- your favorite package manager here, [make a pull request](https://github.com/Rubberduckycooly/RSDKv5-Decompilation/fork) <3 (also update Sonic Manias' repo!)

## Compiling

Compiling is as simple as typing the following:
```
mkdir build
cd build
cmake .. # arguments go here
cmake --build ..
```

### RSDKv5 flags
- `TRO_REVISION`= What revision to compile for. Takes an integer, defaults to `3` (RSDKv5U).
- `RETRO_DISABLE_PLUS`= What it says on the tin. Takes a boolean (on/off); build with `=on` when compiling for distribution. Defaults to `=off`.
- `RETRO_MOD_LOADER`= Enables or disables the mod loader. Takes a boolean, defaults to `=on`.
- `RETRO_MOD_LOADER_VER`= Manually sets the mod loader version. Takes a string, defaults to `latest`.
- `CMAKE_SYSTEM_NAME`= Manually sets the system to build for. Takes a string, is set by default via `uname`.

After compiling your binary, stick your `libGame.so` and `Data.rsdk` files in the same directory, launch, and voila!
