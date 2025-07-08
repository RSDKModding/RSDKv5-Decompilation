#include "RetroEnginev3.hpp"

int32 RSDK::Legacy::v3::engineMessage = 0;

bool32 RSDK::Legacy::v3::LoadGameConfig(const char *filepath)
{
    char strBuffer[0x40];
    StrCopy(gameVerInfo.gameTitle, "Retro-Engine"); // this is the default window name

    globalVariablesCount = 0;
#if RETRO_USE_MOD_LOADER
    modSettings.playerCount = 0;
#endif

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

#if RETRO_USE_MOD_LOADER
            // needed for PlayerName[] stuff in scripts
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

        if (!totalSceneCount)
            totalSceneCount = 1;

        int32 startScene = 0;
#if !RETRO_USE_ORIGINAL_CODE
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
            scene->filter = sceneInfo.filter;
            GEN_HASH_MD5(scene->name, scene->hash);

            // Override existing values
            sceneInfo.activeCategory = 0;
            startScene               = totalSceneCount;
            currentSceneFolder[0]    = 0;
            currentSceneID[0]        = 0;
        }
        else {
#endif

#if RETRO_USE_MOD_LOADER
            listData.resize(totalSceneCount);
            sceneInfo.listData = listData.data();
#else
        AllocateStorage((void **)&sceneInfo.listData, sizeof(SceneListEntry) * totalSceneCount, DATASET_STG, false);
#endif //! RETRO_USE_MOD_LOADER

#if !RETRO_USE_ORIGINAL_CODE
        }
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

            category->sceneOffsetEnd = category->sceneOffsetStart + category->sceneCount - 1;
            sceneID += category->sceneCount;
        }

        CloseFile(&info);

        sceneInfo.listPos = startScene;

#if RETRO_USE_MOD_LOADER
        v3::LoadGameXML();
        SetGlobalVariableByName("Options.DevMenuFlag", engine.devMenu ? 1 : 0);
        SetGlobalVariableByName("Engine.Standalone", 0);
        SetGlobalVariableByName("game.hasPlusDLC", !RSDK_AUTOBUILD);
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

            ProcessInput();
            if (controller[CONT_ANY].keyStart.press || controller[CONT_ANY].keyA.press) {
                OpenDevMenu();
            }
            else if (controller[CONT_ANY].keyB.press) {
                ResetCurrentStageFolder();
                sceneInfo.activeCategory = 0;
                gameMode                 = ENGINE_MAINGAME;
                stageMode                = STAGEMODE_LOAD;
                sceneInfo.listPos        = 0;
            }
            else if (controller[CONT_ANY].keyC.press) {
                ResetCurrentStageFolder();
#if RETRO_USE_MOD_LOADER
                RefreshModFolders();
#endif
                gameMode  = ENGINE_MAINGAME;
                stageMode = STAGEMODE_LOAD;
            }
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
    NOTIFY_DEATH_EVENT         = 128,
    NOTIFY_TOUCH_SIGNPOST      = 129,
    NOTIFY_HUD_ENABLE          = 130,
    NOTIFY_ADD_COIN            = 131,
    NOTIFY_KILL_ENEMY          = 132,
    NOTIFY_SAVESLOT_SELECT     = 133,
    NOTIFY_FUTURE_PAST         = 134,
    NOTIFY_GOTO_FUTURE_PAST    = 135,
    NOTIFY_BOSS_END            = 136,
    NOTIFY_SPECIAL_END         = 137,
    NOTIFY_DEBUGPRINT          = 138,
    NOTIFY_KILL_BOSS           = 139,
    NOTIFY_TOUCH_EMERALD       = 140,
    NOTIFY_STATS_ENEMY         = 141,
    NOTIFY_STATS_CHARA_ACTION  = 142,
    NOTIFY_STATS_RING          = 143,
    NOTIFY_STATS_MOVIE         = 144,
    NOTIFY_STATS_PARAM_1       = 145,
    NOTIFY_STATS_PARAM_2       = 146,
    NOTIFY_CHARACTER_SELECT    = 147,
    NOTIFY_SPECIAL_RETRY       = 148,
    NOTIFY_TOUCH_CHECKPOINT    = 149,
    NOTIFY_ACT_FINISH          = 150,
    NOTIFY_1P_VS_SELECT        = 151,
    NOTIFY_CONTROLLER_SUPPORT  = 152,
    NOTIFY_STAGE_RETRY         = 153,
    NOTIFY_SOUND_TRACK         = 154,
    NOTIFY_GOOD_ENDING         = 155,
    NOTIFY_BACK_TO_MAINMENU    = 156,
    NOTIFY_LEVEL_SELECT_MENU   = 157,
    NOTIFY_PLAYER_SET          = 158,
    NOTIFY_EXTRAS_MODE         = 159,
    NOTIFY_SPIN_DASH_TYPE      = 160,
    NOTIFY_TIME_OVER           = 161,
    NOTIFY_TIMEATTACK_MODE     = 162,
    NOTIFY_STATS_BREAK_OBJECT  = 163,
    NOTIFY_STATS_SAVE_FUTURE   = 164,
    NOTIFY_STATS_CHARA_ACTION2 = 165,

    // Sega Forever stuff
    // Mod CBs start at about 1000
    CALLBACK_STARTGAME                        = 101,
    CALLBACK_NONE_104                         = 104,
    CALLBACK_SHOWURL                          = 107,
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
    CALLBACK_ONSHOWREWARDADS                  = 1018,
    CALLBACK_ONSHOWBANNER_2                   = 1019,
    CALLBACK_ONSHOWINTERSTITIAL_5             = 1020,

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
    int32 notifyParam2 = GetGlobalVariableByName("game.callbackParam1");
    int32 notifyParam3 = GetGlobalVariableByName("game.callbackParam2");

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
        case NOTIFY_ADD_COIN:
            PrintLog(PRINT_NORMAL, "NOTIFY: AddCoin() -> %d", notifyParam1);
            SetGlobalVariableByName("game.coinCount", GetGlobalVariableByName("game.coinCount") + notifyParam1);
            break;
        case NOTIFY_KILL_ENEMY: PrintLog(PRINT_NORMAL, "NOTIFY: KillEnemy() -> %d", notifyParam1); break;
        case NOTIFY_SAVESLOT_SELECT: PrintLog(PRINT_NORMAL, "NOTIFY: SaveSlotSelect() -> %d", notifyParam1); break;
        case NOTIFY_FUTURE_PAST:
            PrintLog(PRINT_NORMAL, "NOTIFY: FuturePast() -> %d", notifyParam1);
            objectEntityList[objectLoop].state++;
            break;
        case NOTIFY_GOTO_FUTURE_PAST: PrintLog(PRINT_NORMAL, "NOTIFY: GotoFuturePast() -> %d", notifyParam1); break;
        case NOTIFY_BOSS_END: PrintLog(PRINT_NORMAL, "NOTIFY: BossEnd() -> %d", notifyParam1); break;
        case NOTIFY_SPECIAL_END: PrintLog(PRINT_NORMAL, "NOTIFY: SpecialEnd() -> %d", notifyParam1); break;
        case NOTIFY_DEBUGPRINT: PrintLog(PRINT_NORMAL, "NOTIFY: DebugPrint() -> %d, %d, %d", notifyParam1, notifyParam2, notifyParam3); break;
        case NOTIFY_KILL_BOSS: PrintLog(PRINT_NORMAL, "NOTIFY: KillBoss() -> %d", notifyParam1); break;
        case NOTIFY_TOUCH_EMERALD: PrintLog(PRINT_NORMAL, "NOTIFY: TouchEmerald() -> %d", notifyParam1); break;
        case NOTIFY_STATS_ENEMY: PrintLog(PRINT_NORMAL, "NOTIFY: StatsEnemy() -> %d, %d, %d", notifyParam1, notifyParam2, notifyParam3); break;
        case NOTIFY_STATS_CHARA_ACTION: PrintLog(PRINT_NORMAL, "NOTIFY: StatsCharaAction() -> %d, %d, %d", notifyParam1, notifyParam2, notifyParam3); break;
        case NOTIFY_STATS_RING: PrintLog(PRINT_NORMAL, "NOTIFY: StatsRing() -> %d", notifyParam1); break;
        case NOTIFY_STATS_MOVIE:
            PrintLog(PRINT_NORMAL, "NOTIFY: StatsMovie() -> %d", notifyParam1);
            sceneInfo.activeCategory = 0;
            sceneInfo.listPos        = 0;
            gameMode                 = ENGINE_MAINGAME;
            stageMode                = STAGEMODE_LOAD;
            break;
        case NOTIFY_STATS_PARAM_1: PrintLog(PRINT_NORMAL, "NOTIFY: StatsParam1() -> %d, %d, %d", notifyParam1, notifyParam2, notifyParam3); break;
        case NOTIFY_STATS_PARAM_2: PrintLog(PRINT_NORMAL, "NOTIFY: StatsParam2() -> %d", notifyParam1); break;
        case NOTIFY_CHARACTER_SELECT:
            PrintLog(PRINT_NORMAL, "NOTIFY: CharacterSelect() -> %d", notifyParam1);
            SetGlobalVariableByName("game.callbackResult", 1);
            SetGlobalVariableByName("game.continueFlag", 0);
            break;
        case NOTIFY_SPECIAL_RETRY:
            PrintLog(PRINT_NORMAL, "NOTIFY: SpecialRetry() -> %d, %d, %d", notifyParam1, notifyParam2, notifyParam3);
            SetGlobalVariableByName("game.callbackResult", 1);
            break;
        case NOTIFY_TOUCH_CHECKPOINT: PrintLog(PRINT_NORMAL, "NOTIFY: TouchCheckpoint() -> %d", notifyParam1); break;
        case NOTIFY_ACT_FINISH: PrintLog(PRINT_NORMAL, "NOTIFY: ActFinish() -> %d", notifyParam1); break;
        case NOTIFY_1P_VS_SELECT: PrintLog(PRINT_NORMAL, "NOTIFY: 1PVSSelect() -> %d", notifyParam1); break;
        case NOTIFY_CONTROLLER_SUPPORT:
            PrintLog(PRINT_NORMAL, "NOTIFY: ControllerSupport() -> %d", notifyParam1);
            SetGlobalVariableByName("game.callbackResult", 1);
            break;
        case NOTIFY_STAGE_RETRY: PrintLog(PRINT_NORMAL, "NOTIFY: StageRetry() -> %d, %d, %d", notifyParam1, notifyParam2, notifyParam3); break;
        case NOTIFY_SOUND_TRACK: PrintLog(PRINT_NORMAL, "NOTIFY: SoundTrack() -> %d", notifyParam1); break;
        case NOTIFY_GOOD_ENDING: PrintLog(PRINT_NORMAL, "NOTIFY: GoodEnding() -> %d", notifyParam1); break;
        case NOTIFY_BACK_TO_MAINMENU: PrintLog(PRINT_NORMAL, "NOTIFY: BackToMainMenu() -> %d", notifyParam1); break;
        case NOTIFY_LEVEL_SELECT_MENU: PrintLog(PRINT_NORMAL, "NOTIFY: LevelSelectMenu() -> %d", notifyParam1); break;
        case NOTIFY_PLAYER_SET: PrintLog(PRINT_NORMAL, "NOTIFY: PlayerSet() -> %d", notifyParam1); break;
        case NOTIFY_EXTRAS_MODE: PrintLog(PRINT_NORMAL, "NOTIFY: ExtrasMode() -> %d", notifyParam1); break;
        case NOTIFY_SPIN_DASH_TYPE: PrintLog(PRINT_NORMAL, "NOTIFY: SpindashType() -> %d", notifyParam1); break;
        case NOTIFY_TIME_OVER: PrintLog(PRINT_NORMAL, "NOTIFY: TimeOver() -> %d", notifyParam1); break;
        case NOTIFY_TIMEATTACK_MODE: PrintLog(PRINT_NORMAL, "NOTIFY: TimeAttackMode() -> %d", notifyParam1); break;
        case NOTIFY_STATS_BREAK_OBJECT: PrintLog(PRINT_NORMAL, "NOTIFY: StatsBreakObject() -> %d, %d", notifyParam1, notifyParam2); break;
        case NOTIFY_STATS_SAVE_FUTURE: PrintLog(PRINT_NORMAL, "NOTIFY: StatsSaveFuture() -> %d", notifyParam1); break;
        case NOTIFY_STATS_CHARA_ACTION2: PrintLog(PRINT_NORMAL, "NOTIFY: StatsCharaAction2() -> %d, %d, %d", notifyParam1, notifyParam2, notifyParam3); break;

        // Sega Forever stuff
        case CALLBACK_STARTGAME:
            PrintLog(PRINT_NORMAL, "Callback: startGame()");

            // Set lives count and the like
            SetGlobalVariableByName("Config.NumOfLives", 3);
            SetGlobalVariableByName("Config.IsPremiumUser", 1);
            break;
        case CALLBACK_SHOWURL: PrintLog(PRINT_NORMAL, "Callback: showURL(\"https://www.sega.com\")"); break;
        case CALLBACK_SHOWMENU_2: PrintLog(PRINT_NORMAL, "Callback: showMenu(2)"); break;
        case CALLBACK_SHOWHELPCENTER: PrintLog(PRINT_NORMAL, "Callback: Show Help Center"); break;
        case CALLBACK_CHANGEADSTYPE: PrintLog(PRINT_NORMAL, "Callback: Change Ads Type"); break;
        case CALLBACK_ONSHOWINTERSTITIAL: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(2, 0) - Pause_Duration"); break;
        case CALLBACK_ONSHOWBANNER: PrintLog(PRINT_NORMAL, "Callback: onShowBanner()"); break;
        case CALLBACK_ONSHOWBANNER_PAUSESTART: PrintLog(PRINT_NORMAL, "Callback: onShowBanner() - Pause_Start"); break;
        case CALLBACK_ONHIDEBANNER: PrintLog(PRINT_NORMAL, "Callback: onHideBanner()"); break;
        case CALLBACK_REMOVEADSBUTTON_FADEOUT: PrintLog(PRINT_NORMAL, "Callback: RemoveAdsButton_FadeOut()"); break;
        case CALLBACK_REMOVEADSBUTTON_FADEIN: PrintLog(PRINT_NORMAL, "Callback: RemoveAdsButton_FadeIn()"); break;
        case CALLBACK_ONSHOWINTERSTITIAL_2:
        case CALLBACK_ONSHOWINTERSTITIAL_3:
        case CALLBACK_ONSHOWINTERSTITIAL_5: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(0, 0)"); break;
        case CALLBACK_ONSHOWINTERSTITIAL_4: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(1, 0)"); break;
        case CALLBACK_ONVISIBLEGRIDBTN_1: PrintLog(PRINT_NORMAL, "Callback: onVisibleGridBtn(1)"); break;
        case CALLBACK_ONVISIBLEGRIDBTN_0: PrintLog(PRINT_NORMAL, "Callback: onVisibleGridBtn(0)"); break;
        case CALLBACK_ONSHOWINTERSTITIAL_PAUSEDURATION: PrintLog(PRINT_NORMAL, "Callback: onShowInterstitial(0, 0) - Pause_Duration"); break;
        case CALLBACK_SHOWCOUNTDOWNMENU: PrintLog(PRINT_NORMAL, "Callback: showCountDownMenu(0)"); break;
        case CALLBACK_ONVISIBLEMAINMENU_1: PrintLog(PRINT_NORMAL, "Callback: onVisibleMainMenu(1)"); break;
        case CALLBACK_ONVISIBLEMAINMENU_0: PrintLog(PRINT_NORMAL, "Callback: OnVisibleMainMenu(0)"); break;
        case CALLBACK_ONSHOWREWARDADS:
            PrintLog(PRINT_NORMAL, "Callback: onShowRewardAds(0)");

            // small hack to prevent a softlock
            SetGlobalVariableByName("RewardAdCallback", 1);
            break;
        case CALLBACK_ONSHOWBANNER_2: PrintLog(PRINT_NORMAL, "Callback: onShowBanner(4, 0)"); break;

            // Mod loader Only
#if RETRO_USE_MOD_LOADER
        case CALLBACK_SET1P: activePlayerCount = 1; break;
        case CALLBACK_SET2P: activePlayerCount = 2; break;
#endif
    }
}

#if RETRO_USE_MOD_LOADER
void RSDK::Legacy::v3::LoadGameXML(bool pal)
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
                if (pal)
                    LoadXMLPalettes(gameElement);
                else {
                    LoadXMLWindowText(gameElement);
                    LoadXMLVariables(gameElement);
                    LoadXMLObjects(gameElement);
                    LoadXMLSoundFX(gameElement);
                    LoadXMLPlayers(gameElement);
                    LoadXMLStages(gameElement);
                }
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

void RSDK::Legacy::v3::LoadXMLWindowText(const tinyxml2::XMLElement *gameElement)
{
    const tinyxml2::XMLElement *titleElement = gameElement->FirstChildElement("title");
    if (titleElement) {
        const tinyxml2::XMLAttribute *nameAttr = titleElement->FindAttribute("name");
        if (nameAttr)
            StrCopy(gameVerInfo.gameTitle, nameAttr->Value());
    }
}

void RSDK::Legacy::v3::LoadXMLVariables(const tinyxml2::XMLElement *gameElement)
{
    const tinyxml2::XMLElement *variablesElement = gameElement->FirstChildElement("variables");
    if (variablesElement) {
        for (const tinyxml2::XMLElement *varElement = variablesElement->FirstChildElement("variable"); varElement;
             varElement                             = varElement->NextSiblingElement("variable")) {
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
        }
    }
}

void RSDK::Legacy::v3::LoadXMLPalettes(const tinyxml2::XMLElement *gameElement)
{
    const tinyxml2::XMLElement *paletteElement = gameElement->FirstChildElement("palette");
    if (paletteElement) {
        for (const tinyxml2::XMLElement *clrElement = paletteElement->FirstChildElement("color"); clrElement;
            clrElement                             = clrElement->NextSiblingElement("color")) {
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
        }

        for (const tinyxml2::XMLElement *clrsElement = paletteElement->FirstChildElement("colors"); clrsElement;
            clrsElement                             = clrsElement->NextSiblingElement("colors")) {
            const tinyxml2::XMLAttribute *bankAttr = clrsElement->FindAttribute("bank");
            int32 bank                             = 0;
            if (bankAttr)
                bank = bankAttr->IntValue();

            const tinyxml2::XMLAttribute *indAttr = clrsElement->FindAttribute("start");
            int32 index                           = 0;
            if (indAttr)
                index = indAttr->IntValue();

            std::string text = clrsElement->GetText();
            // working: AABBFF #FFaaFF (12, 32, 34) (145 53 234)
            std::regex search(R"((?:#?([0-9A-F]{2})([0-9A-F]{2})([0-9A-F]{2}))|(?:\((\d+),?\s*(\d+),?\s*(\d+)\)))",
                              std::regex_constants::icase | std::regex_constants::ECMAScript);
            std::smatch match;
            while (std::regex_search(text, match, search)) {
                int32 r, g, b;
                int32 base, start;
                if (match[1].matched) {
                    // we have hex
                    base  = 16;
                    start = 1;
                }
                else {
                    // triplet
                    base  = 10;
                    start = 4;
                }

                r = std::stoi(match[start + 0].str(), nullptr, base);
                g = std::stoi(match[start + 1].str(), nullptr, base);
                b = std::stoi(match[start + 2].str(), nullptr, base);

                SetPaletteEntry(bank, index++, r, g, b);
                text = match.suffix();
            }
        }
    }
}

void RSDK::Legacy::v3::LoadXMLObjects(const tinyxml2::XMLElement *gameElement)
{
    modObjCount = 0;

    const tinyxml2::XMLElement *objectsElement = gameElement->FirstChildElement("objects");
    if (objectsElement) {
        for (const tinyxml2::XMLElement *objElement = objectsElement->FirstChildElement("object"); objElement;
            objElement                             = objElement->NextSiblingElement("object")) {
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
        }
    }
}

void RSDK::Legacy::v3::LoadXMLSoundFX(const tinyxml2::XMLElement *gameElement)
{
    const tinyxml2::XMLElement *soundsElement = gameElement->FirstChildElement("sounds");
    if (soundsElement) {
        for (const tinyxml2::XMLElement *sfxElement = soundsElement->FirstChildElement("soundfx"); sfxElement;
            sfxElement                             = sfxElement->NextSiblingElement("soundfx")) {
            const tinyxml2::XMLAttribute *valAttr = sfxElement->FindAttribute("path");
            const char *sfxPath                   = "unknownSFX.wav";
            if (valAttr)
                sfxPath = valAttr->Value();

            SetSfxName(sfxPath, globalSFXCount, true);

            RSDK::Legacy::LoadSfx((char *)sfxPath, globalSFXCount, SCOPE_GLOBAL);
            globalSFXCount++;
        }
    }
}

void RSDK::Legacy::v3::LoadXMLPlayers(const tinyxml2::XMLElement *gameElement)
{
    const tinyxml2::XMLElement *playersElement = gameElement->FirstChildElement("players");
    if (playersElement) {
        for (const tinyxml2::XMLElement *plrElement = playersElement->FirstChildElement("player"); plrElement;
            plrElement                             = plrElement->NextSiblingElement("player")) {
            const tinyxml2::XMLAttribute *nameAttr = plrElement->FindAttribute("name");
            const char *plrName                    = "unknownPlayer";
            if (nameAttr)
                plrName = nameAttr->Value();

            StrCopy(modSettings.playerNames[modSettings.playerCount++], plrName);
        }
    }
}

void RSDK::Legacy::v3::LoadXMLStages(const tinyxml2::XMLElement *gameElement)
{
    const char *elementNames[] = { "presentationStages", "regularStages", "bonusStages", "specialStages" };

    for (int32 l = 0; l < sceneInfo.categoryCount; ++l) {
        const tinyxml2::XMLElement *listElement = gameElement->FirstChildElement(elementNames[l]);
        SceneListInfo *list                     = &sceneInfo.listCategory[l];
        if (listElement) {
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

                listData.emplace(listData.begin() + list->sceneOffsetEnd);
                SceneListEntry *scene = &listData[list->sceneOffsetEnd];

                sprintf_s(scene->name, sizeof(scene->name), "%s", stgName);
                GEN_HASH_MD5(scene->name, scene->hash);
                sprintf_s(scene->folder, sizeof(scene->folder), "%s", stgFolder);
                sprintf_s(scene->id, sizeof(scene->id), "%s", stgID);

                scene->filter = 0xFF;

                list->sceneCount++;
                list->sceneOffsetEnd++;
                for (int32 c = l + 1; c < sceneInfo.categoryCount; ++c) sceneInfo.listCategory[c].sceneOffsetStart++;
            }
        }
    }
    sceneInfo.listData = listData.data();
}
#endif
