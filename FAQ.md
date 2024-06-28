# General
### Q: Why is the DLC disabled in release builds and autobuilds?
A: Long story short, it's to minimize piracy and ensure an extra layer of legal protection for Sonic Mania Plus and Sonic Origins Plus. Giving players paid content for free is not the goal of this project.

### Q: How do I change the username?
A: In the `Game` category of settings.ini, add a tag called `username` and enter the desired username in it.

### Q: The screen is tearing, how do I fix it?
A: Try turning on VSync in settings.ini.

### Q: Why aren't videos/filters working while using GL?
A: There's a mod for it that you have to make. Refer to the following directions:

Create the following directory structure inside your mods directory:
```
GLShaders/
| Data/
| | ...
| mod.ini
```

Inside `mods/GLShaders/Data/`, copy the `RSDKv5/Shaders` directory, and inside the `mod.ini`, paste this:
```
Name=GLShaders
Description=GL3 shaders
Author=Ducky
Version=1.0.0
TargetVersion=5
```

### Q: I found a bug/I have a feature request!
A: Submit an issue in the Issues tab and we might look into it. Keep in mind that this is a decompilation, so bugs that exist in official releases will most likely not be fixed here.

# Using Sonic Origins RSDK Files
### Q: Why is there no audio?
A: Sonic Origins doesn't have any music or sound effects contained in the games' data files, instead storing and handling all in-game audio itself through Hedgehog Engine 2. You can fix this by simply inserting the audio files from other versions of the games.

### Q: Why is the Drop Dash disabled by default? How do I turn it on?
A: By default, the game mode is set to Classic Mode, which disables the Drop Dash. The only way to change this is through a mod, either by changing the default value of the `game.playMode` global variable in `GameConfig.bin` or by setting the variable to another value via scripts.

### Q: How do I play as the Origins Plus characters?
A: Sonic Team implemented Amy (and Knuckles in Sonic CD) in a way where they aren't playable on the decomp out of the box. This can be fixed via mods. **Do not ask about this in an issue, as we will not be able to help you.**
There are also checks implemented in the engine to prevent playing as these characters on release builds and autobuilds.

# Miscellaneous
### Q: Is there a decompilation for RSDKv3 and/or RSDKv4 alone?
A: There is! You can find RSDKv3 [here](https://github.com/RSDKModding/RSDKv3-Decompilation) and RSDKv4 [here](https://github.com/RSDKModding/RSDKv4-Decompilation).

### Q: Are there anymore decompilation projects in the works, such as other RSDK versions?
A: Absolutely not. This project took about 1 and a half years to do, and between the last two and this one, we're done with decompiling, at least for the time being. Please do not expect any more decompilations from us, Sonic or otherwise!