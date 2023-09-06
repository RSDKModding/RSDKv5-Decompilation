
#if RETRO_USE_MOD_LOADER
namespace RSDK
{
namespace Legacy
{
namespace v4
{

bool32 loadGlobalScripts = false; // stored here so I can use it later
int32 globalObjCount     = 0;

} // namespace v4
} // namespace Legacy
} // namespace RSDK
#endif

void RSDK::Legacy::v4::InitFirstStage(void)
{
    xScrollOffset = 0;
    yScrollOffset = 0;
    StopMusic();
    fadeMode = 0;
    ClearGraphicsData();
    ClearAnimationData();
    activePalette = fullPalette[0];
    stageMode     = STAGEMODE_LOAD;
    gameMode      = ENGINE_MAINGAME;
}

void RSDK::Legacy::v4::ProcessStage(void)
{
#if !RETRO_USE_ORIGINAL_CODE
    debugHitboxCount = 0;
#endif

    switch (stageMode) {
        case STAGEMODE_LOAD: // Startup
            SetActivePalette(0, 0, 256);
            gameMenu[0].visibleRowOffset = 0;
            gameMenu[1].visibleRowOffset = 0;
            videoSettings.dimLimit       = (5 * 60) * videoSettings.refreshRate;
            fadeMode                     = 0;
            InitCameras();
            cameraShift       = 0;
            cameraLockedY     = 0;
            xScrollOffset     = 0;
            yScrollOffset     = 0;
            cameraShakeX      = 0;
            cameraShakeY      = 0;
            vertexCount       = 0;
            faceCount         = 0;
            frameCounter      = 0;
            pauseEnabled      = false;
            timeEnabled       = false;
            stageMilliseconds = 0;
            stageSeconds      = 0;
            stageMinutes      = 0;
            stageMode         = STAGEMODE_NORMAL;

#if RSDK_AUTOBUILD
            // Prevent playing as Amy if on autobuilds
            if (GetGlobalVariableByName("PLAYER_AMY") && playerListPos == GetGlobalVariableByName("PLAYER_AMY"))
                playerListPos = 0;
            else if (GetGlobalVariableByName("PLAYER_AMY_TAILS") && playerListPos == GetGlobalVariableByName("PLAYER_AMY_TAILS"))
                playerListPos = 0;
#endif

#if RETRO_USE_MOD_LOADER
            if (devMenu.modsChanged)
                RefreshModFolders();
#endif
            ResetBackgroundSettings();
            LoadStageFiles();
            break;

        case STAGEMODE_NORMAL:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode += STAGEMODE_STEPOVER;
                PauseSound();
            }

            ProcessSceneTimer();

            // Update
            ProcessObjects();
            HandleCameras();
            ProcessParallaxAutoScroll();

#if !RETRO_USE_ORIGINAL_CODE
            for (int32 i = 1; i < engine.gameSpeed; ++i) {
                ProcessSceneTimer();

                // Update
                ProcessObjects();
                HandleCameras();
                ProcessParallaxAutoScroll();
            }
#endif

            currentScreen             = &screens[0];
            sceneInfo.currentScreenID = 0;
            DrawStageGFX();
            break;

        case STAGEMODE_PAUSED:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode += STAGEMODE_STEPOVER;
                PauseSound();
            }

            // Update
            ProcessPausedObjects();

#if !RETRO_USE_ORIGINAL_CODE
            for (int32 i = 1; i < engine.gameSpeed; ++i) {
                ProcessPausedObjects();
            }
#endif

            for (int32 s = 0; s < videoSettings.screenCount; ++s) {
                currentScreen             = &screens[s];
                sceneInfo.currentScreenID = s;

                DrawObjectList(0);
                DrawObjectList(1);
                DrawObjectList(2);
                DrawObjectList(3);
                DrawObjectList(4);
                DrawObjectList(5);
                DrawObjectList(7); // ???
                DrawObjectList(6);
            }

            currentScreen             = &screens[0];
            sceneInfo.currentScreenID = 0;

#if !RETRO_USE_ORIGINAL_CODE
            DrawDebugOverlays();
#endif
            break;

        case STAGEMODE_FROZEN:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode += STAGEMODE_STEPOVER;
                PauseSound();
            }

            // Update
            ProcessFrozenObjects();
            HandleCameras();

#if !RETRO_USE_ORIGINAL_CODE
            for (int32 i = 1; i < engine.gameSpeed; ++i) {
                ProcessFrozenObjects();
                HandleCameras();
            }
#endif

            currentScreen             = &screens[0];
            sceneInfo.currentScreenID = 0;
            DrawStageGFX();
            break;

        case STAGEMODE_2P:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode += STAGEMODE_STEPOVER;
                PauseSound();
            }

            ProcessSceneTimer();

            // Update
            Process2PObjects();
            ProcessParallaxAutoScroll();

#if !RETRO_USE_ORIGINAL_CODE
            for (int32 i = 1; i < engine.gameSpeed; ++i) {
                ProcessSceneTimer();

                Process2PObjects();
                ProcessParallaxAutoScroll();
            }
#endif

            for (int32 s = 0; s < videoSettings.screenCount; ++s) {
                currentScreen             = &screens[s];
                sceneInfo.currentScreenID = s;

                HandleCameras();
                DrawStageGFX();
            }

            currentScreen             = &screens[0];
            sceneInfo.currentScreenID = 0;

            break;

        case STAGEMODE_NORMAL + STAGEMODE_STEPOVER:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (controller[CONT_ANY].keyC.press || engine.frameStep) {
#if !RETRO_USE_ORIGINAL_CODE
                if (!engine.frameStep) {
                    for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
                }
                engine.frameStep = false;
#else
                for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
#endif

                ProcessSceneTimer();

                ProcessObjects();
                HandleCameras();

                DrawStageGFX();
                ProcessParallaxAutoScroll();

#if !RETRO_USE_ORIGINAL_CODE
                engine.frameStep = false;
#endif
            }

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode -= STAGEMODE_STEPOVER;
                ResumeSound();
            }
            break;

        case STAGEMODE_PAUSED + STAGEMODE_STEPOVER:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (controller[CONT_ANY].keyC.press || engine.frameStep) {
#if !RETRO_USE_ORIGINAL_CODE
                if (!engine.frameStep) {
                    for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
                }
                engine.frameStep = false;
#else
                for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
#endif

                ProcessPausedObjects();

                DrawObjectList(0);
                DrawObjectList(1);
                DrawObjectList(2);
                DrawObjectList(3);
                DrawObjectList(4);
                DrawObjectList(5);
                DrawObjectList(7); // ???
                DrawObjectList(6);

#if !RETRO_USE_ORIGINAL_CODE
                DrawDebugOverlays();
#endif
            }

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode -= STAGEMODE_STEPOVER;
                ResumeSound();
            }
            break;

        case STAGEMODE_FROZEN + STAGEMODE_STEPOVER:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (controller[CONT_ANY].keyC.press || engine.frameStep) {
#if !RETRO_USE_ORIGINAL_CODE
                if (!engine.frameStep) {
                    for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
                }
                engine.frameStep = false;
#else
                for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
#endif

                // Update
                ProcessFrozenObjects();
                HandleCameras();

                DrawStageGFX();
            }

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode -= STAGEMODE_STEPOVER;
                ResumeSound();
            }
            break;

        case STAGEMODE_2P + STAGEMODE_STEPOVER:
            if (fadeMode > 0)
                fadeMode--;

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (controller[CONT_ANY].keyC.press || engine.frameStep) {
#if !RETRO_USE_ORIGINAL_CODE
                if (!engine.frameStep) {
                    for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
                }
                engine.frameStep = false;
#else
                for (int32 c = CONT_ANY; c <= CONT_P1; ++c) controller[c].keyC.press = false;
#endif

                ProcessSceneTimer();

                // Update
                Process2PObjects();
                HandleCameras();

                DrawStageGFX();
                ProcessParallaxAutoScroll();
            }

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode -= STAGEMODE_STEPOVER;
                ResumeSound();
            }
            break;
    }
}

void RSDK::Legacy::v4::HandleCameras()
{
    currentCamera = &cameras[sceneInfo.currentScreenID];
    if (currentCamera->target >= LEGACY_v4_ENTITY_COUNT)
        return;

    if (currentCamera->enabled == 1) {
        switch (currentCamera->style) {
            case CAMERASTYLE_FOLLOW: SetPlayerScreenPosition(&objectEntityList[currentCamera->target]); break;
            case CAMERASTYLE_EXTENDED:
            case CAMERASTYLE_EXTENDED_OFFSET_L:
            case CAMERASTYLE_EXTENDED_OFFSET_R: SetPlayerScreenPositionCDStyle(&objectEntityList[currentCamera->target]); break;
            case CAMERASTYLE_HLOCKED: SetPlayerHLockedScreenPosition(&objectEntityList[currentCamera->target]); break;
            case CAMERASTYLE_FIXED: SetPlayerScreenPositionFixed(&objectEntityList[currentCamera->target]); break;
            case CAMERASTYLE_STATIC: SetPlayerScreenPositionStatic(&objectEntityList[currentCamera->target]); break;
            default: break;
        }
    }
    else {
        SetPlayerLockedScreenPosition(&objectEntityList[currentCamera->target]);
    }
}

void RSDK::Legacy::v4::ProcessParallaxAutoScroll()
{
    for (int32 i = 0; i < hParallax.entryCount; ++i) hParallax.scrollPos[i] += hParallax.scrollSpeed[i];
    for (int32 i = 0; i < vParallax.entryCount; ++i) vParallax.scrollPos[i] += vParallax.scrollSpeed[i];
}

void RSDK::Legacy::v4::LoadStageFiles()
{
    int32 scriptID = 1;
    char strBuffer[0x100];

    FileInfo info;
    InitFileInfo(&info);

    if (!CheckCurrentStageFolder()) {
        PrintLog(PRINT_NORMAL, "Loading Scene %s - %s", sceneInfo.listCategory[sceneInfo.activeCategory].name,
                 sceneInfo.listData[sceneInfo.listPos].name);

        // Unload stage sfx & audio channels
        ClearStageSfx();

        ClearScriptData();

        for (int32 i = LEGACY_SURFACE_COUNT; i > 0; i--) RemoveGraphicsFile((char *)"", i - 1);

#if RETRO_USE_MOD_LOADER
        loadGlobalScripts = false;
#else
        bool32 loadGlobalScripts = false;
#endif

        InitFileInfo(&info);
        if (LoadStageFile("StageConfig.bin", &info)) {
            loadGlobalScripts = ReadInt8(&info);
            CloseFile(&info);
        }

        InitFileInfo(&info);
        if (loadGlobalScripts && LoadFile(&info, "Data/Game/GameConfig.bin", FMODE_RB)) {
            ReadString(&info, strBuffer); // Title
            ReadString(&info, strBuffer); // Subtitle

            uint8 buf[3];
            for (int32 c = 0; c < 0x60; ++c) {
                ReadBytes(&info, buf, 3);
                SetPaletteEntry(-1, c, buf[0], buf[1], buf[2]);
            }

            uint8 globalObjectCount = ReadInt8(&info);
            for (uint8 i = 0; i < globalObjectCount; ++i) {
                ReadString(&info, strBuffer);
                SetObjectTypeName(strBuffer, scriptID + i);
            }

#if RETRO_USE_MOD_LOADER && LEGACY_RETRO_USE_COMPILER
            for (uint8 i = 0; i < modObjCount && loadGlobalScripts; ++i) {
                SetObjectTypeName(modTypeNames[i], globalObjectCount + i + 1);
            }
#endif

#if LEGACY_RETRO_USE_COMPILER
#if RETRO_USE_MOD_LOADER
            bool32 bytecodeExists = false;
            FileInfo bytecodeInfo;
            InitFileInfo(&bytecodeInfo);

            if (LoadFile(&bytecodeInfo, "Bytecode/GlobalCode.bin", FMODE_RB)) {
                bytecodeExists = true;
                CloseFile(&bytecodeInfo);
            }

            if (bytecodeExists && !modSettings.forceScripts) {
#else
            if (usingBytecode) {
#endif
                LoadBytecode(scriptID, true);
                scriptID += globalObjectCount;
            }
            else {
                for (uint8 i = 0; i < globalObjectCount; ++i) {
                    ReadString(&info, strBuffer);
                    ParseScriptFile(strBuffer, scriptID++);

                    if (gameMode == ENGINE_SCRIPTERROR)
                        return;
                }
            }
#else
            LoadBytecode(scriptID, true);
            scriptID += globalObjectCount;
#endif
            CloseFile(&info);

#if RETRO_USE_MOD_LOADER
            LoadGameXML(true);
#endif

#if RETRO_USE_MOD_LOADER && LEGACY_RETRO_USE_COMPILER
            globalObjCount = globalObjectCount;
            for (uint8 i = 0; i < modObjCount && loadGlobalScripts; ++i) {
                SetObjectTypeName(modTypeNames[i], scriptID);

                ParseScriptFile(modScriptPaths[i], scriptID++);
                if (gameMode == ENGINE_SCRIPTERROR)
                    return;
            }
#endif
        }

        InitFileInfo(&info);
        if (LoadStageFile("StageConfig.bin", &info)) {
            ReadInt8(&info); // Load Globals

            uint8 clr[3];
            for (int32 i = 0x60; i < 0x80; ++i) {
                ReadBytes(&info, clr, 3);
                SetPaletteEntry(-1, i, clr[0], clr[1], clr[2]);
            }

            stageSFXCount = ReadInt8(&info);
            for (uint8 i = 0; i < stageSFXCount; ++i) {
                ReadString(&info, strBuffer);
                SetSfxName(strBuffer, i + globalSFXCount);
            }

            for (uint8 i = 0; i < stageSFXCount; ++i) {
                ReadString(&info, strBuffer);

                RSDK::Legacy::LoadSfx(strBuffer, globalSFXCount + i, SCOPE_STAGE);
            }

            uint8 stageObjectCount = ReadInt8(&info);
            for (uint8 i = 0; i < stageObjectCount; ++i) {
                ReadString(&info, strBuffer);

                SetObjectTypeName(strBuffer, scriptID + i);
            }

#if LEGACY_RETRO_USE_COMPILER
#if RETRO_USE_MOD_LOADER
            char scriptPath[0x40];
            switch (sceneInfo.activeCategory) {
                case STAGELIST_PRESENTATION:
                case STAGELIST_REGULAR:
                case STAGELIST_BONUS:
                case STAGELIST_SPECIAL:
                    StrCopy(scriptPath, "Bytecode/");
                    StrAdd(scriptPath, sceneInfo.listData[sceneInfo.listPos].folder);
                    StrAdd(scriptPath, ".bin");
                    break;
                default: break;
            }

            bool32 bytecodeExists = false;
            FileInfo bytecodeInfo;
            InitFileInfo(&bytecodeInfo);
            if (LoadFile(&bytecodeInfo, scriptPath, FMODE_RB)) {
                bytecodeExists = true;
                CloseFile(&bytecodeInfo);
            }

            if (bytecodeExists && !modSettings.forceScripts) {
#else
            if (usingBytecode) {
#endif
                for (uint8 i = 0; i < stageObjectCount; ++i) {
                    ReadString(&info, strBuffer);
                }

                LoadBytecode(scriptID, false);
            }
            else {
                for (uint8 i = 0; i < stageObjectCount; ++i) {
                    ReadString(&info, strBuffer);
                    ParseScriptFile(strBuffer, scriptID + i);

                    if (gameMode == ENGINE_SCRIPTERROR)
                        return;
                }
            }
#else
            for (uint8 i = 0; i < stageObjectCount; ++i) {
                ReadString(&info, strBuffer);
            }
            LoadBytecode(scriptID, false);
#endif
            CloseFile(&info);
        }

        LoadStageGIFFile();
        LoadStageCollisions();
        LoadStageBackground();
    }
    else {
        PrintLog(PRINT_NORMAL, "Reloading Scene %s - %s", sceneInfo.listCategory[sceneInfo.activeCategory].name,
                 sceneInfo.listData[sceneInfo.listPos].name);
    }
    LoadStageChunks();

    for (int32 i = 0; i < LEGACY_TRACK_COUNT; ++i) SetMusicTrack("", i, false, 0);

    memset(objectEntityList, 0, LEGACY_v4_ENTITY_COUNT * sizeof(Entity));
    for (int32 i = 0; i < LEGACY_v4_ENTITY_COUNT; ++i) {
        objectEntityList[i].drawOrder          = 3;
        objectEntityList[i].scale              = 512;
        objectEntityList[i].objectInteractions = true;
        objectEntityList[i].visible            = true;
        objectEntityList[i].tileCollisions     = true;
    }

    LoadActLayout();
    Init3DFloorBuffer(0);
    ProcessStartupObjects();
}
void RSDK::Legacy::v4::LoadActLayout()
{
    FileInfo info;
    InitFileInfo(&info);

    for (int32 a = 0; a < 4; ++a) activeTileLayers[a] = 9; // disables missing scenes from rendering

    if (LoadActFile(".bin", &info)) {
        int32 tCardWordLen = ReadInt8(&info);
        titleCardWord2     = tCardWordLen;
        for (int32 i = 0; i < tCardWordLen; ++i) {
            titleCardText[i] = ReadInt8(&info);
            if (titleCardText[i] == '-')
                titleCardWord2 = (uint8)(i + 1);
        }
        titleCardText[tCardWordLen] = '\0';

        // READ TILELAYER
        ReadBytes(&info, activeTileLayers, 4);
        tLayerMidPoint = ReadInt8(&info);

        stageLayouts[0].xsize = ReadInt8(&info);
        ReadInt8(&info); // Unused

        stageLayouts[0].ysize = ReadInt8(&info);
        ReadInt8(&info); // Unused

        curXBoundary1 = 0;
        newXBoundary1 = 0;
        curYBoundary1 = 0;
        newYBoundary1 = 0;
        curXBoundary2 = stageLayouts[0].xsize << 7;
        curYBoundary2 = stageLayouts[0].ysize << 7;
        waterLevel    = curYBoundary2 + 128;
        newXBoundary2 = stageLayouts[0].xsize << 7;
        newYBoundary2 = stageLayouts[0].ysize << 7;

        memset(stageLayouts[0].tiles, 0, LEGACY_TILELAYER_CHUNK_COUNT * sizeof(uint16));
        memset(stageLayouts[0].lineScroll, 0, 0x7FFF);

        for (int32 y = 0; y < stageLayouts[0].ysize; ++y) {
            uint16 *tiles = &stageLayouts[0].tiles[(y * LEGACY_TILELAYER_CHUNK_H)];
            for (int32 x = 0; x < stageLayouts[0].xsize; ++x) {
                tiles[x] = ReadInt8(&info);
                tiles[x] |= ReadInt8(&info) << 8;
            }
        }

        // READ OBJECTS
        int32 objectCount = ReadInt8(&info);
        objectCount |= ReadInt8(&info) << 8;
#if !RETRO_USE_ORIGINAL_CODE
        if (objectCount > 0x400)
            PrintLog(PRINT_NORMAL, "WARNING: object count %d exceeds the object limit", objectCount);
#endif

#if RETRO_USE_MOD_LOADER
        int32 offsetCount = 0;
        for (int32 m = 0; m < modObjCount; ++m)
            if (modScriptFlags[m])
                ++offsetCount;
#endif

        Entity *object = &objectEntityList[32];
        for (int32 i = 0; i < objectCount; ++i) {
            int32 activeVars = ReadInt8(&info);
            activeVars |= ReadInt8(&info) << 8;

            object->type = ReadInt8(&info);

#if RETRO_USE_MOD_LOADER
            if (loadGlobalScripts && offsetCount && object->type > globalObjCount)
                object->type += offsetCount; // offset it by our mod count
#endif

            object->propertyValue = ReadInt8(&info);

            object->xpos = ReadInt8(&info);
            object->xpos |= ReadInt8(&info) << 8;
            object->xpos |= ReadInt8(&info) << 16;
            object->xpos |= ReadInt8(&info) << 24;

            object->ypos = ReadInt8(&info);
            object->ypos |= ReadInt8(&info) << 8;
            object->ypos |= ReadInt8(&info) << 16;
            object->ypos |= ReadInt8(&info) << 24;

            if (activeVars & 0x1) {
                object->state = ReadInt8(&info);
                object->state |= ReadInt8(&info) << 8;
                object->state |= ReadInt8(&info) << 16;
                object->state |= ReadInt8(&info) << 24;
            }

            if (activeVars & 0x2)
                object->direction = ReadInt8(&info);

            if (activeVars & 0x4) {
                object->scale = ReadInt8(&info);
                object->scale |= ReadInt8(&info) << 8;
                object->scale |= ReadInt8(&info) << 16;
                object->scale |= ReadInt8(&info) << 24;
            }

            if (activeVars & 0x8) {
                object->rotation = ReadInt8(&info);
                object->rotation |= ReadInt8(&info) << 8;
                object->rotation |= ReadInt8(&info) << 16;
                object->rotation |= ReadInt8(&info) << 24;
            }

            if (activeVars & 0x10)
                object->drawOrder = ReadInt8(&info);

            if (activeVars & 0x20)
                object->priority = ReadInt8(&info);

            if (activeVars & 0x40)
                object->alpha = ReadInt8(&info);

            if (activeVars & 0x80)
                object->animation = ReadInt8(&info);

            if (activeVars & 0x100) {
                object->animationSpeed = ReadInt8(&info);
                object->animationSpeed |= ReadInt8(&info) << 8;
                object->animationSpeed |= ReadInt8(&info) << 16;
                object->animationSpeed |= ReadInt8(&info) << 24;
            }

            if (activeVars & 0x200)
                object->frame = ReadInt8(&info);

            if (activeVars & 0x400)
                object->inkEffect = ReadInt8(&info);

            if (activeVars & 0x800) {
                object->values[0] = ReadInt8(&info);
                object->values[0] |= ReadInt8(&info) << 8;
                object->values[0] |= ReadInt8(&info) << 16;
                object->values[0] |= ReadInt8(&info) << 24;
            }

            if (activeVars & 0x1000) {
                object->values[1] = ReadInt8(&info);
                object->values[1] |= ReadInt8(&info) << 8;
                object->values[1] |= ReadInt8(&info) << 16;
                object->values[1] |= ReadInt8(&info) << 24;
            }

            if (activeVars & 0x2000) {
                object->values[2] = ReadInt8(&info);
                object->values[2] |= ReadInt8(&info) << 8;
                object->values[2] |= ReadInt8(&info) << 16;
                object->values[2] |= ReadInt8(&info) << 24;
            }

            if (activeVars & 0x4000) {
                object->values[3] = ReadInt8(&info);
                object->values[3] |= ReadInt8(&info) << 8;
                object->values[3] |= ReadInt8(&info) << 16;
                object->values[3] |= ReadInt8(&info) << 24;
            }

            ++object;
        }

        CloseFile(&info);
    }

    stageLayouts[0].type = LAYER_HSCROLL;
}
void RSDK::Legacy::v4::LoadStageBackground()
{
    for (int32 i = 0; i < LEGACY_LAYER_COUNT; ++i) {
        stageLayouts[i].type               = LAYER_NOSCROLL;
        stageLayouts[i].deformationOffset  = 0;
        stageLayouts[i].deformationOffsetW = 0;
    }

    for (int32 i = 0; i < LEGACY_PARALLAX_COUNT; ++i) {
        hParallax.scrollPos[i] = 0;
        vParallax.scrollPos[i] = 0;
    }

    FileInfo info;
    InitFileInfo(&info);
    if (LoadStageFile("Backgrounds.bin", &info)) {
        uint8 layerCount = ReadInt8(&info);

        hParallax.entryCount = ReadInt8(&info);
        for (uint8 i = 0; i < hParallax.entryCount; ++i) {
            hParallax.parallaxFactor[i] = ReadInt8(&info);
            hParallax.parallaxFactor[i] |= ReadInt8(&info) << 8;

            hParallax.scrollSpeed[i] = ReadInt8(&info) << 10;

            hParallax.scrollPos[i] = 0;

            hParallax.deform[i] = ReadInt8(&info);
        }

        vParallax.entryCount = ReadInt8(&info);
        for (uint8 i = 0; i < vParallax.entryCount; ++i) {
            vParallax.parallaxFactor[i] = ReadInt8(&info);
            vParallax.parallaxFactor[i] |= ReadInt8(&info) << 8;

            vParallax.scrollSpeed[i] = ReadInt8(&info) << 10;

            vParallax.scrollPos[i] = 0;

            vParallax.deform[i] = ReadInt8(&info);
        }

        for (uint8 i = 1; i < layerCount + 1; ++i) {
            stageLayouts[i].xsize = ReadInt8(&info);
            ReadInt8(&info); // unused

            stageLayouts[i].ysize = ReadInt8(&info);
            ReadInt8(&info);

            stageLayouts[i].type = ReadInt8(&info);

            stageLayouts[i].parallaxFactor = ReadInt8(&info);
            stageLayouts[i].parallaxFactor |= ReadInt8(&info) << 8;

            stageLayouts[i].scrollSpeed = ReadInt8(&info) << 10;
            stageLayouts[i].scrollPos   = 0;

            memset(stageLayouts[i].tiles, 0, LEGACY_TILELAYER_CHUNK_COUNT * sizeof(uint16));
            uint8 *lineScrollPtr = stageLayouts[i].lineScroll;
            memset(stageLayouts[i].lineScroll, 0, 0x7FFF);

            // Read Line Scroll
            uint8 buf[3];
            while (true) {
                buf[0] = ReadInt8(&info);

                if (buf[0] == 0xFF) {
                    buf[1] = ReadInt8(&info);
                    if (buf[1] == 0xFF) {
                        break;
                    }
                    else {
                        buf[2]      = ReadInt8(&info);
                        int32 index = buf[1];
                        int32 cnt   = buf[2] - 1;
                        for (int32 c = 0; c < cnt; ++c) *lineScrollPtr++ = index;
                    }
                }
                else {
                    *lineScrollPtr++ = buf[0];
                }
            }

            // Read Layout
            for (int32 y = 0; y < stageLayouts[i].ysize; ++y) {
                uint16 *chunks = &stageLayouts[i].tiles[y * LEGACY_TILELAYER_CHUNK_H];
                for (int32 x = 0; x < stageLayouts[i].xsize; ++x) {
                    *chunks = ReadInt8(&info);
                    *chunks |= ReadInt8(&info) << 8;

                    ++chunks;
                }
            }
        }

        CloseFile(&info);
    }
}

void RSDK::Legacy::v4::SetPlayerScreenPosition(Entity *target)
{
    int32 targetX = target->xpos >> 16;
    int32 targetY = currentCamera->adjustY + (target->ypos >> 16);

    if (newYBoundary1 > curYBoundary1) {
        if (newYBoundary1 >= yScrollOffset)
            curYBoundary1 = yScrollOffset;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary1 < curYBoundary1) {
        if (curYBoundary1 >= yScrollOffset)
            --curYBoundary1;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary2 < curYBoundary2) {
        if (curYBoundary2 <= yScrollOffset + SCREEN_YSIZE || newYBoundary2 >= yScrollOffset + SCREEN_YSIZE)
            --curYBoundary2;
        else
            curYBoundary2 = yScrollOffset + SCREEN_YSIZE;
    }

    if (newYBoundary2 > curYBoundary2) {
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2) {
            ++curYBoundary2;
            if (target->yvel > 0) {
                int32 buf = curYBoundary2 + (target->yvel >> 16);
                if (newYBoundary2 < buf) {
                    curYBoundary2 = newYBoundary2;
                }
                else {
                    curYBoundary2 += target->yvel >> 16;
                }
            }
        }
        else
            curYBoundary2 = newYBoundary2;
    }
    if (newXBoundary1 > curXBoundary1) {
        if (xScrollOffset <= newXBoundary1)
            curXBoundary1 = xScrollOffset;
        else
            curXBoundary1 = newXBoundary1;
    }
    if (newXBoundary1 < curXBoundary1) {
        if (xScrollOffset <= curXBoundary1) {
            --curXBoundary1;
            if (target->xvel < 0) {
                curXBoundary1 += target->xvel >> 16;
                if (curXBoundary1 < newXBoundary1)
                    curXBoundary1 = newXBoundary1;
            }
        }
        else {
            curXBoundary1 = newXBoundary1;
        }
    }

    if (newXBoundary2 < curXBoundary2) {
        if (newXBoundary2 > SCREEN_XSIZE + xScrollOffset)
            curXBoundary2 = newXBoundary2;
        else
            curXBoundary2 = SCREEN_XSIZE + xScrollOffset;
    }

    if (newXBoundary2 > curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2) {
            ++curXBoundary2;
            if (target->xvel > 0) {
                curXBoundary2 += target->xvel >> 16;
                if (curXBoundary2 > newXBoundary2)
                    curXBoundary2 = newXBoundary2;
            }
        }
        else {
            curXBoundary2 = newXBoundary2;
        }
    }

    int32 xPosDif = targetX - currentCamera->xpos;
    if (targetX > currentCamera->xpos) {
        xPosDif -= 8;
        if (xPosDif >= 0) {
            if (xPosDif >= 17)
                xPosDif = 16;
        }
        else {
            xPosDif = 0;
        }
    }
    else {
        xPosDif += 8;
        if (xPosDif > 0) {
            xPosDif = 0;
        }
        else if (xPosDif <= -17) {
            xPosDif = -16;
        }
    }

    int32 centeredXBound1 = currentCamera->xpos + xPosDif;
    currentCamera->xpos   = centeredXBound1;
    if (centeredXBound1 < SCREEN_CENTERX + curXBoundary1) {
        currentCamera->xpos = SCREEN_CENTERX + curXBoundary1;
        centeredXBound1     = SCREEN_CENTERX + curXBoundary1;
    }

    int32 centeredXBound2 = curXBoundary2 - SCREEN_CENTERX;
    if (centeredXBound2 < centeredXBound1) {
        currentCamera->xpos = centeredXBound2;
        centeredXBound1     = centeredXBound2;
    }

    int32 yPosDif = 0;
    if (target->scrollTracking) {
        if (targetY <= currentCamera->ypos) {
            yPosDif = (targetY - currentCamera->ypos) + 32;
            if (yPosDif <= 0) {
                if (yPosDif <= -17)
                    yPosDif = -16;
            }
            else
                yPosDif = 0;
        }
        else {
            yPosDif = (targetY - currentCamera->ypos) - 32;
            if (yPosDif >= 0) {
                if (yPosDif >= 17)
                    yPosDif = 16;
            }
            else
                yPosDif = 0;
        }
        cameraLockedY = false;
    }
    else if (cameraLockedY) {
        yPosDif             = 0;
        currentCamera->ypos = targetY;
    }
    else if (targetY <= currentCamera->ypos) {
        yPosDif = targetY - currentCamera->ypos;
        if (targetY - currentCamera->ypos <= 0) {
            if (yPosDif >= -32 && abs(target->yvel) <= 0x60000) {
                if (yPosDif < -6) {
                    yPosDif = -6;
                }
            }
            else if (yPosDif < -16) {
                yPosDif = -16;
            }
        }
        else {
            yPosDif       = 0;
            cameraLockedY = true;
        }
    }
    else {
        yPosDif = targetY - currentCamera->ypos;
        if (targetY - currentCamera->ypos < 0) {
            yPosDif       = 0;
            cameraLockedY = true;
        }
        else if (yPosDif > 32 || abs(target->yvel) > 0x60000) {
            if (yPosDif > 16) {
                yPosDif = 16;
            }
            else {
                cameraLockedY = true;
            }
        }
        else {
            if (yPosDif <= 6) {
                cameraLockedY = true;
            }
            else {
                yPosDif = 6;
            }
        }
    }

    int32 newCamY = currentCamera->ypos + yPosDif;
    if (newCamY <= curYBoundary1 + (LEGACY_SCREEN_SCROLL_UP - 1))
        newCamY = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
    currentCamera->ypos = newCamY;
    if (curYBoundary2 - (LEGACY_SCREEN_SCROLL_DOWN - 1) <= newCamY) {
        currentCamera->ypos = curYBoundary2 - LEGACY_SCREEN_SCROLL_DOWN;
    }

    xScrollOffset = cameraShakeX + centeredXBound1 - SCREEN_CENTERX;

    int32 pos = currentCamera->ypos + target->lookPosY - LEGACY_SCREEN_SCROLL_UP;
    if (pos < curYBoundary1) {
        yScrollOffset = curYBoundary1;
    }
    else {
        yScrollOffset = currentCamera->ypos + target->lookPosY - LEGACY_SCREEN_SCROLL_UP;
    }

    int32 y = curYBoundary2 - SCREEN_YSIZE;
    if (curYBoundary2 - (SCREEN_YSIZE - 1) > yScrollOffset)
        y = yScrollOffset;
    yScrollOffset = cameraShakeY + y;

    if (cameraShakeX) {
        if (cameraShakeX <= 0) {
            cameraShakeX = ~cameraShakeX;
        }
        else {
            cameraShakeX = -cameraShakeX;
        }
    }

    if (cameraShakeY) {
        if (cameraShakeY <= 0) {
            cameraShakeY = ~cameraShakeY;
        }
        else {
            cameraShakeY = -cameraShakeY;
        }
    }
}
void RSDK::Legacy::v4::SetPlayerScreenPositionCDStyle(Entity *target)
{
    int32 targetX = target->xpos >> 16;
    int32 targetY = currentCamera->adjustY + (target->ypos >> 16);

    if (newYBoundary1 > curYBoundary1) {
        if (newYBoundary1 >= yScrollOffset)
            curYBoundary1 = yScrollOffset;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary1 < curYBoundary1) {
        if (curYBoundary1 >= yScrollOffset)
            --curYBoundary1;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary2 < curYBoundary2) {
        if (curYBoundary2 <= yScrollOffset + SCREEN_YSIZE || newYBoundary2 >= yScrollOffset + SCREEN_YSIZE)
            --curYBoundary2;
        else
            curYBoundary2 = yScrollOffset + SCREEN_YSIZE;
    }

    if (newYBoundary2 > curYBoundary2) {
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2) {
            ++curYBoundary2;
            if (target->yvel > 0) {
                int32 buf = curYBoundary2 + (target->yvel >> 16);
                if (newYBoundary2 < buf) {
                    curYBoundary2 = newYBoundary2;
                }
                else {
                    curYBoundary2 += target->yvel >> 16;
                }
            }
        }
        else
            curYBoundary2 = newYBoundary2;
    }

    if (newXBoundary1 > curXBoundary1) {
        if (xScrollOffset <= newXBoundary1)
            curXBoundary1 = xScrollOffset;
        else
            curXBoundary1 = newXBoundary1;
    }

    if (newXBoundary1 < curXBoundary1) {
        if (xScrollOffset <= curXBoundary1) {
            --curXBoundary1;
            if (target->xvel < 0) {
                curXBoundary1 += target->xvel >> 16;
                if (curXBoundary1 < newXBoundary1)
                    curXBoundary1 = newXBoundary1;
            }
        }
        else {
            curXBoundary1 = newXBoundary1;
        }
    }

    if (newXBoundary2 < curXBoundary2) {
        if (newXBoundary2 > SCREEN_XSIZE + xScrollOffset)
            curXBoundary2 = newXBoundary2;
        else
            curXBoundary2 = SCREEN_XSIZE + xScrollOffset;
    }

    if (newXBoundary2 > curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2) {
            ++curXBoundary2;
            if (target->xvel > 0) {
                curXBoundary2 += target->xvel >> 16;
                if (curXBoundary2 > newXBoundary2)
                    curXBoundary2 = newXBoundary2;
            }
        }
        else {
            curXBoundary2 = newXBoundary2;
        }
    }

    if (!target->gravity) {
        if (target->direction) {
            if (currentCamera->style == CAMERASTYLE_EXTENDED_OFFSET_R || target->speed < -0x5F5C2) {
                cameraShift = 2;
                if (target->lookPosX <= 63) {
                    target->lookPosX += 2;
                }
            }
            else {
                cameraShift = 0;
                if (target->lookPosX < 0) {
                    target->lookPosX += 2;
                }

                if (target->lookPosX > 0) {
                    target->lookPosX -= 2;
                }
            }
        }
        else {
            if (currentCamera->style == CAMERASTYLE_EXTENDED_OFFSET_L || target->speed > 0x5F5C2) {
                cameraShift = 1;
                if (target->lookPosX >= -63) {
                    target->lookPosX -= 2;
                }
            }
            else {
                cameraShift = 0;
                if (target->lookPosX < 0) {
                    target->lookPosX += 2;
                }

                if (target->lookPosX > 0) {
                    target->lookPosX -= 2;
                }
            }
        }
    }
    else {
        if (cameraShift == 1) {
            if (target->lookPosX >= -63) {
                target->lookPosX -= 2;
            }
        }
        else if (cameraShift < 1) {
            if (target->lookPosX < 0) {
                target->lookPosX += 2;
            }
            if (target->lookPosX > 0) {
                target->lookPosX -= 2;
            }
        }
        else if (cameraShift == 2) {
            if (target->lookPosX <= 63) {
                target->lookPosX += 2;
            }
        }
    }
    currentCamera->xpos = targetX - target->lookPosX;

    if (!target->scrollTracking) {
        if (cameraLockedY) {
            currentCamera->ypos = targetY;
            if (currentCamera->ypos < curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
            }
        }
        else if (targetY > currentCamera->ypos) {
            int32 dif = targetY - currentCamera->ypos;
            if (targetY - currentCamera->ypos < 0) {
                cameraLockedY = true;
                if (currentCamera->ypos < curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                    currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                }
            }
            else {
                if (dif > 32 || abs(target->yvel) > 0x60000) {
                    if (dif > 16) {
                        dif = 16;
                        if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                            currentCamera->ypos += dif;
                        }
                        else {
                            currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                        }
                    }
                    else {
                        cameraLockedY = true;
                        if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                            currentCamera->ypos += dif;
                        }
                        else {
                            currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                        }
                    }
                }
                else if (dif > 6) {
                    dif = 6;
                    if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                        currentCamera->ypos += dif;
                    }
                    else {
                        currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                    }
                }
                else {
                    cameraLockedY = true;
                    if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                        currentCamera->ypos += dif;
                    }
                    else {
                        currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                    }
                }
            }
        }
        else {
            int32 dif = targetY - currentCamera->ypos;
            if (targetY - currentCamera->ypos <= 0) {
                if (dif < -32 || abs(target->yvel) > 0x60000) {
                    if (dif < -16) {
                        dif = -16;
                        if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                            currentCamera->ypos += dif;
                        }
                        else {
                            currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                        }
                    }
                    else {
                        cameraLockedY = true;
                        if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                            currentCamera->ypos += dif;
                        }
                        else {
                            currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                        }
                    }
                }
                else if (dif < -6) {
                    dif = -6;
                    if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                        currentCamera->ypos += dif;
                    }
                    else {
                        currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                    }
                }
            }
            else {
                dif = 0;
                if (abs(target->yvel) > 0x60000) {
                    cameraLockedY = true;
                    if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                        currentCamera->ypos += dif;
                    }
                    else {
                        currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                    }
                }
                else {
                    cameraLockedY = true;
                    if (currentCamera->ypos + dif >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                        currentCamera->ypos += dif;
                    }
                    else {
                        currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                    }
                }
            }
        }
    }
    else {
        int32 dif  = targetY - currentCamera->ypos;
        int32 difY = 0;
        if (targetY > currentCamera->ypos) {
            difY = dif - 32;
            if (difY >= 0) {
                if (difY >= 17)
                    difY = 16;

                cameraLockedY = false;
                if (currentCamera->ypos + difY >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                    currentCamera->ypos += difY;
                }
                else {
                    currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                }
            }
            else {
                cameraLockedY = false;
                if (currentCamera->ypos < curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                    currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                }
            }
        }
        else {
            difY = dif + 32;
            if (difY > 0) {
                difY = 0;

                cameraLockedY = false;
                if (currentCamera->ypos < curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                    currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                }
            }
            else if (difY <= -17) {
                difY = -16;

                cameraLockedY = false;
                if (currentCamera->ypos + difY >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                    currentCamera->ypos += difY;
                }
                else {
                    currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                }
            }
            else {
                cameraLockedY = false;
                if (currentCamera->ypos + difY >= curYBoundary1 + LEGACY_SCREEN_SCROLL_UP) {
                    currentCamera->ypos += difY;
                }
                else {
                    currentCamera->ypos = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
                }
            }
        }
    }

    if (currentCamera->ypos >= curYBoundary2 - LEGACY_SCREEN_SCROLL_DOWN - 1) {
        currentCamera->ypos = curYBoundary2 - LEGACY_SCREEN_SCROLL_DOWN;
    }

    xScrollOffset = currentCamera->xpos - SCREEN_CENTERX;
    yScrollOffset = target->lookPosY + currentCamera->ypos - LEGACY_SCREEN_SCROLL_UP;

    int32 x = curXBoundary1;
    if (x <= xScrollOffset)
        x = xScrollOffset;
    else
        xScrollOffset = x;

    if (x > curXBoundary2 - SCREEN_XSIZE) {
        x             = curXBoundary2 - SCREEN_XSIZE;
        xScrollOffset = curXBoundary2 - SCREEN_XSIZE;
    }

    int32 y = curYBoundary1;
    if (yScrollOffset >= y)
        y = yScrollOffset;
    else
        yScrollOffset = y;

    if (curYBoundary2 - SCREEN_YSIZE - 1 <= y)
        y = curYBoundary2 - SCREEN_YSIZE;

    xScrollOffset = cameraShakeX + x;
    yScrollOffset = cameraShakeY + y;

    if (cameraShakeX) {
        if (cameraShakeX <= 0) {
            cameraShakeX = ~cameraShakeX;
        }
        else {
            cameraShakeX = -cameraShakeX;
        }
    }

    if (cameraShakeY) {
        if (cameraShakeY <= 0) {
            cameraShakeY = ~cameraShakeY;
        }
        else {
            cameraShakeY = -cameraShakeY;
        }
    }
}
void RSDK::Legacy::v4::SetPlayerHLockedScreenPosition(Entity *target)
{
    int32 targetY = currentCamera->adjustY + (target->ypos >> 16);

    if (newYBoundary1 <= curYBoundary1) {
        if (curYBoundary1 > yScrollOffset)
            --curYBoundary1;
        else
            curYBoundary1 = newYBoundary1;
    }
    else {
        if (newYBoundary1 >= yScrollOffset)
            curYBoundary1 = yScrollOffset;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary2 < curYBoundary2) {
        if (curYBoundary2 <= yScrollOffset + SCREEN_YSIZE || newYBoundary2 >= yScrollOffset + SCREEN_YSIZE)
            --curYBoundary2;
        else
            curYBoundary2 = yScrollOffset + SCREEN_YSIZE;
    }
    if (newYBoundary2 > curYBoundary2) {
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2) {
            ++curYBoundary2;
            if (target->yvel > 0) {
                if (newYBoundary2 < curYBoundary2 + (target->yvel >> 16)) {
                    curYBoundary2 = newYBoundary2;
                }
                else {
                    curYBoundary2 += target->yvel >> 16;
                }
            }
        }
        else
            curYBoundary2 = newYBoundary2;
    }

    if (newXBoundary1 > curXBoundary1) {
        if (xScrollOffset <= newXBoundary1)
            curXBoundary1 = xScrollOffset;
        else
            curXBoundary1 = newXBoundary1;
    }
    if (newXBoundary1 < curXBoundary1) {
        if (xScrollOffset <= curXBoundary1) {
            --curXBoundary1;
            if (target->xvel < 0) {
                curXBoundary1 += target->xvel >> 16;
                if (curXBoundary1 < newXBoundary1)
                    curXBoundary1 = newXBoundary1;
            }
        }
        else {
            curXBoundary1 = newXBoundary1;
        }
    }

    if (newXBoundary2 < curXBoundary2) {
        if (newXBoundary2 > SCREEN_XSIZE + xScrollOffset)
            curXBoundary2 = newXBoundary2;
        else
            curXBoundary2 = SCREEN_XSIZE + xScrollOffset;
    }

    if (newXBoundary2 > curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2) {
            ++curXBoundary2;
            if (target->xvel > 0) {
                curXBoundary2 += target->xvel >> 16;
                if (curXBoundary2 > newXBoundary2)
                    curXBoundary2 = newXBoundary2;
            }
        }
        else {
            curXBoundary2 = newXBoundary2;
        }
    }

    int32 camScroll = 0;
    if (target->scrollTracking) {
        if (targetY <= currentCamera->ypos) {
            camScroll = targetY - currentCamera->ypos + 32;
            if (camScroll <= 0) {
                if (camScroll <= -17)
                    camScroll = -16;
            }
            else
                camScroll = 0;
        }
        else {
            camScroll = targetY - currentCamera->ypos - 32;
            if (camScroll >= 0) {
                if (camScroll >= 17)
                    camScroll = 16;
            }
            else
                camScroll = 0;
        }
        cameraLockedY = false;
    }
    else if (cameraLockedY) {
        camScroll           = 0;
        currentCamera->ypos = targetY;
    }
    else if (targetY > currentCamera->ypos) {
        camScroll = targetY - currentCamera->ypos;
        if (camScroll >= 0) {
            if (camScroll > 32 || abs(target->yvel) > 0x60000) {
                if (camScroll > 16) {
                    camScroll = 16;
                }
                else {
                    cameraLockedY = true;
                }
            }
            else if (camScroll > 6) {
                camScroll = 6;
            }
            else {
                cameraLockedY = true;
            }
        }
        else {
            camScroll     = 0;
            cameraLockedY = true;
        }
    }
    else {
        camScroll = targetY - currentCamera->ypos;
        if (camScroll > 0) {
            camScroll     = 0;
            cameraLockedY = true;
        }
        else if (camScroll < -32 || abs(target->yvel) > 0x60000) {
            if (camScroll < -16) {
                camScroll = -16;
            }
            else {
                cameraLockedY = true;
            }
        }
        else {
            if (camScroll >= -6)
                cameraLockedY = true;
            else
                camScroll = -6;
        }
    }

    int32 newCamY = currentCamera->ypos + camScroll;
    if (newCamY <= curYBoundary1 + (LEGACY_SCREEN_SCROLL_UP - 1))
        newCamY = curYBoundary1 + LEGACY_SCREEN_SCROLL_UP;
    currentCamera->ypos = newCamY;
    if (curYBoundary2 - (LEGACY_SCREEN_SCROLL_DOWN - 1) <= newCamY) {
        newCamY             = curYBoundary2 - LEGACY_SCREEN_SCROLL_DOWN;
        currentCamera->ypos = curYBoundary2 - LEGACY_SCREEN_SCROLL_DOWN;
    }

    xScrollOffset = cameraShakeX + currentCamera->xpos - SCREEN_CENTERX;

    int32 pos = newCamY + target->lookPosY - LEGACY_SCREEN_SCROLL_UP;
    if (pos < curYBoundary1) {
        yScrollOffset = curYBoundary1;
    }
    else {
        yScrollOffset = newCamY + target->lookPosY - LEGACY_SCREEN_SCROLL_UP;
    }
    int32 y1 = curYBoundary2 - (SCREEN_YSIZE - 1);
    int32 y2 = curYBoundary2 - SCREEN_YSIZE;
    if (y1 > yScrollOffset)
        y2 = yScrollOffset;
    yScrollOffset = cameraShakeY + y2;

    if (cameraShakeX) {
        if (cameraShakeX <= 0) {
            cameraShakeX = ~cameraShakeX;
        }
        else {
            cameraShakeX = -cameraShakeX;
        }
    }

    if (cameraShakeY) {
        if (cameraShakeY <= 0) {
            cameraShakeY = ~cameraShakeY;
        }
        else {
            cameraShakeY = -cameraShakeY;
        }
    }
}
void RSDK::Legacy::v4::SetPlayerLockedScreenPosition(Entity *target)
{
    if (newYBoundary1 > curYBoundary1) {
        if (yScrollOffset <= newYBoundary1)
            curYBoundary1 = yScrollOffset;
        else
            curYBoundary1 = newYBoundary1;
    }
    if (newYBoundary1 < curYBoundary1) {
        if (yScrollOffset <= curYBoundary1)
            --curYBoundary1;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary2 < curYBoundary2) {
        if (curYBoundary2 <= yScrollOffset + SCREEN_YSIZE || newYBoundary2 >= yScrollOffset + SCREEN_YSIZE)
            --curYBoundary2;
        else
            curYBoundary2 = yScrollOffset + SCREEN_YSIZE;
    }

    if (newYBoundary2 > curYBoundary2) {
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2)
            ++curYBoundary2;
        else
            curYBoundary2 = newYBoundary2;
    }

    if (newXBoundary1 > curXBoundary1) {
        if (xScrollOffset <= newXBoundary1)
            curXBoundary1 = xScrollOffset;
        else
            curXBoundary1 = newXBoundary1;
    }

    if (newXBoundary1 < curXBoundary1) {
        if (xScrollOffset <= curXBoundary1) {
            --curXBoundary1;
            if (target->xvel < 0) {
                curXBoundary1 += target->xvel >> 16;
                if (curXBoundary1 < newXBoundary1)
                    curXBoundary1 = newXBoundary1;
            }
        }
        else {
            curXBoundary1 = newXBoundary1;
        }
    }

    if (newXBoundary2 < curXBoundary2) {
        if (newXBoundary2 > SCREEN_XSIZE + xScrollOffset)
            curXBoundary2 = newXBoundary2;
        else
            curXBoundary2 = SCREEN_XSIZE + xScrollOffset;
    }

    if (newXBoundary2 > curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2) {
            ++curXBoundary2;
            if (target->xvel > 0) {
                curXBoundary2 += target->xvel >> 16;
                if (curXBoundary2 > newXBoundary2)
                    curXBoundary2 = newXBoundary2;
            }
        }
        else {
            curXBoundary2 = newXBoundary2;
        }
    }

    if (cameraShakeX) {
        if (cameraShakeX <= 0) {
            cameraShakeX = ~cameraShakeX;
        }
        else {
            cameraShakeX = -cameraShakeX;
        }
    }

    if (cameraShakeY) {
        if (cameraShakeY <= 0) {
            cameraShakeY = ~cameraShakeY;
        }
        else {
            cameraShakeY = -cameraShakeY;
        }
    }
}
void RSDK::Legacy::v4::SetPlayerScreenPositionFixed(Entity *target)
{
    int32 targetX = target->xpos >> 16;
    int32 targetY = currentCamera->adjustY + (target->ypos >> 16);

    if (newYBoundary1 > curYBoundary1) {
        if (newYBoundary1 >= yScrollOffset)
            curYBoundary1 = yScrollOffset;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary1 < curYBoundary1) {
        if (curYBoundary1 >= yScrollOffset)
            --curYBoundary1;
        else
            curYBoundary1 = newYBoundary1;
    }

    if (newYBoundary2 < curYBoundary2) {
        if (curYBoundary2 <= yScrollOffset + SCREEN_YSIZE || newYBoundary2 >= yScrollOffset + SCREEN_YSIZE)
            --curYBoundary2;
        else
            curYBoundary2 = yScrollOffset + SCREEN_YSIZE;
    }

    if (newYBoundary2 > curYBoundary2) {
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2) {
            ++curYBoundary2;
            if (target->yvel > 0) {
                int32 buf = curYBoundary2 + (target->yvel >> 16);
                if (newYBoundary2 < buf) {
                    curYBoundary2 = newYBoundary2;
                }
                else {
                    curYBoundary2 += target->yvel >> 16;
                }
            }
        }
        else
            curYBoundary2 = newYBoundary2;
    }

    if (newXBoundary1 > curXBoundary1) {
        if (xScrollOffset <= newXBoundary1)
            curXBoundary1 = xScrollOffset;
        else
            curXBoundary1 = newXBoundary1;
    }

    if (newXBoundary1 < curXBoundary1) {
        if (xScrollOffset <= curXBoundary1) {
            --curXBoundary1;
            if (target->xvel < 0) {
                curXBoundary1 += target->xvel >> 16;
                if (curXBoundary1 < newXBoundary1)
                    curXBoundary1 = newXBoundary1;
            }
        }
        else {
            curXBoundary1 = newXBoundary1;
        }
    }

    if (newXBoundary2 < curXBoundary2) {
        if (newXBoundary2 > SCREEN_XSIZE + xScrollOffset)
            curXBoundary2 = newXBoundary2;
        else
            curXBoundary2 = SCREEN_XSIZE + xScrollOffset;
    }

    if (newXBoundary2 > curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2) {
            ++curXBoundary2;
            if (target->xvel > 0) {
                curXBoundary2 += target->xvel >> 16;
                if (curXBoundary2 > newXBoundary2)
                    curXBoundary2 = newXBoundary2;
            }
        }
        else {
            curXBoundary2 = newXBoundary2;
        }
    }

    currentCamera->xpos = targetX;
    if (targetX < SCREEN_CENTERX + curXBoundary1) {
        targetX             = SCREEN_CENTERX + curXBoundary1;
        currentCamera->xpos = SCREEN_CENTERX + curXBoundary1;
    }
    int32 boundX2 = curXBoundary2 - SCREEN_CENTERX;
    if (boundX2 < targetX) {
        targetX             = boundX2;
        currentCamera->xpos = boundX2;
    }

    if (targetY <= curYBoundary1 + 119) {
        targetY             = curYBoundary1 + 120;
        currentCamera->ypos = curYBoundary1 + 120;
    }
    else {
        currentCamera->ypos = targetY;
    }
    if (curYBoundary2 - ((SCREEN_YSIZE / 2) - 1) <= targetY) {
        targetY             = curYBoundary2 - (SCREEN_YSIZE / 2);
        currentCamera->ypos = curYBoundary2 - (SCREEN_YSIZE / 2);
    }

    xScrollOffset = cameraShakeX + targetX - SCREEN_CENTERX;
    int32 camY    = targetY + target->lookPosY - (SCREEN_YSIZE / 2);
    if (curYBoundary1 > camY) {
        yScrollOffset = curYBoundary1;
    }
    else {
        yScrollOffset = targetY + target->lookPosY - (SCREEN_YSIZE / 2);
    }

    int32 newCamY = curYBoundary2 - SCREEN_YSIZE;
    if (curYBoundary2 - (SCREEN_YSIZE - 1) > yScrollOffset)
        newCamY = yScrollOffset;
    yScrollOffset = cameraShakeY + newCamY;

    if (cameraShakeX) {
        if (cameraShakeX <= 0) {
            cameraShakeX = ~cameraShakeX;
        }
        else {
            cameraShakeX = -cameraShakeX;
        }
    }

    if (cameraShakeY) {
        if (cameraShakeY <= 0) {
            cameraShakeY = ~cameraShakeY;
        }
        else {
            cameraShakeY = -cameraShakeY;
        }
    }
}
void RSDK::Legacy::v4::SetPlayerScreenPositionStatic(Entity *target)
{
    xScrollOffset = currentCamera->xpos - SCREEN_CENTERX;
    yScrollOffset = currentCamera->ypos + target->lookPosY - LEGACY_SCREEN_SCROLL_UP;

    if (yScrollOffset < curYBoundary1)
        yScrollOffset = curYBoundary1;
    if (yScrollOffset > curYBoundary2 - SCREEN_YSIZE)
        yScrollOffset = curYBoundary2 - SCREEN_YSIZE;
}
