

bool32 RSDK::Legacy::v4::LoadGameConfig(const char *filepath)
{
    char strBuffer[0x40];
    StrCopy(gameVerInfo.gameName, "Retro-Engine"); // this is the default window name

    globalVariablesCount = 0;
#if RETRO_USE_MOD_LOADER
    // playerCount = 0;
#endif

    FileInfo info;
    InitFileInfo(&info);

    bool32 loaded = LoadFile(&info, filepath, FMODE_RB);
    if (loaded) {
        ReadString(&info, gameVerInfo.gameName);
        ReadString(&info, gameVerInfo.gameSubName);

        uint8 buf[3];
        for (int32 c = 0; c < 0x60; ++c) {
            ReadBytes(&info, buf, 3);
            SetPaletteEntry(-1, c, buf[0], buf[1], buf[2]);
        }

        // Read Obect Names
        byte objectCount = ReadInt8(&info);
        for (int32 o = 0; o < objectCount; ++o) {
            ReadString(&info, strBuffer);
        }

        // Read Script Paths
        for (int32 s = 0; s < objectCount; ++s) {
            ReadString(&info, strBuffer);
        }

        globalVariablesCount = ReadInt8(&info);
        for (int32 v = 0; v < globalVariablesCount; ++v) {
            // Read Variable Name
            ReadString(&info, globalVariables[v].name);

            // Read Variable Value
            globalVariables[v].value = ReadInt8(&info) << 0;
            globalVariables[v].value |= ReadInt8(&info) << 8;
            globalVariables[v].value |= ReadInt8(&info) << 16;
            globalVariables[v].value |= ReadInt8(&info) << 24;
        }

        // Read SFX
        globalSFXCount = ReadInt8(&info);
        for (int32 s = 0; s < globalSFXCount; ++s) { // Sfx Names
            ReadString(&info, strBuffer);
            SetSfxName(strBuffer, s);
        }

        for (int32 s = 0; s < globalSFXCount; ++s) { // Sfx Paths
            ReadString(&info, strBuffer);
            RSDK::Legacy::LoadSfx(strBuffer, s, SCOPE_GLOBAL);
        }

        // Read Player Names
        byte plrCount = ReadInt8(&info);
        for (byte p = 0; p < plrCount; ++p) {
            ReadString(&info, strBuffer);

            // needed for PlayerName[] stuff in scripts
#if !RETRO_USE_ORIGINAL_CODE
            // StrCopy(playerNames[p], strBuffer);
            // playerCount++;
#endif
        }

        // Read ahead and get scene count
        int32 storedPos = info.readPos;

        int32 totalSceneCount   = 0;
        sceneInfo.categoryCount = STAGELIST_MAX;
        for (byte c = 0; c < sceneInfo.categoryCount; ++c) {
            int32 count = ReadInt8(&info);
            for (int32 s = 0; s < count; ++s) {
                ReadString(&info, strBuffer);
                ReadString(&info, strBuffer);
                ReadString(&info, strBuffer);
                ReadInt8(&info);
            }

            totalSceneCount += count;
        }

        if (!totalSceneCount)
            totalSceneCount = 1;

        int32 startScene = 0;
#if !RETRO_USE_ORIGINAL_CODE
        if (strlen(currentSceneFolder) && strlen(currentSceneID)) {
            AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * (totalSceneCount + 1), DATASET_STG, false);
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
            AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * totalSceneCount, DATASET_STG, false);
        }
#else
        AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * totalSceneCount, DATASET_STG, true);
#endif

        AllocateStorage((void **)&sceneInfo.listCategory, sizeof(SceneListInfo) * sceneInfo.categoryCount, DATASET_STG, true);

        char categoryNames[STAGELIST_MAX][0x20] = {
            "Presentation",
            "Regular",
            "Special",
            "Bonus",
        };

        // Jump back and properly load scene info
        Seek_Set(&info, storedPos);

        int32 sceneID = 0;
        for (byte c = 0; c < sceneInfo.categoryCount; ++c) {
            SceneListInfo *category = &sceneInfo.listCategory[c];
            StrCopy(category->name, categoryNames[c]);
            GEN_HASH_MD5(category->name, category->hash);

            category->sceneOffsetStart = sceneID;
            category->sceneCount       = ReadInt8(&info);
            for (int32 s = 0; s < category->sceneCount; ++s) {
                SceneListEntry *scene = &sceneInfo.listData[sceneID + s];

                ReadString(&info, scene->folder);

                ReadString(&info, scene->id);

                ReadString(&info, scene->name);
                GEN_HASH_MD5(scene->name, scene->hash);

                // highlighted (doesn't care)
                ReadInt8(&info);
            }

            category->sceneOffsetEnd = category->sceneOffsetStart + category->sceneCount;
            sceneID += category->sceneCount;
        }

        CloseFile(&info);

        sceneInfo.listPos = startScene;
#if RETRO_USE_MOD_LOADER
        // LoadXMLVariables();
        // LoadXMLPalettes();
        // LoadXMLObjects();
        // LoadXMLPlayers(NULL);
        // LoadXMLStages(NULL, 0);
        
        SetGlobalVariableByName("options.devMenuFlag", false);
        if (engine.devMenu)
            SetGlobalVariableByName("options.devMenuFlag", true);
#endif

        usingBytecode = false;
        if (useDataPack && LoadFile(&info, "Bytecode/GlobalCode.bin", FMODE_RB))
            usingBytecode = true;
    }

    // These need to be set every time its reloaded
    nativeFunctionCount = 0;

    AddNativeFunction("NotifyCallback", NotifyCallback);

    return loaded;
}

void RSDK::Legacy::v4::ProcessEngine()
{
    switch (gameMode) {
        case ENGINE_DEVMENU:
            ProcessInput();
            currentScreen = &screens[0];

            if (devMenu.state)
                devMenu.state();
            break;

        case ENGINE_MAINGAME: ProcessStage(); break;

        case ENGINE_INITDEVMENU:
            LoadGameConfig("Data/Game/GameConfig.bin");
            InitFirstStage();
            ResetCurrentStageFolder();
            break;

        case ENGINE_WAIT:
        case ENGINE_RESETGAME: break;

        case ENGINE_SCRIPTERROR:
            currentScreen = screens;
            FillScreen(0x000000, 0x10, 0x10, 0x10);
            break;

        case ENGINE_INITPAUSE:
        case ENGINE_EXITPAUSE: gameMode = ENGINE_MAINGAME; break;

        case ENGINE_ENDGAME:
        default: break;
    }
}