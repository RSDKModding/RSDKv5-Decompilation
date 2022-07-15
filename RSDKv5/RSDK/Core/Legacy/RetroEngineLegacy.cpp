
#include "v3/RetroEnginev3.cpp"
#include "v4/RetroEnginev4.cpp"

int32 RSDK::Legacy::gameMode       = Legacy::ENGINE_MAINGAME;
bool32 RSDK::Legacy::usingBytecode = false;

bool32 RSDK::Legacy::trialMode     = false;
int32 RSDK::Legacy::gamePlatformID = Legacy::LEGACY_RETRO_WIN;
int32 RSDK::Legacy::deviceType     = Legacy::DEVICE_STANDARD;
bool32 RSDK::Legacy::onlineActive  = false;
int32 RSDK::Legacy::language       = Legacy::LEGACY_LANGUAGE_EN;
#if LEGACY_RETRO_USE_HAPTICS
bool32 RSDK::Legacy::hapticsEnabled = false;
#endif

int32 RSDK::Legacy::sinM7LookupTable[0x200];
int32 RSDK::Legacy::cosM7LookupTable[0x200];

void RSDK::Legacy::CalculateTrigAnglesM7()
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