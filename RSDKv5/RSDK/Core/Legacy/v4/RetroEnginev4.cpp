
bool32 RSDK::Legacy::v4::LoadGameConfig(const char *filepath)
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
        ReadString(&info, gameVerInfo.gameSubtitle);

        uint8 buf[3];
        for (int32 c = 0; c < 0x60; ++c) {
            ReadBytes(&info, buf, 3);
            SetPaletteEntry(-1, c, buf[0], buf[1], buf[2]);
        }

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
        totalSceneCount += RSDK::Legacy::v4::LoadXMLStages(2, 0);
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
        RSDK::Legacy::v4::LoadXMLVariables();
        RSDK::Legacy::v4::LoadXMLPalettes();
        RSDK::Legacy::v4::LoadXMLObjects();
        RSDK::Legacy::v4::LoadXMLPlayers();
        RSDK::Legacy::v4::LoadXMLStages(0, gcSceneCount);

        SetGlobalVariableByName("options.devMenuFlag", false);
        if (engine.devMenu)
            SetGlobalVariableByName("options.devMenuFlag", true);
#endif

        usingBytecode = false;
        InitFileInfo(&info);
        if (useDataPack && LoadFile(&info, "Bytecode/GlobalCode.bin", FMODE_RB)) {
            usingBytecode = true;
            CloseFile(&info);
        }
    }

    // These need to be set every time its reloaded
    nativeFunctionCount = 0;

    nativeFunctionCount = 0;
    AddNativeFunction("SetAchievement", SetAchievement);
    AddNativeFunction("SetLeaderboard", SetLeaderboard);
    AddNativeFunction("HapticEffect", HapticEffect);

    // AddNativeFunction("Connect2PVS", Connect2PVS);
    // AddNativeFunction("Disconnect2PVS", Disconnect2PVS);
    // AddNativeFunction("SendEntity", SendEntity);
    // AddNativeFunction("SendValue", SendValue);
    // AddNativeFunction("ReceiveEntity", ReceiveEntity);
    // AddNativeFunction("ReceiveValue", ReceiveValue);
    // AddNativeFunction("TransmitGlobal", TransmitGlobal);
    // AddNativeFunction("ShowPromoPopup", ShowPromoPopup);

    AddNativeFunction("NotifyCallback", NotifyCallback);

#if RETRO_USE_MOD_LOADER
    AddNativeFunction("ExitGame", ExitGame);
    AddNativeFunction("FileExists", FileExists);
    AddNativeFunction("OpenModMenu", OpenModMenu); // Opens the dev menu-based mod menu incase you cant be bothered or smth
    AddNativeFunction("AddAchievement", AddGameAchievement);
    AddNativeFunction("SetAchievementDescription", SetAchievementDescription);
    AddNativeFunction("ClearAchievements", ClearAchievements);
    AddNativeFunction("GetAchievementCount", GetAchievementCount);
    AddNativeFunction("GetAchievement", GetAchievement);
    AddNativeFunction("GetAchievementName", GetAchievementName);
    AddNativeFunction("GetAchievementDescription", GetAchievementDescription);
    AddNativeFunction("GetScreenWidth", GetScreenWidth);
    AddNativeFunction("SetScreenWidth", SetScreenWidth);
    AddNativeFunction("GetWindowScale", GetWindowScale);
    AddNativeFunction("SetWindowScale", SetWindowScale);
    AddNativeFunction("GetWindowScaleMode", GetWindowScaleMode);
    AddNativeFunction("SetWindowScaleMode", SetWindowScaleMode);
    AddNativeFunction("GetWindowFullScreen", GetWindowFullScreen);
    AddNativeFunction("SetWindowFullScreen", SetWindowFullScreen);
    AddNativeFunction("GetWindowBorderless", GetWindowBorderless);
    AddNativeFunction("SetWindowBorderless", SetWindowBorderless);
    AddNativeFunction("GetWindowVSync", GetWindowVSync);
    AddNativeFunction("SetWindowVSync", SetWindowVSync);
    AddNativeFunction("ApplyWindowChanges", ApplyWindowChanges); // Refresh window after changing window options
    AddNativeFunction("GetModCount", GetModCount);
    AddNativeFunction("GetModName", GetModName);
    AddNativeFunction("GetModDescription", GetModDescription);
    AddNativeFunction("GetModAuthor", GetModAuthor);
    AddNativeFunction("GetModVersion", GetModVersion);
    AddNativeFunction("GetModActive", GetModActive);
    AddNativeFunction("SetModActive", SetModActive);
    AddNativeFunction("MoveMod", MoveMod);
    AddNativeFunction("RefreshEngine", RefreshEngine); // Reload engine after changing mod status
#endif

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
            break;
            // case ENGINE_RESETGAME: break;

        case ENGINE_SCRIPTERROR: {
            currentScreen = screens;
            FillScreen(0x000000, 0x10, 0x10, 0x10);

            int32 yOff = DevOutput_GetStringYSize(scriptErrorMessage);
            DrawDevString(scriptErrorMessage, 8, currentScreen->center.y - (yOff >> 1) + 8, 0, 0xF0F0F0);
            break;
        }

        case ENGINE_INITPAUSE:
        case ENGINE_EXITPAUSE: gameMode = ENGINE_MAINGAME; break;

        case ENGINE_ENDGAME:
        case ENGINE_RESETGAME:
            sceneInfo.activeCategory = 0;
            sceneInfo.listPos        = 0;
            gameMode                 = ENGINE_MAINGAME;
            stageMode                = STAGEMODE_LOAD;
            break;

        default: break;
    }
}

#if RETRO_USE_MOD_LOADER
void RSDK::Legacy::v4::LoadXMLVariables()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
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
void RSDK::Legacy::v4::LoadXMLPalettes()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
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
void RSDK::Legacy::v4::LoadXMLObjects()
{
    FileInfo info;
    modObjCount = 0;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
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
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v4::LoadXMLSoundFX()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
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

                            SetSfxName(sfxName, globalSFXCount);

                            RSDK::Legacy::LoadSfx((char *)sfxPath, globalSFXCount, SCOPE_GLOBAL);
                            globalSFXCount++;

                        } while ((sfxElement = sfxElement->NextSiblingElement("soundfx")));
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
void RSDK::Legacy::v4::LoadXMLPlayers()
{
    FileInfo info;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
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
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
            }

            delete[] xmlData;
            delete doc;

            CloseFile(&info);
        }
    }
    SetActiveMod(-1);
}
int32 RSDK::Legacy::v4::LoadXMLStages(int32 mode, int32 gcStageCount)
{
    FileInfo info;
    int32 stageCount = 0;

    int32 activeModCount = (int32)ActiveMods().size();
    for (int32 m = 0; m < activeModCount; ++m) {
        SetActiveMod(m);

        InitFileInfo(&info);
        if (LoadFile(&info, "Data/Game/Game.xml", FMODE_RB)) {
            tinyxml2::XMLDocument *doc = new tinyxml2::XMLDocument;

            char *xmlData = new char[info.fileSize + 1];
            ReadBytes(&info, xmlData, info.fileSize);
            xmlData[info.fileSize] = 0;

            bool success = doc->Parse(xmlData) == tinyxml2::XML_SUCCESS;

            if (success) {
                const tinyxml2::XMLElement *gameElement = doc->FirstChildElement("game");
                const char *elementNames[]              = { "presentationStages", "regularStages", "specialStages", "bonusStages" };
                const char *categoryNames[]             = {
                    "Presentation",
                    "Regular",
                    "Special"
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
                            sceneInfo.categoryCount++;
                        }
                    }
                }
            }
            else {
                PrintLog(PRINT_NORMAL, "Failed to parse Game.xml File!");
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
