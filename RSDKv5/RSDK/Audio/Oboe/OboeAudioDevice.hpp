#define LockAudioDevice()   pthread_mutex_lock(&AudioDevice::mutex);
#define UnlockAudioDevice() pthread_mutex_unlock(&AudioDevice::mutex);

#include <oboe/Oboe.h>

namespace RSDK
{
struct AudioDevice : public AudioDeviceBase, public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback {
    static bool32 Init();
    static void Release();

    static void ProcessAudioMixing(void *stream, int32 length);

    static void FrameInit();

    static void HandleStreamLoad(ChannelInfo *channel, bool32 async);

    static pthread_mutex_t mutex;

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *, void *data, int32 len);
    bool onError(oboe::AudioStream *, oboe::Result error);

private:
    static uint8 contextInitialized;
    static oboe::Result status;
    static oboe::AudioStream *stream;

    static AudioDevice *audioDevice; // can't believe i have to do this

    static void InitAudioChannels();
    static void InitMixBuffer() {}

    static void *LoadStreamASync(void *channel)
    {
        LoadStream((ChannelInfo *)channel);
        pthread_exit(NULL);
    };
};
} // namespace RSDK
