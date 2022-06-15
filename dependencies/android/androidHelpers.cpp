#include <RSDK/Core/RetroEngine.hpp>
#include <main.hpp>
#include <pthread.h>

bool32 launched = false;
pthread_t mainthread;
int32 returnCode;


using namespace RSDK;

void *threadCallback(void *a)
{
    while (!RenderDevice::window)
        ;
    return (void *)RSDK_main(0, NULL, (void *)LinkGameLogic);
}

JNIEXPORT void jnifuncN(nativeOnPause, RSDKv5)
{
#if RETRO_REV02
    if (SKU::userCore)
        SKU::userCore->focusState = 1;
#endif
    videoSettings.windowState = WINDOWSTATE_UNINITIALIZED;
    RenderDevice::Release(true);
}
JNIEXPORT void jnifuncN(nativeOnResume, RSDKv5)
{
#if RETRO_REV02
    if (SKU::userCore)
        SKU::userCore->focusState = 0;
#endif
    if (RenderDevice::isInitialized)
        RenderDevice::Release(true);
    if (RenderDevice::isRunning)
        RenderDevice::Init();
}
JNIEXPORT void jnifunc(nativeOnStart, RSDKv5, jstring basepath)
{
    if (!launched) {
        char buffer[0x200];
        strcpy(buffer, env->GetStringUTFChars((jstring)basepath, NULL));
        SKU::SetUserFileCallbacks(buffer, NULL, NULL);
        RenderDeviceBase::isRunning = false;
        pthread_create(&mainthread, NULL, &threadCallback, NULL);
        launched = true;
    }
}
JNIEXPORT void jnifuncN(nativeOnStop, RSDKv5)
{
    RenderDevice::isRunning = false;
    pthread_join(mainthread, NULL);
}

JNIEXPORT void jnifunc(nativeSetSurface, RSDKSurface, jobject surface)
{
    videoSettings.windowState = WINDOWSTATE_UNINITIALIZED;
    if (RenderDevice::isInitialized)
        RenderDevice::Release(true);

    if (surface) {
        RenderDevice::window = ANativeWindow_fromSurface(env, surface);
        if (RenderDevice::isRunning)
            RenderDevice::Init();
    }
    else {
        ANativeWindow_release(RenderDevice::window);
    }
}

#define ACTION_DOWN 0 // 0b00
#define ACTION_UP 1 // 0b01
#define ACTION_MOVE 2 // 0b10
/* #define ACTION_CANCEL 3 */ // 0b11
/* #define ACTION_OUTSIDE 4 */ // 0b100
#define ACTION_POINTER_DOWN 5 // 0b101
#define ACTION_POINTER_UP 6 // 0b110

JNIEXPORT void jnifunc(nativeOnTouch, RSDKSurface, jint finger, jint action, jfloat x, jfloat y)
{
    if (finger > 0x10)
        return; // nah cause how tf

    bool32 down = (action == ACTION_DOWN) || (action == ACTION_MOVE) || (action == ACTION_POINTER_DOWN);

    if (down) {
        touchInfo.x[finger]    = x;
        touchInfo.y[finger]    = y;
        touchInfo.down[finger] = true;
        if (touchInfo.count < finger + 1)
            touchInfo.count = finger + 1;
    }
    else {
        touchInfo.down[finger] = false;
        if (touchInfo.count >= finger + 1) {
            for (; touchInfo.count > 0; touchInfo.count--) {
                if (touchInfo.down[touchInfo.count - 1])
                    break;
            }
        }
    }
}
