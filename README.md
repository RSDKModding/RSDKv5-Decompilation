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

## Get the source code

* Clone the repo **recursively**, using:
```git clone --recursive https://github.com/Rubberduckycooly/RSDKv5-Decompilation.git```
or if you've already cloned the repo, run inside:
```git submodule update --init```

## Follow the build steps

* [Windows](./dependencies/windows/README.md)
* [Mac](./dependencies/mac/README.md)
* [Linux/Switch](./dependencies/ogl/README.md)
* [Android](./dependencies/android/README.md)

## Other Platforms
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

### Q: Are there anymore decompilation projects in the works, such as other RSDK versions?
A: Absolutely not. This project took about 1 and a half years to do, and between the last two and this one, we're done with decompiling, at least for the time being. Please do not expect any more decompilations from us, Sonic or otherwise!

# Special Thanks
* [Chuli](https://github.com/MGRich) for leading ModAPI development, porting to other platforms, general decompilation assistance, helping me fix bugs, tweaking up my sometimes sloppy code and generally being really helpful and fun to work with on this project
* The Weigman for creating the asset bases such as the header and icons (originally made for RSDKv3 and v4, modified by Chuli)
* Everyone in the [Retro Engine Modding Server](https://dc.railgun.works/retroengine) for being supportive of me and for giving me a place to show off these things that I've found

# Contact:
Join the [Retro Engine Modding Discord Server](https://dc.railgun.works/retroengine) for any extra questions you may need to know about the decompilation or modding it.
