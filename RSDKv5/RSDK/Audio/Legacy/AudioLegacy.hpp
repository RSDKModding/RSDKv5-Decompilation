
namespace Legacy
{
#define LEGACY_TRACK_COUNT (0x10)

struct TrackInfo {
    char fileName[0x40];
    bool32 trackLoop;
    uint32 loopPoint;
};

extern int32 globalSFXCount;
extern int32 stageSFXCount;

extern int32 musicVolume;
extern int32 sfxVolume;
extern int32 bgmVolume;
extern int32 musicCurrentTrack;
extern int32 musicChannel;

extern TrackInfo musicTracks[LEGACY_TRACK_COUNT];

void SetMusicTrack(const char *filePath, uint8 trackID, bool32 loop, uint32 loopPoint);
int32 PlayMusic(int32 trackID);
inline void StopMusic() { StopChannel(musicChannel); }
void SetMusicVolume(uint32 volume);

// helper func to ensure compatibility with how v3/v4 expects it (no plays var and slot is set manually)
void LoadSfx(char *filename, uint8 slot, uint8 scope);
inline void PlaySfx(int32 sfxID, bool32 loop) { RSDK::PlaySfx(sfxID, loop, 0xFF); }
inline void StopSfx(int32 sfxID) { RSDK::StopSfx(sfxID); }
void SetSfxAttributes(int32 sfxID, int32 loop, int8 pan);

namespace v3
{
extern char globalSfxNames[SFX_COUNT][0x40];
extern char stageSfxNames[SFX_COUNT][0x40];

void SetSfxName(const char *sfxName, int32 sfxID, bool32 global);
} // namespace v3

namespace v4
{
extern float musicRatio;
extern char sfxNames[SFX_COUNT][0x40];

void SwapMusicTrack(const char *filePath, uint8 trackID, uint32 loopPoint, uint32 ratio);

void SetSfxName(const char *sfxName, int32 sfxID);
} // namespace v4

} // namespace Legacy