
#include "v3/RetroEnginev3.hpp"
#include "v4/RetroEnginev4.hpp"

namespace Legacy
{

#define LEGACY_RETRO_USE_HAPTICS (1)

enum RetroStates {
    ENGINE_DEVMENU     = 0,
    ENGINE_MAINGAME    = 1,
    ENGINE_INITDEVMENU = 2,

    ENGINE_SCRIPTERROR = 4,
};

enum DeviceTypes { DEVICE_STANDARD = 0, DEVICE_MOBILE = 1 };

enum GamePlatformID {
    LEGACY_RETRO_WIN      = 0,
    LEGACY_RETRO_OSX      = 1,
    LEGACY_RETRO_XBOX_360 = 2,
    LEGACY_RETRO_PS3      = 3,
    LEGACY_RETRO_iOS      = 4,
    LEGACY_RETRO_ANDROID  = 5,
    LEGACY_RETRO_WP7      = 6
};

enum RetroLanguages {
    LEGACY_LANGUAGE_EN = 0,
    LEGACY_LANGUAGE_FR = 1,
    LEGACY_LANGUAGE_IT = 2,
    LEGACY_LANGUAGE_DE = 3,
    LEGACY_LANGUAGE_ES = 4,
    LEGACY_LANGUAGE_JP = 5,
    LEGACY_LANGUAGE_PT = 6,
    LEGACY_LANGUAGE_RU = 7,
    LEGACY_LANGUAGE_KO = 8,
    LEGACY_LANGUAGE_ZH = 9,
    LEGACY_LANGUAGE_ZS = 10
};

extern int32 gameMode;
extern bool32 usingBytecode;

extern bool32 trialMode;
extern int32 gamePlatformID;
extern int32 deviceType;
extern bool32 onlineActive;
extern int32 language;
#if LEGACY_RETRO_USE_HAPTICS
extern bool32 hapticsEnabled;
#endif

void CalculateTrigAnglesM7();

} // namespace Legacy