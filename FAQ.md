# General
### Q: Why is the DLC disabled in release builds and autobuilds?
A: Long story short, it's to minimize piracy and ensure an extra layer of legal protection for Mania Plus. Giving players paid content for free is not the goal of this project.

### Q: How do I change the username?
A: In the `Game` category of settings.ini, add a tag called `username` and enter the desired username in it.

### Q: The screen is tearing, how do I fix it?
A: Try turning on VSync in settings.ini.

### Q: I found a bug/I have a feature request!
A: Submit an issue in the issues tab and we _might_ fix it in the main branch. Don't expect any major future releases, however.

# Using Sonic Origins RSDK Files
### Q: Why is there no audio?
A: Sonic Origins doesn't have any music or sound effects contained in the games' data files, instead storing and handling all in-game audio itself through Hedgehog Engine 2. You can fix this by simply inserting the audio files other versions of the games.

### Q: Why is the Drop Dash disabled by default? How do I turn it on?
A: By default, the game mode is set to Classic Mode, which disables the Drop Dash. The only way to change this is through a mod, either by changing the default value of the `game.playMode` global variable in `GameConfig.bin` or by setting the variable to another value via scripts.

# Miscellaneous
### Q: Is there a decompilation for RSDKv3 and/or RSDKv4 alone?
A: There is! You can find RSDKv3 [here](https://github.com/Rubberduckycooly/Sonic-CD-11-Decompilation) and RSDKv4 [here](https://github.com/Rubberduckycooly/Sonic-1-2-2013-Decompilation).

### Q: Are there anymore decompilation projects in the works, such as other RSDK versions?
A: Absolutely not. This project took about 1 and a half years to do, and between the last two and this one, we're done with decompiling, at least for the time being. Please do not expect any more decompilations from us, Sonic or otherwise!