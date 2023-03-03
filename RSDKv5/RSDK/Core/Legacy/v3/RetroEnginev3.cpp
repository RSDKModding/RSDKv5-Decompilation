
int32 RSDK::Legacy::v3::engineMessage = 0;

bool32 RSDK::Legacy::v3::LoadGameConfig(const char *filepath)
{
    char strBuffer[0x40];
    StrCopy(gameVerInfo.gameTitle, "Retro-Engine"); // this is the default window name

    globalVariablesCount = 0;
#if RETRO_USE_MOD_LOADER
    modSettings.playerCount = 0;
#endif

    int32 gcSceneCount = 0;

    FileInfo info;
    InitFileInfo(&info);

    bool32 loaded = LoadFile(&info, filepath, FMODE_RB);
    if (loaded) {
        ReadString(&info, gameVerInfo.gameTitle);
        ReadString(&info, strBuffer); // Load 'Data'
        ReadString(&info, gameVerInfo.gameSubtitle);

        // Read Obect Names
        uint8 objectCount = ReadInt8(&info);
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
            globalVariables[v].value = ReadInt8(&info) << 24;
            globalVariables[v].value |= ReadInt8(&info) << 16;
            globalVariables[v].value |= ReadInt8(&info) << 8;
            globalVariables[v].value |= ReadInt8(&info) << 0;
        }

        // Read SFX
        globalSFXCount = ReadInt8(&info);
        for (int32 s = 0; s < globalSFXCount; ++s) { // Sfx Paths
            ReadString(&info, strBuffer);
            RSDK::Legacy::LoadSfx(strBuffer, s, SCOPE_GLOBAL);

#if RETRO_USE_MOD_LOADER
            SetSfxName(strBuffer, s, true);
#endif
        }

        // Read Player Names
        uint8 plrCount = ReadInt8(&info);
        for (uint8 p = 0; p < plrCount; ++p) {
            ReadString(&info, strBuffer);

            // needed for PlayerName[] stuff in scripts
#if RETRO_USE_MOD_LOADER
            StrCopy(modSettings.playerNames[p], strBuffer);
            modSettings.playerCount++;
#endif
        }

        // Read ahead and get scene count
        int32 storedPos = info.readPos;

        int32 totalSceneCount   = 0;
        sceneInfo.categoryCount = STAGELIST_MAX;
        for (uint8 c = 0; c < sceneInfo.categoryCount; ++c) {
            int32 count = ReadInt8(&info);
            for (int32 s = 0; s < count; ++s) {
                ReadString(&info, strBuffer);
                ReadString(&info, strBuffer);
                ReadString(&info, strBuffer);
                ReadInt8(&info);
            }

            totalSceneCount += count;
        }

#if RETRO_USE_MOD_LOADER
        gcSceneCount = totalSceneCount;
        totalSceneCount += RSDK::Legacy::v3::LoadXMLStages(2, 0);
#endif
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
            scene->filter = sceneInfo.filter;
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
        for (uint8 c = 0; c < sceneInfo.categoryCount; ++c) {
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
        RSDK::Legacy::v3::LoadXMLVariables();
        RSDK::Legacy::v3::LoadXMLPalettes();
        RSDK::Legacy::v3::LoadXMLObjects();
        RSDK::Legacy::v3::LoadXMLPlayers();
        RSDK::Legacy::v3::LoadXMLStages(0, gcSceneCount);

        SetGlobalVariableByName("options.devMenuFlag", engine.devMenu ? 1 : 0);
#endif

        SetGlobalVariableByName("Engine.PlatformId", gamePlatformID);
        SetGlobalVariableByName("Engine.DeviceType", deviceType);

        usingBytecode = false;
        InitFileInfo(&info);
        if (useDataPack && LoadFile(&info, "Data/Scripts/Bytecode/GlobalCode.bin", FMODE_RB)) {
            usingBytecode = true;
            CloseFile(&info);
        }
    }

    return loaded;
}
void RSDK::Legacy::v3::ProcessEngine()
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

        case ENGINE_WAIT: break;

        case ENGINE_SCRIPTERROR: {
            currentScreen = screens;
            FillScreen(0x000000, 0x10, 0x10, 0x10);

            int32 yOff = DevOutput_GetStringYSize(scriptErrorMessage);
            DrawDevString(scriptErrorMessage, 8, currentScreen->center.y - (yOff >> 1) + 8, 0, 0xF0F0F0);
            break;
        }

        case ENGINE_ENTER_HIRESMODE:
        case ENGINE_EXIT_HIRESMODE: gameMode = ENGINE_MAINGAME; break;

        case ENGINE_VIDEOWAIT:
            ProcessInput();
            ProcessVideo();
            break;

        case ENGINE_PAUSE: break;

        default: break;
    }
}

enum RetroEngineCallbacks {
    CALLBACK_DISPLAYLOGOS            = 0,
    CALLBACK_PRESS_START             = 1,
    CALLBACK_TIMEATTACK_NOTIFY_ENTER = 2,
    CALLBACK_TIMEATTACK_NOTIFY_EXIT  = 3,
    CALLBACK_FINISHGAME_NOTIFY       = 4,
    CALLBACK_RETURNSTORE_SELECTED    = 5,
    CALLBACK_RESTART_SELECTED        = 6,
    CALLBACK_EXIT_SELECTED           = 7,
    CALLBACK_BUY_FULL_GAME_SELECTED  = 8,
    CALLBACK_TERMS_SELECTED          = 9,
    CALLBACK_PRIVACY_SELECTED        = 10,
    CALLBACK_TRIAL_ENDED             = 11,
    CALLBACK_SETTINGS_SELECTED       = 12,
    CALLBACK_PAUSE_REQUESTED         = 13,
    CALLBACK_FULL_VERSION_ONLY       = 14,
    CALLBACK_STAFF_CREDITS           = 15,
    CALLBACK_MOREGAMES               = 16,
    CALLBACK_SHOWREMOVEADS           = 20,
    CALLBACK_AGEGATE                 = 100,

    // Sonic Origins Notify Callbacks
    NOTIFY_DEATH_EVENT        = 128,
    NOTIFY_TOUCH_SIGNPOST     = 129,
    NOTIFY_HUD_ENABLE         = 130,
    NOTIFY_ADD_COIN           = 131,
    NOTIFY_KILL_ENEMY         = 132,
    NOTIFY_SAVESLOT_SELECT    = 133,
    NOTIFY_FUTURE_PAST        = 134,
    NOTIFY_GOTO_FUTURE_PAST   = 135,
    NOTIFY_BOSS_END           = 136,
    NOTIFY_SPECIAL_END        = 137,
    NOTIFY_DEBUGPRINT         = 138,
    NOTIFY_KILL_BOSS          = 139,
    NOTIFY_TOUCH_EMERALD      = 140,
    NOTIFY_STATS_ENEMY        = 141,
    NOTIFY_STATS_CHARA_ACTION = 142,
    NOTIFY_STATS_RING         = 143,
    NOTIFY_STATS_MOVIE        = 144,
    NOTIFY_STATS_PARAM_1      = 145,
    NOTIFY_STATS_PARAM_2      = 146,
    NOTIFY_CHARACTER_SELECT   = 147,
    NOTIFY_SPECIAL_RETRY      = 148,
    NOTIFY_TOUCH_CHECKPOINT   = 149,
    NOTIFY_ACT_FINISH         = 150,
    NOTIFY_1P_VS_SELECT       = 151,
    NOTIFY_CONTROLLER_SUPPORT = 152,
    NOTIFY_STAGE_RETRY        = 153,
    NOTIFY_SOUND_TRACK        = 154,
    NOTIFY_GOOD_ENDING        = 155,
    NOTIFY_BACK_TO_MAINMENU   = 156,
    NOTIFY_LEVEL_SELECT_MENU  = 157,
    NOTIFY_PLAYER_SET         = 158,
    NOTIFY_EXTRAS_MODE        = 159,
    NOTIFY_SPIN_DASH_TYPE     = 160,
    NOTIFY_TIME_OVER          = 161,

    // Sega Forever stuff
    // Mod CBs start at about 1000
    CALLBACK_SHOWMENU_2                       = 997,
    CALLBACK_SHOWHELPCENTER                   = 998,
    CALLBACK_CHANGEADSTYPE                    = 999,
    CALLBACK_NONE_1000                        = 1000,
    CALLBACK_NONE_1001                        = 1001,
    CALLBACK_NONE_1006                        = 1002,
    CALLBACK_ONSHOWINTERSTITIAL               = 1003,
    CALLBACK_ONSHOWBANNER                     = 1004,
    CALLBACK_ONSHOWBANNER_PAUSESTART          = 1005,
    CALLBACK_ONHIDEBANNER                     = 1006,
    CALLBACK_REMOVEADSBUTTON_FADEOUT          = 1007,
    CALLBACK_REMOVEADSBUTTON_FADEIN           = 1008,
    CALLBACK_ONSHOWINTERSTITIAL_2             = 1009,
    CALLBACK_ONSHOWINTERSTITIAL_3             = 1010,
    CALLBACK_ONSHOWINTERSTITIAL_4             = 1011,
    CALLBACK_ONVISIBLEGRIDBTN_1               = 1012,
    CALLBACK_ONVISIBLEGRIDBTN_0               = 1013,
    CALLBACK_ONSHOWINTERSTITIAL_PAUSEDURATION = 1014,
    CALLBACK_SHOWCOUNTDOWNMENU                = 1015,
    CALLBACK_ONVISIBLEMAINMENU_1              = 1016,
    CALLBACK_ONVISIBLEMAINMENU_0              = 1017,

#if RETRO_USE_MOD_LOADER
    // Mod CBs start at 0x1000
    CALLBACK_SET1P = 0x1001,
    CALLBACK_SET2P = 0x1002,
#endif
};

void RSDK::Legacy::v3::RetroEngineCallback(int32 callbackID)
{
    // Sonic Origins Params
    int32 notifyParam1 = GetGlobalVariableByName("game.callbackParam0");
    // int32 notifyParam2 = GetGlobalVariableByName("game.callbackParam1");
    // int32 notifyParam3 = GetGlobalVariableByName("game.callbackParam2");
    // int32 notifyParam4 = GetGlobalVariableByName("game.callbackParam3");

    switch (callbackID) {
        default: PrintLog(PRINT_NORMAL, "Callback: Unknown (%d)", callbackID); break;
        case CALLBACK_DISPLAYLOGOS: PrintLog(PRINT_NORMAL, "Callback: Display Logos"); break;
        case CALLBACK_PRESS_START: PrintLog(PRINT_NORMAL, "Callback: Press Start"); break;
        case CALLBACK_TIMEATTACK_NOTIFY_ENTER: PrintLog(PRINT_NORMAL, "Callback: Time Attack Notify Enter"); break;
        case CALLBACK_TIMEATTACK_NOTIFY_EXIT: PrintLog(PRINT_NORMAL, "Callback: Time Attack Notify Exit"); break;
        case CALLBACK_FINISHGAME_NOTIFY: PrintLog(PRINT_NORMAL, "Callback: Finish Game Notify"); break;
        case CALLBACK_RETURNSTORE_SELECTED:
            gameMode = ENGINE_EXITGAME;
            PrintLog(PRINT_NORMAL, "Callback: Return To Store Selected");
            break;
        case CALLBACK_RESTART_SELECTED:
            PrintLog(PRINT_NORMAL, "Callback: Restart Selected");
            stageMode = STAGEMODE_LOAD;
            break;
        case CALLBACK_EXIT_SELECTED:
            sceneInfo.activeCategory = 0;
            sceneInfo.listPos        = 0;
            stageMode                = STAGEMODE_LOAD;
            break;
        case CALLBACK_BUY_FULL_GAME_SELECTED:
            gameMode = ENGINE_EXITGAME;
            PrintLog(PRINT_NORMAL, "Callback: Buy Full Game Selected");
            break;
        case CALLBACK_TERMS_SELECTED: PrintLog(PRINT_NORMAL, "Callback: PC = How to play Menu, Mobile = Terms & Conditions Screen"); break;
        case CALLBACK_PRIVACY_SELECTED: PrintLog(PRINT_NORMAL, "Callback: PC = Controls Menu, Mobile = Privacy Screen"); break;
        case CALLBACK_TRIAL_ENDED:
            if (trialMode) {
                PrintLog(PRINT_NORMAL, "Callback: Trial Ended Screen Requested");
            }
            else {
                PrintLog(PRINT_NORMAL, "Callback: Sega Website Requested");
            }
            break;
        case CALLBACK_SETTINGS_SELECTED:
            if (trialMode)
                PrintLog(PRINT_NORMAL, "Callback: Full Game Only Requested");
            else
                PrintLog(PRINT_NORMAL, "Callback: Terms Requested");
            break;
        case CALLBACK_PAUSE_REQUESTED: // PC/Mobile = Pause Requested (Mobile uses in-game menu, PC does as well if devMenu is active)
            PrintLog(PRINT_NORMAL, "Callback: Pause Menu Requested");
            break;
        case CALLBACK_FULL_VERSION_ONLY: PrintLog(PRINT_NORMAL, "Callback: Full Version Only Notify"); break;
        case CALLBACK_STAFF_CREDITS: PrintLog(PRINT_NORMAL, "Callback: Privacy Requested"); break;
        case CALLBACK_MOREGAMES: PrintLog(PRINT_NORMAL, "Callback: Show More Games"); break;

        case CALLBACK_SHOWREMOVEADS: PrintLog(PRINT_NORMAL, "Callback: Show Remove Ads"); break;

        case CALLBACK_AGEGATE:
            PrintLog(PRINT_NORMAL, "Callback: Age Gate");
            // Newer versions of the game wont continue without this
            // Thanks to Sappharad for pointing this out
            SetGlobalVariableByName("HaveLoadAllGDPRValue", 1);
            break;

        // Sonic Origins
        case NOTIFY_DEATH_EVENT: PrintLog(PRINT_NORMAL, "NOTIFY: DeathEvent() -> %d", notifyParam1); break;
        case NOTIFY_TOUCH_SIGNPOST: PrintLog(PRINT_NORMAL, "NOTIFY: TouchSignPost() -> %d", notifyParam1); break;
        case NOTIFY_HUD_ENABLE: PrintLog(PRINT_NORMAL, "NOTIFY: HUDEnable() -> %d", notifyParam1); break;
        case NOTIFY_ADD_COIN: PrintLog(PRINT_NORMAL, "NOTIFY: AddCoin() -> %d", notifyParam1); break;
        case NOTIFY_KILL_ENEMY: PrintLog(PRINT_NORMAL, "NOTIFY: KillEnemy() -> %d", notifyParam1); break;
        case NOTIFY_SAVESLOT_SELECT: PrintLog(PRINT_NORMAL, "NOTIFY: SaveSlotSelect() -> %d", notifyParam1); break;
        case NOTIFY_FUTURE_PAST:
            PrintLog(PRINT_NORMAL, "NOTIFY: FuturePast() -> %d", notifyParam1);
            objectEntityList[objectLoop].state++;
            break;
        case NOTIFY_GOTO_FUTURE_PAST: PrintLog(PRINT_NORMAL, "NOTIFY: GotoFuturePast() -> %d", notifyParam1); break;
        case NOTIFY_BOSS_END: PrintLog(PRINT_NORMAL, "NOTIFY: BossEnd() -> %d", notifyParam1); break;
        case NOTIFY_SPECIAL_END: PrintLog(PRINT_NORMAL, "NOTIFY: SpecialEnd() -> %d", notifyParam1); break;
        case NOTIFY_DEBUGPRINT: PrintLog(PRINT_NORMAL, "NOTIFY: DebugPrint() -> %d", notifyParam1); break;
        case NOTIFY_KILL_BOSS: PrintLog(PRINT_NORMAL, "NOTIFY: KillBoss() -> %d", notifyParam1); break;
        case NOTIFY_TOUCH_EMERALD: PrintLog(PRINT_NORMAL, "NOTIFY: TouchEmerald() -> %d", notifyParam1); break;
        case NOTIFY_STATS_ENEMY: PrintLog(PRINT_NORMAL, "NOTIFY: StatsEnemy() -> %d", notifyParam1); break;
        case NOTIFY_STATS_CHARA_ACTION: PrintLog(PRINT_NORMAL, "NOTIFY: StatsCharaAction() -> %d", notifyParam1); break;
        case NOTIFY_STATS_RING: PrintLog(PRINT_NORMAL, "NOTIFY: StatsRing() -> %d", notifyParam1); break;
        case NOTIFY_STATS_MOVIE:
            PrintLog(PRINT_NORMAL, "NOTIFY: StatsMovie() -> %d", notifyParam1);
            sceneInfo.activeCategory = 0;
            sceneInfo.listPos        = 0;
            gameMode                 = ENGINE_MAINGAME;
            stageMode                = STAGEMODE_LOAD;
            break;
        case NOTIFY_STATS_PARAM_1: PrintLog(PRINT_NORMAL, "NOTIFY: StatsParam1() -> %d", notifyParam1); break;
        case NOTIFY_STATS_PARAM_2: PrintLog(PRINT_NORMAL, "NOTIFY: StatsParam2() -> %d", notifyParam1); break;
        case NOTIFY_CHARACTER_SELECT:
            PrintLog(PRINT_NORMAL, "NOTIFY: CharacterSelect() -> %d", notifyParam1);
            SetGlobalVariableByName("game.callbackResult", 1);
            SetGlobalVariableByName("game.continueFlag", 0);
            break;
        case NOTIFY_SPECIAL_RETRY: SetGlobalVariableByName("game.callbackResult", 1); break;
        case NOTIFY_TOUCH_CHECKPOINT: PrintLog(PRINT_NORMAL, "NOTIFY: TouchCheckpoint() -> %d", notifyParam1); break;
        case NOTIFY_ACT_FINISH: PrintLog(PRINT_NORMAL, "NOTIFY: ActFinish() -> %d", notifyParam1); break;
        case NOTIFY_1P_VS_SELECT: PrintLog(PRINT_NORMAL, "NOTIFY: 1PVSSelect() -> %d", notifyParam1); break;
        case NOTIFY_CONTROLLER_SUPPORT: PrintLog(PRINT_NORMAL, "NOTIFY: ControllerSupport() -> %d", notifyParam1); break;
        case NOTIFY_STAGE_RETRY: PrintLog(PRINT_NORMAL, "NOTIFY: StageRetry() -> %d", notifyParam1); break;
        case NOTIFY_SOUND_TRACK: PrintLog(PRINT_NORMAL, "NOTIFY: SoundTrack() -> %d", notifyParam1); break;
        case NOTIFY_GOOD_ENDING: PrintLog(PRINT_NORMAL, "NOTIFY: GoodEnding() -> %d", notifyParam1); break;
        case NOTIFY_BACK_TO_MAINMENU: PrintLog(PRINT_NORMAL, "NOTIFY: BackToMainMenu() -> %d", notifyParam1); break;
        case NOTIFY_LEVEL_SELECT_MENU: PrintLog(PRINT_NORMAL, "NOTIFY: LevelSelectMenu() -> %d", notifyParam1); break;
        case NOTIFY_PLAYER_SET: PrintLog(PRINT_NORMAL, "NOTIFY: PlayerSet() -> %d", notifyParam1); break;
        case NOTIFY_EXTRAS_MODE: PrintLog(PRINT_NORMAL, "NOTIFY: ExtrasMode() -> %d", notifyParam1); break;
        case NOTIFY_SPIN_DASH_TYPE: PrintLog(PRINT_NORMAL, "NOTIFY: SpindashType() -> %d", notifyParam1); break;
        case NOTIFY_TIME_OVER: PrintLog(PRINT_NORMAL, "NOTIFY: TimeOver() -> %d", notifyParam1); break;

        // Sega Forever stuff
        case CALLBACK_SHOWMENU_2: PrintLog(PRINT_NORMAL, "Callback: showMenu(2)"); break;
        case CALLBACK_SHOWHELPCENTER: PrintLog(PRINT_NORMAL, "Callback: Show Help Center"); break;
        case CALLBACK_CHANGEADSTYPE: PrintLog(PRINT_NORMAL, "Callback: Change Ads Type"); break;
        case CALLBACK_NONE_1000:
        case CALLBACK_NONE_1001:
        case CALLBACK_NONE_1006: PrintLog(PRINT_NORMAL, "Callback: Unknown - %d", callbackID); break;
        case CALLBACK_ONSHOWINTERSTITIAL: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(2, 0) - Pause_Duration"); break;
        case CALLBACK_ONSHOWBANNER: PrintLog(PRINT_NORMAL, "Callback: onShowBanner()"); break;
        case CALLBACK_ONSHOWBANNER_PAUSESTART: PrintLog(PRINT_NORMAL, "Callback: onShowBanner() - Pause_Start"); break;
        case CALLBACK_ONHIDEBANNER: PrintLog(PRINT_NORMAL, "Callback: onHideBanner()"); break;
        case CALLBACK_REMOVEADSBUTTON_FADEOUT: PrintLog(PRINT_NORMAL, "Callback: RemoveAdsButton_FadeOut()"); break;
        case CALLBACK_REMOVEADSBUTTON_FADEIN: PrintLog(PRINT_NORMAL, "Callback: RemoveAdsButton_FadeIn()"); break;
        case CALLBACK_ONSHOWINTERSTITIAL_2:
        case CALLBACK_ONSHOWINTERSTITIAL_3: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(0, 0)"); break;
        case CALLBACK_ONSHOWINTERSTITIAL_4: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(1, 0)"); break;
        case CALLBACK_ONVISIBLEGRIDBTN_1: PrintLog(PRINT_NORMAL, "Callback: onVisibleGridBtn(1)"); break;
        case CALLBACK_ONVISIBLEGRIDBTN_0: PrintLog(PRINT_NORMAL, "Callback: onVisibleGridBtn(0)"); break;
        case CALLBACK_ONSHOWINTERSTITIAL_PAUSEDURATION: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(0, 0) - Pause_Duration"); break;
        case CALLBACK_SHOWCOUNTDOWNMENU: PrintLog(PRINT_NORMAL, "Callback: showCountDownMenu(0)"); break;
        case CALLBACK_ONVISIBLEMAINMENU_1: PrintLog(PRINT_NORMAL, "Callback: onVisibleMainMenu(1)"); break;
        case CALLBACK_ONVISIBLEMAINMENU_0:
            PrintLog(PRINT_NORMAL, "Callback: OnVisibleMainMenu(0)");
            break;

            // Mod loader Only
#if RETRO_USE_MOD_LOADER
        case CALLBACK_SET1P: activePlayerCount = 1; break;
        case CALLBACK_SET2P: activePlayerCount = 2; break;
#endif
    }
}

#if RETRO_USE_MOD_LOADER
void RSDK::Legacy::v3::LoadXMLVariables()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement      = doc->FirstChildElement("game");
                const tinyxml2::XMLElement *variablesElement = gameElement->FirstChildElement("variables");
                if (variablesElement) {
                    const tinyxml2::XMLElement *varElement = variablesElement->FirstChildElement("variable");
                    if (varElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = varElement->FindAttribute("name");
                            const char *varName                    = "unknownVariable";
                            if (nameAttr)
                                varName = nameAttr->Value();

                            const tinyxml2::XMLAttribute *valAttr = varElement->FindAttribute("value");
                            int32 varValue                        = 0;
                            if (valAttr)
                                varValue = valAttr->IntValue();

                            StrCopy(globalVariables[globalVariablesCount].name, varName);
                            globalVariables[globalVariablesCount].value = varValue;
                            globalVariablesCount++;

                        } while ((varElement = varElement->NextSiblingElement("variable")));
                    }
                }
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v3::LoadXMLPalettes()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement    = doc->FirstChildElement("game");
                const tinyxml2::XMLElement *paletteElement = gameElement->FirstChildElement("palette");
                if (paletteElement) {
                    const tinyxml2::XMLElement *clrElement = paletteElement->FirstChildElement("color");
                    if (clrElement) {
                        do {
                            const tinyxml2::XMLAttribute *bankAttr = clrElement->FindAttribute("bank");
                            int32 clrBank                          = 0;
                            if (bankAttr)
                                clrBank = bankAttr->IntValue();

                            const tinyxml2::XMLAttribute *indAttr = clrElement->FindAttribute("index");
                            int32 clrInd                          = 0;
                            if (indAttr)
                                clrInd = indAttr->IntValue();

                            const tinyxml2::XMLAttribute *rAttr = clrElement->FindAttribute("r");
                            int32 clrR                          = 0;
                            if (rAttr)
                                clrR = rAttr->IntValue();

                            const tinyxml2::XMLAttribute *gAttr = clrElement->FindAttribute("g");
                            int32 clrG                          = 0;
                            if (gAttr)
                                clrG = gAttr->IntValue();

                            const tinyxml2::XMLAttribute *bAttr = clrElement->FindAttribute("b");
                            int32 clrB                          = 0;
                            if (bAttr)
                                clrB = bAttr->IntValue();

                            SetPaletteEntry(clrBank, clrInd, clrR, clrG, clrB);

                        } while ((clrElement = clrElement->NextSiblingElement("color")));
                    }
                }
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v3::LoadXMLObjects()
{
    FileInfo info;
    modObjCount = 0;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement    = doc->FirstChildElement("game");
                const tinyxml2::XMLElement *objectsElement = gameElement->FirstChildElement("objects");
                if (objectsElement) {
                    const tinyxml2::XMLElement *objElement = objectsElement->FirstChildElement("object");
                    if (objElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = objElement->FindAttribute("name");
                            const char *objName                    = "unknownObject";
                            if (nameAttr)
                                objName = nameAttr->Value();

                            const tinyxml2::XMLAttribute *scrAttr = objElement->FindAttribute("script");
                            const char *objScript                 = "unknownObject.txt";
                            if (scrAttr)
                                objScript = scrAttr->Value();

                            uint8 flags = 0;

                            // forces the object to be loaded, this means the object doesn't have to be and *SHOULD NOT* be in the stage object list
                            // if it is, it'll cause issues!!!!
                            const tinyxml2::XMLAttribute *loadAttr = objElement->FindAttribute("forceLoad");
                            int32 objForceLoad                     = false;
                            if (loadAttr)
                                objForceLoad = loadAttr->BoolValue();

                            flags |= (objForceLoad & 1);

                            StrCopy(modTypeNames[modObjCount], objName);
                            StrCopy(modScriptPaths[modObjCount], objScript);
                            modScriptFlags[modObjCount] = flags;
                            modObjCount++;

                        } while ((objElement = objElement->NextSiblingElement("object")));
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v3::LoadXMLSoundFX()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement   = doc->FirstChildElement("game");
                const tinyxml2::XMLElement *soundsElement = gameElement->FirstChildElement("sounds");
                if (soundsElement) {
                    const tinyxml2::XMLElement *sfxElement = soundsElement->FirstChildElement("soundfx");
                    if (sfxElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = sfxElement->FindAttribute("name");
                            const char *sfxName                    = "unknownSFX";
                            if (nameAttr)
                                sfxName = nameAttr->Value();

                            const tinyxml2::XMLAttribute *valAttr = sfxElement->FindAttribute("path");
                            const char *sfxPath                   = "unknownSFX.wav";
                            if (valAttr)
                                sfxPath = valAttr->Value();

                            SetSfxName(sfxName, globalSFXCount, true);

                            RSDK::Legacy::LoadSfx((char *)sfxPath, globalSFXCount, SCOPE_GLOBAL);
                            globalSFXCount++;

                        } while ((sfxElement = sfxElement->NextSiblingElement("soundfx")));
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v3::LoadXMLPlayers()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement    = doc->FirstChildElement("game");
                const tinyxml2::XMLElement *playersElement = gameElement->FirstChildElement("players");
                if (playersElement) {
                    const tinyxml2::XMLElement *plrElement = playersElement->FirstChildElement("player");
                    if (plrElement) {
                        do {
                            const tinyxml2::XMLAttribute *nameAttr = plrElement->FindAttribute("name");
                            const char *plrName                    = "unknownPlayer";
                            if (nameAttr)
                                plrName = nameAttr->Value();

                            StrCopy(modSettings.playerNames[modSettings.playerCount++], plrName);

                        } while ((plrElement = plrElement->NextSiblingElement("player")));
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
int32 RSDK::Legacy::v3::LoadXMLStages(int32 mode, int32 gcStageCount)
{
    FileInfo info;
    int32 stageCount = 0;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement = doc->FirstChildElement("game");
                const char *elementNames[]              = { "presentationStages", "regularStages", "bonusStages", "specialStages" };
                const char *categoryNames[]             = {
                    "Presentation",
                    "Regular",
                    "Special",
                    "Bonus",
                };

                for (int32 l = 0; l < sceneInfo.categoryCount; ++l) {
                    const tinyxml2::XMLElement *listElement = gameElement->FirstChildElement(elementNames[l]);
                    if (listElement) {
                        const tinyxml2::XMLElement *stgElement = listElement->FirstChildElement("stage");

                        SceneListInfo *list = &sceneInfo.listCategory[l];
                        if (!mode) {
                            sprintf_s(list->name, sizeof(list->name), "%s", categoryNames[l]);
                            GEN_HASH_MD5(list->name, list->hash);

                            list->sceneOffsetStart = gcStageCount;
                            list->sceneOffsetEnd   = gcStageCount;
                            list->sceneCount       = 0;
                        }

                        if (stgElement) {
                            do {
                                if (!mode) {
                                    const tinyxml2::XMLAttribute *nameAttr = stgElement->FindAttribute("name");
                                    const char *stgName                    = "unknownStage";
                                    if (nameAttr)
                                        stgName = nameAttr->Value();

                                    const tinyxml2::XMLAttribute *folderAttr = stgElement->FindAttribute("folder");
                                    const char *stgFolder                    = "unknownStageFolder";
                                    if (nameAttr)
                                        stgFolder = folderAttr->Value();

                                    const tinyxml2::XMLAttribute *idAttr = stgElement->FindAttribute("id");
                                    const char *stgID                    = "unknownStageID";
                                    if (idAttr)
                                        stgID = idAttr->Value();

                                    SceneListEntry *scene = &sceneInfo.listData[gcStageCount];

                                    sprintf_s(scene->name, sizeof(scene->name), "%s", stgName);
                                    GEN_HASH_MD5(scene->name, scene->hash);
                                    sprintf_s(scene->folder, sizeof(scene->folder), "%s", stgFolder);
                                    sprintf_s(scene->id, sizeof(scene->id), "%s", stgID);

                                    scene->filter = 0xFF;

                                    gcStageCount++;
                                    sceneInfo.listCategory[l].sceneCount++;
                                }
                                ++stageCount;
                            } while ((stgElement = stgElement->NextSiblingElement("stage")));
                        }

                        if (!mode) {
                            list->sceneOffsetEnd += list->sceneCount;
                        }
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }

    SetActiveMod(-1);

    return stageCount;
}
#endif
