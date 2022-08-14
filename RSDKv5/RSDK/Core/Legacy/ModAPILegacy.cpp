
#if RETRO_USE_MOD_LOADER

char RSDK::Legacy::modTypeNames[OBJECT_COUNT][0x40];
char RSDK::Legacy::modScriptPaths[OBJECT_COUNT][0x40];
uint8 RSDK::Legacy::modScriptFlags[OBJECT_COUNT];
uint8 RSDK::Legacy::modObjCount = 0;


int32 RSDK::Legacy::v4::OpenModMenu()
{
    OpenDevMenu();
    devMenu.state = DevMenu_ModsMenu;

    return 1;
}

void RSDK::Legacy::v4::RefreshEngine() { RSDK::ApplyModChanges(); }
void RSDK::Legacy::v4::GetModCount() { scriptEng.checkResult = RSDK::GetModCount(false); }
void RSDK::Legacy::v4::GetModName(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused)
{
    if (*id >= modList.size())
        return;

    TextMenu *menu                       = &gameMenu[*textMenu];
    menu->entryHighlight[menu->rowCount] = *highlight;
    AddTextMenuEntry(menu, modList[*id].name.c_str());
}
void RSDK::Legacy::v4::GetModDescription(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused)
{
    if (*id >= modList.size())
        return;

    TextMenu *menu                       = &gameMenu[*textMenu];
    menu->entryHighlight[menu->rowCount] = *highlight;
    AddTextMenuEntry(menu, modList[*id].desc.c_str());
}
void RSDK::Legacy::v4::GetModAuthor(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused)
{
    if (*id >= modList.size())
        return;

    TextMenu *menu                       = &gameMenu[*textMenu];
    menu->entryHighlight[menu->rowCount] = *highlight;
    AddTextMenuEntry(menu, modList[*id].author.c_str());
}
void RSDK::Legacy::v4::GetModVersion(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused)
{
    if (*id >= modList.size())
        return;

    TextMenu *menu                       = &gameMenu[*textMenu];
    menu->entryHighlight[menu->rowCount] = *highlight;
    AddTextMenuEntry(menu, modList[*id].version.c_str());
}
void RSDK::Legacy::v4::GetModActive(uint32 *id, int32 *unused)
{
    scriptEng.checkResult = false;
    if (*id >= modList.size())
        return;

    scriptEng.checkResult = modList[*id].active;
}
void RSDK::Legacy::v4::SetModActive(uint32 *id, int32 *active)
{
    if (*id >= modList.size())
        return;

    modList[*id].active = *active;
}
void RSDK::Legacy::v4::MoveMod(uint32 *id, int32 *up)
{
    if (!id || !up)
        return;

    int32 preOption = *id;
    int32 option    = preOption + (*up ? -1 : 1);
    if (option < 0 || preOption < 0)
        return;

    if (option >= (int32)modList.size() || preOption >= (int32)modList.size())
        return;

    ModInfo swap       = modList[preOption];
    modList[preOption] = modList[option];
    modList[option]    = swap;
}

void RSDK::Legacy::v4::ExitGame() { RSDK::SKU::ExitGame(); }

void RSDK::Legacy::v4::FileExists(int32 *unused, const char *filePath)
{
    FileInfo info;
    InitFileInfo(&info);

    scriptEng.checkResult = false;
    if (LoadFile(&info, filePath, FMODE_RB)) {
        scriptEng.checkResult = true;
        CloseFile(&info);
    }
}

void RSDK::Legacy::v4::AddGameAchievement(int32 *unused, const char *name) { RSDK::RegisterAchievement(name, name, ""); }
void RSDK::Legacy::v4::SetAchievementDescription(uint32 *id, const char *desc)
{
    if (*id >= (int32)achievementList.size())
        return;

    achievementList[*id].description = desc;
}
void RSDK::Legacy::v4::ClearAchievements() { achievementList.clear(); }
void RSDK::Legacy::v4::GetAchievementCount() { scriptEng.checkResult = (int32)achievementList.size(); }
void RSDK::Legacy::v4::GetAchievementName(uint32 *id, int32 *textMenu)
{
    if (*id >= (int32)achievementList.size())
        return;

    TextMenu *menu                       = &gameMenu[*textMenu];
    menu->entryHighlight[menu->rowCount] = false;
    AddTextMenuEntry(menu, achievementList[*id].name.c_str());
}
void RSDK::Legacy::v4::GetAchievementDescription(uint32 *id, int32 *textMenu)
{
    if (*id >= (int32)achievementList.size())
        return;

    TextMenu *menu                       = &gameMenu[*textMenu];
    menu->entryHighlight[menu->rowCount] = false;
    AddTextMenuEntry(menu, achievementList[*id].description.c_str());
}
void RSDK::Legacy::v4::GetAchievement(uint32 *id, void *unused)
{
    if (*id >= (int32)achievementList.size())
        return;
    scriptEng.checkResult = achievementList[*id].achieved ? 100 : 0;
}

void RSDK::Legacy::v4::GetScreenWidth() { scriptEng.checkResult = videoSettings.pixWidth; }
void RSDK::Legacy::v4::SetScreenWidth(int32 *width, int32 *unused)
{
    if (!width)
        return;

    int32 scale = GetVideoSetting(VIDEOSETTING_WINDOW_WIDTH) / videoSettings.pixWidth;

    videoSettings.pixWidth = *width;
    SetVideoSetting(VIDEOSETTING_WINDOW_WIDTH, videoSettings.pixWidth * scale);
}
void RSDK::Legacy::v4::GetWindowScale() { scriptEng.checkResult = GetVideoSetting(VIDEOSETTING_WINDOW_WIDTH) / videoSettings.pixWidth; }
void RSDK::Legacy::v4::SetWindowScale(int32 *scale, int32 *unused)
{
    if (!scale)
        return;

    SetVideoSetting(VIDEOSETTING_WINDOW_WIDTH, videoSettings.pixWidth * *scale);
}
void RSDK::Legacy::v4::GetWindowScaleMode() { scriptEng.checkResult = GetVideoSetting(VIDEOSETTING_SHADERID); }
void RSDK::Legacy::v4::SetWindowScaleMode(int32 *mode, int32 *unused)
{
    if (!mode)
        return;

    SetVideoSetting(VIDEOSETTING_SHADERID, *mode % shaderCount);
}
void RSDK::Legacy::v4::GetWindowFullScreen() { scriptEng.checkResult = GetVideoSetting(VIDEOSETTING_WINDOWED) ^ 1; }
void RSDK::Legacy::v4::SetWindowFullScreen(int32 *fullscreen, int32 *unused)
{
    if (!fullscreen)
        return;

    SetVideoSetting(VIDEOSETTING_WINDOWED, *fullscreen ^ 1);
}
void RSDK::Legacy::v4::GetWindowBorderless() { scriptEng.checkResult = GetVideoSetting(VIDEOSETTING_BORDERED) ^ 1; }
void RSDK::Legacy::v4::SetWindowBorderless(int32 *borderless, int32 *unused)
{
    if (!borderless)
        return;

    SetVideoSetting(VIDEOSETTING_BORDERED, *borderless ^ 1);
}
void RSDK::Legacy::v4::GetWindowVSync() { scriptEng.checkResult = GetVideoSetting(VIDEOSETTING_VSYNC); }
void RSDK::Legacy::v4::SetWindowVSync(int32 *enabled, int32 *unused)
{
    if (!enabled)
        return;

    SetVideoSetting(VIDEOSETTING_VSYNC, *enabled);
}
void RSDK::Legacy::v4::ApplyWindowChanges() { RSDK::UpdateGameWindow(); }

#endif