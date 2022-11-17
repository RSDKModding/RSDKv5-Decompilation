#ifndef RETROENGINE_H
#define RETROENGINE_H

// ================
// STANDARD LIBS
// ================
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <ctime>

// ================
// STANDARD TYPES
// ================
typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;
typedef signed long long int64;
typedef unsigned long long uint64;

typedef uint32 bool32;
typedef uint32 color;

namespace RSDK
{

enum GamePlatforms {
    PLATFORM_PC,
    PLATFORM_PS4,
    PLATFORM_XB1,
    PLATFORM_SWITCH,

    PLATFORM_DEV = 0xFF,
};

enum GameLanguages {
    LANGUAGE_EN,
    LANGUAGE_FR,
    LANGUAGE_IT,
    LANGUAGE_GE,
    LANGUAGE_SP,
    LANGUAGE_JP,
    LANGUAGE_KO,
    LANGUAGE_SC,
    LANGUAGE_TC,
};

enum GameRegions {
    REGION_US,
    REGION_JP,
    REGION_EU,
};

} // namespace RSDK

// =================
// INTELLISENSE HACKS (hopefully rdc doesn't kill me)
// =================

#ifdef _INTELLISENSE_NX
#undef __unix__
#undef __linux__
#endif

#ifndef RETRO_USE_ORIGINAL_CODE
#define RETRO_USE_ORIGINAL_CODE (0)
#endif

#ifndef RETRO_STANDALONE
#define RETRO_STANDALONE (1)
#endif

// ============================
// PLATFORMS
// ============================
#define RETRO_WIN    (0)
#define RETRO_PS4    (1)
#define RETRO_XB1    (2)
#define RETRO_SWITCH (3)
// CUSTOM
#define RETRO_OSX     (4)
#define RETRO_LINUX   (5)
#define RETRO_iOS     (6)
#define RETRO_ANDROID (7)
#define RETRO_UWP     (8)

// ============================
// PLATFORMS (used mostly in legacy but could come in handy here)
// ============================
#define RETRO_STANDARD (0)
#define RETRO_MOBILE   (1)

#define sprintf_s(x, _, ...) snprintf(x, _, __VA_ARGS__)

#if defined _WIN32
#undef sprintf_s

#if defined WINAPI_FAMILY
#if WINAPI_FAMILY != WINAPI_FAMILY_APP
#define RETRO_PLATFORM   (RETRO_WIN)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#else
#define RETRO_PLATFORM   (RETRO_UWP)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#endif
#else
#define RETRO_PLATFORM   (RETRO_WIN)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#endif

#elif defined __APPLE__
#define RETRO_USING_MOUSE
#define RETRO_USING_TOUCH
#include <TargetConditionals.h>

#if TARGET_IPHONE_SIMULATOR
#define RETRO_PLATFORM   (RETRO_iOS)
#define RETRO_DEVICETYPE (RETRO_MOBILE)
#elif TARGET_OS_IPHONE
#define RETRO_PLATFORM   (RETRO_iOS)
#define RETRO_DEVICETYPE (RETRO_MOBILE)
#elif TARGET_OS_MAC
#define RETRO_PLATFORM   (RETRO_OSX)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#else
#error "Unknown Apple platform"
#endif
#elif defined __ANDROID__
#define RETRO_PLATFORM   (RETRO_ANDROID)
#define RETRO_DEVICETYPE (RETRO_MOBILE)
#elif defined __SWITCH__
#define RETRO_PLATFORM   (RETRO_SWITCH)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#elif defined __linux__
#define RETRO_PLATFORM   (RETRO_LINUX)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#else
#define RETRO_PLATFORM   (RETRO_WIN)
#define RETRO_DEVICETYPE (RETRO_STANDARD)
#endif

#ifndef SCREEN_XMAX
#define SCREEN_XMAX (1280)
#endif

#ifndef SCREEN_YSIZE
#define SCREEN_YSIZE (240)
#endif

#define SCREEN_CENTERY (SCREEN_YSIZE / 2)

#ifndef BASE_PATH
#define BASE_PATH ""
#endif

// ============================
// RENDER DEVICE BACKENDS
// ============================
#define RETRO_RENDERDEVICE_DIRECTX9  (0)
#define RETRO_RENDERDEVICE_DIRECTX11 (0)
#define RETRO_RENDERDEVICE_NX        (0)
// CUSTOM
#define RETRO_RENDERDEVICE_SDL2 (0)
#define RETRO_RENDERDEVICE_GLFW (0)
#define RETRO_RENDERDEVICE_EGL  (0)

// ============================
// AUDIO DEVICE BACKENDS
// ============================
#define RETRO_AUDIODEVICE_XAUDIO (0)
#define RETRO_AUDIODEVICE_NX     (0)
// CUSTOM
#define RETRO_AUDIODEVICE_SDL2 (0)
#define RETRO_AUDIODEVICE_OBOE (0)

// ============================
// INPUT DEVICE BACKENDS
// ============================
#define RETRO_INPUTDEVICE_KEYBOARD (1)
#define RETRO_INPUTDEVICE_XINPUT   (0)
#define RETRO_INPUTDEVICE_RAWINPUT (0)
#define RETRO_INPUTDEVICE_STEAM    (0)
#define RETRO_INPUTDEVICE_NX       (0)
// CUSTOM
#define RETRO_INPUTDEVICE_SDL2   (0)
#define RETRO_INPUTDEVICE_GLFW   (0)
#define RETRO_INPUTDEVICE_PDBOAT (0)

// ============================
// USER CORE BACKENDS
// ============================
#define RETRO_USERCORE_ID (0)

#define RETRO_USERCORE_DUMMY (!(RETRO_USERCORE_ID & 0x80)) // bit 7 disables the dummy core stuff if you ever need that for some odd reason
#define RETRO_USERCORE_STEAM (RETRO_USERCORE_ID == 1)
#define RETRO_USERCORE_PS4   (RETRO_USERCORE_ID == 2)
#define RETRO_USERCORE_XB1   (RETRO_USERCORE_ID == 3)
#define RETRO_USERCORE_NX    (RETRO_USERCORE_ID == 4)
#define RETRO_USERCORE_EOS   (RETRO_USERCORE_ID == 5)

// ============================
// ENGINE CONFIG
// ============================

// Determines if the engine is RSDKv5 rev01 (all versions of mania pre-plus), rev02 (all versions of mania post-plus) or RSDKv5U (sonic origins)
#ifndef RETRO_REVISION
#define RETRO_REVISION (3)
#endif

// RSDKv5 Rev02 (Used prior to Sonic Mania Plus)
#define RETRO_REV01 (RETRO_REVISION >= 1)

// RSDKv5 Rev02 (Used in Sonic Mania Plus)
#define RETRO_REV02 (RETRO_REVISION >= 2)

// RSDKv5U (Used in Sonic Origins)
#define RETRO_REV0U (RETRO_REVISION >= 3)

// Determines if the engine should use EGS features like achievements or not (must be rev02)
#define RETRO_VER_EGS (RETRO_REV02 && 0)

// enables only EGS's ingame achievements popup without enabling anything else
#define RETRO_USE_DUMMY_ACHIEVEMENTS (RETRO_REV02 && 1)

// enables the use of the mod loader
#ifndef RETRO_USE_MOD_LOADER
#define RETRO_USE_MOD_LOADER (!RETRO_USE_ORIGINAL_CODE && 1)
#endif

// defines the version of the mod loader, this should be changed ONLY if the ModFunctionTable is updated in any way
#ifndef RETRO_MOD_LOADER_VER
#define RETRO_MOD_LOADER_VER (2)
#endif

// ============================
// PLATFORM INIT
// ============================

#if RETRO_PLATFORM == RETRO_WIN

#ifdef RSDK_USE_SDL2
#undef RETRO_RENDERDEVICE_SDL2
#define RETRO_RENDERDEVICE_SDL2 (1)

#undef RETRO_INPUTDEVICE_SDL2
#define RETRO_INPUTDEVICE_SDL2 (1)

#elif defined(RSDK_USE_DX9)
#undef RETRO_RENDERDEVICE_DIRECTX9
#define RETRO_RENDERDEVICE_DIRECTX9 (1)

#undef RETRO_INPUTDEVICE_XINPUT
#define RETRO_INPUTDEVICE_XINPUT (1)

#undef RETRO_INPUTDEVICE_RAWINPUT
#define RETRO_INPUTDEVICE_RAWINPUT (1)

#elif defined(RSDK_USE_DX11)
#undef RETRO_RENDERDEVICE_DIRECTX11
#define RETRO_RENDERDEVICE_DIRECTX11 (1)

#undef RETRO_INPUTDEVICE_XINPUT
#define RETRO_INPUTDEVICE_XINPUT (1)

#undef RETRO_INPUTDEVICE_RAWINPUT
#define RETRO_INPUTDEVICE_RAWINPUT (1)

#elif defined(RSDK_USE_OGL)
#undef RETRO_RENDERDEVICE_GLFW
#define RETRO_RENDERDEVICE_GLFW (1)

#undef RETRO_INPUTDEVICE_GLFW
#define RETRO_INPUTDEVICE_GLFW (1)
#else
#error One of RSDK_USE_DX9, RSDK_USE_DX11, RSDK_USE_SDL2, or RSDK_USE_OGL must be defined.
#endif

#if !defined(_MINGW) && !defined(RSDK_USE_SDL2)
#undef RETRO_AUDIODEVICE_XAUDIO
#define RETRO_AUDIODEVICE_XAUDIO (1)
#else
#undef RETRO_AUDIODEVICE_SDL2
#define RETRO_AUDIODEVICE_SDL2 (1)
#endif

#elif RETRO_PLATFORM == RETRO_XB1

#undef RETRO_RENDERDEVICE_DIRECTX11
#define RETRO_RENDERDEVICE_DIRECTX11 (1)

#undef RETRO_AUDIODEVICE_XAUDIO
#define RETRO_AUDIODEVICE_XAUDIO (1)

#undef RETRO_INPUTDEVICE_XINPUT
#define RETRO_INPUTDEVICE_XINPUT (1)

#elif RETRO_PLATFORM == RETRO_LINUX

#ifdef RSDK_USE_SDL2
#undef RETRO_RENDERDEVICE_SDL2
#define RETRO_RENDERDEVICE_SDL2 (1)
#undef RETRO_AUDIODEVICE_SDL2
#define RETRO_AUDIODEVICE_SDL2 (1)
#undef RETRO_INPUTDEVICE_SDL2
#define RETRO_INPUTDEVICE_SDL2 (1)

#elif defined(RSDK_USE_OGL)
#undef RETRO_RENDERDEVICE_GLFW
#define RETRO_RENDERDEVICE_GLFW (1)
#undef RETRO_INPUTDEVICE_GLFW
#define RETRO_INPUTDEVICE_GLFW (1)
#undef RETRO_AUDIODEVICE_SDL2
#define RETRO_AUDIODEVICE_SDL2 (1)

#else
#error RSDK_USE_SDL2 or RSDK_USE_OGL must be defined.
#endif //! RSDK_USE_SDL2

#elif RETRO_PLATFORM == RETRO_SWITCH
// #undef RETRO_USERCORE_ID
// #define RETRO_USERCORE_ID (4)
// #define RETRO_USERCORE_ID (4 | 0x80)

#ifdef RSDK_USE_SDL2
#undef RETRO_RENDERDEVICE_SDL2
#define RETRO_RENDERDEVICE_SDL2 (1)
#undef RETRO_AUDIODEVICE_SDL2
#define RETRO_AUDIODEVICE_SDL2 (1)
#undef RETRO_INPUTDEVICE_SDL2
#define RETRO_INPUTDEVICE_SDL2 (1)

#elif defined(RSDK_USE_OGL)
#undef RETRO_RENDERDEVICE_EGL
#define RETRO_RENDERDEVICE_EGL (1)
#undef RETRO_INPUTDEVICE_NX
#define RETRO_INPUTDEVICE_NX (1)
#undef RETRO_AUDIODEVICE_SDL2
#define RETRO_AUDIODEVICE_SDL2 (1)

#elif defined(RSDK_USE_NX)
#undef RETRO_RENDERDEVICE_NX
#define RETRO_RENDERDEVICE_NX (1)
#undef RETRO_INPUTDEVICE_NX
#define RETRO_INPUTDEVICE_NX (1)
#undef RETRO_AUDIODEVICE_NX
#define RETRO_AUDIODEVICE_NX (1)
#else

#error RSDK_USE_NX, RSDK_USE_SDL2, or RSDK_USE_OGL must be defined.
#endif //! RSDK_USE_SDL2

#undef RETRO_INPUTDEVICE_KEYBOARD
#define RETRO_INPUTDEVICE_KEYBOARD (0)
#undef RETRO_USING_MOUSE

#elif RETRO_PLATFORM == RETRO_ANDROID

#if defined RSDK_USE_OGL
#undef RETRO_RENDERDEVICE_EGL
#define RETRO_RENDERDEVICE_EGL (1)
#undef RETRO_INPUTDEVICE_PDBOAT
#define RETRO_INPUTDEVICE_PDBOAT (1)
#undef RETRO_AUDIODEVICE_OBOE
#define RETRO_AUDIODEVICE_OBOE (1)
#else
#error RSDK_USE_OGL must be defined.
#endif

#elif RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_iOS

#undef RETRO_RENDERDEVICE_SDL2
#define RETRO_RENDERDEVICE_SDL2 (1)

#undef RETRO_AUDIODEVICE_SDL2
#define RETRO_AUDIODEVICE_SDL2 (1)

#undef RETRO_INPUTDEVICE_SDL2
#define RETRO_INPUTDEVICE_SDL2 (1)

#endif

#if RETRO_PLATFORM == RETRO_WIN || RETRO_PLATFORM == RETRO_UWP

#if RETRO_AUDIODEVICE_XAUDIO
#include <XAudio2.h>
#endif

#if RETRO_INPUTDEVICE_XINPUT
#include <Xinput.h>
#endif

// All windows systems need windows API for LoadLibrary()
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if RETRO_RENDERDEVICE_DIRECTX9 || RETRO_RENDERDEVICE_DIRECTX11
#include <timeapi.h>
#include <commctrl.h>
#include <dbt.h>

#include <string>

#if RETRO_RENDERDEVICE_DIRECTX9
#include <d3d9.h>
#elif RETRO_RENDERDEVICE_DIRECTX11
#include <d3d11_1.h>
#endif

#undef LoadImage
#elif RETRO_RENDERDEVICE_GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#endif

#endif // ! RETRO_WIN

#if RETRO_PLATFORM == RETRO_OSX

#include "cocoaHelpers.hpp"
#elif RETRO_PLATFORM == RETRO_iOS

#include "cocoaHelpers.hpp"
#elif RETRO_PLATFORM == RETRO_LINUX || RETRO_PLATFORM == RETRO_SWITCH

#if RETRO_RENDERDEVICE_GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#elif RETRO_RENDERDEVICE_EGL
#include <glad/glad.h>
#include <EGL/egl.h> // EGL library
#include <EGL/eglext.h> // EGL extensions
#endif

#if RETRO_PLATFORM == RETRO_SWITCH
#define PrintConsole _PrintConsole
#include <switch.h>
#undef PrintConsole
#endif

#elif RETRO_PLATFORM == RETRO_ANDROID

#if RETRO_RENDERDEVICE_EGL
#include <EGL/egl.h> // EGL library
#include <GLES2/gl2.h>
#endif

#include <androidHelpers.hpp>

#undef RETRO_USING_MOUSE
#endif

#if RETRO_RENDERDEVICE_SDL2 || RETRO_INPUTDEVICE_SDL2 || RETRO_AUDIODEVICE_SDL2
#if RETRO_PLATFORM == RETRO_OSX
// yeah, I dunno how you're meant to do the below with macOS frameworks so leaving this as is for rn :P
#include <SDL2/SDL.h>
#else
// This is the way of including SDL that is recommended by the devs themselves:
// https://wiki.libsdl.org/FAQDevelopment#do_i_include_sdl.h_or_sdlsdl.h
#include "SDL.h"
#endif
#endif

#include <theora/theoradec.h>

// ============================
// ENGINE INCLUDES
// ============================

#include "RSDK/Storage/Storage.hpp"
#include "RSDK/Core/Math.hpp"
#include "RSDK/Storage/Text.hpp"
#include "RSDK/Core/Reader.hpp"
#include "RSDK/Graphics/Animation.hpp"
#include "RSDK/Audio/Audio.hpp"
#include "RSDK/Input/Input.hpp"
#include "RSDK/Scene/Object.hpp"
#include "RSDK/Graphics/Palette.hpp"
#include "RSDK/Graphics/Drawing.hpp"
#include "RSDK/Graphics/Scene3D.hpp"
#include "RSDK/Scene/Scene.hpp"
#include "RSDK/Scene/Collision.hpp"
#include "RSDK/Graphics/Sprite.hpp"
#include "RSDK/Graphics/Video.hpp"
#include "RSDK/Dev/Debug.hpp"
#include "RSDK/User/Core/UserCore.hpp"
#include "RSDK/User/Core/UserAchievements.hpp"
#include "RSDK/User/Core/UserLeaderboards.hpp"
#include "RSDK/User/Core/UserStats.hpp"
#include "RSDK/User/Core/UserPresence.hpp"
#include "RSDK/User/Core/UserStorage.hpp"
#include "RSDK/Core/Link.hpp"
#if RETRO_USE_MOD_LOADER
#include "RSDK/Core/ModAPI.hpp"
#endif

// Default Objects
#include "RSDK/Scene/Objects/DefaultObject.hpp"
#if RETRO_REV02
#include "RSDK/Scene/Objects/DevOutput.hpp"
#endif

#if !RETRO_REV0U
#define ENGINE_VERSION (5)
#else
#define ENGINE_VERSION (engine.version)
#endif

namespace RSDK
{

struct RetroEngine {
    RetroEngine() {}

#if RETRO_STANDALONE
    bool32 useExternalCode = true;
#else
    bool32 useExternalCode = false;
#endif

    bool32 devMenu        = false;
    bool32 consoleEnabled = false;

    bool32 confirmFlip = false; // swaps A/B, used for nintendo and etc controllers
    bool32 XYFlip      = false; // swaps X/Y, used for nintendo and etc controllers

    uint8 focusState = 0;
    uint8 inFocus    = 0;
#if !RETRO_USE_ORIGINAL_CODE
    uint8 focusPausedChannel[CHANNEL_COUNT];
#endif

    bool32 initialized = false;
    bool32 hardPause   = false;

#if RETRO_REV0U
    uint8 version = 5; // determines what RSDK version to use, default to RSDKv5 since thats the "core" version

    const char *gamePlatform;
    const char *gameRenderType;
    const char *gameHapticSetting;

#if !RETRO_USE_ORIGINAL_CODE
    int32 gameReleaseID     = 0;
    const char *releaseType = "USE_STANDALONE";
#endif
#endif

    int32 storedShaderID      = SHADER_NONE;
    int32 storedState         = ENGINESTATE_LOAD;
    int32 gameSpeed           = 1;
    int32 fastForwardSpeed    = 8;
    bool32 frameStep          = false;
    bool32 showPaletteOverlay = false;
    uint8 showUpdateRanges    = 0;
    uint8 showEntityInfo      = 0;
    bool32 drawGroupVisible[DRAWGROUP_COUNT];

    // Image/Video support
    double displayTime       = 0.0;
    double videoStartDelay   = 0.0;
    double imageFadeSpeed    = 0.0;
    bool32 (*skipCallback)() = NULL;

    bool32 streamsEnabled = true;
    float streamVolume    = 1.0f;
    float soundFXVolume   = 1.0f;
};

extern RetroEngine engine;

#if RETRO_REV02
typedef void (*LogicLinkHandle)(GameInfo *info);
#else
typedef void (*LogicLinkHandle)(GameInfo info);
#endif

extern LogicLinkHandle linkGameLogic;

// ============================
// CORE ENGINE FUNCTIONS
// ============================

int32 RunRetroEngine(int32 argc, char *argv[]);
void ProcessEngine();

void ParseArguments(int32 argc, char *argv[]);

void InitEngine();
void StartGameObjects();

#if RETRO_USE_MOD_LOADER
void LoadXMLObjects();
void LoadXMLSoundFX();
int32 LoadXMLStages(int32 mode, int32 gcListCount, int32 gcStageCount);
#endif

void LoadGameConfig();
void InitGameLink();

void ProcessDebugCommands();

inline void SetEngineState(uint8 state)
{
    bool32 stepOver = (sceneInfo.state & ENGINESTATE_STEPOVER) == ENGINESTATE_STEPOVER;
    sceneInfo.state = state;
    if (stepOver)
        sceneInfo.state |= ENGINESTATE_STEPOVER;
}

#if RETRO_REV0U
inline void SetGameFinished() { sceneInfo.state = ENGINESTATE_GAME_FINISHED; }
#endif

extern int32 *globalVarsPtr;

#if RETRO_REV0U
extern void (*globalVarsInitCB)(void *globals);

inline void RegisterGlobalVariables(void **globals, int32 size, void (*initCB)(void *globals))
{
    AllocateStorage(globals, size, DATASET_STG, true);
    globalVarsPtr    = (int32 *)*globals;
    globalVarsInitCB = initCB;
}
#else
inline void RegisterGlobalVariables(void **globals, int32 size)
{
    AllocateStorage(globals, size, DATASET_STG, true);
    globalVarsPtr = (int32 *)*globals;
}
#endif

// Some misc API stuff that needs a home

// Used to Init API stuff that should be done regardless of Render/Audio/Input device APIs
void InitCoreAPI();
void ReleaseCoreAPI();

void InitConsole();
void ReleaseConsole();

void SendQuitMsg();

#if RETRO_REV0U
#include "Legacy/RetroEngineLegacy.hpp"
#endif

} // namespace RSDK

#include "Link.hpp"

#endif //! RETROENGINE_H
