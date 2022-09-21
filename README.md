![header](https://user-images.githubusercontent.com/29069561/183143615-d7f77921-13cf-4c58-8c5f-6a1e76ea20e2.svg)

A complete decompilation of Retro Engine v5 and v5Ultimate.

# **SUPPORT THE DEVELOPERS OF THE RETRO ENGINE**
We do not own the Retro Engine in any way, shape or form, and this project would not have been possible had they not developed RSDKv5(U) in the first place. Retro Engine is currently owned by [Evening Star](https://eveningstar.studio/); we highly urge you to follow & support their projects if you enjoyed this project of ours!

## **DO NOT USE THIS DECOMPILATION PROJECT AS A MEANS TO PIRATE SONIC MANIA OR ANY OTHER RSDKv5(U) GAMES.**
We do not condone using this project as a means for piracy in any form. This project was made with love and care for the source material and was created for purely educational purposes.

# Additional Tweaks
* Added a built-in mod loader and API allowing to easily create and play mods with features such as save file redirection and XML asset loading, supported by all sub-versions of v5U.
* Added a built-in shader compiler for backends/platforms that support it.
* Added various other backends to windows aside from the usual DirectX 9 backends

# How to Build
First, follow the steps in [the common dependency README](./dependencies/all/README.md), then follow the steps for your platform of choice:

* ### [Windows](./dependencies/windows/README.md)

* ### [Mac](./dependencies/mac/README.md)

* ## Linux/Switch
  * Follow the [GL3 README](./dependencies/gl3/README.md) to setup the renderer's dependencies.
  * **For Linux, SDL for audio** (or if you use the SDL2 backend) **will be required:**
    * On Ubuntu: `sudo apt install libsdl2-dev`
    * On Fedora: `sudo dnf install SDL2-devel`
    * On Arch: `sudo pacman -S sdl2` 
  * Then, for both platforms, the makefile can be used by running `make`.
    * For Switch, pass `PLATFORM=Switch` to the `make` command to ensure you're building for Switch.

* ### [Android](./dependencies/android/README.md)

### Other Platforms
Currently, the only officially supported platforms are the ones listed above. However, the backend is very modular, so the codebase is very multiplatform.

**However,** since release, there have been a multitude of forks made by the community (keep in mind that many of these ports are still a WIP!:) 
* ### [WebASM](https://github.com/heyjoeway/RSDKv5-Decompilation/tree/emscripten) by heyjoeway 
* ### [New 3DS](https://github.com/SaturnSH2x2/RSDKv5-Decompilation/tree/3ds-main) by SaturnSH2x2
* ### [Wii U](https://github.com/Radfordhound/RSDKv5-Decompilation) by Radfordhound
* ### [Wii U](https://github.com/Clownacy/Sonic-Mania-Decompilation) by Clownacy
* ### [Vita](https://github.com/SonicMastr/Sonic-Mania-Vita) by SonicMastr
* #### and a [general optimization fork](https://github.com/smb123w64gb/RSDKv5-Decompilation) by smb123w64gb

# FAQ
### Q: The screen is tearing, how do I fix it?
A: Try turning on VSync in settings.ini.

### Q: I found a bug/I have a feature request!
A: Submit an issue in the issues tab and we _might_ fix it in the main branch. Don't expect any major future releases, however.

### Q: Is there a decompilation for RSDKv3 and/or RSDKv4 alone?
A: There is! You can find RSDKv3 [here](https://github.com/Rubberduckycooly/Sonic-CD-11-Decompilation) and RSDKv4 [here](https://github.com/Rubberduckycooly/Sonic-1-2-2013-Decompilation).

### Q: Will there be a decompilation for any other RSDK versions?
A: No. This is the last decompilation from us. This project took about 1 and a half years to do, and with it completed, we're ready to move onto other endeavours rather than continue decompiling programs forever.

### Q: Are there anymore decompilation projects in the works?
A: Absolutely not. Between the last two and this one, we're done with decompiling, at least for the time being. Please do not expect any more decompilations from us, Sonic or otherwise!

# Special Thanks
* [Chuli](https://github.com/MGRich) for leading ModAPI development, porting to other platforms, general decompilation assistance, helping me fix bugs, tweaking up my sometimes sloppy code and generally being really helpful and fun to work with on this project
* The Weigman for creating the asset bases such as the header and icons (originally made for RSDKv3 and v4, modified by Chuli)
* Everyone in the [Retro Engine Modding Server](https://dc.railgun.works/retroengine) for being supportive of me and for giving me a place to show off these things that I've found

# Contact:
Join the [Retro Engine Modding Discord Server](https://dc.railgun.works/retroengine) for any extra questions you may need to know about the decompilation or modding it.
