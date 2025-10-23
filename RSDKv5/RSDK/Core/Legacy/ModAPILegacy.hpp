

namespace Legacy
{

#if RETRO_USE_MOD_LOADER

#define LEGACY_PLAYERNAME_COUNT (0x10)

extern char modTypeNames[OBJECT_COUNT][0x40];
extern char modScriptPaths[OBJECT_COUNT][0x40];
extern uint8 modScriptFlags[OBJECT_COUNT];
extern uint8 modObjCount;

namespace v4
{

// Native Functions
int32 OpenModMenu();

void RefreshEngine();
void GetModCount();
void GetModName(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused);
void GetModDescription(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused);
void GetModAuthor(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused);
void GetModVersion(int32 *textMenu, int32 *highlight, uint32 *id, int32 *unused);
void GetModID(int32 *unused, const char *modFolder);
void GetModActive(uint32 *id, int32 *unused);
void SetModActive(uint32 *id, int32 *active);
void MoveMod(uint32 *id, int32 *up);

void ExitGame();
void FileExists(int32 *unused, const char *filePath);

void AddGameAchievement(int32 *unused, const char *name);
void SetAchievementDescription(uint32 *id, const char *desc);
void ClearAchievements();
void GetAchievementCount();
void GetAchievementName(uint32 *id, int32 *textMenu);
void GetAchievementDescription(uint32 *id, int32 *textMenu);
void GetAchievement(uint32 *id, void *unused);

void GetScreenWidth();
void SetScreenWidth(int32 *width, int32 *unused);
void GetWindowScale();
void SetWindowScale(int32 *scale, int32 *unused);
void GetWindowScaleMode();
void SetWindowScaleMode(int32 *mode, int32 *unused);
void GetWindowFullScreen();
void SetWindowFullScreen(int32 *fullscreen, int32 *unused);
void GetWindowBorderless();
void SetWindowBorderless(int32 *borderless, int32 *unused);
void GetWindowVSync();
void SetWindowVSync(int32 *enabled, int32 *unused);
void ApplyWindowChanges();
} // namespace v4

#endif

} // namespace Legacy