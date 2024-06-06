![header](header.png?raw=true)

A complete decompilation of Retro Engine v5 and v5Ultimate.

# **SUPPORT THE DEVELOPERS OF THE RETRO ENGINE**
We do not own the Retro Engine in any way, shape or form, and this project would not have been possible had they not developed RSDKv5(U) in the first place. Retro Engine is currently owned by [Evening Star](https://eveningstar.studio/); we highly urge you to follow & support their projects if you enjoyed this project of ours!

## **DO NOT USE THIS DECOMPILATION PROJECT AS A MEANS TO PIRATE SONIC MANIA OR ANY OTHER RSDKv5(U) GAMES.**
We do not condone using this project as a means for piracy in any form. This project was made with love and care for the source material and was created for purely educational purposes.

# Additional Tweaks
* Added a built-in mod loader and API allowing to easily create and play mods with features such as save file redirection and XML asset loading, supported by all sub-versions of v5U.
* Added a built-in shader compiler for backends/platforms that support it.
* Added various other backends to windows aside from the usual DirectX 9 backends

## If you are here for Sonic Mania:
You have the option of building RSDKv5 alongside Mania in [the Sonic Mania Decompilation repo](https://github.com/Rubberduckycooly/Sonic-Mania-Decompilation).

# How to Build

This project uses [CMake](https://cmake.org/), a versatile building system that supports many different compilers and platforms. You can download CMake [here](https://cmake.org/download/). **(Make sure to enable the feature to add CMake to the system PATH during the installation if you're on Windows!)**

## Get the source code

In order to clone the repository, you need to install Git, which you can get [here](https://git-scm.com/downloads).

Clone the repo **recursively**, using:
`git clone --recursive https://github.com/Rubberduckycooly/RSDKv5-Decompilation`

If you've already cloned the repo, run this command inside of the repository:
```git submodule update --init```

## Follow the build steps

### Windows
To handle dependencies, you'll need to install [Visual Studio Community](https://visualstudio.microsoft.com/downloads/) (make sure to install the `Desktop development with C++` package during the installation) and [vcpkg](https://github.com/microsoft/vcpkg#quick-start-windows).

After installing those, run the following in Command Prompt (make sure to replace `[vcpkg root]` with the path to the vcpkg installation!):
- `[vcpkg root]/vcpkg.exe install libtheora libogg --triplet=x64-windows-static` (If you're compiling a 32-bit build, replace `x64-windows-static` with `x86-windows-static`.)

Finally, follow the [compilation steps below](#compiling) using `-DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static` as arguments for `cmake -B build`.
  - Make sure to replace `[vcpkg root]` with the path to the vcpkg installation!
  - If you're compiling a 32-bit build, replace `x64-windows-static` with `x86-windows-static`.

### Linux
Install the following dependencies: then follow the [compilation steps below](#compiling):
- **pacman (Arch):** `sudo pacman -S base-devel cmake glew glfw libtheora`
- **apt (Debian/Ubuntu):** `sudo apt install build-essential cmake libglew-dev libglfw3-dev libtheora-dev`
- **rpm (Fedora):** `sudo dnf install make cmake gcc glew-devel glfw-devel libtheora-devel zlib-devel`
- **xbps (Void):** `sudo xbps-install make cmake gcc pkg-config glew-devel glfw-devel libtheora-devel zlib-devel`
- Your favorite package manager here, [make a pull request](https://github.com/Rubberduckycooly/RSDKv5-Decompilation/fork) (also update [Mania](https://github.com/Rubberduckycooly/Sonic-Mania-Decompilation)!)

#### (make sure to [install GL shaders!](FAQ.md#q-why-arent-videosfilters-working-while-using-gl))

## Switch
[Setup devKitPro](https://devkitpro.org/wiki/Getting_Started), then run the following:
- `(dkp-)pacman -Syuu switch-dev switch-libogg switch-libtheora switch-sdl2 switch-glad`

Finally, follow the [compilation steps below](#compiling) using `-DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake` as arguments for `cmake -B build`.

#### (make sure to [install GL shaders!](FAQ.md#q-why-arent-videosfilters-working-while-using-gl))

## Android
Follow the android build instructions [here.](./dependencies/android/README.md)

### Compiling

Compiling is as simple as typing the following in the root repository directory:
```
cmake -B build
cmake --build build --config release
```

The resulting build will be located somewhere in `build/` depending on your system.

The following cmake arguments are available when compiling:
- Use these on the `cmake -B build` step like so: `cmake -B build -DRETRO_DISABLE_PLUS=on`

### RSDKv5 flags
- `RETRO_REVISION`: What revision to compile for. Takes an integer, defaults to `3` (RSDKv5U).
- `RETRO_DISABLE_PLUS`: Whether or not to disable the Plus DLC. Takes a boolean (on/off): build with `on` when compiling for distribution. Defaults to `off`.
- `RETRO_MOD_LOADER`: Enables or disables the mod loader. Takes a boolean, defaults to `on`.
- `RETRO_MOD_LOADER_VER`: Manually sets the mod loader version. Takes an integer, defaults to the current latest version.
- `RETRO_SUBSYSTEM`: *Only change this if you know what you're doing.* Changes the subsystem that RSDKv5 will be built for. Defaults to the most standard subsystem for the platform.

## Other Platforms
Currently, the only officially supported platforms are the ones listed above.

**However,** since release, there have been a multitude of forks made by the community (keep in mind that many of these ports are still a WIP, and some may be out of date): 
* ### [WebASM](https://github.com/heyjoeway/RSDKv5-Decompilation/tree/emscripten) by heyjoeway 
* ### [New 3DS](https://github.com/SaturnSH2x2/RSDKv5-Decompilation/tree/3ds-main) by SaturnSH2x2
* ### [Wii U](https://github.com/Radfordhound/RSDKv5-Decompilation) by Radfordhound
* ### [Wii U](https://github.com/Clownacy/Sonic-Mania-Decompilation) by Clownacy
* ### [Wii](https://github.com/Mefiresu/RSDKv5-Decompilation/tree/dev/wii-port) by Mefiresu
* ### [Vita](https://github.com/SonicMastr/Sonic-Mania-Vita) by SonicMastr
* #### and a [general optimization fork](https://github.com/smb123w64gb/RSDKv5-Decompilation) by smb123w64gb

# FAQ
You can find the FAQ [here](./FAQ.md).

# Special Thanks
* [st×tic](https://github.com/stxticOVFL) for leading ModAPI development, porting to other platforms, general decompilation assistance, helping me fix bugs, tweaking up my sometimes sloppy code and generally being really helpful and fun to work with on this project
* [The Weigman](https://github.com/TheWeigman) for making v5 and v5U assets with st×tic such as the header and icons
* Everyone in the [Retro Engine Modding Server](https://dc.railgun.works/retroengine) for being supportive of me and for giving me a place to show off these things that I've found

# Contact:
Join the [Retro Engine Modding Discord Server](https://dc.railgun.works/retroengine) for any extra questions you may need to know about the decompilation or modding it.
