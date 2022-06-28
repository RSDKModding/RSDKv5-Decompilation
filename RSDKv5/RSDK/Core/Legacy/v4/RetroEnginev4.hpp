
namespace Legacy
{
namespace v4
{
enum RetroStates {
    ENGINE_WAIT        = 3,
    ENGINE_SCRIPTERROR = 4,
    ENGINE_INITPAUSE   = 5,
    ENGINE_EXITPAUSE   = 6,
    ENGINE_ENDGAME     = 7,
    ENGINE_RESETGAME   = 8,
};

bool32 LoadGameConfig(const char *filepath);
void ProcessEngine();

} // namespace v4
} // namespace Legacy