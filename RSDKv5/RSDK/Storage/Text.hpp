#ifndef TEXT_H
#define TEXT_H

namespace RSDK
{

struct GameVersionInfo {
    char gameName[0x40];
    char gameSubName[0x100];
    char gameVersion[0x10];
#if !RETRO_REV02
    uint8 platform;
    uint8 language;
    uint8 region;
#endif
};

extern GameVersionInfo gameVerInfo;

struct String {
    uint16 *chars; // text
    int16 length;  // string length
    int16 size;    // total alloc length
};

#if RETRO_REV0U
inline void StrCopy(char *dest, const char *src)
{
#ifdef USE_STDLIB
    strcpy(dest, src);
#else
    int i = 0;
    for (; src[i]; ++i) dest[i] = src[i];
    dest[i] = 0;
#endif
}

inline void StrAdd(char *dest, const char *src)
{
    int32 destStrPos = 0;
    int32 srcStrPos  = 0;
    while (dest[destStrPos]) ++destStrPos;
    while (true) {
        if (!src[srcStrPos]) {
            break;
        }
        dest[destStrPos++] = src[srcStrPos++];
    }
    dest[destStrPos] = 0;
}

inline bool StrComp(const char *stringA, const char *stringB)
{
    bool32 match    = true;
    bool32 finished = false;
    while (!finished) {
        if (*stringA == *stringB || *stringA == *stringB + ' ' || *stringA == *stringB - ' ') {
            if (*stringA) {
                ++stringA;
                ++stringB;
            }
            else {
                finished = true;
            }
        }
        else {
            match    = false;
            finished = true;
        }
    }

    return match;
}

inline int StrLength(const char *string)
{
    int len = 0;
    for (len = 0; string[len]; len++)
        ;
    return len;
}

int32 FindStringToken(const char *string, const char *token, uint8 stopID);
#endif

inline void StringLowerCase(char *dest, const char *src)
{
    int32 destPos = 0;
    int32 curChar = *src;
    if (*src) {
        int32 srcPos = 0;
        do {
            while ((uint32)curChar - 'A' < 26) {
                destPos       = srcPos;
                dest[destPos] = curChar + ' ';
                curChar       = src[++srcPos];
                if (!curChar) {
                    dest[++destPos] = 0;
                    return;
                }
            }
            destPos       = srcPos;
            dest[destPos] = curChar;
            curChar       = src[++srcPos];
        } while (curChar);
    }
    dest[++destPos] = 0;
}

inline void StringUpperCase(char *dest, const char *src)
{
    int32 destPos = 0;
    int32 curChar = *src;
    if (*src) {
        int32 srcPos = 0;
        do {
            while ((uint32)curChar - 'a' < 26) {
                destPos       = srcPos;
                dest[destPos] = curChar - ' ';
                curChar       = src[++srcPos];
                if (!curChar) {
                    dest[++destPos] = 0;
                    return;
                }
            }
            destPos       = srcPos;
            dest[destPos] = curChar;
            curChar       = src[++srcPos];
        } while (curChar);
    }
    dest[++destPos] = 0;
}

extern char textBuffer[0x400];
void GenerateHashMD5(uint32 *buffer, int32 len);
void GenerateHashCRC(uint32 *id, char *inputString);

#define RETRO_HASH_MD5(name) uint32 name[4]
#define HASH_SIZE_MD5        (4 * sizeof(uint32))
#define HASH_MATCH_MD5(a, b) (memcmp(a, b, HASH_SIZE_MD5) == 0)
#define GEN_HASH_MD5(text, hash)                                                                                                                     \
    strcpy(textBuffer, text);                                                                                                                        \
    GenerateHashMD5(hash, (int32)strlen(textBuffer))
#define HASH_COPY_MD5(dst, src) memcpy(dst, src, HASH_SIZE_MD5)
#define HASH_CLEAR_MD5(hash)    MEM_ZERO(hash)

inline void InitString(String *string, char *text, uint32 textLength)
{
    if (text) {
        string->chars = NULL;

        int32 pos = 0;
        while (text[pos]) string->length = ++pos;

        if (textLength && textLength >= string->length)
            string->size = textLength;
        else
            string->size = string->length;

        if (!string->size)
            string->size = 1;

        AllocateStorage((void **)&string->chars, sizeof(uint16) * string->size, DATASET_STR, false);

        pos = 0;
        while (text[pos]) {
            string->chars[pos] = text[pos];
            ++pos;
        }
    }
}
void SetString(String *string, char *text);

inline void CopyString(String *dst, String *src)
{
    if (dst == src)
        return;

    int32 srcLength = src->length;
    dst->chars      = NULL;
    if (dst->size >= srcLength) {
        if (!dst->chars) {
            AllocateStorage((void **)&dst->chars, sizeof(uint16) * dst->size, DATASET_STR, false);
        }
    }
    else {
        dst->size = srcLength;
        AllocateStorage((void **)&dst->chars, sizeof(uint16) * dst->size, DATASET_STR, false);
    }

    dst->length = src->length;
    for (int32 c = 0; c < dst->length; ++c) dst->chars[c] = src->chars[c];
}

inline void GetCString(char *destChars, String *string)
{
    if (!string->chars)
        return;

    char *cString = destChars ? destChars : (char *)"";
    int32 textLen = destChars ? string->length : 0x400;

    int32 c = 0;
    for (; c < textLen; ++c) cString[c] = string->chars[c];
    cString[c] = 0;
}

void AppendText(String *string, char *appendText);
void AppendString(String *string, String *appendString);
bool32 CompareStrings(String *string1, String *string2, bool32 exactMatch);

void InitStringList(String *stringList, int32 size);
void LoadStringList(String *stringList, const char *filePath, uint32 charSize);
bool32 SplitStringList(String *splitStrings, String *stringList, int32 startStringID, int32 stringCount);

#if RETRO_REV0U
#include "Legacy/TextLegacy.hpp"
#endif

} // namespace RSDK

#endif
