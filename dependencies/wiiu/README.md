# WII U SETUP

* Install devkitPro.
* Install devkitPro's 'wiiu-dev' package group.
* Clone the repo, then install the dependencies listed below.
* Build using the CMake script in the 'wiiu' directory.
  * `cmake -B build wiiu -DCMAKE_BUILD_TYPE=Release --toolchain=$DEVKITPRO/cmake/WiiU.cmake`
  * `cmake --build build`

The working directory will be at `[sdcard root]/RSDK/v5`.

## Dependencies:

* libtheora & libogg
  * Run `makepkg -cirs` in this directory to build and install libtheora for Wii U homebrew development.

* SDL2
  * Install the Wii U SDL2 package with `pacman -S wiiu-sdl2`.
