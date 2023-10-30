

void *RSDK::Legacy::nativeFunction[LEGACY_v4_NATIIVEFUNCTION_COUNT];
int32 RSDK::Legacy::nativeFunctionCount = 0;

int32 RSDK::Legacy::globalVariablesCount = 0;
RSDK::Legacy::GlobalVariable RSDK::Legacy::globalVariables[LEGACY_GLOBALVAR_COUNT];

int32 RSDK::Legacy::saveRAM[LEGACY_SAVEDATA_SIZE];

int32 RSDK::Legacy::GetGlobalVariableByName(const char *name)
{
    for (int32 v = 0; v < globalVariablesCount; ++v) {
        if (StrComp(name, globalVariables[v].name))
            return globalVariables[v].value;
    }
    return 0;
}

void RSDK::Legacy::SetGlobalVariableByName(const char *name, int32 value)
{
    for (int32 v = 0; v < globalVariablesCount; ++v) {
        if (StrComp(name, globalVariables[v].name)) {
            globalVariables[v].value = value;
            break;
        }
    }
}
int32 RSDK::Legacy::GetGlobalVariableID(const char *name)
{
    for (int32 v = 0; v < globalVariablesCount; ++v) {
        if (StrComp(name, globalVariables[v].name))
            return v;
    }
    return 0xFF;
}

bool32 RSDK::Legacy::ReadSaveRAM() { return SKU::LoadUserFile("SGame.bin", saveRAM, sizeof(saveRAM)); }
bool32 RSDK::Legacy::WriteSaveRAM() { return SKU::SaveUserFile("SGame.bin", saveRAM, sizeof(saveRAM)); }

void RSDK::Legacy::v3::SetAchievement(int32 achievementID, int32 achievementDone)
{
    PrintLog(PRINT_NORMAL, "[RSDKv3] Achieved achievement: %d (%d)!", achievementID, achievementDone);
}
void RSDK::Legacy::v3::SetLeaderboard(int32 leaderboardID, int32 score)
{
    PrintLog(PRINT_NORMAL, "[RSDKv3] Setting Leaderboard %d score to %d...", leaderboardID, score);
}

// Native Functions

void RSDK::Legacy::v4::SetAchievement(int32 *achievementID, int32 *status)
{
    if (!achievementID || !status)
        return;

    PrintLog(PRINT_NORMAL, "[RSDKv4] Achieved achievement: %d (%d)!", *achievementID, *status);

    if ((uint32)*achievementID >= achievementList.size())
        return;

    achievementList[*achievementID].achieved = *status ? true : false;
}
void RSDK::Legacy::v4::SetLeaderboard(int32 *leaderboardID, int32 *score)
{
    if (!leaderboardID || !score)
        return;

    PrintLog(PRINT_NORMAL, "[RSDKv4] Setting Leaderboard %d score to %d...", *leaderboardID, *score);
}
void RSDK::Legacy::v4::HapticEffect(int32 *id, int32 *unknown1, int32 *unknown2, int32 *unknown3) {}

enum NotifyCallbackIDs {
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
};

void RSDK::Legacy::v4::NotifyCallback(int32 *callback, int32 *param1, int32 *param2, int32 *param3)
{
    if (!callback || !param1)
        return;

    switch (*callback) {
        default: PrintLog(PRINT_NORMAL, "NOTIFY: Unknown Callback -> %d", *param1); break;
        case NOTIFY_DEATH_EVENT: PrintLog(PRINT_NORMAL, "NOTIFY: DeathEvent() -> %d", *param1); break;
        case NOTIFY_TOUCH_SIGNPOST: PrintLog(PRINT_NORMAL, "NOTIFY: TouchSignPost() -> %d", *param1); break;
        case NOTIFY_HUD_ENABLE: PrintLog(PRINT_NORMAL, "NOTIFY: HUDEnable() -> %d", *param1); break;
        case NOTIFY_ADD_COIN:
            PrintLog(PRINT_NORMAL, "NOTIFY: AddCoin() -> %d", *param1);
            SetGlobalVariableByName("game.coinCount", GetGlobalVariableByName("game.coinCount") + *param1);
            break;
        case NOTIFY_KILL_ENEMY: PrintLog(PRINT_NORMAL, "NOTIFY: KillEnemy() -> %d", *param1); break;
        case NOTIFY_SAVESLOT_SELECT: PrintLog(PRINT_NORMAL, "NOTIFY: SaveSlotSelect() -> %d", *param1); break;
        case NOTIFY_FUTURE_PAST: PrintLog(PRINT_NORMAL, "NOTIFY: FuturePast() -> %d", *param1); break;
        case NOTIFY_GOTO_FUTURE_PAST: PrintLog(PRINT_NORMAL, "NOTIFY: GotoFuturePast() -> %d", *param1); break;
        case NOTIFY_BOSS_END: PrintLog(PRINT_NORMAL, "NOTIFY: BossEnd() -> %d", *param1); break;
        case NOTIFY_SPECIAL_END: PrintLog(PRINT_NORMAL, "NOTIFY: SpecialEnd() -> %d", *param1); break;
        case NOTIFY_DEBUGPRINT:
            // Although there are instances of this being called from both CallNativeFunction2 and CallNativeFunction4 in Origins' scripts, there's no way we can tell which one was used here to handle possible errors
            // Due to this, we'll only print param1 regardless of the opcode used
            PrintLog(PRINT_NORMAL, "NOTIFY: DebugPrint() -> %d", *param1);
            break;
        case NOTIFY_KILL_BOSS: PrintLog(PRINT_NORMAL, "NOTIFY: KillBoss() -> %d", *param1); break;
        case NOTIFY_TOUCH_EMERALD: PrintLog(PRINT_NORMAL, "NOTIFY: TouchEmerald() -> %d", *param1); break;
        case NOTIFY_STATS_ENEMY: PrintLog(PRINT_NORMAL, "NOTIFY: StatsEnemy() -> %d, %d, %d", *param1, *param2, *param3); break;
        case NOTIFY_STATS_CHARA_ACTION: PrintLog(PRINT_NORMAL, "NOTIFY: StatsCharaAction() -> %d, %d, %d", *param1, *param2, *param3); break;
        case NOTIFY_STATS_RING: PrintLog(PRINT_NORMAL, "NOTIFY: StatsRing() -> %d", *param1); break;
        case NOTIFY_STATS_MOVIE:
            PrintLog(PRINT_NORMAL, "NOTIFY: StatsMovie() -> %d", *param1);
            sceneInfo.activeCategory = 0;
            sceneInfo.listPos        = 0;
            gameMode                 = ENGINE_MAINGAME;
            stageMode                = STAGEMODE_LOAD;
            break;
        case NOTIFY_STATS_PARAM_1: PrintLog(PRINT_NORMAL, "NOTIFY: StatsParam1() -> %d, %d, %d", *param1, *param2, *param3); break;
        case NOTIFY_STATS_PARAM_2: PrintLog(PRINT_NORMAL, "NOTIFY: StatsParam2() -> %d", *param1); break;
        case NOTIFY_CHARACTER_SELECT:
            PrintLog(PRINT_NORMAL, "NOTIFY: CharacterSelect() -> %d", *param1);
            SetGlobalVariableByName("game.callbackResult", 1);
            SetGlobalVariableByName("game.continueFlag", 0);
            break;
        case NOTIFY_SPECIAL_RETRY:
            PrintLog(PRINT_NORMAL, "NOTIFY: SpecialRetry() -> %d, %d, %d", *param1, *param2, *param3);
            SetGlobalVariableByName("game.callbackResult", 1);
            break;
        case NOTIFY_TOUCH_CHECKPOINT: PrintLog(PRINT_NORMAL, "NOTIFY: TouchCheckpoint() -> %d", *param1); break;
        case NOTIFY_ACT_FINISH: PrintLog(PRINT_NORMAL, "NOTIFY: ActFinish() -> %d", *param1); break;
        case NOTIFY_1P_VS_SELECT: PrintLog(PRINT_NORMAL, "NOTIFY: 1PVSSelect() -> %d", *param1); break;
        case NOTIFY_CONTROLLER_SUPPORT:
            PrintLog(PRINT_NORMAL, "NOTIFY: ControllerSupport() -> %d", *param1);
            SetGlobalVariableByName("game.callbackResult", 1);
            break;
        case NOTIFY_STAGE_RETRY: PrintLog(PRINT_NORMAL, "NOTIFY: StageRetry() -> %d", *param1); break;
        case NOTIFY_SOUND_TRACK: PrintLog(PRINT_NORMAL, "NOTIFY: SoundTrack() -> %d", *param1); break;
        case NOTIFY_GOOD_ENDING: PrintLog(PRINT_NORMAL, "NOTIFY: GoodEnding() -> %d", *param1); break;
        case NOTIFY_BACK_TO_MAINMENU: PrintLog(PRINT_NORMAL, "NOTIFY: BackToMainMenu() -> %d", *param1); break;
        case NOTIFY_LEVEL_SELECT_MENU: PrintLog(PRINT_NORMAL, "NOTIFY: LevelSelectMenu() -> %d", *param1); break;
        case NOTIFY_PLAYER_SET: PrintLog(PRINT_NORMAL, "NOTIFY: PlayerSet() -> %d", *param1); break;
        case NOTIFY_EXTRAS_MODE: PrintLog(PRINT_NORMAL, "NOTIFY: ExtrasMode() -> %d", *param1); break;
        case NOTIFY_SPIN_DASH_TYPE: PrintLog(PRINT_NORMAL, "NOTIFY: SpindashType() -> %d", *param1); break;
        case NOTIFY_TIME_OVER: PrintLog(PRINT_NORMAL, "NOTIFY: TimeOver() -> %d", *param1); break;
        case NOTIFY_TIMEATTACK_MODE: PrintLog(PRINT_NORMAL, "NOTIFY: TimeAttackMode() -> %d", *param1); break;
        case NOTIFY_STATS_BREAK_OBJECT: PrintLog(PRINT_NORMAL, "NOTIFY: StatsBreakObject() -> %d, %d", *param1, *param2); break;
        case NOTIFY_STATS_SAVE_FUTURE: PrintLog(PRINT_NORMAL, "NOTIFY: StatsSaveFuture() -> %d", *param1); break;
        case NOTIFY_STATS_CHARA_ACTION2: PrintLog(PRINT_NORMAL, "NOTIFY: StatsCharaAction2() -> %d, %d, %d", *param1, *param2, *param3); break;
    }
}