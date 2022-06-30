
namespace Legacy
{
namespace v4
{
enum RetroStates {
    ENGINE_WAIT        = 3,
    ENGINE_INITPAUSE   = 5,
    ENGINE_EXITPAUSE   = 6,
    ENGINE_ENDGAME     = 7,
    ENGINE_RESETGAME   = 8,
};

bool32 LoadGameConfig(const char *filepath);
void ProcessEngine();

#if RETRO_USE_MOD_LOADER
void LoadXMLVariables();
void LoadXMLPalettes();
void LoadXMLObjects();
void LoadXMLSoundFX();
void LoadXMLPlayers();
int32 LoadXMLStages(int32 mode, int32 gcStageCount);
#endif

} // namespace v4
} // namespace Legacy