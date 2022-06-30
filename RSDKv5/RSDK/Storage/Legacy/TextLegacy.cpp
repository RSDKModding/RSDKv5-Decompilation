
RSDK::Legacy::TextMenu RSDK::Legacy::gameMenu[LEGACY_TEXTMENU_COUNT];
int32 RSDK::Legacy::textMenuSurfaceNo = 0;

RSDK::Legacy::v3::FontCharacter RSDK::Legacy::v3::fontCharacterList[LEGACY_v3_FONTCHAR_COUNT];

void RSDK::Legacy::v3::LoadTextFile(TextMenu *menu, const char *filePath, uint8 mapCode)
{
    bool32 finished = false;
    FileInfo info;
    InitFileInfo(&info);

    if (LoadFile(&info, filePath, FMODE_RB)) {
        menu->textDataPos                = 0;
        menu->rowCount                   = 0;
        menu->entryStart[menu->rowCount] = menu->textDataPos;
        menu->entrySize[menu->rowCount]  = 0;

        uint8 fileBuffer = ReadInt8(&info);
        if (fileBuffer == 0xFF) {
            fileBuffer = ReadInt8(&info);

            while (!finished) {
                uint16 character = ReadInt8(&info);
                character |= ReadInt8(&info) << 8;

                if (character != '\n') {
                    if (character == '\r') {
                        menu->rowCount += 1;
                        if (menu->rowCount >= 512) {
                            finished = true;
                        }
                        else {
                            menu->entryStart[menu->rowCount] = menu->textDataPos;
                            menu->entrySize[menu->rowCount]  = 0;
                        }
                    }
                    else {
                        if (mapCode) {
                            int32 i = 0;
                            while (i < 1024) {
                                if (fontCharacterList[i].id == character) {
                                    character = i;
                                    i         = 1025;
                                }
                                else {
                                    ++i;
                                }
                            }

                            if (i == 1024)
                                character = 0;
                        }
                        menu->textData[menu->textDataPos++] = character;
                        menu->entrySize[menu->rowCount]++;
                    }
                }

                if (!finished) {
                    finished = info.readPos >= info.fileSize;
                    if (menu->textDataPos >= LEGACY_TEXTDATA_COUNT)
                        finished = true;
                }
            }
        }
        else {
            uint16 character = fileBuffer;
            if (character != '\n') {
                if (character == '\r') {
                    menu->rowCount++;
                    menu->entryStart[menu->rowCount] = menu->textDataPos;
                    menu->entrySize[menu->rowCount]  = 0;
                }
                else {
                    if (mapCode) {
                        int32 i = 0;
                        while (i < 1024) {
                            if (fontCharacterList[i].id == character) {
                                character = i;
                                i         = 1025;
                            }
                            else {
                                ++i;
                            }
                        }

                        if (i == 1024)
                            character = 0;
                    }
                    menu->textData[menu->textDataPos++] = character;
                    menu->entrySize[menu->rowCount]++;
                }
            }

            while (!finished) {
                character = ReadInt8(&info);
                if (character != '\n') {
                    if (character == '\r') {
                        menu->rowCount++;
                        if (menu->rowCount > 511) {
                            finished = true;
                        }
                        else {
                            menu->entryStart[menu->rowCount] = menu->textDataPos;
                            menu->entrySize[menu->rowCount]  = 0;
                        }
                    }
                    else {
                        if (mapCode) {
                            int32 i = 0;
                            while (i < 1024) {
                                if (fontCharacterList[i].id == character) {
                                    character = i;
                                    i         = 1025;
                                }
                                else {
                                    ++i;
                                }
                            }

                            if (i == 1024)
                                character = 0;
                        }

                        menu->textData[menu->textDataPos++] = character;
                        menu->entrySize[menu->rowCount]++;
                    }
                }

                if (!finished) {
                    finished = info.readPos >= info.fileSize;
                    if (menu->textDataPos >= LEGACY_TEXTDATA_COUNT)
                        finished = true;
                }
            }
        }
        menu->rowCount++;

        CloseFile(&info);
    }
}

void RSDK::Legacy::v4::LoadTextFile(TextMenu *menu, const char *filePath)
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadFile(&info, filePath, FMODE_RB)) {
        menu->textDataPos                = 0;
        menu->rowCount                   = 0;
        menu->entryStart[menu->rowCount] = menu->textDataPos;
        menu->entrySize[menu->rowCount]  = 0;

        while (menu->textDataPos < LEGACY_TEXTDATA_COUNT && info.readPos < info.fileSize) {
            uint8 character = ReadInt8(&info);
            if (character != '\n') {
                if (character == '\r') {
                    menu->rowCount++;
                    menu->entryStart[menu->rowCount] = menu->textDataPos;
                    menu->entrySize[menu->rowCount]  = 0;
                }
                else {
                    menu->textData[menu->textDataPos++] = character;
                    menu->entrySize[menu->rowCount]++;
                }
            }
        }

        menu->rowCount++;
        CloseFile(&info);
    }
}

void RSDK::Legacy::SetupTextMenu(TextMenu *menu, int32 rowCount)
{
    menu->textDataPos = 0;
    menu->rowCount    = rowCount;
}
void RSDK::Legacy::AddTextMenuEntry(TextMenu *menu, const char *text)
{
    menu->entryStart[menu->rowCount]     = menu->textDataPos;
    menu->entrySize[menu->rowCount]      = 0;
    menu->entryHighlight[menu->rowCount] = false;

    int32 textLength = StrLength(text);
    for (int32 i = 0; i < textLength;) {
        if (text[i] != '\0') {
            menu->textData[menu->textDataPos++] = text[i];
            menu->entrySize[menu->rowCount]++;
            ++i;
        }
        else {
            break;
        }
    }
    menu->rowCount++;
}
void RSDK::Legacy::SetTextMenuEntry(TextMenu *menu, const char *text, int32 rowID)
{
    menu->entryStart[rowID]              = menu->textDataPos;
    menu->entrySize[rowID]               = 0;
    menu->entryHighlight[menu->rowCount] = false;

    int32 textLength = StrLength(text);
    for (int32 i = 0; i < textLength;) {
        if (text[i] != '\0') {
            menu->textData[menu->textDataPos++] = text[i];
            menu->entrySize[rowID]++;
            ++i;
        }
        else {
            break;
        }
    }
}
void RSDK::Legacy::EditTextMenuEntry(TextMenu *menu, const char *text, int32 rowID)
{
    int32 entryPos                       = menu->entryStart[rowID];
    menu->entrySize[rowID]               = 0;
    menu->entryHighlight[menu->rowCount] = false;

    int32 textLength = StrLength(text);
    for (int32 i = 0; i < textLength;) {
        if (text[i] != '\0') {
            menu->textData[entryPos++] = text[i];
            menu->entrySize[rowID]++;
            ++i;
        }
        else {
            break;
        }
    }
}

void RSDK::Legacy::v3::LoadFontFile(const char *filePath)
{
    FileInfo info;
    InitFileInfo(&info);

    if (LoadFile(&info, filePath, FMODE_RB)) {
        int32 count = 0;
        while (info.readPos < info.fileSize) {
            fontCharacterList[count].id = ReadInt8(&info);
            fontCharacterList[count].id += ReadInt8(&info) << 8;
            fontCharacterList[count].id += ReadInt8(&info) << 16;
            fontCharacterList[count].id += ReadInt8(&info) << 24;

            fontCharacterList[count].srcX = ReadInt8(&info);
            fontCharacterList[count].srcX += ReadInt8(&info) << 8;

            fontCharacterList[count].srcY = ReadInt8(&info);
            fontCharacterList[count].srcY += ReadInt8(&info) << 8;

            fontCharacterList[count].width = ReadInt8(&info) + 1;
            fontCharacterList[count].width += ReadInt8(&info) << 8;

            fontCharacterList[count].height = ReadInt8(&info) + 1;
            fontCharacterList[count].height += ReadInt8(&info) << 8;

            fontCharacterList[count].pivotX = ReadInt8(&info);
            uint8 fileBuffer                = ReadInt8(&info);
            if (fileBuffer > 0x80) {
                fontCharacterList[count].pivotX += (fileBuffer - 0x80) << 8;
                fontCharacterList[count].pivotX += -0x8000;
            }
            else {
                fontCharacterList[count].pivotX += fileBuffer << 8;
            }

            fontCharacterList[count].pivotY = ReadInt8(&info);
            fileBuffer                      = ReadInt8(&info);
            if (fileBuffer > 0x80) {
                fontCharacterList[count].pivotY += (fileBuffer - 0x80) << 8;
                fontCharacterList[count].pivotY += -0x8000;
            }
            else {
                fontCharacterList[count].pivotY += fileBuffer << 8;
            }

            fontCharacterList[count].xAdvance = ReadInt8(&info);
            fileBuffer                        = ReadInt8(&info);
            if (fileBuffer > 0x80) {
                fontCharacterList[count].xAdvance += (fileBuffer - 0x80) << 8;
                fontCharacterList[count].xAdvance += -0x8000;
            }
            else {
                fontCharacterList[count].xAdvance += fileBuffer << 8;
            }

            // Unused
            ReadInt8(&info);
            ReadInt8(&info);

            count++;
        }
        CloseFile(&info);
    }
}

void RSDK::Legacy::v3::DrawBitmapText(void *menu, int32 XPos, int32 YPos, int32 scale, int32 spacing, int32 rowStart, int32 rowCount)
{
    TextMenu *tMenu = (TextMenu *)menu;
    int32 Y         = YPos << 9;
    if (rowCount < 0)
        rowCount = tMenu->rowCount;
    if (rowStart + rowCount > tMenu->rowCount)
        rowCount = tMenu->rowCount - rowStart;

    while (rowCount > 0) {
        int32 X = XPos << 9;
        for (int32 i = 0; i < tMenu->entrySize[rowStart]; ++i) {
            uint16 c             = tMenu->textData[tMenu->entryStart[rowStart] + i];
            FontCharacter *fChar = &fontCharacterList[c];
            DrawSpriteScaled(FLIP_NONE, X >> 9, Y >> 9, -fChar->pivotX, -fChar->pivotY, scale, scale, fChar->width, fChar->height, fChar->srcX,
                             fChar->srcY, textMenuSurfaceNo);
            X += fChar->xAdvance * scale;
        }
        Y += spacing * scale;
        rowStart++;
        rowCount--;
    }
}