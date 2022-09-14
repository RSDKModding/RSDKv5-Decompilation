
#define LockAudioDevice()   ;
#define UnlockAudioDevice() ;

namespace RSDK
{
class AudioDevice : public AudioDeviceBase
{
public:
    static bool32 Init();
    static void Release();

    static void FrameInit() {}

    inline static void HandleStreamLoad(ChannelInfo *channel, bool32 async) { LoadStream(channel); }

private:
    static uint8 contextInitialized;

    static void InitAudioChannels();
    static void InitMixBuffer() {}
};
} // namespace RSDK