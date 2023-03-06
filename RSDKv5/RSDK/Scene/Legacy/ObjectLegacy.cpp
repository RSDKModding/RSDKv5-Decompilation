#include "v3/ObjectLegacyv3.cpp"
#include "v3/PlayerLegacyv3.cpp"
#include "v3/ScriptLegacyv3.cpp"

#include "v4/ObjectLegacyv4.cpp"
#include "v4/ScriptLegacyv4.cpp"

int32 RSDK::Legacy::OBJECT_BORDER_X1 = 0x80;
int32 RSDK::Legacy::OBJECT_BORDER_X2 = Legacy::SCREEN_XSIZE + 0x80;
int32 RSDK::Legacy::OBJECT_BORDER_X3 = 0x20;
int32 RSDK::Legacy::OBJECT_BORDER_X4 = Legacy::SCREEN_XSIZE + 0x20;

const int32 RSDK::Legacy::OBJECT_BORDER_Y1 = 0x100;
const int32 RSDK::Legacy::OBJECT_BORDER_Y2 = SCREEN_YSIZE + 0x100;
const int32 RSDK::Legacy::OBJECT_BORDER_Y3 = 0x80;
const int32 RSDK::Legacy::OBJECT_BORDER_Y4 = SCREEN_YSIZE + 0x80;

char RSDK::Legacy::scriptErrorMessage[0x400];

bool32 RSDK::Legacy::ConvertStringToInteger(const char *text, int32 *value)
{
    int32 charID    = 0;
    bool32 negative = false;
    int32 base      = 10;
    *value          = 0;

    if (*text != '+' && !(*text >= '0' && *text <= '9') && *text != '-')
        return false;

    int32 strLength = StrLength(text) - 1;
    uint32 charVal  = 0;

    if (*text == '-') {
        negative = true;
        charID   = 1;
        --strLength;
    }
    else if (*text == '+') {
        charID = 1;
        --strLength;
    }

    if (text[charID] == '0') {
        if (text[charID + 1] == 'x' || text[charID + 1] == 'X')
            base = 0x10;
#if !RETRO_USE_ORIGINAL_CODE
        else if (text[charID + 1] == 'b' || text[charID + 1] == 'B')
            base = 0b10;
        else if (text[charID + 1] == 'o' || text[charID + 1] == 'O')
            base = 0010; // base 8
#endif

        if (base != 10) {
            charID += 2;
            strLength -= 2;
        }
    }

    while (strLength > -1) {
        bool32 flag = text[charID] < '0';
        if (!flag) {
            if (base == 0x10 && text[charID] > 'f')
                flag = true;
#if !RETRO_USE_ORIGINAL_CODE
            if (base == 0010 && text[charID] > '7')
                flag = true;
            if (base == 0b10 && text[charID] > '1')
                flag = true;
#endif
        }

        if (flag) {
            return 0;
        }
        if (strLength <= 0) {
            if (text[charID] >= '0' && text[charID] <= '9') {
                *value = text[charID] + *value - '0';
            }
            else if (text[charID] >= 'a' && text[charID] <= 'f') {
                charVal = text[charID] - 'a';
                charVal += 10;
                *value += charVal;
            }
            else if (text[charID] >= 'A' && text[charID] <= 'F') {
                charVal = text[charID] - 'A';
                charVal += 10;
                *value += charVal;
            }
        }
        else {
            int32 strlen = strLength + 1;
            charVal      = 0;
            if (text[charID] >= '0' && text[charID] <= '9') {
                charVal = text[charID] - '0';
            }
            else if (text[charID] >= 'a' && text[charID] <= 'f') {
                charVal = text[charID] - 'a';
                charVal += 10;
            }
            else if (text[charID] >= 'A' && text[charID] <= 'F') {
                charVal = text[charID] - 'A';
                charVal += 10;
            }
            for (; --strlen; charVal *= base)
                ;
            *value += charVal;
        }

        --strLength;
        ++charID;
    }

    if (negative)
        *value = -*value;

    return true;
}