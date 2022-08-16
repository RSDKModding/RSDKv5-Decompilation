#include "RSDK/Core/RetroEngine.hpp"
#include "main.hpp"

#ifdef __SWITCH__
#include <switch.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>

static int32 s_nxlinkSock = -1;

static void initNxLink()
{
    if (R_FAILED(socketInitializeDefault()))
        return;

    s_nxlinkSock = nxlinkStdio();
    if (s_nxlinkSock >= 0)
        printf("printf output now goes to nxlink server\n");
    else
        socketExit();
}
#endif

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

void android_main(struct android_app *ap)
{
    app                                 = ap;
    app->onAppCmd                       = AndroidCommandCallback;
    app->activity->callbacks->onKeyDown = AndroidKeyDownCallback;
    app->activity->callbacks->onKeyUp   = AndroidKeyUpCallback;

    JNISetup *jni = GetJNISetup();
    // we make sure we do it here so init can chill safely before any callbacks occur
    Paddleboat_init(jni->env, jni->thiz);
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
}
#else
int32 main(int32 argc, char *argv[]) { return RSDK_main(argc, argv, (void *)RSDK::LinkGameLogic); }
#endif

#endif

int32 RSDK_main(int32 argc, char **argv, void *linkLogicPtr)
{
#ifdef __SWITCH__
    // initNxLink();
#endif

    RSDK::linkGameLogic = (RSDK::LogicLinkHandle)linkLogicPtr;

    int32 exitCode = RSDK::RunRetroEngine(argc, argv);

#ifdef __SWITCH__
    // socketExit();
#endif

    return exitCode;
}