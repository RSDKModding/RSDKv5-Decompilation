# ANDROID SETUP

* Clone the repo, then install the dependencies listed below
* Ensure the symbolic links in `[root]/android/app/jni` are correct: 
  * `RSDKv5` -> root of the RSDKv5 repository
  * `Game` -> root of the Mania repository (or any other game that you may be compiling)
  * Extra symbolic links can be added for things like mods, **as mods do not use the local files.** Just ensure that there is an `Android.mk` file at the root of them. 
    * (to add symbolic links, use the followig:)
      * Windows: `mklink /D RSDKv5 "[path]"`
      * Linux: `ln -s RSDKv5 "[path]"`
* Open `[root]/android/` in Android Studio, install the NDK and everything else that it asks for, and build.

## Known issues
These issues will not be fixed here as they are minor, but feel free to have a look at them for yourself in a fork!
### Audio device doesn't disconnect properly
The audio callback for the lost stream will continue to fire (AAudioDevice's AudioCallback,) so you only end up hearing half on the new stream. All examples I (RMG) looked at while porting didn't show any issues or workarounds for this. I tried to remedy by checking the stream's pointer, but that didn't fix anything.

### HID devices (controllers) are unimplemented
HID, quite honestly, just seems annoying to implement. I've left files for an Android input device to be implemented. You'd need to use `androidHelpers.cpp` as a middleman to passoff from Java-side to RSDKv5. I'd recommend eyeing how [SDL does it](https://github.com/libsdl-org/SDL/tree/main/android-project/app/src/main/java/org/libsdl/app).

## Dependencies:
* libogg: https://xiph.org/downloads/ (libogg)
  
  Download libogg and unzip it in "./libogg/".

* libtheora: https://xiph.org/downloads/ (libtheora)
  
  Download libvorbis and unzip it in "./libtheora/".