
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
void LoadGameXML(bool pal = false);
void LoadXMLVariables(const tinyxml2::XMLElement* gameElement);
void LoadXMLPalettes(const tinyxml2::XMLElement* gameElement);
void LoadXMLObjects(const tinyxml2::XMLElement* gameElement);
void LoadXMLSoundFX(const tinyxml2::XMLElement* gameElement);
void LoadXMLPlayers(const tinyxml2::XMLElement* gameElement);
void LoadXMLStages(const tinyxml2::XMLElement* gameElement);
#endif

} // namespace v3
} // namespace Legacy