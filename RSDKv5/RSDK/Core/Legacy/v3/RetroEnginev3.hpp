
namespace Legacy
{
namespace v3
{
enum RetroStates {
    ENGINE_EXITGAME        = 3,
    ENGINE_ENTER_HIRESMODE = 5,
    ENGINE_EXIT_HIRESMODE  = 6,
    ENGINE_PAUSE           = 7,
    ENGINE_WAIT            = 8,
    ENGINE_VIDEOWAIT       = 9,
};

extern int32 engineMessage;

bool32 LoadGameConfig(const char *filepath);
void ProcessEngine();

void RetroEngineCallback(int32 callbackID);

#if RETRO_USE_MOD_LOADER
void LoadXMLVariables();
void LoadXMLPalettes();
void LoadXMLObjects();
void LoadXMLSoundFX();
void LoadXMLPlayers();
int32 LoadXMLStages(int32 mode, int32 gcStageCount);
#endif

} // namespace v3
} // namespace Legacy