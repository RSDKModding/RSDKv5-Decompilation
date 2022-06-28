
#include "v3/RetroEnginev3.hpp"
#include "v4/RetroEnginev4.hpp"

namespace Legacy
{

#define LEGACY_RETRO_USE_HAPTICS (1)

enum RetroStates {
    ENGINE_DEVMENU     = 0,
    ENGINE_MAINGAME    = 1,
    ENGINE_INITDEVMENU = 2,
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