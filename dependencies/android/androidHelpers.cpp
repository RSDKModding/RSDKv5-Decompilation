#include <RSDK/Core/RetroEngine.hpp>
#include <main.hpp>
#include <pthread.h>

bool32 launched = false;
pthread_t mainthread;

using namespace RSDK;

int32 AndroidToWinAPIMappings(int32 mapping)
{
    switch (mapping) {
        default: return KEYMAP_NO_MAPPING;
        case AKEYCODE_HOME: return VK_HOME;
        case AKEYCODE_0: return VK_0;
        case AKEYCODE_1: return VK_1;
        case AKEYCODE_2: return VK_2;
        case AKEYCODE_3: return VK_3;
        case AKEYCODE_4: return VK_4;
        case AKEYCODE_5: return VK_5;
        case AKEYCODE_6: return VK_6;
        case AKEYCODE_7: return VK_7;
        case AKEYCODE_8: return VK_8;
        case AKEYCODE_9: return VK_9;
        case AKEYCODE_DPAD_UP: return VK_UP;
        case AKEYCODE_DPAD_DOWN: return VK_DOWN;
        case AKEYCODE_DPAD_LEFT: return VK_LEFT;
        case AKEYCODE_DPAD_RIGHT: return VK_RIGHT;
        case AKEYCODE_DPAD_CENTER: return VK_SELECT;
        case AKEYCODE_VOLUME_UP: return VK_VOLUME_UP;
        case AKEYCODE_VOLUME_DOWN: return VK_VOLUME_DOWN;
        case AKEYCODE_CLEAR: return VK_CLEAR;
        case AKEYCODE_A: return VK_A;
        case AKEYCODE_B: return VK_B;
        case AKEYCODE_C: return VK_C;
        case AKEYCODE_D: return VK_D;
        case AKEYCODE_E: return VK_E;
        case AKEYCODE_F: return VK_F;
        case AKEYCODE_G: return VK_G;
        case AKEYCODE_H: return VK_H;
        case AKEYCODE_I: return VK_I;
        case AKEYCODE_J: return VK_J;
        case AKEYCODE_K: return VK_K;
        case AKEYCODE_L: return VK_L;
        case AKEYCODE_M: return VK_M;
        case AKEYCODE_N: return VK_N;
        case AKEYCODE_O: return VK_O;
        case AKEYCODE_P: return VK_P;
        case AKEYCODE_Q: return VK_Q;
        case AKEYCODE_R: return VK_R;
        case AKEYCODE_S: return VK_S;
        case AKEYCODE_T: return VK_T;
        case AKEYCODE_U: return VK_U;
        case AKEYCODE_V: return VK_V;
        case AKEYCODE_W: return VK_W;
        case AKEYCODE_X: return VK_X;
        case AKEYCODE_Y: return VK_Y;
        case AKEYCODE_Z: return VK_Z;
        case AKEYCODE_COMMA: return VK_OEM_COMMA;
        case AKEYCODE_PERIOD: return VK_OEM_PERIOD;
        case AKEYCODE_ALT_LEFT: return VK_LMENU;
        case AKEYCODE_ALT_RIGHT: return VK_RMENU;
        case AKEYCODE_SHIFT_LEFT: return VK_LSHIFT;
        case AKEYCODE_SHIFT_RIGHT: return VK_RSHIFT;
        case AKEYCODE_TAB: return VK_TAB;
        case AKEYCODE_SPACE: return VK_SPACE;
        case AKEYCODE_ENVELOPE: return VK_LAUNCH_MAIL;
        case AKEYCODE_ENTER: return VK_RETURN;
        case AKEYCODE_MINUS: return VK_OEM_MINUS;
        case AKEYCODE_MENU: return VK_MENU;
        case AKEYCODE_MEDIA_PLAY_PAUSE: return VK_MEDIA_PLAY_PAUSE;
        case AKEYCODE_MEDIA_STOP: return VK_MEDIA_STOP;
        case AKEYCODE_MEDIA_NEXT: return VK_MEDIA_NEXT_TRACK;
        case AKEYCODE_MEDIA_PREVIOUS: return VK_MEDIA_PREV_TRACK;
        case AKEYCODE_MUTE: return VK_VOLUME_MUTE;
        case AKEYCODE_PAGE_UP: return VK_PRIOR;
        case AKEYCODE_PAGE_DOWN: return VK_NEXT;
        case AKEYCODE_ESCAPE: return VK_ESCAPE;
        case AKEYCODE_DEL: return VK_BACK; // ???????????? i hate android
        case AKEYCODE_FORWARD_DEL: return VK_DELETE;
        case AKEYCODE_CTRL_LEFT: return VK_LCONTROL;
        case AKEYCODE_CTRL_RIGHT: return VK_RCONTROL;
        case AKEYCODE_CAPS_LOCK: return VK_CAPITAL;
        case AKEYCODE_SCROLL_LOCK: return VK_SCROLL;
        case AKEYCODE_SYSRQ: return VK_SNAPSHOT;
        case AKEYCODE_BREAK: return VK_PAUSE;
        case AKEYCODE_MOVE_HOME: return VK_HOME;
        case AKEYCODE_MOVE_END: return VK_END;
        case AKEYCODE_INSERT: return VK_INSERT;
        case AKEYCODE_F1: return VK_F1;
        case AKEYCODE_F2: return VK_F2;
        case AKEYCODE_F3: return VK_F3;
        case AKEYCODE_F4: return VK_F4;
        case AKEYCODE_F5: return VK_F5;
        case AKEYCODE_F6: return VK_F6;
        case AKEYCODE_F7: return VK_F7;
        case AKEYCODE_F8: return VK_F8;
        case AKEYCODE_F9: return VK_F9;
        case AKEYCODE_F10: return VK_F10;
        case AKEYCODE_F11: return VK_F11;
        case AKEYCODE_F12: return VK_F12;
        case AKEYCODE_NUM_LOCK: return VK_NUMLOCK;
        case AKEYCODE_NUMPAD_0: return VK_NUMPAD0;
        case AKEYCODE_NUMPAD_1: return VK_NUMPAD1;
        case AKEYCODE_NUMPAD_2: return VK_NUMPAD2;
        case AKEYCODE_NUMPAD_3: return VK_NUMPAD3;
        case AKEYCODE_NUMPAD_4: return VK_NUMPAD4;
        case AKEYCODE_NUMPAD_5: return VK_NUMPAD5;
        case AKEYCODE_NUMPAD_6: return VK_NUMPAD6;
        case AKEYCODE_NUMPAD_7: return VK_NUMPAD7;
        case AKEYCODE_NUMPAD_8: return VK_NUMPAD8;
        case AKEYCODE_NUMPAD_9: return VK_NUMPAD9;
        case AKEYCODE_NUMPAD_DIVIDE: return VK_DIVIDE;
        case AKEYCODE_NUMPAD_MULTIPLY: return VK_MULTIPLY;
        case AKEYCODE_NUMPAD_SUBTRACT: return VK_SUBTRACT;
        case AKEYCODE_NUMPAD_ADD: return VK_ADD;
        case AKEYCODE_NUMPAD_DOT: return VK_DECIMAL;
        case AKEYCODE_NUMPAD_COMMA: return VK_OEM_COMMA;
        case AKEYCODE_NUMPAD_ENTER: return VK_RETURN;
        case AKEYCODE_VOLUME_MUTE: return VK_VOLUME_MUTE;
        case AKEYCODE_ZOOM_IN: return VK_ZOOM;
        case AKEYCODE_ZOOM_OUT: return VK_ZOOM;
        case AKEYCODE_SLEEP: return VK_SLEEP;
        case AKEYCODE_HELP: return VK_HELP;
    }
}

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
    PrintLog(PRINT_NORMAL, "on pause");
    videoSettings.windowState = WINDOWSTATE_INACTIVE;
}
JNIEXPORT void jnifuncN(nativeOnResume, RSDKv5)
{
#if RETRO_REV02
    if (SKU::userCore)
        SKU::userCore->focusState = 0;
#endif
    PrintLog(PRINT_NORMAL, "on resume");
    videoSettings.windowState = WINDOWSTATE_ACTIVE;
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
    pthread_mutex_lock(&RenderDevice::mutex);
    videoSettings.windowState = WINDOWSTATE_INACTIVE;
    RenderDevice::isInitialized = false;

    if (surface) {
        if (RenderDevice::window)
            ANativeWindow_release(RenderDevice::window);
        RenderDevice::window = ANativeWindow_fromSurface(env, surface);
        videoSettings.windowState = WINDOWSTATE_ACTIVE;
        PrintLog(PRINT_NORMAL, "Surface pass");
    }
    else {
        RenderDevice::window = NULL;
        PrintLog(PRINT_NORMAL, "Surface null");
    }
    pthread_mutex_unlock(&RenderDevice::mutex);
}

#define ACTION_DOWN 0 // 0b00
#define ACTION_UP   1 // 0b01
#define ACTION_MOVE 2 // 0b10
/* #define ACTION_CANCEL 3 */  // 0b11
/* #define ACTION_OUTSIDE 4 */ // 0b100
#define ACTION_POINTER_DOWN 5  // 0b101
#define ACTION_POINTER_UP   6 // 0b110

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

JNIEXPORT void jnifunc(nativeKeyDown, RSDKSurface, jint keycode)
{
#if !RETRO_REV02
    ++RSDK::SKU::buttonDownCount;
#endif
    switch (keycode) {
        case AKEYCODE_ENTER:

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
            RSDK::SKU::specialKeyStates[1] = true;
#endif
            // [fallthrough]

        default:
#if RETRO_INPUTDEVICE_KEYBOARD
            SKU::UpdateKeyState(AndroidToWinAPIMappings(keycode));
#endif
            break;

        case AKEYCODE_ESCAPE:
            if (engine.devMenu) {
                if (sceneInfo.state == ENGINESTATE_DEVMENU)
                    CloseDevMenu();
                else
                    OpenDevMenu();
            }
            else {
#if RETRO_INPUTDEVICE_KEYBOARD
                SKU::UpdateKeyState(AndroidToWinAPIMappings(keycode));
#endif
            }

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
            RSDK::SKU::specialKeyStates[0] = true;
#endif
            break;

#if !RETRO_USE_ORIGINAL_CODE
        case AKEYCODE_F1:
            sceneInfo.listPos--;
            if (sceneInfo.listPos < sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart) {
                sceneInfo.activeCategory--;
                if (sceneInfo.activeCategory >= sceneInfo.categoryCount) {
                    sceneInfo.activeCategory = sceneInfo.categoryCount - 1;
                }
                sceneInfo.listPos = sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd - 1;
            }

            LoadScene();
            break;

        case AKEYCODE_F2:
            sceneInfo.listPos++;
            if (sceneInfo.listPos >= sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd) {
                sceneInfo.activeCategory++;
                if (sceneInfo.activeCategory >= sceneInfo.categoryCount) {
                    sceneInfo.activeCategory = 0;
                }
                sceneInfo.listPos = sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart;
            }

            LoadScene();
            break;
#endif

        case AKEYCODE_F3:
            if (userShaderCount)
                videoSettings.shaderID = (videoSettings.shaderID + 1) % userShaderCount;
            break;

#if !RETRO_USE_ORIGINAL_CODE
        case AKEYCODE_F5:
            // Quick-Reload
            LoadScene();
            break;

        case AKEYCODE_F6:
            if (engine.devMenu && videoSettings.screenCount > 1)
                videoSettings.screenCount--;
            break;

        case AKEYCODE_F7:
            if (engine.devMenu && videoSettings.screenCount < SCREEN_COUNT)
                videoSettings.screenCount++;
            break;

        case AKEYCODE_F9:
            if (engine.devMenu)
                showHitboxes ^= 1;
            break;

        case AKEYCODE_F10:
            if (engine.devMenu)
                engine.showPaletteOverlay ^= 1;
            break;
#endif
        case AKEYCODE_DEL:
            if (engine.devMenu)
                engine.gameSpeed = engine.fastForwardSpeed;
            break;

        case AKEYCODE_F11:
        case AKEYCODE_INSERT:
            if ((sceneInfo.state & ENGINESTATE_STEPOVER) == ENGINESTATE_STEPOVER)
                engine.frameStep = true;
            break;

        case AKEYCODE_F12:
        case AKEYCODE_BREAK:
            if (engine.devMenu) {
                sceneInfo.state ^= ENGINESTATE_STEPOVER;
            }
            break;
    }
}

JNIEXPORT void jnifunc(nativeKeyUp, RSDKSurface, jint keycode)
{
#if !RETRO_REV02
    --RSDK::SKU::buttonDownCount;
#endif
    switch (keycode) {
        default:
#if RETRO_INPUTDEVICE_KEYBOARD
            SKU::ClearKeyState(AndroidToWinAPIMappings(keycode));
#endif
            break;

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
        case AKEYCODE_ESCAPE: RSDK::SKU::specialKeyStates[0] = false; break;
        case AKEYCODE_ENTER: RSDK::SKU::specialKeyStates[1] = false; break;
#endif
        case AKEYCODE_DEL: engine.gameSpeed = 1; break;
    }
}