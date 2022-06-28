
namespace Legacy
{
namespace v3
{
enum RetroStates {
    ENGINE_EXITGAME        = 3,
    ENGINE_SCRIPTERROR     = 4,
    ENGINE_ENTER_HIRESMODE = 5,
    ENGINE_EXIT_HIRESMODE  = 6,
    ENGINE_PAUSE           = 7,
    ENGINE_WAIT            = 8,
    ENGINE_VIDEOWAIT       = 9,
};

bool32 LoadGameConfig(const char *filepath);
void ProcessEngine();

} // namespace v3
} // namespace Legacy