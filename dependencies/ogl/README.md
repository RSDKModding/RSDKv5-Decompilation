# Linux

## Installing dependencies 

Building for Windows on Linux requires `vcpkg`. i (fox) don't do this, so dont take my word for it.
apparently you run `vcpkg install glfw3 glew --triplet=(wtv wanted)`?
if you can figure this out, [take this section out and fix it yourself](https://github.com/Rubberduckycooly/RSDKv5-Decompilation/fork)

For Switch you'll need [devkitPro](https://devkitpro.org/) and GLAD, install GLAD with `sudo dkp-pacman -S switch-glad`.

For Linux you can install the dependencies using your distro package manager:
- **pacman (Arch):** `sudo pacman -S base-devel glew glfw libtheora zlib sdl2 portaudio`
- **apt (Debian/Ubuntu):** `sudo apt install build-essential libglew-dev libglfw3-dev libtheora-dev zlib1g-dev libsdl2-dev portaudio19-dev`
- **rpm (Fedora):** `sudo dnf install make gcc glew-devel glfw-devel libtheora-devel zlib-devel SDL2-devel portaudio`
- your favorite package manager here, [make a pull request](https://github.com/Rubberduckycooly/RSDKv5-Decompilation/fork) <3

## Compiling

Compiling is as simple as typing the following:
```
mkdir build
cd build
cmake .. # arguments go here
cmake --build ..
```

The following cmake arguments are available when compiling.
- `TRO_REVISION`= what revision to compile for. takes an integer, defaults to `3` (RSDKv5U).
- `RETRO_DISABLE_PLUS`= what it says on the tin. takes a boolean (on/off); build with `=on` when compiling for distribution.
- `RETRO_MOD_LOADER`= enables or disables the mod loader. takes a boolean (on/off).
- `RETRO_MOD_LOADER_VER`= manually sets the mod loader version. takes a string, defaults to `latest`.

After compiling your binary, stick your `libGame.so` and `Data.rsdk` files in the same directory, launch, and voila!
