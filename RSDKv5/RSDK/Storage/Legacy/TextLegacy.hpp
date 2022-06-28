
namespace Legacy
{

#define LEGACY_TEXTDATA_COUNT  (0x2800)
#define LEGACY_TEXTENTRY_COUNT (0x200)
#define LEGACY_TEXTMENU_COUNT  (0x2)

enum TextInfoTypes { TEXTINFO_TEXTDATA = 0, TEXTINFO_TEXTSIZE = 1, TEXTINFO_ROWCOUNT = 2 };

struct TextMenu {
    uint16 textData[LEGACY_TEXTDATA_COUNT];
    int32 entryStart[LEGACY_TEXTENTRY_COUNT];
    int32 entrySize[LEGACY_TEXTENTRY_COUNT];
    uint8 entryHighlight[LEGACY_TEXTENTRY_COUNT];
    int32 textDataPos;
    int32 selection1;
    int32 selection2;
    uint16 rowCount;
    uint16 visibleRowCount;
    uint16 visibleRowOffset;
    uint8 alignment;
    uint8 selectionCount;
    int8 timer;
};

enum TextMenuAlignments {
    MENU_ALIGN_LEFT,
    MENU_ALIGN_RIGHT,
    MENU_ALIGN_CENTER,
};

extern TextMenu gameMenu[LEGACY_TEXTMENU_COUNT];
extern int32 textMenuSurfaceNo;

void LoadTextFile(TextMenu *menu, const char *filePath, uint8 mapCode);

void SetupTextMenu(TextMenu *menu, int32 rowCount);
void AddTextMenuEntry(TextMenu *menu, const char *text);
void SetTextMenuEntry(TextMenu *menu, const char *text, int32 rowID);
void EditTextMenuEntry(TextMenu *menu, const char *text, int32 rowID);

}