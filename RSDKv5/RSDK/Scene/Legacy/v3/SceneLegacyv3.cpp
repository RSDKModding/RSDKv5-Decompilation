
int32 RSDK::Legacy::v3::yScrollA    = 0;
int32 RSDK::Legacy::v3::yScrollB    = SCREEN_YSIZE;
int32 RSDK::Legacy::v3::xScrollA    = 0;
int32 RSDK::Legacy::v3::xScrollB    = Legacy::SCREEN_XSIZE;
int32 RSDK::Legacy::v3::yScrollMove = 0;

#if RETRO_USE_MOD_LOADER
namespace RSDK
{
namespace Legacy
{
namespace v3
{

bool32 loadGlobalScripts = false; // stored here so I can use it later
int32 globalObjCount     = 0;

} // namespace v3
} // namespace Legacy
} // namespace RSDK
#endif

void RSDK::Legacy::v3::InitFirstStage()
{
    xScrollOffset = 0;
    yScrollOffset = 0;
    StopMusic();
    fadeMode = 0;
    ClearGraphicsData();
    ClearAnimationData();
    activePalette = fullPalette[0];
    LoadPalette("MasterPalette.act", 0, 0, 0, 256);
#if RETRO_USE_MOD_LOADER
    LoadGameXML(true);
#endif

    stageMode = STAGEMODE_LOAD;
    gameMode  = ENGINE_MAINGAME;
}

void RSDK::Legacy::v3::ProcessStage()
{

#if !RETRO_USE_ORIGINAL_CODE
    debugHitboxCount = 0;
#endif

    switch (stageMode) {
        case STAGEMODE_LOAD: // Startup
            fadeMode = 0;
            SetActivePalette(0, 0, 256);
            InitCameras();
            videoSettings.dimLimit = (5 * 60) * videoSettings.refreshRate;

            xScrollOffset = 0;
            yScrollOffset = 0;
            yScrollA      = 0;
            yScrollB      = SCREEN_YSIZE;
            xScrollA      = 0;
            xScrollB      = SCREEN_XSIZE;
            yScrollMove   = 0;
            cameraShakeX  = 0;
            cameraShakeY  = 0;

            vertexCount = 0;
            faceCount   = 0;

#if RSDK_AUTOBUILD
            // Prevent playing as Knuckles or Amy if on autobuilds
            if (GetGlobalVariableByName("PLAYER_KNUCKLES") && playerListPos == GetGlobalVariableByName("PLAYER_KNUCKLES"))
                playerListPos = 0;
            else if (GetGlobalVariableByName("PLAYER_KNUCKLES_TAILS") && playerListPos == GetGlobalVariableByName("PLAYER_KNUCKLES_TAILS"))
                playerListPos = 0;
            else if (GetGlobalVariableByName("PLAYER_AMY") && playerListPos == GetGlobalVariableByName("PLAYER_AMY"))
                playerListPos = 0;
            else if (GetGlobalVariableByName("PLAYER_AMY_TAILS") && playerListPos == GetGlobalVariableByName("PLAYER_AMY_TAILS"))
                playerListPos = 0;
#endif

            for (int32 p = 0; p < LEGACY_v3_PLAYER_COUNT; ++p) {
                memset(&playerList[p], 0, sizeof(playerList[p]));
                playerList[p].visible            = true;
                playerList[p].gravity            = 1; // Air
                playerList[p].tileCollisions     = true;
                playerList[p].objectInteractions = true;
            }

            pauseEnabled      = false;
            timeEnabled       = false;
            frameCounter      = 0;
            stageMilliseconds = 0;
            stageSeconds      = 0;
            stageMinutes      = 0;
            stageMode         = STAGEMODE_NORMAL;

            sceneInfo.currentScreenID = 0;
            currentScreen             = screens;
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

            if (paletteMode > 0) {
                paletteMode = 0;
                SetActivePalette(0, 0, 256);
            }

            lastXSize = -1;
            lastYSize = -1;

            ProcessInput();

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode = STAGEMODE_PAUSED;
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

            DrawStageGFX();
            break;

        case STAGEMODE_PAUSED:
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

                if (fadeMode)
                    DrawRectangle(0, 0, SCREEN_XSIZE, SCREEN_YSIZE, fadeR, fadeG, fadeB, fadeA);
            }
            else {
                // Update
                ProcessPausedObjects();

                DrawObjectList(0);
                DrawObjectList(1);
                DrawObjectList(2);
                DrawObjectList(3);
                DrawObjectList(4);
                DrawObjectList(5);
                DrawObjectList(7); // ???
                DrawObjectList(6);
            }

#if !RETRO_USE_ORIGINAL_CODE
            DrawDebugOverlays();
#endif

            if (pauseEnabled && controller[CONT_ANY].keyStart.press) {
                stageMode = STAGEMODE_NORMAL;
                ResumeSound();
            }
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
    }
}

void RSDK::Legacy::v3::HandleCameras()
{
    if (currentCamera->target >= LEGACY_v3_ENTITY_COUNT)
        return;

    if (currentCamera->enabled) {
        switch (currentCamera->style) {
            case CAMERASTYLE_FOLLOW: SetPlayerScreenPosition(&playerList[currentCamera->target]); break;
            case CAMERASTYLE_EXTENDED:
            case CAMERASTYLE_EXTENDED_OFFSET_L:
            case CAMERASTYLE_EXTENDED_OFFSET_R: SetPlayerScreenPositionCDStyle(&playerList[currentCamera->target]); break;
            case CAMERASTYLE_HLOCKED: SetPlayerHLockedScreenPosition(&playerList[currentCamera->target]); break;
            default: break;
        }
    }
    else {
        SetPlayerLockedScreenPosition(&playerList[currentCamera->target]);
    }
}

void RSDK::Legacy::v3::ProcessParallaxAutoScroll()
{
    for (int32 i = 0; i < hParallax.entryCount; ++i) hParallax.scrollPos[i] += hParallax.scrollSpeed[i];
    for (int32 i = 0; i < vParallax.entryCount; ++i) vParallax.scrollPos[i] += vParallax.scrollSpeed[i];
}

void RSDK::Legacy::v3::LoadStageFiles()
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

        LoadPalette("MasterPalette.act", 0, 0, 0, 256);
#if RETRO_USE_MOD_LOADER
        LoadGameXML(true);
#endif

        ClearScriptData();

        for (int32 i = LEGACY_SURFACE_COUNT; i > 0; i--) RemoveGraphicsFile((char *)"", i - 1);

#if RETRO_USE_MOD_LOADER
        loadGlobalScripts = false;
#else
        bool32 loadGlobalScripts = false;
#endif

        if (LoadStageFile("StageConfig.bin", &info)) {
            loadGlobalScripts = ReadInt8(&info);
            CloseFile(&info);
        }

        InitFileInfo(&info);
        if (loadGlobalScripts && LoadFile(&info, "Data/Game/GameConfig.bin", FMODE_RB)) {
            ReadString(&info, strBuffer); // Title
            ReadString(&info, strBuffer); // "Data"
            ReadString(&info, strBuffer); // Description

            uint8 globalObjectCount = ReadInt8(&info);
            for (uint8 i = 0; i < globalObjectCount; ++i) {
                ReadString(&info, strBuffer);
                SetObjectTypeName(strBuffer, i + scriptID);
            }

#if RETRO_USE_MOD_LOADER
            for (uint8 i = 0; i < modObjCount && loadGlobalScripts; ++i) {
                SetObjectTypeName(modTypeNames[i], globalObjectCount + i + 1);
            }
#endif

#if LEGACY_RETRO_USE_COMPILER
#if RETRO_USE_MOD_LOADER
            char scriptPath[0x40];
            StrCopy(scriptPath, "Data/Scripts/ByteCode/GlobalCode.bin");

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

            for (int32 i = 96; i < 128; ++i) {
                uint8 clr[3];
                ReadBytes(&info, &clr, 3);
                SetPaletteEntry(-1, i, clr[0], clr[1], clr[2]);
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
                    StrCopy(scriptPath, "Data/Scripts/ByteCode/");
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
                for (uint8 i = 0; i < stageObjectCount; ++i) ReadString(&info, strBuffer);

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
            for (uint8 i = 0; i < stageObjectCount; ++i) ReadString(&info, strBuffer);

            LoadBytecode(scriptID, false);
#endif

            stageSFXCount = ReadInt8(&info);
            for (int32 i = 0; i < stageSFXCount; ++i) {
                ReadString(&info, strBuffer);

                RSDK::Legacy::LoadSfx(strBuffer, globalSFXCount + i, SCOPE_STAGE);

#if RETRO_USE_MOD_LOADER
                SetSfxName(strBuffer, i, false);
#endif
            }
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

    for (int32 i = 0; i < LEGACY_TRACK_COUNT; ++i) SetMusicTrack((char *)"", i, 0, 0);
    for (int32 i = 0; i < LEGACY_v3_ENTITY_COUNT; ++i) {
        memset(&objectEntityList[i], 0, sizeof(objectEntityList[i]));

        objectEntityList[i].drawOrder = 3;
        objectEntityList[i].scale     = 512;
    }

    LoadActLayout();
    Init3DFloorBuffer(0);
    ProcessStartupObjects();

    xScrollA = (playerList[0].XPos >> 16) - SCREEN_CENTERX;
    xScrollB = (playerList[0].XPos >> 16) - SCREEN_CENTERX + SCREEN_XSIZE;
    yScrollA = (playerList[0].YPos >> 16) - LEGACY_SCREEN_SCROLL_UP;
    yScrollB = (playerList[0].YPos >> 16) - LEGACY_SCREEN_SCROLL_UP + SCREEN_YSIZE;
}
void RSDK::Legacy::v3::LoadActLayout()
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadActFile(".bin", &info)) {
        uint8 length   = ReadInt8(&info);
        titleCardWord2 = (uint8)length;
        for (int32 i = 0; i < length; i++) {
            titleCardText[i] = ReadInt8(&info);
            if (titleCardText[i] == '-')
                titleCardWord2 = (uint8)(i + 1);
        }
        titleCardText[length] = '\0';

        // READ TILELAYER
        ReadBytes(&info, activeTileLayers, 4);
        tLayerMidPoint = ReadInt8(&info);

        stageLayouts[0].xsize = ReadInt8(&info);
        stageLayouts[0].ysize = ReadInt8(&info);
        curXBoundary1         = 0;
        newXBoundary1         = 0;
        curYBoundary1         = 0;
        newYBoundary1         = 0;
        curXBoundary2         = stageLayouts[0].xsize << 7;
        curYBoundary2         = stageLayouts[0].ysize << 7;
        waterLevel            = curYBoundary2 + 128;
        newXBoundary2         = stageLayouts[0].xsize << 7;
        newYBoundary2         = stageLayouts[0].ysize << 7;

        for (int32 i = 0; i < 0x10000; ++i) stageLayouts[0].tiles[i] = 0;

        for (int32 y = 0; y < stageLayouts[0].ysize; ++y) {
            uint16 *tiles = &stageLayouts[0].tiles[y * 0x100];
            for (int32 x = 0; x < stageLayouts[0].xsize; ++x) {
                tiles[x] = ReadInt8(&info) << 8;
                tiles[x] |= ReadInt8(&info);
            }
        }

        // READ TYPENAMES
        int32 typenameCount = ReadInt8(&info);
        for (int32 i = 0; i < typenameCount; ++i) {
            int32 nameLen = ReadInt8(&info);
            for (int32 l = 0; l < nameLen; ++l) ReadInt8(&info);
        }

        // READ OBJECTS
        int32 entityCount = ReadInt8(&info) << 8;
        entityCount |= ReadInt8(&info);

#if !RETRO_USE_ORIGINAL_CODE
        if (entityCount > 0x400)
            PrintLog(PRINT_NORMAL, "WARNING: entity count %d exceeds the entity limit", entityCount);
#endif

#if RETRO_USE_MOD_LOADER
        int32 offsetCount = 0;
        for (int32 m = 0; m < modObjCount; ++m)
            if (modScriptFlags[m])
                ++offsetCount;
#endif

        Entity *entity = &objectEntityList[32];
        for (int32 i = 0; i < entityCount; ++i) {
            entity->type = ReadInt8(&info);

#if RETRO_USE_MOD_LOADER
            if (loadGlobalScripts && offsetCount && entity->type > globalObjCount)
                entity->type += offsetCount; // offset it by our mod count
#endif

            entity->propertyValue = ReadInt8(&info);

            entity->XPos = ReadInt8(&info) << 8;
            entity->XPos |= ReadInt8(&info);
            entity->XPos <<= 16;

            entity->YPos = ReadInt8(&info) << 8;
            entity->YPos |= ReadInt8(&info);
            entity->YPos <<= 16;

            ++entity;
        }
        CloseFile(&info);
    }
    stageLayouts[0].type = LAYER_HSCROLL;
}
void RSDK::Legacy::v3::LoadStageBackground()
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
        for (int32 i = 0; i < hParallax.entryCount; ++i) {
            hParallax.parallaxFactor[i] = ReadInt8(&info) << 8;
            hParallax.parallaxFactor[i] |= ReadInt8(&info);

            hParallax.scrollSpeed[i] = ReadInt8(&info) << 10;

            hParallax.scrollPos[i] = 0;

            hParallax.deform[i] = ReadInt8(&info);
        }

        vParallax.entryCount = ReadInt8(&info);
        for (int32 i = 0; i < vParallax.entryCount; ++i) {
            vParallax.parallaxFactor[i] = ReadInt8(&info) << 8;
            vParallax.parallaxFactor[i] |= ReadInt8(&info);

            vParallax.scrollSpeed[i] = ReadInt8(&info) << 10;

            vParallax.scrollPos[i] = 0;

            vParallax.deform[i] = ReadInt8(&info);
        }

        for (int32 i = 1; i < layerCount + 1; ++i) {
            stageLayouts[i].xsize = ReadInt8(&info);
            stageLayouts[i].ysize = ReadInt8(&info);

            stageLayouts[i].type = ReadInt8(&info);

            stageLayouts[i].parallaxFactor = ReadInt8(&info) << 8;
            stageLayouts[i].parallaxFactor |= ReadInt8(&info);

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
                        buf[2]    = ReadInt8(&info);
                        int32 val = buf[1];
                        int32 cnt = buf[2] - 1;
                        for (int32 c = 0; c < cnt; ++c) *lineScrollPtr++ = val;
                    }
                }
                else {
                    *lineScrollPtr++ = buf[0];
                }
            }

            // Read Layout
            for (int32 y = 0; y < stageLayouts[i].ysize; ++y) {
                uint16 *chunks = &stageLayouts[i].tiles[y * 0x100];
                for (int32 x = 0; x < stageLayouts[i].xsize; ++x) {
                    *chunks = ReadInt8(&info) << 8;
                    *chunks |= ReadInt8(&info);

                    ++chunks;
                }
            }
        }

        CloseFile(&info);
    }
}

void RSDK::Legacy::v3::SetPlayerScreenPosition(Player *player)
{
    int32 playerXPos = player->XPos >> 16;
    int32 playerYPos = player->YPos >> 16;
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
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2 || yScrollOffset + SCREEN_YSIZE <= newYBoundary2)
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
            if (player->XVelocity < 0) {
                curXBoundary1 += player->XVelocity >> 16;
                if (curXBoundary1 < newXBoundary1)
                    curXBoundary1 = newXBoundary1;
            }
        }
        else {
            curXBoundary1 = newXBoundary1;
        }
    }
    if (newXBoundary2 < curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2)
            curXBoundary2 = SCREEN_XSIZE + xScrollOffset;
        else
            curXBoundary2 = newXBoundary2;
    }
    if (newXBoundary2 > curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2) {
            ++curXBoundary2;
            if (player->XVelocity > 0) {
                curXBoundary2 += player->XVelocity >> 16;
                if (curXBoundary2 > newXBoundary2)
                    curXBoundary2 = newXBoundary2;
            }
        }
        else {
            curXBoundary2 = newXBoundary2;
        }
    }
    int32 xscrollA     = xScrollA;
    int32 xscrollB     = xScrollB;
    int32 scrollAmount = playerXPos - (SCREEN_CENTERX + xScrollA);
    if (abs(playerXPos - (SCREEN_CENTERX + xScrollA)) >= 25) {
        if (scrollAmount <= 0)
            xscrollA -= 16;
        else
            xscrollA += 16;
        xscrollB = SCREEN_XSIZE + xscrollA;
    }
    else {
        if (playerXPos > (SCREEN_CENTERX + 8) + xscrollA) {
            xscrollA = playerXPos - (SCREEN_CENTERX + 8);
            xscrollB = SCREEN_XSIZE + playerXPos - (SCREEN_CENTERX + 8);
        }
        if (playerXPos < (SCREEN_CENTERX - 8) + xscrollA) {
            xscrollA = playerXPos - (SCREEN_CENTERX - 8);
            xscrollB = SCREEN_XSIZE + playerXPos - (SCREEN_CENTERX - 8);
        }
    }
    if (xscrollA < curXBoundary1) {
        xscrollA = curXBoundary1;
        xscrollB = SCREEN_XSIZE + curXBoundary1;
    }
    if (xscrollB > curXBoundary2) {
        xscrollB = curXBoundary2;
        xscrollA = curXBoundary2 - SCREEN_XSIZE;
    }

    xScrollA = xscrollA;
    xScrollB = xscrollB;
    if (playerXPos <= SCREEN_CENTERX + xscrollA) {
        player->screenXPos = cameraShakeX + playerXPos - xscrollA;
        xScrollOffset      = xscrollA - cameraShakeX;
    }
    else {
        xScrollOffset      = cameraShakeX + playerXPos - SCREEN_CENTERX;
        player->screenXPos = SCREEN_CENTERX - cameraShakeX;
        if (playerXPos > xscrollB - SCREEN_CENTERX) {
            player->screenXPos = cameraShakeX + SCREEN_CENTERX + playerXPos - (xscrollB - SCREEN_CENTERX);
            xScrollOffset      = xscrollB - SCREEN_XSIZE - cameraShakeX;
        }
    }

    int32 yscrollA     = yScrollA;
    int32 yscrollB     = yScrollB;
    int32 adjustYPos   = currentCamera->adjustY + playerYPos;
    int32 adjustAmount = player->lookPos + adjustYPos - (yscrollA + LEGACY_SCREEN_SCROLL_UP);
    if (player->trackScroll) {
        yScrollMove = 32;
    }
    else {
        if (yScrollMove == 32) {
            yScrollMove = 2 * ((LEGACY_SCREEN_SCROLL_UP - player->screenYPos - player->lookPos) >> 1);
            if (yScrollMove > 32)
                yScrollMove = 32;
            if (yScrollMove < -32)
                yScrollMove = -32;
        }
        if (yScrollMove > 0)
            yScrollMove -= 6;
        yScrollMove += yScrollMove < 0 ? 6 : 0;
    }

    if (abs(adjustAmount) >= abs(yScrollMove) + 17) {
        if (adjustAmount <= 0)
            yscrollA -= 16;
        else
            yscrollA += 16;
        yscrollB = yscrollA + SCREEN_YSIZE;
    }
    else if (yScrollMove == 32) {
        if (player->lookPos + adjustYPos > yscrollA + yScrollMove + LEGACY_SCREEN_SCROLL_UP) {
            yscrollA = player->lookPos + adjustYPos - (yScrollMove + LEGACY_SCREEN_SCROLL_UP);
            yscrollB = yscrollA + SCREEN_YSIZE;
        }
        if (player->lookPos + adjustYPos < yscrollA + LEGACY_SCREEN_SCROLL_UP - yScrollMove) {
            yscrollA = player->lookPos + adjustYPos - (LEGACY_SCREEN_SCROLL_UP - yScrollMove);
            yscrollB = yscrollA + SCREEN_YSIZE;
        }
    }
    else {
        yscrollA = player->lookPos + adjustYPos + yScrollMove - LEGACY_SCREEN_SCROLL_UP;
        yscrollB = yscrollA + SCREEN_YSIZE;
    }
    if (yscrollA < curYBoundary1) {
        yscrollA = curYBoundary1;
        yscrollB = curYBoundary1 + SCREEN_YSIZE;
    }
    if (yscrollB > curYBoundary2) {
        yscrollB = curYBoundary2;
        yscrollA = curYBoundary2 - SCREEN_YSIZE;
    }
    yScrollA = yscrollA;
    yScrollB = yscrollB;
    if (player->lookPos + adjustYPos <= yScrollA + LEGACY_SCREEN_SCROLL_UP) {
        player->screenYPos = adjustYPos - yScrollA - cameraShakeY;
        yScrollOffset      = cameraShakeY + yScrollA;
    }
    else {
        yScrollOffset      = cameraShakeY + adjustYPos + player->lookPos - LEGACY_SCREEN_SCROLL_UP;
        player->screenYPos = LEGACY_SCREEN_SCROLL_UP - player->lookPos - cameraShakeY;
        if (player->lookPos + adjustYPos > yScrollB - LEGACY_SCREEN_SCROLL_DOWN) {
            player->screenYPos = adjustYPos - (yScrollB - LEGACY_SCREEN_SCROLL_DOWN) + cameraShakeY + LEGACY_SCREEN_SCROLL_UP;
            yScrollOffset      = yScrollB - SCREEN_YSIZE - cameraShakeY;
        }
    }
    player->screenYPos -= currentCamera->adjustY;

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
void RSDK::Legacy::v3::SetPlayerScreenPositionCDStyle(Player *player)
{
    int32 playerXPos = player->XPos >> 16;
    int32 playerYPos = player->YPos >> 16;

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
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2 || yScrollOffset + SCREEN_YSIZE <= newYBoundary2)
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
            if (player->XVelocity < 0) {
                curXBoundary1 += player->XVelocity >> 16;
                if (curXBoundary1 < newXBoundary1)
                    curXBoundary1 = newXBoundary1;
            }
        }
        else {
            curXBoundary1 = newXBoundary1;
        }
    }

    if (newXBoundary2 < curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2)
            curXBoundary2 = SCREEN_XSIZE + xScrollOffset;
        else
            curXBoundary2 = newXBoundary2;
    }

    if (newXBoundary2 > curXBoundary2) {
        if (SCREEN_XSIZE + xScrollOffset >= curXBoundary2) {
            ++curXBoundary2;
            if (player->XVelocity > 0) {
                curXBoundary2 += player->XVelocity >> 16;
                if (curXBoundary2 > newXBoundary2)
                    curXBoundary2 = newXBoundary2;
            }
        }
        else {
            curXBoundary2 = newXBoundary2;
        }
    }

    if (!player->gravity) {
        if (player->boundEntity->direction) {
            if (currentCamera->style == CAMERASTYLE_EXTENDED_OFFSET_R || player->speed < -0x5F5C2)
                cameraLagStyle = 2;
            else
                cameraLagStyle = 0;
        }
        else {
            cameraLagStyle = (currentCamera->style == CAMERASTYLE_EXTENDED_OFFSET_L || player->speed > 0x5F5C2) != 0;
        }
    }

    if (cameraLagStyle) {
        if (cameraLagStyle == 1) {
            if (cameraLag > -64)
                cameraLag -= 2;
        }
        else if (cameraLagStyle == 2 && cameraLag < 64) {
            cameraLag += 2;
        }
    }
    else {
        cameraLag += cameraLag < 0 ? 2 : 0;
        if (cameraLag > 0)
            cameraLag -= 2;
    }
    if (playerXPos <= cameraLag + SCREEN_CENTERX + curXBoundary1) {
        player->screenXPos = cameraShakeX + playerXPos - curXBoundary1;
        xScrollOffset      = curXBoundary1 - cameraShakeX;
    }
    else {
        xScrollOffset      = cameraShakeX + playerXPos - SCREEN_CENTERX - cameraLag;
        player->screenXPos = cameraLag + SCREEN_CENTERX - cameraShakeX;
        if (playerXPos - cameraLag > curXBoundary2 - SCREEN_CENTERX) {
            player->screenXPos = cameraShakeX + SCREEN_CENTERX + playerXPos - (curXBoundary2 - SCREEN_CENTERX);
            xScrollOffset      = curXBoundary2 - SCREEN_XSIZE - cameraShakeX;
        }
    }
    xScrollA           = xScrollOffset;
    xScrollB           = SCREEN_XSIZE + xScrollOffset;
    int32 yscrollA     = yScrollA;
    int32 yscrollB     = yScrollB;
    int32 adjustY      = currentCamera->adjustY + playerYPos;
    int32 adjustOffset = player->lookPos + adjustY - (yScrollA + LEGACY_SCREEN_SCROLL_UP);
    if (player->trackScroll == 1) {
        yScrollMove = 32;
    }
    else {
        if (yScrollMove == 32) {
            yScrollMove = 2 * ((LEGACY_SCREEN_SCROLL_UP - player->screenYPos - player->lookPos) >> 1);
            if (yScrollMove > 32)
                yScrollMove = 32;
            if (yScrollMove < -32)
                yScrollMove = -32;
        }
        if (yScrollMove > 0)
            yScrollMove -= 6;
        yScrollMove += yScrollMove < 0 ? 6 : 0;
    }

    int32 absAdjust = abs(adjustOffset);
    if (absAdjust >= abs(yScrollMove) + 17) {
        if (adjustOffset <= 0)
            yscrollA -= 16;
        else
            yscrollA += 16;
        yscrollB = yscrollA + SCREEN_YSIZE;
    }
    else if (yScrollMove == 32) {
        if (player->lookPos + adjustY > yscrollA + yScrollMove + LEGACY_SCREEN_SCROLL_UP) {
            yscrollA = player->lookPos + adjustY - (yScrollMove + LEGACY_SCREEN_SCROLL_UP);
            yscrollB = yscrollA + SCREEN_YSIZE;
        }
        if (player->lookPos + adjustY < yscrollA + LEGACY_SCREEN_SCROLL_UP - yScrollMove) {
            yscrollA = player->lookPos + adjustY - (LEGACY_SCREEN_SCROLL_UP - yScrollMove);
            yscrollB = yscrollA + SCREEN_YSIZE;
        }
    }
    else {
        yscrollA = player->lookPos + adjustY + yScrollMove - LEGACY_SCREEN_SCROLL_UP;
        yscrollB = yscrollA + SCREEN_YSIZE;
    }
    if (yscrollA < curYBoundary1) {
        yscrollA = curYBoundary1;
        yscrollB = curYBoundary1 + SCREEN_YSIZE;
    }
    if (yscrollB > curYBoundary2) {
        yscrollB = curYBoundary2;
        yscrollA = curYBoundary2 - SCREEN_YSIZE;
    }
    yScrollA = yscrollA;
    yScrollB = yscrollB;
    if (player->lookPos + adjustY <= yscrollA + LEGACY_SCREEN_SCROLL_UP) {
        player->screenYPos = adjustY - yscrollA - cameraShakeY;
        yScrollOffset      = cameraShakeY + yscrollA;
    }
    else {
        yScrollOffset      = cameraShakeY + adjustY + player->lookPos - LEGACY_SCREEN_SCROLL_UP;
        player->screenYPos = LEGACY_SCREEN_SCROLL_UP - player->lookPos - cameraShakeY;
        if (player->lookPos + adjustY > yscrollB - LEGACY_SCREEN_SCROLL_DOWN) {
            player->screenYPos = adjustY - (yscrollB - LEGACY_SCREEN_SCROLL_DOWN) + cameraShakeY + LEGACY_SCREEN_SCROLL_UP;
            yScrollOffset      = yscrollB - SCREEN_YSIZE - cameraShakeY;
        }
    }
    player->screenYPos -= currentCamera->adjustY;

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
void RSDK::Legacy::v3::SetPlayerHLockedScreenPosition(Player *player)
{
    int32 playerXPos = player->XPos >> 16;
    int32 playerYPos = player->YPos >> 16;
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
        if (yScrollOffset + SCREEN_YSIZE >= curYBoundary2 || yScrollOffset + SCREEN_YSIZE <= newYBoundary2)
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

    int32 xscrollA = xScrollA;
    int32 xscrollB = xScrollB;
    if (playerXPos <= SCREEN_CENTERX + xScrollA) {
        player->screenXPos = cameraShakeX + playerXPos - xScrollA;
        xScrollOffset      = xscrollA - cameraShakeX;
    }
    else {
        xScrollOffset      = cameraShakeX + playerXPos - SCREEN_CENTERX;
        player->screenXPos = SCREEN_CENTERX - cameraShakeX;
        if (playerXPos > xscrollB - SCREEN_CENTERX) {
            player->screenXPos = cameraShakeX + SCREEN_CENTERX + playerXPos - (xscrollB - SCREEN_CENTERX);
            xScrollOffset      = xscrollB - SCREEN_XSIZE - cameraShakeX;
        }
    }

    int32 yscrollA   = yScrollA;
    int32 yscrollB   = yScrollB;
    int32 adjustY    = currentCamera->adjustY + playerYPos;
    int32 lookOffset = player->lookPos + adjustY - (yScrollA + LEGACY_SCREEN_SCROLL_UP);
    if (player->trackScroll == 1) {
        yScrollMove = 32;
    }
    else {
        if (yScrollMove == 32) {
            yScrollMove = 2 * ((LEGACY_SCREEN_SCROLL_UP - player->screenYPos - player->lookPos) >> 1);
            if (yScrollMove > 32)
                yScrollMove = 32;
            if (yScrollMove < -32)
                yScrollMove = -32;
        }
        if (yScrollMove > 0)
            yScrollMove -= 6;
        yScrollMove += yScrollMove < 0 ? 6 : 0;
    }

    int32 absLook = abs(lookOffset);
    if (absLook >= abs(yScrollMove) + 17) {
        if (lookOffset <= 0)
            yscrollA -= 16;
        else
            yscrollA += 16;
        yscrollB = yscrollA + SCREEN_YSIZE;
    }
    else if (yScrollMove == 32) {
        if (player->lookPos + adjustY > yscrollA + yScrollMove + LEGACY_SCREEN_SCROLL_UP) {
            yscrollA = player->lookPos + adjustY - (yScrollMove + LEGACY_SCREEN_SCROLL_UP);
            yscrollB = yscrollA + SCREEN_YSIZE;
        }
        if (player->lookPos + adjustY < yscrollA + LEGACY_SCREEN_SCROLL_UP - yScrollMove) {
            yscrollA = player->lookPos + adjustY - (LEGACY_SCREEN_SCROLL_UP - yScrollMove);
            yscrollB = yscrollA + SCREEN_YSIZE;
        }
    }
    else {
        yscrollA = player->lookPos + adjustY + yScrollMove - LEGACY_SCREEN_SCROLL_UP;
        yscrollB = yscrollA + SCREEN_YSIZE;
    }
    if (yscrollA < curYBoundary1) {
        yscrollA = curYBoundary1;
        yscrollB = curYBoundary1 + SCREEN_YSIZE;
    }
    if (yscrollB > curYBoundary2) {
        yscrollB = curYBoundary2;
        yscrollA = curYBoundary2 - SCREEN_YSIZE;
    }
    yScrollA = yscrollA;
    yScrollB = yscrollB;
    if (player->lookPos + adjustY <= yscrollA + LEGACY_SCREEN_SCROLL_UP) {
        player->screenYPos = adjustY - yscrollA - cameraShakeY;
        yScrollOffset      = cameraShakeY + yscrollA;
    }
    else {
        yScrollOffset      = cameraShakeY + adjustY + player->lookPos - LEGACY_SCREEN_SCROLL_UP;
        player->screenYPos = LEGACY_SCREEN_SCROLL_UP - player->lookPos - cameraShakeY;
        if (player->lookPos + adjustY > yscrollB - LEGACY_SCREEN_SCROLL_DOWN) {
            player->screenYPos = adjustY - (yscrollB - LEGACY_SCREEN_SCROLL_DOWN) + cameraShakeY + LEGACY_SCREEN_SCROLL_UP;
            yScrollOffset      = yscrollB - SCREEN_YSIZE - cameraShakeY;
        }
    }
    player->screenYPos -= currentCamera->adjustY;

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
void RSDK::Legacy::v3::SetPlayerLockedScreenPosition(Player *player)
{
    int32 playerXPos = player->XPos >> 16;
    int32 playerYPos = player->YPos >> 16;
    int32 xscrollA   = xScrollA;
    int32 xscrollB   = xScrollB;
    if (playerXPos <= SCREEN_CENTERX + xScrollA) {
        player->screenXPos = cameraShakeX + playerXPos - xScrollA;
        xScrollOffset      = xscrollA - cameraShakeX;
    }
    else {
        xScrollOffset      = cameraShakeX + playerXPos - SCREEN_CENTERX;
        player->screenXPos = SCREEN_CENTERX - cameraShakeX;
        if (playerXPos > xscrollB - SCREEN_CENTERX) {
            player->screenXPos = cameraShakeX + SCREEN_CENTERX + playerXPos - (xscrollB - SCREEN_CENTERX);
            xScrollOffset      = xscrollB - SCREEN_XSIZE - cameraShakeX;
        }
    }

    int32 yscrollA = yScrollA;
    int32 yscrollB = yScrollB;
    int32 adjustY  = currentCamera->adjustY + playerYPos;
    // int32 adjustOffset = player->lookPos + adjustY - (yScrollA + LEGACY_SCREEN_SCROLL_UP);
    if (player->lookPos + adjustY <= yScrollA + LEGACY_SCREEN_SCROLL_UP) {
        player->screenYPos = adjustY - yScrollA - cameraShakeY;
        yScrollOffset      = cameraShakeY + yscrollA;
    }
    else {
        yScrollOffset      = cameraShakeY + adjustY + player->lookPos - LEGACY_SCREEN_SCROLL_UP;
        player->screenYPos = LEGACY_SCREEN_SCROLL_UP - player->lookPos - cameraShakeY;
        if (player->lookPos + adjustY > yscrollB - LEGACY_SCREEN_SCROLL_DOWN) {
            player->screenYPos = adjustY - (yscrollB - LEGACY_SCREEN_SCROLL_DOWN) + cameraShakeY + LEGACY_SCREEN_SCROLL_UP;
            yScrollOffset      = yscrollB - SCREEN_YSIZE - cameraShakeY;
        }
    }
    player->screenYPos -= currentCamera->adjustY;

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
