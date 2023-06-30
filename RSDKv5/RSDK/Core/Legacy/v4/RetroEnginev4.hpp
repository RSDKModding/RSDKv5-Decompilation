
namespace Legacy
{
namespace v4
{
enum RetroStates {
    ENGINE_WAIT      = 3,
    ENGINE_INITPAUSE = 5,
    ENGINE_EXITPAUSE = 6,
    ENGINE_ENDGAME   = 7,
    ENGINE_RESETGAME = 8,
};

bool32 LoadGameConfig(const char *filepath);
void ProcessEngine();

#if RETRO_USE_MOD_LOADER
void LoadGameXML(bool pal = false);
void LoadXMLVariables(const tinyxml2::XMLElement* gameElement);
void LoadXMLPalettes(const tinyxml2::XMLElement* gameElement);
void LoadXMLObjects(const tinyxml2::XMLElement* gameElement);
void LoadXMLSoundFX(const tinyxml2::XMLElement* gameElement);
void LoadXMLPlayers(const tinyxml2::XMLElement* gameElement);
void LoadXMLStages(const tinyxml2::XMLElement* gameElement);
#endif

} // namespace v4
} // namespace Legacy