#define LockAudioDevice()   pthread_mutex_lock(&AudioDevice::mutex);
#define UnlockAudioDevice() pthread_mutex_unlock(&AudioDevice::mutex);

#include <aaudio/AAudio.h>

struct AudioDevice : public AudioDeviceBase {
    static bool32 Init();
    static void Release();

    static void ProcessAudioMixing(void *stream, int32 length);

    static void FrameInit();

    static void HandleStreamLoad(ChannelInfo *channel, bool32 async);

    static pthread_mutex_t mutex;
private:
    static uint8 contextInitialized;
    static aaudio_result_t status;
    static AAudioStream *stream;


    static void InitAudioChannels();
    static void InitMixBuffer() {}

    static void ErrorCallback(AAudioStream* stream, void*, aaudio_result_t error);
    static aaudio_data_callback_result_t AudioCallback(AAudioStream* stream, void *, void *data, int32 len);
    
    static void* LoadStreamASync(void* channel)
    {
        LoadStream((ChannelInfo *)channel);
        pthread_exit(NULL);
    };
};