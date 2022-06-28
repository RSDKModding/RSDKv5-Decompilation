
RSDK::Legacy::TextMenu RSDK::Legacy::gameMenu[LEGACY_TEXTMENU_COUNT];
int32 RSDK::Legacy::textMenuSurfaceNo = 0;

void RSDK::Legacy::LoadTextFile(TextMenu *menu, const char *filePath, uint8 mapCode)
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