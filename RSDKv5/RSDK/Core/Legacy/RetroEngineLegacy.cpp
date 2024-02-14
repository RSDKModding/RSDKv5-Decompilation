#if RETRO_USE_MOD_LOADER
// both v3 and v4 use these
std::vector<SceneListEntry> listData;
std::vector<SceneListInfo> listCategory;
#endif

namespace RSDK
{
namespace Legacy
{

#include "v3/RetroEnginev3.cpp"
#include "v4/RetroEnginev4.cpp"

int32 gameMode       = ENGINE_MAINGAME;
bool32 usingBytecode = false;

bool32 trialMode     = false;
int32 gamePlatformID = LEGACY_RETRO_WIN;
int32 deviceType     = DEVICE_STANDARD;
bool32 onlineActive  = false;
int32 language       = LEGACY_LANGUAGE_EN;
#if LEGACY_RETRO_USE_HAPTICS
bool32 hapticsEnabled = false;
#endif

int32 sinM7LookupTable[0x200];
int32 cosM7LookupTable[0x200];

void CalculateTrigAnglesM7()
{
    for (int32 i = 0; i < 0x200; ++i) {
        sinM7LookupTable[i] = (int32)(sinf((i / 256.0) * RSDK_PI) * 4096.0);
        cosM7LookupTable[i] = (int32)(cosf((i / 256.0) * RSDK_PI) * 4096.0);
    }

    cosM7LookupTable[0x00]  = 0x1000;
    cosM7LookupTable[0x80]  = 0;
    cosM7LookupTable[0x100] = -0x1000;
    cosM7LookupTable[0x180] = 0;

    sinM7LookupTable[0x00]  = 0;
    sinM7LookupTable[0x80]  = 0x1000;
    sinM7LookupTable[0x100] = 0;
    sinM7LookupTable[0x180] = -0x1000;
}

} // namespace Legacy
} // namespace RSDK