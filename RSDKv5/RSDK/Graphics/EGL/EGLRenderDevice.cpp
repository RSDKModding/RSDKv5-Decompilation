#ifdef _INTELLISENSE
#ifdef _INTELLISENSE_NX
#undef __unix__
#undef __linux__
#endif
#include <glad/glad.h>
#include "EGLRenderDevice.hpp"
//#include "RetroEngine.hpp"
#endif

#include <chrono>

#if RETRO_PLATFORM == RETRO_SWITCH
#define _GLVERSION "#version 330 core\n#define in_V in\n#define in_F in\n"

#define _glVPrecision ""
#define _glFPrecision ""

#define _YOFF 16
#define _UOFF 8
#define _VOFF 0
#elif RETRO_PLATFORM == RETRO_ANDROID
#define _GLVERSION                                                                                                                                   \
    "#version 100\n#extension GL_OES_standard_derivatives : enable\n#define in_V attribute\n#define out varying\n#define in_F varying\n"

#define GL_BGRA                     GL_RGBA
#define GL_UNSIGNED_INT_8_8_8_8_REV GL_UNSIGNED_BYTE

char _glVPrecision[30]; // len("precision mediump float;\n") -> 25
char _glFPrecision[30]; // len("precision mediump float;\n") -> 25

#define _YOFF 0
#define _UOFF 8
#define _VOFF 16
#endif

#if RETRO_REV02
#define _GLDEFINE "#define RETRO_REV02 (1)\n"
#else
#define _GLDEFINE "\n"
#endif

const GLchar *backupVertex = R"aa(
in_V vec3 in_pos;
in_V vec2 in_UV;
out vec4 ex_color;
out vec2 ex_UV;

void main()
{
    gl_Position = vec4(in_pos, 1.0);
    ex_color    = vec4(1.0, 1.0, 1.0, 1.0);
    ex_UV       = in_UV;
}
)aa";

const GLchar *backupFragment = R"aa(
in_F vec2 ex_UV;
in_F vec4 ex_color;

uniform sampler2D texDiffuse;

void main()
{
    gl_FragColor = texture2D(texDiffuse, ex_UV);
}
)aa";

EGLDisplay RenderDevice::display;
EGLContext RenderDevice::context;
EGLSurface RenderDevice::surface;
EGLConfig RenderDevice::config;

#if RETRO_PLATFORM == RETRO_SWITCH
NWindow *RenderDevice::window;
#elif RETRO_PLATFORM == RETRO_ANDROID
ANativeWindow *RenderDevice::window;
#endif

GLuint RenderDevice::VAO;
GLuint RenderDevice::VBO;

GLuint RenderDevice::screenTextures[SCREEN_COUNT];
GLuint RenderDevice::imageTexture;

double RenderDevice::lastFrame;
double RenderDevice::targetFreq;

int32 RenderDevice::monitorIndex;

uint32 *RenderDevice::videoBuffer;

bool32 RenderDevice::isInitialized = false;

bool RenderDevice::Init()
{
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (!display) {
        PrintLog(PRINT_NORMAL, "[EGL] Could not connect to display: %d", eglGetError());
        return false;
    }

    eglInitialize(display, nullptr, nullptr);

#if RETRO_PLATFORM == RETRO_SWITCH
    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        PrintLog(PRINT_NORMAL, "[EGL] eglBindApi failure: %d", eglGetError());
        return false;
    }
#elif RETRO_PLATFORM == RETRO_ANDROID
#endif

    EGLint numConfigs;
    // clang-format off
    static const EGLint framebufferAttributeList[] = {
#if RETRO_PLATFORM == RETRO_SWITCH
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#else
        EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#endif 
        EGL_RED_SIZE,        8, 
        EGL_GREEN_SIZE,      8, 
        EGL_BLUE_SIZE,       8, 
        EGL_NONE
    };
    // clang-format on
    eglChooseConfig(display, framebufferAttributeList, &config, 1, &numConfigs);
    if (numConfigs == 0) {
        PrintLog(PRINT_NORMAL, "[EGL] No configs found: %d", eglGetError());
        return false;
    }

    if (!SetupRendering())
        return false;
    if (!isRunning) {
        if (!AudioDevice::Init())
            return false;
        InitInputDevices();
    }
    isInitialized = true;
    return true;
}

bool RenderDevice::SetupRendering()
{
#if RETRO_PLATFORM == RETRO_SWITCH
    window = nwindowGetDefault();
    nwindowSetDimensions(window, 1920, 1080);
#elif RETRO_PLATFORM == RETRO_ANDROID
    EGLint format;
    if (!eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format)) {
        PrintLog(PRINT_NORMAL, "[EGL] EGL_NATIVE_VISUAL_ID fetch failed: %d", eglGetError());
        return false;
    }
    if (!window) {
#if RETRO_REV02
        if (SKU::userCore)
            SKU::userCore->focusState = 1;
#else
        engine.focusState = 1;
#endif
        return true; // lie so we can properly swtup later
    }
    ANativeWindow_setBuffersGeometry(window, 0, 0, format);
    SwappyGL_setSwapIntervalNS(SWAPPY_SWAP_60FPS);
    SwappyGL_setMaxAutoSwapIntervalNS(SWAPPY_SWAP_60FPS);
#endif

    surface = eglCreateWindowSurface(display, config, window, nullptr);
    if (!surface) {
        PrintLog(PRINT_NORMAL, "[EGL] Surface creation failed: %d", eglGetError());
        return false;
    }

#if RETRO_PLATFORM == RETRO_SWITCH
    // clang-format off
    static const int32 listCount = 1;
    static const EGLint attributeListList[listCount][7] = { {
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_CONTEXT_MAJOR_VERSION,       4, 
        EGL_CONTEXT_MINOR_VERSION,       3,
        EGL_NONE 
    } };
    int32 i = 0;
    // clang-format on
#elif RETRO_PLATFORM == RETRO_ANDROID
    static const int32 listCount                     = 1;
    static const EGLint attributeListList[listCount][3] = { { EGL_CONTEXT_MAJOR_VERSION, 2, EGL_NONE } }; // lol. lmao
    int32 i                                          = 0;
#endif

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, (EGLint *)attributeListList[i]);
    while (!context) {
        PrintLog(PRINT_NORMAL, "[EGL] Context creation failed: %d", eglGetError());
        if (++i < listCount) {
            PrintLog(PRINT_NORMAL, "[EGL] Trying next context...");
            context = eglCreateContext(display, config, EGL_NO_CONTEXT, (EGLint *)attributeListList[i]);
        }
        else
            return false;
    }

    eglMakeCurrent(display, surface, surface, context);
    eglQueryContext(display, context, EGL_CONTEXT_CLIENT_VERSION, &i);
    PrintLog(PRINT_NORMAL, "[EGL] Context client version: %d", i);

    GetDisplays();

    if (!InitGraphicsAPI() || !InitShaders())
        return false;

    int32 size = videoSettings.pixWidth >= SCREEN_YSIZE ? videoSettings.pixWidth : SCREEN_YSIZE;
    if (scanlines)
        free(scanlines);
    scanlines = (ScanlineInfo *)malloc(size * sizeof(ScanlineInfo));
    memset(scanlines, 0, size * sizeof(ScanlineInfo));

    videoSettings.windowState = WINDOWSTATE_ACTIVE;
    videoSettings.dimMax      = 1.0;
    videoSettings.dimPercent  = 1.0;

    return true;
}

void RenderDevice::GetDisplays()
{

    displayCount = 1;
    GetWindowSize(&displayWidth[0], &displayHeight[0]);
    // reacting to me lying
    displayInfo.displays                 = (decltype(displayInfo.displays))malloc(sizeof(displayInfo.displays->internal));
    displayInfo.displays[0].width        = displayWidth[0];
    displayInfo.displays[0].height       = displayHeight[0];
    displayInfo.displays[0].refresh_rate = videoSettings.refreshRate;
}

bool RenderDevice::InitGraphicsAPI()
{
#if RETRO_PLATFORM == RETRO_SWITCH
    if (!gladLoadGL()) {
        PrintLog(PRINT_NORMAL, "[EGL] gladLoadGL failure");
        return false;
    }
#else
    GLint range[2], precision;

    glGetShaderPrecisionFormat(GL_VERTEX_SHADER, GL_HIGH_FLOAT, range, &precision);
    if (!precision)
        strcpy(_glVPrecision, "precision mediump float;\n");
    else
        strcpy(_glVPrecision, "precision highp float;\n");

    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
    if (!precision)
        strcpy(_glFPrecision, "precision mediump float;\n");
    else
        strcpy(_glFPrecision, "precision highp float;\n");

#endif
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);

    // setup buffers
#if RETRO_PLATFORM == RETRO_SWITCH
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
#endif

    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RenderVertex) * (!RETRO_REV02 ? 24 : 60), NULL, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), 0);
    glEnableVertexAttribArray(0);
    // glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RenderVertex), (void *)offsetof(RenderVertex, color));
    // glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), (void *)offsetof(RenderVertex, tex));
    glEnableVertexAttribArray(1);

#if RETRO_PLATFORM == RETRO_SWITCH
    videoSettings.fsWidth  = 1920;
    videoSettings.fsHeight = 1080;
#elif RETRO_PLATFORM == RETRO_ANDROID
    customSettings.maxPixWidth = 0;
    videoSettings.fsWidth      = 0;
    videoSettings.fsHeight     = 0;
#endif

    // EGL should only be fullscreen only apps
#if false
    if (videoSettings.windowed || !videoSettings.exclusiveFS) {
        if (videoSettings.windowed) {
            viewSize.x = videoSettings.windowWidth;
            viewSize.y = videoSettings.windowHeight;
        }
        else {
            viewSize.x = displayWidth[monitorIndex];
            viewSize.y = displayHeight[monitorIndex];
        }
    }
    else
#endif
    {
        int32 bufferWidth  = videoSettings.fsWidth;
        int32 bufferHeight = videoSettings.fsHeight;
        if (videoSettings.fsWidth <= 0 || videoSettings.fsHeight <= 0) {
            bufferWidth  = displayWidth[monitorIndex];
            bufferHeight = displayHeight[monitorIndex];
        }
        viewSize.x = bufferWidth;
        viewSize.y = bufferHeight;
    }

    int32 maxPixHeight = 0;
#if !RETRO_USE_ORIGINAL_CODE
    int32 screenWidth = 0;
#endif
    for (int32 s = 0; s < SCREEN_COUNT; ++s) {
        if (videoSettings.pixHeight > maxPixHeight)
            maxPixHeight = videoSettings.pixHeight;

        screens[s].size.y = videoSettings.pixHeight;

        float viewAspect = viewSize.x / viewSize.y;
#if !RETRO_USE_ORIGINAL_CODE
        screenWidth = (int32)((viewAspect * videoSettings.pixHeight) + 3) & 0xFFFFFFFC;
#else
        int32 screenWidth = (int32)((viewAspect * videoSettings.pixHeight) + 3) & 0xFFFFFFFC;
#endif
        if (screenWidth < videoSettings.pixWidth)
            screenWidth = videoSettings.pixWidth;

#if !RETRO_USE_ORIGINAL_CODE
        if (customSettings.maxPixWidth && screenWidth > customSettings.maxPixWidth)
            screenWidth = customSettings.maxPixWidth;
#else
        if (screenWidth > DEFAULT_PIXWIDTH)
            screenWidth = DEFAULT_PIXWIDTH;
#endif

        memset(&screens[s].frameBuffer, 0, sizeof(screens[s].frameBuffer));
        SetScreenSize(s, screenWidth, screens[s].size.y);
    }

    pixelSize.x     = screens[0].size.x;
    pixelSize.y     = screens[0].size.y;
    float pixAspect = pixelSize.x / pixelSize.y;

#if RETRO_PLATFORM == RETRO_ANDROID
    auto* jni = GetJNISetup();
    jni->env->CallVoidMethod(jni->thiz, setPixSize, (int)pixelSize.x, (int)pixelSize.y);
#endif

    Vector2 viewportPos{};
    Vector2 viewportSize{ displayWidth[0], displayHeight[0] };

    if ((viewSize.x / viewSize.y) <= (pixAspect + 0.1)) {
        if ((pixAspect - 0.1) > (viewSize.x / viewSize.y)) {
            viewSize.y     = (pixelSize.y / pixelSize.x) * viewSize.x;
            viewportPos.y  = (displayHeight[0] >> 1) - (viewSize.y * 0.5);
            viewportSize.y = viewSize.y;
        }
    }
    else {
        viewSize.x     = pixAspect * viewSize.y;
        viewportPos.x  = (displayWidth[0] >> 1) - ((pixAspect * viewSize.y) * 0.5);
        viewportSize.x = (pixAspect * viewSize.y);
    }

#if !RETRO_USE_ORIGINAL_CODE
    if (screenWidth <= 512 && maxPixHeight <= 256) {
#else
    if (maxPixHeight <= 256) {
#endif
        textureSize.x = 512.0;
        textureSize.y = 256.0;
    }
    else {
        textureSize.x = 1024.0;
        textureSize.y = 512.0;
    }

    glViewport(viewportPos.x, viewportPos.y, viewportSize.x, viewportSize.y);

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(SCREEN_COUNT, screenTextures);

    for (int32 i = 0; i < SCREEN_COUNT; ++i) {
        glBindTexture(GL_TEXTURE_2D, screenTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureSize.x, textureSize.y, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glGenTextures(1, &imageTexture);
    glBindTexture(GL_TEXTURE_2D, imageTexture);
#if RETRO_PLATFORM == RETRO_SWITCH
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RETRO_VIDEO_TEXTURE_W, RETRO_VIDEO_TEXTURE_H, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RETRO_VIDEO_TEXTURE_W, RETRO_VIDEO_TEXTURE_H, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
#endif

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (videoBuffer)
        delete[] videoBuffer;
    videoBuffer = new uint32[RETRO_VIDEO_TEXTURE_W * RETRO_VIDEO_TEXTURE_H];

    lastShaderID = -1;
    InitVertexBuffer();
    engine.inFocus          = 1;
    videoSettings.viewportX = viewportPos.x;
    videoSettings.viewportY = viewportPos.y;
    videoSettings.viewportW = 1.0 / viewSize.x;
    videoSettings.viewportH = 1.0 / viewSize.y;

    // PrintLog(PRINT_NORMAL, "%d %d %f %f %d %d %d %d %f %f", displayWidth[0], displayHeight[0], pixelSize.x, pixelSize.y, viewportPos.x,
    // viewportPos.y,
    //         viewportSize.x, viewportSize.y, viewSize.x, viewSize.y);

    return true;
}

// CUSTOM BUFFER FOR SHENANIGAN PURPOSES
// GL hates us and it's coordinate system is reverse of DX
// for shader output equivalency, we havee to flip everything
// X and Y are negated, some verts are specifically moved to match
// U and V are 0/1s and flipped from what it was originally
// clang-format off
#if RETRO_REV02
const RenderVertex rsdkGLVertexBuffer[60] = {
    // 1 Screen (0)
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 2 Screens - Bordered (Top Screen) (6)
    { { +0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +0.5, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -0.5, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -0.5, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 2 Screens - Bordered (Bottom Screen) (12)
    { { +0.5, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +0.5, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -0.5, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 2 Screens - Stretched (Top Screen) (18)
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
  
    // 2 Screens - Stretched (Bottom Screen) (24)
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 4 Screens (Top-Left) (30)
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },

    // 4 Screens (Top-Right) (36)
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { {  0.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { {  0.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 4 Screens (Bottom-Right) (42)
    { {  0.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { {  0.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 4 Screens (Bottom-Left) (48)
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // Image/Video (54)
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } }
};
#else
const RenderVertex rsdkGLVertexBuffer[24] =
{
    // 1 Screen (0)
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },

    // 2 Screens - Stretched (Top Screen) (6)
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
  
    // 2 Screens - Stretched (Bottom Screen) (12)
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
  
    // Image/Video (18)
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, +1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } }
};
#endif


void RenderDevice::InitVertexBuffer()
{
    RenderVertex vertBuffer[sizeof(rsdkGLVertexBuffer) / sizeof(RenderVertex)];
    memcpy(vertBuffer, rsdkGLVertexBuffer, sizeof(rsdkGLVertexBuffer));

    float x = 0.5 / (float)viewSize.x;
    float y = 0.5 / (float)viewSize.y;

    // ignore the last 6 verts, they're scaled to the 1024x512 textures already!
    int32 vertCount = (RETRO_REV02 ? 60 : 24) - 6;
    for (int32 v = 0; v < vertCount; ++v) {
        RenderVertex *vertex = &vertBuffer[v];
        vertex->pos.x        = vertex->pos.x + x;
        vertex->pos.y        = vertex->pos.y - y;

        if (vertex->tex.x)
            vertex->tex.x = screens[0].size.x * (1.0 / textureSize.x);

        if (vertex->tex.y)
            vertex->tex.y = screens[0].size.y * (1.0 / textureSize.y);
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(RenderVertex) * (!RETRO_REV02 ? 24 : 60), vertBuffer);
}

void RenderDevice::InitFPSCap() {}

bool RenderDevice::CheckFPSCap() { return true; }
void RenderDevice::UpdateFPSCap() {}

void RenderDevice::CopyFrameBuffer()
{
    if (!isInitialized)
        return;

    for (int32 s = 0; s < videoSettings.screenCount; ++s) {
        glBindTexture(GL_TEXTURE_2D, screenTextures[s]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screens[s].pitch, SCREEN_YSIZE, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, screens[s].frameBuffer);
    }
}

bool RenderDevice::ProcessEvents()
{
    // events aren't processed by EGL
#if RETRO_PLATFORM == RETRO_ANDROID
    // unless you're android!!
    int32 events;
    struct android_poll_source *source;
    while (ALooper_pollAll(0, NULL, &events, (void **)&source) >= 0) {
        if (source)
            source->process(app, source);

        if (app->destroyRequested) {
            isRunning = false;
            return false;
        }
    }

    if (videoSettings.windowState == WINDOWSTATE_INACTIVE) {
        Release(true);
    }
    else if (videoSettings.windowState == WINDOWSTATE_ACTIVE && !RenderDevice::isInitialized && RenderDevice::window) {
        Release(true);
        Init();
    }
#endif
    return true;
}

void RenderDevice::FlipScreen()
{
    if (!isInitialized)
        return;

    if (lastShaderID != videoSettings.shaderID) {
        lastShaderID = videoSettings.shaderID;

        SetLinear(shaderList[videoSettings.shaderID].linear);

        if (videoSettings.shaderSupport)
            glUseProgram(shaderList[videoSettings.shaderID].programID);
    }

    if (windowRefreshDelay > 0) {
        windowRefreshDelay--;
        if (!windowRefreshDelay)
            UpdateGameWindow();
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    if (videoSettings.shaderSupport) {
        glUniform2fv(glGetUniformLocation(shaderList[videoSettings.shaderID].programID, "textureSize"), 1, &textureSize.x);
        glUniform2fv(glGetUniformLocation(shaderList[videoSettings.shaderID].programID, "pixelSize"), 1, &pixelSize.x);
        glUniform2fv(glGetUniformLocation(shaderList[videoSettings.shaderID].programID, "viewSize"), 1, &viewSize.x);
        glUniform1f(glGetUniformLocation(shaderList[videoSettings.shaderID].programID, "screenDim"), videoSettings.dimMax * videoSettings.dimPercent);
    }

    int32 startVert = 0;
    switch (videoSettings.screenCount) {
        default:
        case 0:
#if RETRO_REV02
            startVert = 54;
#else
            startVert = 18;
#endif
            glBindTexture(GL_TEXTURE_2D, imageTexture);
            glDrawArrays(GL_TRIANGLES, startVert, 6);

            break;

        case 1:
            glBindTexture(GL_TEXTURE_2D, screenTextures[0]);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            break;

        case 2:
#if RETRO_REV02
            startVert = startVertex_2P[0];
#else
            startVert = 6;
#endif
            glBindTexture(GL_TEXTURE_2D, screenTextures[0]);
            glDrawArrays(GL_TRIANGLES, startVert, 6);

#if RETRO_REV02
            startVert = startVertex_2P[1];
#else
            startVert = 12;
#endif
            glBindTexture(GL_TEXTURE_2D, screenTextures[1]);
            glDrawArrays(GL_TRIANGLES, startVert, 6);
            break;

#if RETRO_REV02
        case 3:
            glBindTexture(GL_TEXTURE_2D, screenTextures[0]);
            glDrawArrays(GL_TRIANGLES, startVertex_3P[0], 6);

            glBindTexture(GL_TEXTURE_2D, screenTextures[1]);
            glDrawArrays(GL_TRIANGLES, startVertex_3P[1], 6);

            glBindTexture(GL_TEXTURE_2D, screenTextures[2]);
            glDrawArrays(GL_TRIANGLES, startVertex_3P[2], 6);
            break;

        case 4:
            glBindTexture(GL_TEXTURE_2D, screenTextures[0]);
            glDrawArrays(GL_TRIANGLES, 30, 6);

            glBindTexture(GL_TEXTURE_2D, screenTextures[1]);
            glDrawArrays(GL_TRIANGLES, 36, 6);

            glBindTexture(GL_TEXTURE_2D, screenTextures[2]);
            glDrawArrays(GL_TRIANGLES, 42, 6);

            glBindTexture(GL_TEXTURE_2D, screenTextures[3]);
            glDrawArrays(GL_TRIANGLES, 48, 6);
            break;
#endif
    }

#if RETRO_PLATFORM != RETRO_ANDROID
    if (!eglSwapBuffers(display, surface)) {
        PrintLog(PRINT_NORMAL, "[EGL] Failed to swap buffers: %d", eglGetError());
    }
#else
    if (!SwappyGL_swap(display, surface)) {
        if (SwappyGL_isEnabled()) {
            PrintLog(PRINT_NORMAL, "[EGL] Failed to swap buffers: %d", eglGetError());
        }
    }
#endif
}

void RenderDevice::Release(bool32 isRefresh)
{
    if (display != EGL_NO_DISPLAY) {
        glDeleteTextures(SCREEN_COUNT, screenTextures);
        glDeleteTextures(1, &imageTexture);

        if (videoBuffer)
            delete[] videoBuffer;
        videoBuffer = NULL;

        for (int32 i = 0; i < shaderCount; ++i) {
            glDeleteProgram(shaderList[i].programID);
        }
    
#if RETRO_PLATFORM == RETRO_SWITCH
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
#endif

        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);

        display = EGL_NO_DISPLAY;
        surface = EGL_NO_SURFACE;
        context = EGL_NO_CONTEXT;

        isInitialized = false;
    }

    if (!isRefresh) {
        shaderCount = 0;
#if RETRO_USE_MOD_LOADER
        userShaderCount = 0;
#endif

        if (displayInfo.displays)
            free(displayInfo.displays);
        displayInfo.displays = NULL;

        if (scanlines)
            free(scanlines);
        scanlines = NULL;
    }
}

bool RenderDevice::InitShaders()
{
    videoSettings.shaderSupport = true;
    int32 maxShaders            = 0;
    shaderCount                 = 0;

    LoadShader("None", false);
    LoadShader("Clean", true);
    LoadShader("CRT-Yeetron", true);
    LoadShader("CRT-Yee64", true);

#if RETRO_USE_MOD_LOADER
    // a place for mods to load custom shaders
    RunModCallbacks(MODCB_ONSHADERLOAD, NULL);
    userShaderCount = shaderCount;
#endif

    LoadShader("YUV-420", true);
    LoadShader("YUV-422", true);
    LoadShader("YUV-444", true);
    LoadShader("RGB-Image", true);
    maxShaders = shaderCount;

    // no shaders == no support
    if (!maxShaders) {
        ShaderEntry *shader         = &shaderList[0];
        videoSettings.shaderSupport = false;

        // let's load
        maxShaders  = 1;
        shaderCount = 1;

        GLint success;
        char infoLog[0x1000];

        GLuint vert, frag;
        const GLchar *vchar[] = { _GLVERSION, _GLDEFINE, _glVPrecision, backupVertex };
        vert                  = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 4, vchar, NULL);
        glCompileShader(vert);

        const GLchar *fchar[] = { _GLVERSION, _GLDEFINE, _glFPrecision, backupFragment };
        frag                  = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 4, fchar, NULL);
        glCompileShader(frag);

        glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vert, 0x1000, NULL, infoLog);
            PrintLog(PRINT_NORMAL, "BACKUP vertex shader compiling failed:\n%s", infoLog);
        }        

        glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(frag, 0x1000, NULL, infoLog);
            PrintLog(PRINT_NORMAL, "BACKUP fragment shader compiling failed:\n%s", infoLog);
        }     

        shader->programID = glCreateProgram();
        glAttachShader(shader->programID, vert);
        glAttachShader(shader->programID, frag);

        glBindAttribLocation(shader->programID, 0, "in_pos");
        //glBindAttribLocation(shader->programID, 1, "in_color");
        glBindAttribLocation(shader->programID, 1, "in_UV");

        glLinkProgram(shader->programID);
        glDeleteShader(vert);
        glDeleteShader(frag);

        glUseProgram(shader->programID);

        shader->linear = videoSettings.windowed ? false : shader->linear;
    }

    videoSettings.shaderID = MAX(videoSettings.shaderID >= maxShaders ? 0 : videoSettings.shaderID, 0);
    SetLinear(shaderList[videoSettings.shaderID].linear || videoSettings.screenCount > 1);

    return true;
}

void RenderDevice::LoadShader(const char *fileName, bool32 linear)
{
    char fullFilePath[0x100];
    FileInfo info;

    for (int32 i = 0; i < shaderCount; ++i) {
        if (strcmp(shaderList[i].name, fileName) == 0)
            return;
    }

    if (shaderCount == SHADER_COUNT)
        return;

    ShaderEntry *shader = &shaderList[shaderCount];
    shader->linear      = linear;
    sprintf_s(shader->name, sizeof(shader->name), "%s", fileName);

    GLint success;
    char infoLog[0x1000];
    GLuint vert, frag;
    sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Shaders/OGL/None.vs");
    InitFileInfo(&info);
    if (LoadFile(&info, fullFilePath, FMODE_RB)) {
        uint8 *fileData = NULL;
        AllocateStorage((void **)&fileData, info.fileSize + 1, DATASET_TMP, false);
        ReadBytes(&info, fileData, info.fileSize);
        fileData[info.fileSize] = 0;
        CloseFile(&info);

        const GLchar *glchar[] = { _GLVERSION, _GLDEFINE, _glVPrecision, (const GLchar *)fileData };
        vert                   = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 4, glchar, NULL);
        glCompileShader(vert);
        RemoveStorageEntry((void **)&fileData);

        glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vert, 0x1000, NULL, infoLog);
            PrintLog(PRINT_NORMAL, "Vertex shader compiling failed:\n%s", infoLog);
            return;
        }     
    }
    else
        return;

    sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Shaders/OGL/%s.fs", fileName);
    InitFileInfo(&info);
    if (LoadFile(&info, fullFilePath, FMODE_RB)) {
        uint8 *fileData = NULL;
        AllocateStorage((void **)&fileData, info.fileSize + 1, DATASET_TMP, false);
        ReadBytes(&info, fileData, info.fileSize);
        fileData[info.fileSize] = 0;
        CloseFile(&info);

        const GLchar *glchar[] = { _GLVERSION, _GLDEFINE, _glFPrecision, (const GLchar *)fileData };
        frag                   = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 4, glchar, NULL);
        glCompileShader(frag);
        RemoveStorageEntry((void **)&fileData);

        glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(frag, 0x1000, NULL, infoLog);
            PrintLog(PRINT_NORMAL, "Fragment shader compiling failed:\n%s", infoLog);
            return;
        }     
    }
    else
        return;

    shader->programID = glCreateProgram();
    glAttachShader(shader->programID, vert);
    glAttachShader(shader->programID, frag);
    
    glBindAttribLocation(shader->programID, 0, "in_pos");
    //glBindAttribLocation(shader->programID, 1, "in_color");
    glBindAttribLocation(shader->programID, 1, "in_UV");

    glLinkProgram(shader->programID);
    glGetProgramiv(shader->programID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader->programID, 0x1000, NULL, infoLog);
        PrintLog(PRINT_NORMAL, "OpenGL shader linking failed:\n%s", infoLog);
        return;
    }
    glDeleteShader(vert);
    glDeleteShader(frag);
    shaderCount++;
};

void RenderDevice::RefreshWindow()
{
    // do nothing probably
    // there's literally 0 moment where this is needed
}

void RenderDevice::GetWindowSize(int32 *width, int32 *height)
{
#if RETRO_PLATFORM == RETRO_ANDROID
    if (width)
        eglQuerySurface(display, surface, EGL_WIDTH, width);
    if (height)
        eglQuerySurface(display, surface, EGL_HEIGHT, height);
#elif RETRO_PLATFORM == RETRO_SWITCH
    if (width)
        *width = 1920;
    if (height)
        *height = 1080;
#endif
}

void RenderDevice::SetupImageTexture(int32 width, int32 height, uint8 *imagePixels)
{
    if (imagePixels && isInitialized) {
        glBindTexture(GL_TEXTURE_2D, imageTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, imagePixels);
    }
}

void RenderDevice::SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    if (!isInitialized)
        return;
    uint32 *pixels = videoBuffer;
    uint32 *preY   = pixels;
    int32 pitch    = RETRO_VIDEO_TEXTURE_W - width;
    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = (yPlane[x] << _YOFF) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }

        pixels = preY;
        pitch  = RETRO_VIDEO_TEXTURE_W - (width >> 1);
        for (int32 y = 0; y < (height >> 1); ++y) {
            for (int32 x = 0; x < (width >> 1); ++x) {
                *pixels++ |= (vPlane[x] << _VOFF) | (uPlane[x] << _UOFF) | 0xFF000000;
            }

            pixels += pitch;
            uPlane += strideU;
            vPlane += strideV;
        }
    }
    else {
        // No shader support means no YUV support! at least use the brightness to show it in grayscale!
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                int32 brightness = yPlane[x];
                *pixels++        = (brightness << 0) | (brightness << 8) | (brightness << 16) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }
    }

    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RETRO_VIDEO_TEXTURE_W, RETRO_VIDEO_TEXTURE_H, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoBuffer);
}

void RenderDevice::SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    if (!isInitialized)
        return;
    uint32 *pixels = videoBuffer;
    uint32 *preY   = pixels;
    int32 pitch    = RETRO_VIDEO_TEXTURE_W - width;

    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = (yPlane[x] << _YOFF) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }

        pixels = preY;
        pitch  = RETRO_VIDEO_TEXTURE_W - (width >> 1);
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < (width >> 1); ++x) {
                *pixels++ |= (vPlane[x] << _VOFF) | (uPlane[x] << _UOFF) | 0xFF000000;
            }

            pixels += pitch;
            uPlane += strideU;
            vPlane += strideV;
        }
    }
    else {
        // No shader support means no YUV support! at least use the brightness to show it in grayscale!
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                int32 brightness = yPlane[x];
                *pixels++        = (brightness << 0) | (brightness << 8) | (brightness << 16) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }
    }

    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RETRO_VIDEO_TEXTURE_W, RETRO_VIDEO_TEXTURE_H, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoBuffer);
}
void RenderDevice::SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    if (!isInitialized)
        return;
    uint32 *pixels = videoBuffer;
    int32 pitch    = RETRO_VIDEO_TEXTURE_W - width;
    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            int32 pos1  = yPlane - vPlane;
            int32 pos2  = uPlane - vPlane;
            uint8 *pixV = vPlane;
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = (pixV[0] << _VOFF) | (pixV[pos2] << _UOFF) | (pixV[pos1] << _YOFF) | 0xFF000000;
                pixV++;
            }

            pixels += pitch;
            yPlane += strideY;
            uPlane += strideU;
            vPlane += strideV;
        }
    }
    else {
        // No shader support means no YUV support! at least use the brightness to show it in grayscale!
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                int32 brightness = yPlane[x];
                *pixels++        = (brightness << 0) | (brightness << 8) | (brightness << 16) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }
    }

    glBindTexture(GL_TEXTURE_2D, imageTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RETRO_VIDEO_TEXTURE_W, RETRO_VIDEO_TEXTURE_H, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, videoBuffer);
}

void RenderDevice::SetLinear(bool32 linear)
{
    if (!isInitialized)
        return;
    for (int32 i = 0; i < SCREEN_COUNT; ++i) {
        glBindTexture(GL_TEXTURE_2D, screenTextures[i]);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    }
}