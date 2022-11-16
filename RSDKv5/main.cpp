#include "RSDK/Core/RetroEngine.hpp"
#include "main.hpp"

#if RETRO_STANDALONE

#if RETRO_PLATFORM == RETRO_WIN && !RETRO_RENDERDEVICE_SDL2

#if RETRO_RENDERDEVICE_DIRECTX9 || RETRO_RENDERDEVICE_DIRECTX11
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nShowCmd)
{
    RSDK::RenderDevice::hInstance     = hInstance;
    RSDK::RenderDevice::hPrevInstance = hPrevInstance;
    RSDK::RenderDevice::nShowCmd      = nShowCmd;

    return RSDK_main(1, &lpCmdLine, RSDK::LinkGameLogic);
}
#else
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nShowCmd)
{
    return RSDK_main(1, &lpCmdLine, RSDK::LinkGameLogic);
}
#endif

#elif RETRO_PLATFORM == RETRO_ANDROID
extern "C" {
void android_main(struct android_app *app);
}

#include <swappy/swappyGL_extra.h>

void android_main(struct android_app *ap)
{
    app                                 = ap;
    app->onAppCmd                       = AndroidCommandCallback;
    app->activity->callbacks->onKeyDown = AndroidKeyDownCallback;
    app->activity->callbacks->onKeyUp   = AndroidKeyUpCallback;

    JNISetup *jni = GetJNISetup();
    // we make sure we do it here so init can chill safely before any callbacks occur
    Paddleboat_init(jni->env, jni->thiz);
    
    SwappyGL_init(jni->env, jni->thiz);
    SwappyGL_setSwapIntervalNS(1000000000L / RSDK::videoSettings.refreshRate);
    SwappyGL_setAutoSwapInterval(false);

    char buffer[0x200];
    jmethodID method = jni->env->GetMethodID(jni->clazz, "getBasePath", "()Ljava/lang/String;");
    auto ret         = jni->env->CallObjectMethod(jni->thiz, method);
    strcpy(buffer, jni->env->GetStringUTFChars((jstring)ret, NULL));
    RSDK::SKU::SetUserFileCallbacks(buffer, NULL, NULL);

    GameActivity_setWindowFlags(app->activity,
                                AWINDOW_FLAG_KEEP_SCREEN_ON | AWINDOW_FLAG_TURN_SCREEN_ON | AWINDOW_FLAG_LAYOUT_NO_LIMITS | AWINDOW_FLAG_FULLSCREEN
                                    | AWINDOW_FLAG_SHOW_WHEN_LOCKED,
                                0);

    RSDK_main(0, NULL, (void *)RSDK::LinkGameLogic);

    Paddleboat_destroy(jni->env);
    SwappyGL_destroy();
}
#else
int32 main(int32 argc, char *argv[]) { return RSDK_main(argc, argv, (void *)RSDK::LinkGameLogic); }
#endif

#endif

int32 RSDK_main(int32 argc, char **argv, void *linkLogicPtr)
{
    RSDK::linkGameLogic = (RSDK::LogicLinkHandle)linkLogicPtr;

    RSDK::InitCoreAPI();

    int32 exitCode = RSDK::RunRetroEngine(argc, argv);

    RSDK::ReleaseCoreAPI();

    return exitCode;
}