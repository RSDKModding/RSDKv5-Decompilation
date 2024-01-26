#ifndef _GLVERSION
#define _GLVERSION 20
//#define _GLVERSION 33
#endif

#if _GLVERSION > 30
#define _GLSLVERSION "#version 130\n#define in_V in\n#define in_F in\n"
#else
#define _GLSLVERSION "#version 110\n#define in_V attribute\n#define out varying\n#define in_F varying\n"
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
    ex_color    = vec4(1.0);
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

GLFWwindow *RenderDevice::window;
GLuint RenderDevice::VAO;
GLuint RenderDevice::VBO;

GLuint RenderDevice::screenTextures[SCREEN_COUNT];
GLuint RenderDevice::imageTexture;

double RenderDevice::lastFrame;
double RenderDevice::targetFreq;

int32 RenderDevice::monitorIndex;

uint32 *RenderDevice::videoBuffer;

// Creates a window using the video settings
GLFWwindow *RenderDevice::CreateGLFWWindow(void)
{
    GLFWwindow *window;
    GLFWmonitor *monitor = NULL;
    int32 w, h;

    glfwWindowHint(GLFW_DECORATED, videoSettings.bordered);

    if (videoSettings.windowed) {
        w = videoSettings.windowWidth;
        h = videoSettings.windowHeight;
    }
    else if (videoSettings.fsWidth <= 0 || videoSettings.fsHeight <= 0) {
        monitor                 = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        w                       = mode->width;
        h                       = mode->height;
    }
    else {
        monitor = glfwGetPrimaryMonitor();
        w       = videoSettings.fsWidth;
        h       = videoSettings.fsHeight;
    }

    window = glfwCreateWindow(w, h, gameVerInfo.gameTitle, monitor, NULL);
    if (!window) {
        PrintLog(PRINT_NORMAL, "ERROR: [GLFW] window creation failed");
        return NULL;
    }
    if (videoSettings.windowed) {
        // Center the window
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        int x, y;
        glfwGetMonitorPos(monitor, &x, &y);
        glfwSetWindowPos(window, x + (mode->width - videoSettings.windowWidth) / 2, y + (mode->height - videoSettings.windowHeight) / 2);
    }
    glfwShowWindow(window);
    PrintLog(PRINT_NORMAL, "w: %d h: %d windowed: %d", w, h, videoSettings.windowed);

    glfwSetKeyCallback(window, ProcessKeyEvent);
    glfwSetMouseButtonCallback(window, ProcessMouseEvent);
    glfwSetWindowFocusCallback(window, ProcessFocusEvent);
    glfwSetWindowMaximizeCallback(window, ProcessMaximizeEvent);

    return window;
}

bool RenderDevice::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, _GLVERSION / 10);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, _GLVERSION % 10);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    if ((window = CreateGLFWWindow()) == NULL)
        return false;

    glfwSetJoystickCallback(ProcessJoystickEvent);

    if (!SetupRendering() || !AudioDevice::Init())
        return false;

    InitInputDevices();
    return true;
}

bool RenderDevice::SetupRendering()
{
    glfwMakeContextCurrent(window);
    GLenum err;
    if ((err = glewInit()) != GLEW_OK) {
        PrintLog(PRINT_NORMAL, "ERROR: failed to initialize GLEW: %s", glewGetErrorString(err));
        return false;
    }

    glfwSwapInterval(videoSettings.vsync ? 1 : 0);

    GetDisplays();

    if (!InitGraphicsAPI() || !InitShaders())
        return false;

    int32 size = videoSettings.pixWidth >= SCREEN_YSIZE ? videoSettings.pixWidth : SCREEN_YSIZE;
    scanlines  = (ScanlineInfo *)malloc(size * sizeof(ScanlineInfo));
    memset(scanlines, 0, size * sizeof(ScanlineInfo));

    videoSettings.windowState = WINDOWSTATE_ACTIVE;
    videoSettings.dimMax      = 1.0;
    videoSettings.dimPercent  = 1.0;

    return true;
}

void RenderDevice::GetDisplays()
{
    GLFWmonitor *monitor = glfwGetWindowMonitor(window);
    if (!monitor)
        monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *displayMode = glfwGetVideoMode(monitor);
    int32 monitorCount;
    GLFWmonitor **monitors = glfwGetMonitors(&monitorCount);

    for (int32 m = 0; m < monitorCount; ++m) {
        const GLFWvidmode *vidMode = glfwGetVideoMode(monitors[m]);
        displayWidth[m]            = vidMode->width;
        displayHeight[m]           = vidMode->height;
        if (!memcmp(vidMode, displayMode, sizeof(GLFWvidmode))) {
            monitorIndex = m;
        }
    }

    const GLFWvidmode *displayModes = glfwGetVideoModes(monitor, &displayCount);
    if (displayInfo.displays)
        free(displayInfo.displays);

    displayInfo.displays          = (decltype(displayInfo.displays))malloc(sizeof(GLFWvidmode) * displayCount);
    int32 newDisplayCount         = 0;
    bool32 foundFullScreenDisplay = false;

    for (int32 d = 0; d < displayCount; ++d) {
        memcpy(&displayInfo.displays[newDisplayCount].internal, &displayModes[d], sizeof(GLFWvidmode));

        int32 refreshRate = displayInfo.displays[newDisplayCount].refresh_rate;
        if (refreshRate >= 59 && (refreshRate <= 60 || refreshRate >= 120) && displayInfo.displays[newDisplayCount].height >= (SCREEN_YSIZE * 2)) {
            if (d && refreshRate == 60 && displayInfo.displays[newDisplayCount - 1].refresh_rate == 59)
                --newDisplayCount;

            if (videoSettings.fsWidth == displayInfo.displays[newDisplayCount].width
                && videoSettings.fsHeight == displayInfo.displays[newDisplayCount].height)
                foundFullScreenDisplay = true;

            ++newDisplayCount;
        }
    }

    displayCount = newDisplayCount;
    if (!foundFullScreenDisplay) {
        videoSettings.fsWidth     = 0;
        videoSettings.fsHeight    = 0;
        videoSettings.refreshRate = 60; // 0;
    }
}

bool RenderDevice::InitGraphicsAPI()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);

    // setup buffers

#if _GLVERSION > 30
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
    else {
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

    Vector2 viewportPos{};
    Vector2 lastViewSize;

    glfwGetWindowSize(window, &lastViewSize.x, &lastViewSize.y);
    Vector2 viewportSize = lastViewSize;

    if ((viewSize.x / viewSize.y) <= ((pixelSize.x / pixelSize.y) + 0.1)) {
        if ((pixAspect - 0.1) > (viewSize.x / viewSize.y)) {
            viewSize.y     = (pixelSize.y / pixelSize.x) * viewSize.x;
            viewportPos.y  = (lastViewSize.y >> 1) - (viewSize.y * 0.5);
            viewportSize.y = viewSize.y;
        }
    }
    else {
        viewSize.x     = pixAspect * viewSize.y;
        viewportPos.x  = (lastViewSize.x >> 1) - ((pixAspect * viewSize.y) * 0.5);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RETRO_VIDEO_TEXTURE_W, RETRO_VIDEO_TEXTURE_H, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    videoBuffer = new uint32[RETRO_VIDEO_TEXTURE_W * RETRO_VIDEO_TEXTURE_H];

    lastShaderID = -1;
    InitVertexBuffer();
    engine.inFocus          = 1;
    videoSettings.viewportX = viewportPos.x;
    videoSettings.viewportY = viewportPos.y;
    videoSettings.viewportW = 1.0 / viewSize.x;
    videoSettings.viewportH = 1.0 / viewSize.y;

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

void RenderDevice::InitFPSCap()
{
    lastFrame  = glfwGetTime();
    targetFreq = 1.0 / videoSettings.refreshRate;
}
bool RenderDevice::CheckFPSCap()
{
    if (lastFrame + targetFreq < glfwGetTime())
        return true;

    return false;
}
void RenderDevice::UpdateFPSCap() { lastFrame = glfwGetTime(); }

void RenderDevice::CopyFrameBuffer()
{
    for (int32 s = 0; s < videoSettings.screenCount; ++s) {
        glBindTexture(GL_TEXTURE_2D, screenTextures[s]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screens[s].pitch, SCREEN_YSIZE, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, screens[s].frameBuffer);
    }
}

bool RenderDevice::ProcessEvents()
{
    glfwPollEvents();
    if (glfwWindowShouldClose(window))
        isRunning = false;
    return false;
}

void RenderDevice::FlipScreen()
{
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
            // also flipped
            glBindTexture(GL_TEXTURE_2D, screenTextures[0]);
            glDrawArrays(GL_TRIANGLES, startVertex_3P[0], 6);

            glBindTexture(GL_TEXTURE_2D, screenTextures[1]);
            glDrawArrays(GL_TRIANGLES, startVertex_3P[1], 6);

            glBindTexture(GL_TEXTURE_2D, screenTextures[2]);
            glDrawArrays(GL_TRIANGLES, startVertex_3P[2], 6);
            break;

        case 4:
            // this too
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

    glFlush();
    glfwSwapBuffers(window);
}

void RenderDevice::Release(bool32 isRefresh)
{
    glDeleteTextures(SCREEN_COUNT, screenTextures);
    glDeleteTextures(1, &imageTexture);
    if (videoBuffer)
        delete[] videoBuffer;
    for (int32 i = 0; i < shaderCount; ++i) {
        glDeleteProgram(shaderList[i].programID);
    }
    
#if _GLVERSION > 30
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
#endif

    shaderCount = 0;
#if RETRO_USE_MOD_LOADER
    userShaderCount = 0;
#endif

    glfwDestroyWindow(window);

    if (!isRefresh) {
        if (displayInfo.displays)
            free(displayInfo.displays);
        displayInfo.displays = NULL;

        if (scanlines)
            free(scanlines);
        scanlines = NULL;

        glfwTerminate();
    }
}

bool RenderDevice::InitShaders()
{
    videoSettings.shaderSupport = true;
    int32 maxShaders            = 0;
#if RETRO_USE_MOD_LOADER
    shaderCount                 = 0;
#endif

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
        const GLchar *vchar[] = { _GLSLVERSION, _GLDEFINE, backupVertex };
        vert                  = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 3, vchar, NULL);
        glCompileShader(vert);

        const GLchar *fchar[] = { _GLSLVERSION, _GLDEFINE, backupFragment };
        frag                  = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 3, fchar, NULL);
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

        const GLchar *glchar[] = { _GLSLVERSION, _GLDEFINE, (const GLchar *)fileData };
        vert                   = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 3, glchar, NULL);
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

        const GLchar *glchar[] = { _GLSLVERSION, _GLDEFINE, (const GLchar *)fileData };
        frag                   = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 3, glchar, NULL);
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

    shader->linear = linear;

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
    videoSettings.windowState = WINDOWSTATE_UNINITIALIZED;

    Release(true);

    if ((window = CreateGLFWWindow()) == NULL)
        return;

    glfwMakeContextCurrent(window);

    if (!InitGraphicsAPI() || !InitShaders())
        return;

    videoSettings.windowState = WINDOWSTATE_ACTIVE;
}

void RenderDevice::GetWindowSize(int32 *width, int32 *height)
{
    int32 widest = 0, highest = 0, count = 0;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    for (int32 i = 0; i < count; i++) {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
        if (mode->height > highest) {
            highest = mode->height;
            widest  = mode->width;
        }
    }
    if (width)
        *width = widest;
    if (height)
        *height = highest;
}

void RenderDevice::SetupImageTexture(int32 width, int32 height, uint8 *imagePixels)
{
    if (imagePixels) {
        glBindTexture(GL_TEXTURE_2D, imageTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, imagePixels);
    }
}

void RenderDevice::SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    uint32 *pixels = videoBuffer;
    uint32 *preY   = pixels;
    int32 pitch    = RETRO_VIDEO_TEXTURE_W - width;

    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = (yPlane[x] << 16) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }

        pixels = preY;
        pitch  = RETRO_VIDEO_TEXTURE_W - (width >> 1);
        for (int32 y = 0; y < (height >> 1); ++y) {
            for (int32 x = 0; x < (width >> 1); ++x) {
                *pixels++ |= (vPlane[x] << 0) | (uPlane[x] << 8) | 0xFF000000;
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
    uint32 *pixels = videoBuffer;
    uint32 *preY   = pixels;
    int32 pitch    = RETRO_VIDEO_TEXTURE_W - width;

    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = (yPlane[x] << 16) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }

        pixels = preY;
        pitch  = RETRO_VIDEO_TEXTURE_W - (width >> 1);
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < (width >> 1); ++x) {
                *pixels++ |= (vPlane[x] << 0) | (uPlane[x] << 8) | 0xFF000000;
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
    uint32 *pixels = videoBuffer;
    int32 pitch    = RETRO_VIDEO_TEXTURE_W - width;

    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            int32 pos1  = yPlane - vPlane;
            int32 pos2  = uPlane - vPlane;
            uint8 *pixV = vPlane;
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = pixV[0] | (pixV[pos2] << 8) | (pixV[pos1] << 16) | 0xFF000000;
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

void RenderDevice::ProcessKeyEvent(GLFWwindow *, int32 key, int32 scancode, int32 action, int32 mods)
{
    switch (action) {
        case GLFW_PRESS: {
#if !RETRO_REV02
            ++RSDK::SKU::buttonDownCount;
#endif
            switch (key) {
                case GLFW_KEY_ENTER:
                    if (mods & GLFW_MOD_ALT) {
                        videoSettings.windowed ^= 1;
                        UpdateGameWindow();
                        changedVideoSettings = false;
                        break;
                    }

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                    RSDK::SKU::specialKeyStates[1] = true;
#endif

                    // [fallthrough]

                default:
#if RETRO_INPUTDEVICE_KEYBOARD
                    SKU::UpdateKeyState(key);
#endif
                    break;

                case GLFW_KEY_ESCAPE:
                    if (engine.devMenu) {
#if RETRO_REV0U
                        if (sceneInfo.state == ENGINESTATE_DEVMENU || RSDK::Legacy::gameMode == RSDK::Legacy::ENGINE_DEVMENU)
#else
                        if (sceneInfo.state == ENGINESTATE_DEVMENU)
#endif
                            CloseDevMenu();
                        else
                            OpenDevMenu();
                    }
                    else {
#if RETRO_INPUTDEVICE_KEYBOARD
                        SKU::UpdateKeyState(key);
#endif
                    }

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                    RSDK::SKU::specialKeyStates[0] = true;
#endif
                    break;

#if !RETRO_USE_ORIGINAL_CODE
                case GLFW_KEY_F1:
                    if (engine.devMenu) {
                        sceneInfo.listPos--;
                        if (sceneInfo.listPos < sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart
                            || sceneInfo.listPos >= sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd) {
                            sceneInfo.activeCategory--;
                            if (sceneInfo.activeCategory >= sceneInfo.categoryCount) {
                                sceneInfo.activeCategory = sceneInfo.categoryCount - 1;
                            }
                            sceneInfo.listPos = sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd - 1;
                        }

#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5: LoadScene(); break;
                            case 4:
                            case 3: RSDK::Legacy::stageMode = RSDK::Legacy::STAGEMODE_LOAD; break;
                        }
#else
                        LoadScene();
#endif
                    }
                    break;

                case GLFW_KEY_F2:
                    if (engine.devMenu) {
                        sceneInfo.listPos++;
                        if (sceneInfo.listPos >= sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd || sceneInfo.listPos == 0) {
                            sceneInfo.activeCategory++;
                            if (sceneInfo.activeCategory >= sceneInfo.categoryCount) {
                                sceneInfo.activeCategory = 0;
                            }
                            sceneInfo.listPos = sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart;
                        }

#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5: LoadScene(); break;
                            case 4:
                            case 3: RSDK::Legacy::stageMode = RSDK::Legacy::STAGEMODE_LOAD; break;
                        }
#else
                        LoadScene();
#endif
                    }
                    break;
#endif

                case GLFW_KEY_F3:
                    if (userShaderCount)
                        videoSettings.shaderID = (videoSettings.shaderID + 1) % userShaderCount;
                    break;

#if !RETRO_USE_ORIGINAL_CODE
                case GLFW_KEY_F4:
                    if (engine.devMenu)
                        engine.showEntityInfo ^= 1;
                    break;

                case GLFW_KEY_F5:
                    if (engine.devMenu) {
                        // Quick-Reload
#if RETRO_USE_MOD_LOADER
                        if (mods & GLFW_MOD_CONTROL)
                            RefreshModFolders();   
#endif

#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5: LoadScene(); break;
                            case 4:
                            case 3: RSDK::Legacy::stageMode = RSDK::Legacy::STAGEMODE_LOAD; break;
                        }
#else
                        LoadScene();
#endif
                    }
                    break;

                case GLFW_KEY_F6:
                    if (engine.devMenu && videoSettings.screenCount > 1)
                        videoSettings.screenCount--;
                    break;

                case GLFW_KEY_F7:
                    if (engine.devMenu && videoSettings.screenCount < SCREEN_COUNT)
                        videoSettings.screenCount++;
                    break;

                case GLFW_KEY_F8:
                    if (engine.devMenu)
                        engine.showUpdateRanges ^= 1;
                    break;

                case GLFW_KEY_F9:
                    if (engine.devMenu)
                        showHitboxes ^= 1;
                    break;

                case GLFW_KEY_F10:
                    if (engine.devMenu)
                        engine.showPaletteOverlay ^= 1;
                    break;
#endif
                case GLFW_KEY_BACKSPACE:
                    if (engine.devMenu)
                        engine.gameSpeed = engine.fastForwardSpeed;
                    break;

                case GLFW_KEY_F11:
                case GLFW_KEY_INSERT:
                    if (engine.devMenu)
                        engine.frameStep = true;
                    break;

                case GLFW_KEY_F12:
                case GLFW_KEY_PAUSE:
                    if (engine.devMenu) {
#if RETRO_REV0U
                        switch (engine.version) {
                            default: break;
                            case 5:
                                if (sceneInfo.state != ENGINESTATE_NONE)
                                    sceneInfo.state ^= ENGINESTATE_STEPOVER;
                                break;
                            case 4:
                            case 3:
                                if (RSDK::Legacy::stageMode != ENGINESTATE_NONE)
                                    RSDK::Legacy::stageMode ^= RSDK::Legacy::STAGEMODE_STEPOVER;
                                break;
                        }
#else
                        if (sceneInfo.state != ENGINESTATE_NONE)
                            sceneInfo.state ^= ENGINESTATE_STEPOVER;
#endif
                    }
                    break;
            }
            break;
        }
        case GLFW_RELEASE: {
#if !RETRO_REV02
            --RSDK::SKU::buttonDownCount;
#endif
            switch (key) {
                default:
#if RETRO_INPUTDEVICE_KEYBOARD
                    SKU::ClearKeyState(key);
#endif
                    break;

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                case GLFW_KEY_ESCAPE:
                    RSDK::SKU::specialKeyStates[0] = false;
                    SKU::ClearKeyState(key);
                    break;

                case GLFW_KEY_ENTER:
                    RSDK::SKU::specialKeyStates[1] = false;
                    SKU::ClearKeyState(key);
                    break;
#endif
                case GLFW_KEY_BACKSPACE: engine.gameSpeed = 1; break;
            }
            break;
        }
    }
}
void RenderDevice::ProcessFocusEvent(GLFWwindow *, int32 focused)
{
    if (!focused) {
#if RETRO_REV02
        SKU::userCore->focusState = 1;
#endif
    }
    else {
#if RETRO_REV02
        SKU::userCore->focusState = 0;
#endif
    }
}
void RenderDevice::ProcessMouseEvent(GLFWwindow *, int32 button, int32 action, int32 mods)
{
    switch (action) {
        case GLFW_PRESS: {
            switch (button) {
                case GLFW_MOUSE_BUTTON_LEFT: touchInfo.down[0] = true; touchInfo.count = 1;
#if !RETRO_REV02
                    RSDK::SKU::buttonDownCount++;
#endif
                    break;

                case GLFW_MOUSE_BUTTON_RIGHT:
#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                    RSDK::SKU::specialKeyStates[3] = true;
                    RSDK::SKU::buttonDownCount++;
#endif
                    break;
            }
            break;
        }

        case GLFW_RELEASE: {
            switch (button) {
                case GLFW_MOUSE_BUTTON_LEFT: touchInfo.down[0] = false; touchInfo.count = 0;
#if !RETRO_REV02
                    RSDK::SKU::buttonDownCount--;
#endif
                    break;

                case GLFW_MOUSE_BUTTON_RIGHT:
#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                    RSDK::SKU::specialKeyStates[3] = false;
                    RSDK::SKU::buttonDownCount--;
#endif
                    break;
            }
            break;
        }
    }
}
void RenderDevice::ProcessJoystickEvent(int32 ID, int32 event)
{
#if RETRO_INPUTDEVICE_GLFW
    if (!glfwJoystickIsGamepad(ID))
        return;
    uint32 hash;
    char idBuffer[0x20];
    sprintf_s(idBuffer, sizeof(idBuffer), "%s%d", "GLFWDevice", ID);
    GenerateHashCRC(&hash, idBuffer);

    if (event == GLFW_CONNECTED)
        SKU::InitGLFWInputDevice(hash, ID);
    else
        RemoveInputDevice(InputDeviceFromID(hash));
#endif
}
void RenderDevice::ProcessMaximizeEvent(GLFWwindow *, int32 maximized)
{
    // i don't know why this is a thing
    if (maximized) {
        // set fullscreen idk about the specifics rn
    }
}

void RenderDevice::SetLinear(bool32 linear)
{
    for (int32 i = 0; i < SCREEN_COUNT; ++i) {
        glBindTexture(GL_TEXTURE_2D, screenTextures[i]);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
    }
}
