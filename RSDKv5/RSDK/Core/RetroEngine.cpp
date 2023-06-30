#include "RSDK/Core/RetroEngine.hpp"

using namespace RSDK;

#if RETRO_REV0U
#include "Legacy/RetroEngineLegacy.cpp"
#endif

LogicLinkHandle RSDK::linkGameLogic = NULL;

Link::Handle gameLogicHandle = NULL;

#if RETRO_PLATFORM == RETRO_ANDROID
#include <jni.h>
#include <unistd.h>
#endif

int32 *RSDK::globalVarsPtr = NULL;
#if RETRO_REV0U
void (*RSDK::globalVarsInitCB)(void *globals) = NULL;
#endif

RetroEngine RSDK::engine = RetroEngine();

int32 RSDK::RunRetroEngine(int32 argc, char *argv[])
{
    ParseArguments(argc, argv);

    if (engine.consoleEnabled)
        InitConsole();
    RenderDevice::isRunning = false;

    if (InitStorage()) {
        SKU::InitUserCore();
        LoadSettingsINI();

#if RETRO_USE_MOD_LOADER
        // do it early so we can render funny little loading bar for mods
        int32 shader = videoSettings.shaderID;
        strcpy(gameVerInfo.gameTitle, "RSDK" ENGINE_V_NAME);
        if (RenderDevice::Init()) {
            RenderDevice::isRunning   = true;
            currentScreen             = &screens[0];
            videoSettings.screenCount = 1;
        }
        else {
            // No render device, throw a "QUIT" msg onto the message loop and call it a day :)
            SendQuitMsg();
        }
#if RETRO_PLATFORM == RETRO_ANDROID
        // wait until we have a window
        while (!RenderDevice::window) {
            RenderDevice::ProcessEvents();
        }
#endif

#if RETRO_REV0U
        engine.version = 0;
        InitModAPI(true); // check for versions
        engine.version = 5;
#endif
#endif

#if RETRO_REV0U
        DetectEngineVersion();
#endif

        // By Default we use the dummy system so this'll never be false
        // its used in cases like steam where it gives the "Steam must be running to play this game" message and closes
#if RETRO_REV02
        if (!SKU::userCore->CheckAPIInitialized()) {
#else
        if (false) { // it's more hardcoded in rev01, so lets pretend it's here
#endif
            // popup a message box saying the API failed to validate or something
            // on steam this is the "steam must be running to play this game" message
            return 0;
        }

        InitEngine();
#if RETRO_USE_MOD_LOADER
        // we confirmed the game actually is valid & running, lets start some callbacks
        RunModCallbacks(MODCB_ONGAMESTARTUP, NULL);
        videoSettings.shaderID = shader;
        RenderDevice::InitShaders();
        RenderDevice::SetWindowTitle();
        RenderDevice::lastShaderID = -1;
#else
        if (RenderDevice::Init()) {
            RenderDevice::isRunning = true;
        }
        else {
            // No render device, throw a "QUIT" msg onto the message loop and call it a day :)
            SendQuitMsg();
        }
#endif
    }

    RenderDevice::InitFPSCap();

    while (RenderDevice::isRunning) {
        RenderDevice::ProcessEvents();

        if (!RenderDevice::isRunning)
            break;

        if (RenderDevice::CheckFPSCap()) {
            RenderDevice::UpdateFPSCap();

            AudioDevice::FrameInit();

#if RETRO_REV02
            SKU::userCore->FrameInit();

            if (SKU::userCore->CheckEnginePause())
                continue;

                // Focus Checks
#if !RETRO_USE_ORIGINAL_CODE
            if (customSettings.disableFocusPause)
                engine.focusState = 0;
            else if (SKU::userCore->CheckFocusLost()) {
#else
            if (SKU::userCore->CheckFocusLost()) {
#endif
                if (!(engine.focusState & 1)) {
                    engine.focusState = 1;

#if !RETRO_USE_ORIGINAL_CODE
                    for (int32 c = 0; c < CHANNEL_COUNT; ++c) {
                        engine.focusPausedChannel[c] = false;
                        if (!(channels[c].state & CHANNEL_PAUSED)) {
                            PauseChannel(c);
                            engine.focusPausedChannel[c] = true;
                        }
                    }
#else
                    PauseSound();
#endif
                }
            }
            else if (engine.focusState) {
                engine.focusState = 0;

#if !RETRO_USE_ORIGINAL_CODE
                for (int32 c = 0; c < CHANNEL_COUNT; ++c) {
                    if (engine.focusPausedChannel[c])
                        ResumeChannel(c);
                    engine.focusPausedChannel[c] = false;
                }
#else
                ResumeSound();
#endif
            }
#endif

            if (!engine.initialized || (engine.focusState & 1)) {
                if (videoSettings.windowState != WINDOWSTATE_ACTIVE)
                    continue;
            }
            else {
                if (!engine.hardPause) {
                    // common stuff
                    foreachStackPtr = foreachStackList;
#if !RETRO_USE_ORIGINAL_CODE
                    debugHitboxCount = 0;
#endif

#if RETRO_USE_MOD_LOADER
#if RETRO_REV0U
                    if (((engine.version == 5 && sceneInfo.state != ENGINESTATE_DEVMENU)
                         || (engine.version != 5 && RSDK::Legacy::gameMode != RSDK::Legacy::ENGINE_DEVMENU))
                        && devMenu.modsChanged) {
                        engine.version = 0;
#else
                    if (sceneInfo.state != ENGINESTATE_DEVMENU && devMenu.modsChanged) {
#endif
                        devMenu.modsChanged = false;
                        SaveMods();
                        RefreshModFolders(true);
                        LoadModSettings();
                        for (int32 c = 0; c < CHANNEL_COUNT; ++c) StopChannel(c);
#if RETRO_REV02
                        forceHardReset = true;
#endif

#if RETRO_REV0U
                        int32 preVersion = engine.version;

                        DetectEngineVersion();
                        if (!engine.version)
                            engine.version = preVersion;

                        SceneInfo pre      = sceneInfo;
                        int32 preGameMode  = RSDK::Legacy::gameMode;
                        int32 preStageMode = RSDK::Legacy::stageMode;

                        // Clear some stuff
                        sceneInfo.listData     = NULL;
                        sceneInfo.listCategory = NULL;

                        globalVarsPtr    = NULL;
                        globalVarsInitCB = NULL;

                        dataStorage[DATASET_STG].entryCount  = 0;
                        dataStorage[DATASET_STG].usedStorage = 0;
                        dataStorage[DATASET_SFX].entryCount  = 0;
                        dataStorage[DATASET_SFX].usedStorage = 0;

                        for (int32 o = 0; o < objectClassCount; ++o) {
                            if (objectClassList[o].staticVars && *objectClassList[o].staticVars)
                                (*objectClassList[o].staticVars) = NULL;
                        }

                        InitEngine();

                        switch (engine.version) {
                            default:
                            case 5:
                                sceneInfo.classCount = pre.classCount;
                                if (pre.state == ENGINESTATE_LOAD) {
                                    sceneInfo.activeCategory = pre.activeCategory;
                                    sceneInfo.listPos        = pre.listPos;
                                }
                                break;
                            case 4:
                            case 3:
                                if (preGameMode == RSDK::Legacy::ENGINE_MAINGAME && preStageMode == RSDK::Legacy::STAGEMODE_LOAD) {
                                    sceneInfo.activeCategory = pre.activeCategory;
                                    sceneInfo.listPos        = pre.listPos;
                                }
                                break;
                        }

#else
                        SceneInfo pre = sceneInfo;
                        InitEngine();
                        sceneInfo.classCount = pre.classCount;
                        if (pre.state == ENGINESTATE_LOAD) {
                            sceneInfo.activeCategory = pre.activeCategory;
                            sceneInfo.listPos        = pre.listPos;
                        }
#endif
                        RenderDevice::SetWindowTitle();
                        sceneInfo.state = ENGINESTATE_LOAD;
                    }
#endif

                    // update device states and other stuff
                    ProcessInputDevices();

                    if (engine.devMenu)
                        ProcessDebugCommands();

#if RETRO_REV0U
                    switch (engine.version) {
                        default:
                        case 5: ProcessEngine(); break;
                        case 4: Legacy::v4::ProcessEngine(); break;
                        case 3: Legacy::v3::ProcessEngine(); break;
                    }
#else
                    ProcessEngine();
#endif
                }

#if RETRO_PLATFORM == RETRO_ANDROID
                HideLoadingIcon(); // best spot to do it
#endif

                if (videoSettings.windowState != WINDOWSTATE_ACTIVE)
                    continue;

#if !RETRO_USE_ORIGINAL_CODE
                for (int32 t = 0; t < touchInfo.count; ++t) {
                    if (touchInfo.down[t]) {
                        int32 tx = (int32)(touchInfo.x[t] * screens->size.x);
                        int32 ty = (int32)(touchInfo.y[t] * screens->size.y);

                        if (tx <= 32 && ty <= 32) {
                            if (engine.devMenu) {
#if RETRO_REV0U
                                if (sceneInfo.state != ENGINESTATE_DEVMENU && RSDK::Legacy::gameMode != RSDK::Legacy::ENGINE_DEVMENU)
#else
                                if (sceneInfo.state != ENGINESTATE_DEVMENU)
#endif
                                    OpenDevMenu();
                            }
                        }
                    }
                }
#endif
                if (engine.inFocus == 1) {
                    // Uncomment this code to add the build number to dev menu
                    // overrides the game subtitle, used in switch dev menu
                    if (currentScreen && sceneInfo.state == ENGINESTATE_DEVMENU) {
                        // Switch 1.00 build # is 17051, 1.04 is 18403
                        // char buffer[0x40];
                        // sprintf(buffer, "Build #%d", 18403);
                        // DrawRectangle(currentScreen->center.x - 128, currentScreen->center.y - 48, 256, 8, 0x008000, 0xFF, INK_NONE, true);
                        // DrawDevString(buffer, currentScreen->center.x, currentScreen->center.y - 48, 1, 0xF0F0F0);
                    }

                    RenderDevice::CopyFrameBuffer();
                }
            }

            if ((engine.focusState & 1) || engine.inFocus == 1)
                RenderDevice::ProcessDimming();

            RenderDevice::FlipScreen();
        }
    }

    // Shutdown

    ReleaseInputDevices();
    AudioDevice::Release();
    RenderDevice::Release(false);
    SaveSettingsINI(false);
    SKU::ReleaseUserCore();
    ReleaseStorage();
#if RETRO_USE_MOD_LOADER
    UnloadMods();
#endif

    Link::Close(gameLogicHandle);
    gameLogicHandle = NULL;

    if (engine.consoleEnabled)
        ReleaseConsole();

    return 0;
}

void RSDK::ProcessEngine()
{
    switch (sceneInfo.state) {
        default: break;

        case ENGINESTATE_LOAD:
            if (!sceneInfo.listData) {
                sceneInfo.state = ENGINESTATE_NONE;
            }
            else {
#if RETRO_USE_MOD_LOADER
                if (devMenu.modsChanged)
                    RefreshModFolders();
#endif
                LoadSceneFolder();
                LoadSceneAssets();
                InitObjects();

#if RETRO_REV02
#if !RETRO_USE_ORIGINAL_CODE
                AddViewableVariable("Show Hitboxes", &showHitboxes, VIEWVAR_BOOL, false, true);
                AddViewableVariable("Show Palettes", &engine.showPaletteOverlay, VIEWVAR_BOOL, false, true);
                AddViewableVariable("Show Obj Range", &engine.showUpdateRanges, VIEWVAR_UINT8, 0, 2);
                AddViewableVariable("Show Obj Info", &engine.showEntityInfo, VIEWVAR_UINT8, 0, 2);
#endif
                SKU::userCore->StageLoad();
                for (int32 v = 0; v < DRAWGROUP_COUNT; ++v)
                    AddViewableVariable(drawGroupNames[v], &engine.drawGroupVisible[v], VIEWVAR_BOOL, false, true);
#endif

                // dim after 5 mins
                videoSettings.dimLimit = (5 * 60) * videoSettings.refreshRate;
                ProcessInput();
                ProcessObjects();
#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
                SKU::LoadAchievementAssets();
#endif
            }

            break;

        case ENGINESTATE_REGULAR:
            ProcessInput();
            ProcessSceneTimer();
            ProcessObjects();
            ProcessParallaxAutoScroll();

            for (int32 i = 1; i < engine.gameSpeed; ++i) {
                if (sceneInfo.state != ENGINESTATE_REGULAR)
                    break;

                ProcessSceneTimer();
                ProcessObjects();
                ProcessParallaxAutoScroll();
            }

#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
            SKU::ProcessAchievements();
#endif
            ProcessObjectDrawLists();

            break;

        case ENGINESTATE_PAUSED:
            ProcessInput();
            ProcessPausedObjects();

            for (int32 i = 1; i < engine.gameSpeed; ++i) {
                if (sceneInfo.state != ENGINESTATE_PAUSED)
                    break;

                ProcessPausedObjects();
            }

#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
            SKU::ProcessAchievements();
#endif
            ProcessObjectDrawLists();
            break;

        case ENGINESTATE_FROZEN:
            ProcessInput();
            ProcessFrozenObjects();

            for (int32 i = 1; i < engine.gameSpeed; ++i) {
                if (sceneInfo.state != ENGINESTATE_FROZEN)
                    break;

                ProcessFrozenObjects();
            }

#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
            SKU::ProcessAchievements();
#endif
            ProcessObjectDrawLists();
            break;

        case ENGINESTATE_LOAD | ENGINESTATE_STEPOVER:
#if RETRO_USE_MOD_LOADER
            if (devMenu.modsChanged)
                RefreshModFolders();
#endif
            LoadSceneFolder();
            LoadSceneAssets();
            InitObjects();

#if RETRO_REV02
#if !RETRO_USE_ORIGINAL_CODE
            AddViewableVariable("Show Hitboxes", &showHitboxes, VIEWVAR_BOOL, false, true);
            AddViewableVariable("Show Palettes", &engine.showPaletteOverlay, VIEWVAR_BOOL, false, true);
            AddViewableVariable("Show Obj Range", &engine.showUpdateRanges, VIEWVAR_UINT8, 0, 2);
            AddViewableVariable("Show Obj Info", &engine.showEntityInfo, VIEWVAR_UINT8, 0, 2);
#endif
            SKU::userCore->StageLoad();
            for (int32 v = 0; v < DRAWGROUP_COUNT; ++v)
                AddViewableVariable(drawGroupNames[v], &engine.drawGroupVisible[v], VIEWVAR_BOOL, false, true);
#endif

            ProcessInput();
            ProcessObjects();
            sceneInfo.state = ENGINESTATE_REGULAR | ENGINESTATE_STEPOVER;
#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
            SKU::LoadAchievementAssets();
#endif
            break;

        case ENGINESTATE_REGULAR | ENGINESTATE_STEPOVER:
            ProcessInput();

            if (engine.frameStep) {
                ProcessSceneTimer();
                ProcessObjects();
                ProcessParallaxAutoScroll();
#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
                SKU::ProcessAchievements();
#endif
                ProcessObjectDrawLists();
                engine.frameStep = false;
            }
            break;

        case ENGINESTATE_PAUSED | ENGINESTATE_STEPOVER:
            ProcessInput();

            if (engine.frameStep) {
                ProcessPausedObjects();
#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
                SKU::ProcessAchievements();
#endif
                ProcessObjectDrawLists();
                engine.frameStep = false;
            }
            break;

        case ENGINESTATE_FROZEN | ENGINESTATE_STEPOVER:
            ProcessInput();

            if (engine.frameStep) {
                ProcessFrozenObjects();
#if RETRO_VER_EGS || RETRO_USE_DUMMY_ACHIEVEMENTS
                SKU::ProcessAchievements();
#endif
                ProcessObjectDrawLists();
                engine.frameStep = false;
            }
            break;

        case ENGINESTATE_DEVMENU:
            ProcessInput();
            currentScreen = &screens[0];

            if (devMenu.state)
                devMenu.state();
            break;

        case ENGINESTATE_VIDEOPLAYBACK:
            ProcessInput();
            ProcessVideo();
            break;

        case ENGINESTATE_SHOWIMAGE:
            ProcessInput();

            if (engine.imageFadeSpeed <= 0.0 || videoSettings.dimMax >= 1.0) {
                if (engine.displayTime <= 0.0) {
                    videoSettings.dimMax += engine.imageFadeSpeed;
                    if (videoSettings.dimMax <= 0.0) {
                        videoSettings.shaderID    = engine.storedShaderID;
                        videoSettings.screenCount = 1;
                        sceneInfo.state           = engine.storedState;
                        videoSettings.dimMax      = 1.0;
                    }
                }
                else {
                    engine.displayTime -= (1.0 / 60.0); // deltaTime frame-step;
#if RETRO_USE_MOD_LOADER
                    RunModCallbacks(MODCB_ONVIDEOSKIPCB, (void *)engine.skipCallback);
#endif
                    if (engine.skipCallback && engine.skipCallback()) {
                        engine.displayTime = 0.0;
                    }
                }
            }
            else {
                videoSettings.dimMax += engine.imageFadeSpeed;
                if (videoSettings.dimMax >= 1.0) {
                    engine.imageFadeSpeed = -engine.imageFadeSpeed;
                    videoSettings.dimMax  = 1.0;
                }
            }
            break;

#if RETRO_REV02
        case ENGINESTATE_ERRORMSG: {
            ProcessInput();

            if (controller[0].keyStart.down)
                sceneInfo.state = engine.storedState;

            currentScreen = &screens[0];
            int32 yOff    = DevOutput_GetStringYSize(outputString);
            DrawRectangle(0, currentScreen->center.y - (yOff >> 1), currentScreen->size.x, yOff, 128, 255, INK_NONE, true);
            DrawDevString(outputString, 8, currentScreen->center.y - (yOff >> 1) + 8, 0, 0xF0F0F0);
            break;
        }
        case ENGINESTATE_ERRORMSG_FATAL: {
            ProcessInput();

            if (controller[0].keyStart.down)
                RenderDevice::isRunning = false;

            currentScreen = &screens[0];
            int32 yOff    = DevOutput_GetStringYSize(outputString);
            DrawRectangle(0, currentScreen->center.y - (yOff >> 1), currentScreen->size.x, yOff, 0xF00000, 255, INK_NONE, true);
            DrawDevString(outputString, 8, currentScreen->center.y - (yOff >> 1) + 8, 0, 0xF0F0F0);
            break;
        }
#endif
    }
}

void RSDK::ParseArguments(int32 argc, char *argv[])
{
    memset(currentSceneFolder, 0, sizeof(currentSceneFolder));
    memset(currentSceneID, 0, sizeof(currentSceneID));
#if RETRO_REV02
    sceneInfo.filter = 0;
#endif

    for (int32 a = 0; a < argc; ++a) {
        const char *find = "";

        find = strstr(argv[a], "stage=");
        if (find) {
            int32 b = 0;
            int32 c = 6;
            while (find[c] && find[c] != ';') currentSceneFolder[b++] = find[c++];
            currentSceneFolder[b] = 0;
        }

        find = strstr(argv[a], "scene=");
        if (find) {
            int32 b = 0;
            int32 c = 6;
            while (find[c] && find[c] != ';') currentSceneID[b++] = find[c++];
            currentSceneID[b] = 0;
        }

#if RETRO_REV02
        find = strstr(argv[a], "filter=");
        if (find) {
            char buf[0x10];

            int32 b = 0;
            int32 c = 7;
            while (argv[a][c] && find[c] != ';') buf[b++] = find[c++];
            buf[b]           = 0;
            sceneInfo.filter = atoi(buf);
        }
#endif

        find = strstr(argv[a], "console=true");
        if (find) {
            engine.consoleEnabled = true;
            engine.devMenu        = true;
        }
    }
}

void RSDK::InitEngine()
{
#if RETRO_PLATFORM == RETRO_ANDROID
    ShowLoadingIcon(); // if valid
#endif

#if RETRO_REV0U
    switch (engine.version) {
        case 5:
#endif
            StartGameObjects();
#if RETRO_REV0U
            break;

        case 4:
            devMenu.state = DevMenu_MainMenu;
            SetupFunctionTables();

            Legacy::CalculateTrigAnglesM7();

            engine.gamePlatform      = (RETRO_DEVICETYPE == RETRO_STANDARD ? "STANDARD" : "MOBILE");
            engine.gameRenderType    = "SW_RENDERING";
            engine.gameHapticSetting = "NO_F_FEEDBACK";
#if !RETRO_USE_ORIGINAL_CODE
            engine.releaseType = (engine.gameReleaseID ? "USE_ORIGINS" : "USE_STANDALONE");

            Legacy::deviceType = RETRO_DEVICETYPE;
            if (SKU::curSKU.language <= LANGUAGE_JP)
                Legacy::language = SKU::curSKU.language;
            else {
                switch (SKU::curSKU.language) {
                    default: Legacy::language = Legacy::LEGACY_LANGUAGE_EN; break;
                    case LANGUAGE_KO: Legacy::language = Legacy::LEGACY_LANGUAGE_KO; break;
                    case LANGUAGE_SC: Legacy::language = Legacy::LEGACY_LANGUAGE_ZS; break;
                    case LANGUAGE_TC: Legacy::language = Legacy::LEGACY_LANGUAGE_ZH; break;
                }
            }
#endif

            Legacy::v4::LoadGameConfig("Data/Game/GameConfig.bin");
            if (!useDataPack)
                sprintf_s(gameVerInfo.gameTitle, sizeof(gameVerInfo.gameTitle), "%s (Data Folder)", gameVerInfo.gameTitle);
            strcpy(gameVerInfo.version, "Legacy v4 Mode");

            RSDK::GenerateBlendLookupTable();
            Legacy::GenerateBlendLookupTable();
            Legacy::v4::InitFirstStage();
            Legacy::ResetCurrentStageFolder();
            Legacy::v4::ClearScriptData();
            break;

        case 3:
            devMenu.state = DevMenu_MainMenu;
            SetupFunctionTables();

            Legacy::CalculateTrigAnglesM7();

            engine.gamePlatform      = (RETRO_DEVICETYPE == RETRO_STANDARD ? "Standard" : "Mobile");
            engine.gameRenderType    = "SW_Rendering";
            engine.gameHapticSetting = "No_Haptics";
#if !RETRO_USE_ORIGINAL_CODE
            engine.releaseType = (engine.gameReleaseID ? "Use_Origins" : "Use_Standalone");

            Legacy::deviceType = RETRO_DEVICETYPE;
            switch (RETRO_PLATFORM) {
                default:
                case RETRO_WIN: Legacy::gamePlatformID = Legacy::LEGACY_RETRO_WIN; break;
                case RETRO_OSX: Legacy::gamePlatformID = Legacy::LEGACY_RETRO_OSX; break;
                case RETRO_iOS: Legacy::gamePlatformID = Legacy::LEGACY_RETRO_iOS; break;
                case RETRO_XB1: Legacy::gamePlatformID = Legacy::LEGACY_RETRO_XBOX_360; break;
                case RETRO_PS4: Legacy::gamePlatformID = Legacy::LEGACY_RETRO_PS3; break;
                case RETRO_ANDROID: Legacy::gamePlatformID = Legacy::LEGACY_RETRO_ANDROID; break;
            }

            if (SKU::curSKU.language <= LANGUAGE_JP)
                Legacy::language = SKU::curSKU.language;
            else
                Legacy::language = Legacy::LEGACY_LANGUAGE_EN;
#endif

            Legacy::v3::LoadGameConfig("Data/Game/GameConfig.bin");
            if (!useDataPack)
                sprintf_s(gameVerInfo.gameTitle, sizeof(gameVerInfo.gameTitle), "%s (Data Folder)", gameVerInfo.gameTitle);
            strcpy(gameVerInfo.version, "Legacy v3 Mode");

            RSDK::GenerateBlendLookupTable();
            Legacy::GenerateBlendLookupTable();
            Legacy::v3::InitFirstStage();
            Legacy::ResetCurrentStageFolder();
            Legacy::v3::ClearScriptData();
            break;
    }
#endif
    engine.initialized = true;
    engine.hardPause   = false;
#if RETRO_PLATFORM == RETRO_ANDROID
    SetLoadingIcon();
#endif
}

void RSDK::StartGameObjects()
{
    memset(&objectClassList, 0, sizeof(objectClassList));

    sceneInfo.classCount     = 0;
    sceneInfo.activeCategory = 0;
    sceneInfo.listPos        = 0;
    sceneInfo.state          = ENGINESTATE_LOAD;
    sceneInfo.inEditor       = false;
    sceneInfo.debugMode      = engine.devMenu;
    devMenu.state            = DevMenu_MainMenu;

    for (int32 l = 0; l < DRAWGROUP_COUNT; ++l) engine.drawGroupVisible[l] = true;

    SetupFunctionTables();
    InitGameLink();
    LoadGameConfig();
}

#if RETRO_USE_MOD_LOADER

void RSDK::LoadGameXML()
{
    FileInfo info;
    SortMods();
    for (int32 m = 0; m < modList.size(); ++m) {
        if (!modList[m].active)
            break;
        SetActiveMod(m);
        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;
            CloseFile(&info);

            doc->Parse(xmlData);
            const tinyxml2::XMLElement *gameElement = doc->FirstChildElement("game"); // gameElement is nullptr if parse failure

            if (gameElement) {
                LoadXMLObjects(gameElement);
                LoadXMLSoundFX(gameElement);
                LoadXMLStages(gameElement);
            }
            else {
                PrintLog(PRINT_NORMAL, "[MOD] Failed to parse Game.xml file for mod %s", modList[m].id.c_str());
            }

            delete[] xmlData;
            delete doc;
        }
    }
    SetActiveMod(-1);
}

void RSDK::LoadXMLObjects(const tinyxml2::XMLElement *gameElement)
{
    const tinyxml2::XMLElement *objectsElement = gameElement->FirstChildElement("objects");
    if (objectsElement) {
        for (const tinyxml2::XMLElement *objElement = objectsElement->FirstChildElement("object"); objElement;
             objElement                             = objElement->NextSiblingElement("object")) {
            const tinyxml2::XMLAttribute *nameAttr = objElement->FindAttribute("name");
            const char *objName                    = "unknownObject";
            if (nameAttr)
                objName = nameAttr->Value();

            RETRO_HASH_MD5(hash);
            GEN_HASH_MD5(objName, hash);
            globalObjectIDs[globalObjectCount] = 0;
            for (int32 objID = 0; objID < objectClassCount; ++objID) {
                if (HASH_MATCH_MD5(hash, objectClassList[objID].hash)) {
                    globalObjectIDs[globalObjectCount] = objID;
                    globalObjectCount++;
                }
            }
        }
    }
}

void RSDK::LoadXMLSoundFX(const tinyxml2::XMLElement *gameElement)
{
    const tinyxml2::XMLElement *soundsElement = gameElement->FirstChildElement("sounds");
    if (soundsElement) {
        for (const tinyxml2::XMLElement *sfxElement = soundsElement->FirstChildElement("soundfx"); sfxElement;
             sfxElement                             = sfxElement->NextSiblingElement("soundfx")) {
            const tinyxml2::XMLAttribute *valAttr = sfxElement->FindAttribute("path");
            const char *sfxPath                   = "unknownSFX.wav";
            if (valAttr)
                sfxPath = valAttr->Value();

            const tinyxml2::XMLAttribute *playsAttr = sfxElement->FindAttribute("maxConcurrentPlays");
            int32 maxConcurrentPlays                = 0;
            if (playsAttr)
                maxConcurrentPlays = playsAttr->IntValue();

            LoadSfx((char *)sfxPath, maxConcurrentPlays, SCOPE_GLOBAL);
        }
    }
}

#if !RETRO_REV0U
std::vector<SceneListEntry> listData;
std::vector<SceneListInfo> listCategory;
#endif

void RSDK::LoadXMLStages(const tinyxml2::XMLElement *gameElement)
{

    for (const tinyxml2::XMLElement *listElement = gameElement->FirstChildElement("category"); listElement;
         listElement                             = listElement->NextSiblingElement("category")) {
        SceneListInfo *list = nullptr;
        int32 listID;

        const tinyxml2::XMLAttribute *nameAttr = listElement->FindAttribute("name");
        const char *lstName                    = "unknown list";
        if (nameAttr)
            lstName = nameAttr->Value();
        RETRO_HASH_MD5(hash);
        GEN_HASH_MD5(lstName, hash);

        for (int l = 0; l < listCategory.size(); ++l) {
            if (HASH_MATCH_MD5(hash, listCategory[l].hash)) {
                list   = &listCategory[l];
                listID = l;
            }
        }

        if (!list) {
            listCategory.emplace_back();
            list = &listCategory.back();
            sprintf_s(list->name, sizeof(list->name), "%s", lstName);
            HASH_COPY_MD5(list->hash, hash);

            list->sceneOffsetStart = listData.size();
            list->sceneOffsetEnd   = listData.size();
            list->sceneCount       = 0;
            sceneInfo.categoryCount++;
        }

        for (const tinyxml2::XMLElement *stgElement = listElement->FirstChildElement("stage"); stgElement;
             stgElement                             = stgElement->NextSiblingElement("stage")) {
            const tinyxml2::XMLAttribute *nameAttr = stgElement->FindAttribute("name");
            const char *stgName                    = "unknownStage";
            if (nameAttr)
                stgName = nameAttr->Value();

            const tinyxml2::XMLAttribute *folderAttr = stgElement->FindAttribute("folder");
            const char *stgFolder                    = "unknownStageFolder";
            if (folderAttr)
                stgFolder = folderAttr->Value();

            const tinyxml2::XMLAttribute *idAttr = stgElement->FindAttribute("id");
            const char *stgID                    = "unknownStageID";
            if (idAttr)
                stgID = idAttr->Value();

#if RETRO_REV02
            const tinyxml2::XMLAttribute *filterAttr = stgElement->FindAttribute("filter");
            int32 stgFilter                          = 0;
            if (stgFilter)
                stgFilter = filterAttr->IntValue();
#endif
            listData.emplace(listData.begin() + list->sceneOffsetEnd);
            SceneListEntry *scene = &listData[list->sceneOffsetEnd];

            sprintf_s(scene->name, sizeof(scene->name), "%s", stgName);
            GEN_HASH_MD5(scene->name, scene->hash);
            sprintf_s(scene->folder, sizeof(scene->folder), "%s", stgFolder);
            sprintf_s(scene->id, sizeof(scene->id), "%s", stgID);

#if RETRO_REV02
            scene->filter = stgFilter;
            if (scene->filter == 0x00)
                scene->filter = 0xFF;
#endif
            list->sceneCount++;
            list->sceneOffsetEnd++;
            for (int32 l = listID + 1; l < listCategory.size(); ++l) listCategory[l].sceneOffsetStart++;
        }
    }
    sceneInfo.listData = listData.data();
    sceneInfo.listCategory = listCategory.data();
}
#endif

void RSDK::LoadGameConfig()
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadFile(&info, "Data/Game/GameConfig.bin", FMODE_RB)) {
        char buffer[0x100];
        uint32 sig = ReadInt32(&info, false);

        if (sig != RSDK_SIGNATURE_CFG) {
            CloseFile(&info);
            return;
        }

        ReadString(&info, gameVerInfo.gameTitle);
        if (!useDataPack)
            sprintf_s(gameVerInfo.gameTitle, sizeof(gameVerInfo.gameTitle), "%s (Data Folder)", gameVerInfo.gameTitle);
        ReadString(&info, gameVerInfo.gameSubtitle);
        ReadString(&info, gameVerInfo.version);

        sceneInfo.activeCategory = ReadInt8(&info);
        int32 startScene         = ReadInt16(&info);

        uint8 objCnt      = ReadInt8(&info);
        globalObjectCount = TYPE_DEFAULT_COUNT;
        for (int32 i = 0; i < objCnt; ++i) {
            ReadString(&info, textBuffer);

            RETRO_HASH_MD5(hash);
            GEN_HASH_MD5(textBuffer, hash);

            if (objectClassCount > 0) {
                globalObjectIDs[globalObjectCount] = 0;
                for (int32 objID = 0; objID < objectClassCount; ++objID) {
                    if (HASH_MATCH_MD5(hash, objectClassList[objID].hash)) {
                        globalObjectIDs[globalObjectCount] = objID;
                        globalObjectCount++;
                    }
                }
            }
        }

        for (int32 i = 0; i < PALETTE_BANK_COUNT; ++i) {
            activeGlobalRows[i] = ReadInt16(&info);
            for (int32 r = 0; r < 0x10; ++r) {
                if ((activeGlobalRows[i] >> r & 1)) {
                    for (int32 c = 0; c < 0x10; ++c) {
                        uint8 red                      = ReadInt8(&info);
                        uint8 green                    = ReadInt8(&info);
                        uint8 blue                     = ReadInt8(&info);
                        globalPalette[i][(r << 4) + c] = rgb32To16_B[blue] | rgb32To16_G[green] | rgb32To16_R[red];
                    }
                }
                else {
                    for (int32 c = 0; c < 0x10; ++c) globalPalette[i][(r << 4) + c] = 0;
                }
            }
        }

        uint8 sfxCnt = ReadInt8(&info);
        for (int32 i = 0; i < sfxCnt; ++i) {
            ReadString(&info, buffer);
            uint8 maxConcurrentPlays = ReadInt8(&info);
            LoadSfx(buffer, maxConcurrentPlays, SCOPE_GLOBAL);
        }

        uint16 totalSceneCount = ReadInt16(&info);

        if (!totalSceneCount)
            totalSceneCount = 1;

        if (strlen(currentSceneFolder) && strlen(currentSceneID)) {
#if RETRO_USE_MOD_LOADER
            listData.resize(totalSceneCount + 1);
            sceneInfo.listData = listData.data();
#else
            AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * (totalSceneCount + 1), DATASET_STG, false);
#endif
            SceneListEntry *scene = &sceneInfo.listData[totalSceneCount];
            strcpy(scene->name, "_RSDK_SCENE");
            strcpy(scene->folder, currentSceneFolder);
            strcpy(scene->id, currentSceneID);
#if RETRO_REV02
            scene->filter = sceneInfo.filter;
#endif
            GEN_HASH_MD5(scene->name, scene->hash);

            // Override existing values
            sceneInfo.activeCategory = 0;
            startScene               = totalSceneCount;
            currentSceneFolder[0]    = 0;
            currentSceneID[0]        = 0;
        }
        else {
#if RETRO_USE_MOD_LOADER
            listData.resize(totalSceneCount);
            sceneInfo.listData = listData.data();
#else
            AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * totalSceneCount, DATASET_STG, false);
#endif
        }

        sceneInfo.categoryCount = ReadInt8(&info);
        sceneInfo.listPos       = 0;

        int32 categoryCount = sceneInfo.categoryCount;

        if (!categoryCount)
            categoryCount = 1;

#if RETRO_USE_MOD_LOADER
        listCategory.resize(categoryCount);
        sceneInfo.listCategory = listCategory.data();
#else
        AllocateStorage((void **)&sceneInfo.listCategory, sizeof(SceneListInfo) * categoryCount, DATASET_STG, false);
#endif
        sceneInfo.listPos = 0;

        int32 sceneID = 0;
        for (int32 i = 0; i < sceneInfo.categoryCount; ++i) {
            SceneListInfo *category = &sceneInfo.listCategory[i];
            ReadString(&info, category->name);
            GEN_HASH_MD5(category->name, category->hash);

            category->sceneOffsetStart = sceneID;
            category->sceneCount       = ReadInt8(&info);
            for (int32 s = 0; s < category->sceneCount; ++s) {
                SceneListEntry *scene = &sceneInfo.listData[sceneID + s];
                ReadString(&info, scene->name);
                GEN_HASH_MD5(scene->name, scene->hash);

                ReadString(&info, scene->folder);
                ReadString(&info, scene->id);

#if RETRO_REV02
                scene->filter = ReadInt8(&info);
                if (scene->filter == 0x00)
                    scene->filter = 0xFF;
#endif
            }
            category->sceneOffsetEnd = category->sceneOffsetStart + category->sceneCount;
            sceneID += category->sceneCount;
        }

        uint8 varCount = ReadInt8(&info);
        for (int32 i = 0; i < varCount; ++i) {
#if RETRO_REV0U
            // v5U Ditches this in favour of the InitVarsCB method
            ReadInt32(&info, false);
            int32 count = ReadInt32(&info, false);
            for (int32 v = 0; v < count; ++v) ReadInt32(&info, false);
#else
            if (!globalVarsPtr)
                break;
            // standard v5 loads variables directly into the struct
            int32 offset = ReadInt32(&info, false);
            int32 count  = ReadInt32(&info, false);
            for (int32 v = 0; v < count; ++v) {
                globalVarsPtr[offset + v] = ReadInt32(&info, false);
            }
#endif
        }

        CloseFile(&info);
#if RETRO_USE_MOD_LOADER
        LoadGameXML();
#endif

#if RETRO_REV0U
        if (globalVarsInitCB)
            globalVarsInitCB(globalVarsPtr);
#endif

        sceneInfo.listPos = sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart + startScene;
    }
}

void RSDK::InitGameLink()
{
#if RETRO_USE_MOD_LOADER
    objectClassCount = 0;
    memset(globalObjectIDs, 0, sizeof(globalObjectIDs));
    memset(objectEntityList, 0, sizeof(objectEntityList));
    editableVarCount = 0;
    foreachStackPtr  = foreachStackList;
    currentMod       = NULL;
#endif

#if RETRO_REV0U
    RegisterObject((Object **)&DefaultObject, ":DefaultObject:", sizeof(EntityDefaultObject), sizeof(ObjectDefaultObject), DefaultObject_Update,
                   DefaultObject_LateUpdate, DefaultObject_StaticUpdate, DefaultObject_Draw, DefaultObject_Create, DefaultObject_StageLoad,
                   DefaultObject_EditorDraw, DefaultObject_EditorLoad, DefaultObject_Serialize, (void (*)(Object *))DefaultObject_StaticLoad);

    RegisterObject((Object **)&DevOutput, ":DevOutput:", sizeof(EntityDevOutput), sizeof(ObjectDevOutput), DevOutput_Update, DevOutput_LateUpdate,
                   DevOutput_StaticUpdate, DevOutput_Draw, DevOutput_Create, DevOutput_StageLoad, DevOutput_EditorDraw, DevOutput_EditorLoad,
                   DevOutput_Serialize, (void (*)(Object *))DevOutput_StaticLoad);
#else
    RegisterObject((Object **)&DefaultObject, ":DefaultObject:", sizeof(EntityDefaultObject), sizeof(ObjectDefaultObject), DefaultObject_Update,
                   DefaultObject_LateUpdate, DefaultObject_StaticUpdate, DefaultObject_Draw, DefaultObject_Create, DefaultObject_StageLoad,
                   DefaultObject_EditorDraw, DefaultObject_EditorLoad, DefaultObject_Serialize);
#if RETRO_REV02
    RegisterObject((Object **)&DevOutput, ":DevOutput:", sizeof(EntityDevOutput), sizeof(ObjectDevOutput), DevOutput_Update, DevOutput_LateUpdate,
                   DevOutput_StaticUpdate, DevOutput_Draw, DevOutput_Create, DevOutput_StageLoad, DevOutput_EditorDraw, DevOutput_EditorLoad,
                   DevOutput_Serialize);
#endif
#endif

    globalObjectIDs[0] = TYPE_DEFAULTOBJECT;
#if RETRO_REV02
    globalObjectIDs[1] = TYPE_DEVOUTPUT;
#endif

    globalObjectCount = TYPE_DEFAULT_COUNT;

#if RETRO_REV02
    EngineInfo info;

    info.functionTable = RSDKFunctionTable;
    info.APITable      = APIFunctionTable;

    info.currentSKU = &SKU::curSKU;
    info.gameInfo   = &gameVerInfo;
    info.sceneInfo  = &sceneInfo;

    info.controller = controller;
    info.stickL     = stickL;
    info.stickR     = stickR;
    info.triggerL   = triggerL;
    info.triggerR   = triggerR;
    info.touchMouse = &touchInfo;

    info.unknown = &SKU::unknownInfo;

    info.screenInfo = screens;

#if RETRO_USE_MOD_LOADER
    info.modTable = modFunctionTable;
#endif
#else
    EngineInfo info;

    info.functionTable = RSDKFunctionTable;

    info.gameInfo = &gameVerInfo;
    info.sceneInfo = &sceneInfo;

    info.controllerInfo = controller;
    info.stickInfo = stickL;
    info.touchInfo = &touchInfo;

    info.screenInfo = screens;

#if RETRO_USE_MOD_LOADER
    info.modTable = modFunctionTable;
#endif
#endif

    bool32 linked = false;

#if RETRO_USE_MOD_LOADER
    if (!modSettings.disableGameLogic) {
#endif
        if (engine.useExternalCode) {
            char buffer[0x100];
#if RETRO_PLATFORM == RETRO_WIN
            strcpy_s(buffer, 0x100, gameLogicName);
#else
        sprintf(buffer, "%s%s", SKU::userFileDir, gameLogicName);
#endif
            if (!gameLogicHandle)
                gameLogicHandle = Link::Open(buffer);

            if (gameLogicHandle) {
                bool32 canLink = true;
#if !RETRO_USE_ORIGINAL_CODE
                int32 *RSDKRevision = (int32 *)Link::GetSymbol(gameLogicHandle, "RSDKRevision");
                if (RSDKRevision) {
                    canLink = *RSDKRevision == RETRO_REVISION;
                    if (!canLink)
                        PrintLog(PRINT_NORMAL, "ERROR: Game Logic RSDK Revision doesn't match Engine RSDK Revision!");
                }
#endif

                LogicLinkHandle linkGameLogic = (LogicLinkHandle)Link::GetSymbol(gameLogicHandle, "LinkGameLogicDLL");
                if (canLink && linkGameLogic) {
#if RETRO_REV02
                    linkGameLogic(&info);
#else
                linkGameLogic(info);
#endif
                    linked = true;
                }
                else if (canLink && !linkGameLogic) {
                    PrintLog(PRINT_ERROR, "ERROR: Failed to find 'LinkGameLogicDLL' -> %s", Link::GetError());
                }
            }
            else {
                PrintLog(PRINT_ERROR, "ERROR: Failed to open game logic file -> %s", Link::GetError());
            }

            if (!linked)
                PrintLog(PRINT_NORMAL, "ERROR: Failed to link game logic!");
        }
        else {
#if RETRO_REV02
            linkGameLogic(&info);
#else
        linkGameLogic(info);
#endif
        }
#if RETRO_USE_MOD_LOADER
    }

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        currentMod = &modList[m];
        for (modLinkSTD linkModLogic : modList[m].linkModLogic) {
            if (!linkModLogic(&info, modList[m].id.c_str())) {
                modList[m].active = false;
                PrintLog(PRINT_ERROR, "[MOD] Failed to link logic for mod %s!", modList[m].id.c_str());
            }
        }
    }

    currentMod = NULL;
    SortMods();
#endif
}

void RSDK::ProcessDebugCommands()
{
#if !RETRO_USE_ORIGINAL_CODE
    if (!customSettings.enableControllerDebugging)
        return;
#endif

    if (controller[CONT_P1].keySelect.press) {
#if RETRO_REV0U
        if (sceneInfo.state == ENGINESTATE_DEVMENU || RSDK::Legacy::gameMode == RSDK::Legacy::ENGINE_DEVMENU)
#else
        if (sceneInfo.state == ENGINESTATE_DEVMENU)
#endif
            CloseDevMenu();
        else
            OpenDevMenu();
    }

#if RETRO_REV0U
    int32 state          = engine.version == 5 ? sceneInfo.state : Legacy::stageMode;
    const int32 stepOver = engine.version == 5 ? (int32)ENGINESTATE_STEPOVER : (int32)Legacy::STAGEMODE_STEPOVER;
#else
    uint8 state = sceneInfo.state;
    const uint8 stepOver = ENGINESTATE_STEPOVER;
#endif

    bool32 framePaused = (state & stepOver) == stepOver;

#if RETRO_REV02
    if (triggerL[CONT_P1].keyBumper.down) {
        if (triggerL[CONT_P1].keyTrigger.down || triggerL[CONT_P1].triggerDelta >= 0.3) {
            if (!framePaused)
                state ^= stepOver;
        }
        else {
            if (framePaused)
                state ^= stepOver;
        }

        framePaused = (state & stepOver) == stepOver;

        if (framePaused) {
            if (triggerR[CONT_P1].keyBumper.press)
                engine.frameStep = true;
        }
        else {
            engine.gameSpeed = (triggerR[CONT_P1].keyTrigger.down || triggerR[CONT_P1].triggerDelta >= 0.3) ? 8 : 1;
        }
    }
    else {
        if (engine.gameSpeed == 8)
            engine.gameSpeed = 1;

        if (framePaused)
            state ^= stepOver;
    }

#if RETRO_REV0U
    if (engine.version != 5)
        Legacy::stageMode = state;
    else
#endif
        sceneInfo.state = state;
#else
    if (controller[CONT_P1].keyBumperL.down) {
        if (controller[CONT_P1].keyTriggerL.down || stickL[CONT_P1].triggerDeltaL >= 0.3) {
            if (!framePaused)
                sceneInfo.state ^= ENGINESTATE_STEPOVER;
        }
        else {
            if (framePaused)
                sceneInfo.state ^= ENGINESTATE_STEPOVER;
        }

        framePaused = (sceneInfo.state >> 2) & 1;
        if (framePaused) {
            if (controller[CONT_P1].keyBumperR.press)
                engine.frameStep = true;
        }
        else {
            engine.gameSpeed = (controller[CONT_P1].keyTriggerR.down || stickL[CONT_P1].triggerDeltaR >= 0.3) ? 8 : 1;
        }
    }
    else {
        if (engine.gameSpeed == 8)
            engine.gameSpeed = 1;

        if (framePaused)
            sceneInfo.state ^= ENGINESTATE_STEPOVER;
    }
#endif
}

#ifdef __SWITCH__
#include <switch.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "RetroEngine.hpp"

static int32 s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        printf("printf output now goes to nxlink server\n");
    else
        socketExit();
}
#endif

void RSDK::InitCoreAPI()
{
#if RETRO_RENDERDEVICE_DIRECTX9 || RETRO_RENDERDEVICE_DIRECTX11
    MSG Msg;
    PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE);
    InitCommonControls();
#endif

#ifdef __SWITCH__
    Result res;
    if (R_FAILED(res = dynInitialize()))
        diagAbortWithResult(res);
#endif

#if RETRO_RENDERDEVICE_SDL2 || RETRO_AUDIODEVICE_SDL2 || RETRO_INPUTDEVICE_SDL2
    SDL_Init(0);
#endif
}
void RSDK::ReleaseCoreAPI()
{
#if RETRO_RENDERDEVICE_SDL2 || RETRO_AUDIODEVICE_SDL2 || RETRO_INPUTDEVICE_SDL2
    SDL_Quit();
#endif

#ifdef __SWITCH__
    dynExit();
#endif
}

void RSDK::InitConsole()
{
#if RETRO_PLATFORM == RETRO_WIN
    AllocConsole();
    AttachConsole(GetCurrentProcessId());

    freopen("CON", "w", stdout);
#endif

#if RETRO_PLATFORM == RETRO_SWITCH
    consoleInit(NULL);
#endif
}
void RSDK::ReleaseConsole()
{
#if RETRO_PLATFORM == RETRO_WIN
    FreeConsole();
#endif
}

void RSDK::SendQuitMsg()
{
#if RETRO_RENDERDEVICE_DIRECTX9 || RETRO_RENDERDEVICE_DIRECTX11
    PostQuitMessage(0);
#endif
}
