#include <RSDK/Core/RetroEngine.hpp>
using namespace RSDK;

SDL_Window *RenderDevice::window     = nullptr;
SDL_Renderer *RenderDevice::renderer = nullptr;
SDL_GPUDevice *RenderDevice::device  = nullptr;
SDL_Texture *RenderDevice::screenTexture[SCREEN_COUNT];

SDL_Texture *RenderDevice::imageTexture = nullptr;

uint32 RenderDevice::displayModeIndex = 0;
int32 RenderDevice::displayModeCount  = 0;

unsigned long long RenderDevice::targetFreq = 0;
unsigned long long RenderDevice::curTicks   = 0;
unsigned long long RenderDevice::prevTicks  = 0;

RenderVertex RenderDevice::vertexBuffer[!RETRO_REV02 ? 24 : 60];

uint8 RenderDevice::lastTextureFormat = -1;

#define NORMALIZE(val, minVal, maxVal) ((float)(val) - (float)(minVal)) / ((float)(maxVal) - (float)(minVal))

bool RenderDevice::Init()
{
    const char *gameTitle = gameVerInfo.gameTitle;

    SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    uint8 flags = 0;

#if RETRO_PLATFORM == RETRO_ANDROID
    videoSettings.windowed = false;
    SDL_DisplayMode dm;
    SDL_GetDesktopDisplayMode(0, &dm);
    float hdp = 0, vdp = 0;

    bool landscape = dm.h < dm.w;
    int32 h        = landscape ? dm.w : dm.h;
    int32 w        = landscape ? dm.h : dm.w;

    videoSettings.windowWidth = ((float)SCREEN_YSIZE * h / w);

#elif RETRO_PLATFORM == RETRO_SWITCH
    videoSettings.windowed     = false;
    videoSettings.windowWidth  = 1920;
    videoSettings.windowHeight = 1080;
    flags |= SDL_WINDOW_FULLSCREEN;
#endif

    SDL_SetHint(SDL_HINT_RENDER_VSYNC, videoSettings.vsync ? "1" : "0");

    window = SDL_CreateWindow(gameTitle, videoSettings.windowWidth, videoSettings.windowHeight, SDL_WINDOW_HIGH_PIXEL_DENSITY | flags);

    if (!window) {
        PrintLog(PRINT_NORMAL, "ERROR: failed to create window!");
        return false;
    }

    if (!videoSettings.windowed) {
        SDL_RestoreWindow(window);
        SDL_SetWindowFullscreen(window, true);
        SDL_HideCursor();
    }

    if (!videoSettings.bordered) {
        SDL_RestoreWindow(window);
        SDL_SetWindowBordered(window, false);
    }

    PrintLog(PRINT_NORMAL, "w: %d h: %d windowed: %d", videoSettings.windowWidth, videoSettings.windowHeight, videoSettings.windowed);

    if (!SetupRendering() || !AudioDevice::Init())
        return false;

    InitInputDevices();
    return true;
}

void RenderDevice::CopyFrameBuffer()
{
    int32 pitch    = 0;
    uint16 *pixels = NULL;

    for (int32 s = 0; s < videoSettings.screenCount; ++s) {
        SDL_LockTexture(screenTexture[s], NULL, (void **)&pixels, &pitch);

        uint16 *frameBuffer = screens[s].frameBuffer;
        for (int32 y = 0; y < SCREEN_YSIZE; ++y) {
            memcpy(pixels, frameBuffer, screens[s].size.x * sizeof(uint16));
            frameBuffer += screens[s].pitch;
            pixels += pitch / sizeof(uint16);
        }

        SDL_UnlockTexture(screenTexture[s]);
    }
}

void RenderDevice::FlipScreen()
{
    if (videoSettings.shaderSupport) {
        SDL_SetGPURenderState(renderer, shaderList[videoSettings.shaderID].state);
    }

    if (windowRefreshDelay > 0) {
        windowRefreshDelay--;
        if (!windowRefreshDelay)
            UpdateGameWindow();
        return;
    }

    float dimAmount = videoSettings.dimMax * videoSettings.dimPercent;

    // Clear the screen. This is needed to keep the
    // pillarboxes in fullscreen from displaying garbage data.
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
    SDL_RenderClear(renderer);

/*
#if (SDL_VERSION >= SDL_VERSIONNUM(3, 2, 0))
    int32 startVert = 0;
    switch (videoSettings.screenCount) {
        default:
        case 0:
#if RETRO_REV02
            startVert = 54;
#else
            startVert = 18;
#endif
            SDL_RenderGeometryRaw(renderer, imageTexture, &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);
            break;

        case 1:
            startVert = 0;
            SDL_RenderGeometryRaw(renderer, screenTexture[0], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);
            break;

        case 2:
#if RETRO_REV02
            startVert = startVertex_2P[0];
#else
            startVert = 6;
#endif
            SDL_RenderGeometryRaw(renderer, screenTexture[0], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);

#if RETRO_REV02
            startVert = startVertex_2P[1];
#else
            startVert = 12;
#endif
            SDL_RenderGeometryRaw(renderer, screenTexture[1], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);
            break;

#if RETRO_REV02
        case 3:
            startVert = startVertex_3P[0];
            SDL_RenderGeometryRaw(renderer, screenTexture[0], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);

            startVert = startVertex_3P[1];
            SDL_RenderGeometryRaw(renderer, screenTexture[1], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);

            startVert = startVertex_3P[2];
            SDL_RenderGeometryRaw(renderer, screenTexture[2], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);
            break;

        case 4:
            startVert = 30;
            SDL_RenderGeometryRaw(renderer, screenTexture[0], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);

            startVert = 36;
            SDL_RenderGeometryRaw(renderer, screenTexture[1], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);

            startVert = 42;
            SDL_RenderGeometryRaw(renderer, screenTexture[2], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);

            startVert = 48;
            SDL_RenderGeometryRaw(renderer, screenTexture[3], &vertexBuffer[startVert].pos.x, sizeof(RenderVertex),
                                  (SDL_FColor *)&vertexBuffer[startVert].color, sizeof(RenderVertex), &vertexBuffer[startVert].tex.x,
                                  sizeof(RenderVertex), 6, NULL, 0, 0);
            break;
#endif
    }
#else
*/
    int32 startVert = 0;
    SDL_FRect src, dst;

    // some cheating for today
#define _SET_RECTS                                                                                                                                   \
    dst.x = vertexBuffer[startVert].pos.x;                                                                                                           \
    dst.y = vertexBuffer[startVert].pos.y;                                                                                                           \
    dst.w = vertexBuffer[startVert + 2].pos.x - dst.x;                                                                                               \
    dst.h = vertexBuffer[startVert + 2].pos.y - dst.y;                                                                                               \
    src.x = vertexBuffer[startVert].tex.x * textureSize.x;                                                                                           \
    src.y = vertexBuffer[startVert].tex.y * textureSize.y;                                                                                           \
    src.w = vertexBuffer[startVert + 2].tex.x * textureSize.x - src.x;                                                                               \
    src.h = vertexBuffer[startVert + 2].tex.y * textureSize.y - src.y;

    switch (videoSettings.screenCount) {
        default:
        case 0:
#if RETRO_REV02
            startVert = 54;
#else
            startVert = 18;
#endif
            _SET_RECTS;
            src.w = vertexBuffer[startVert + 2].tex.x * 1024 - src.x;
            src.h = vertexBuffer[startVert + 2].tex.y * 512 - src.y;
            SDL_RenderTexture(renderer, imageTexture, &src, &dst);
            break;

        case 1:
            startVert = 0;
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[0], &src, &dst);
            break;

        case 2:
#if RETRO_REV02
            startVert = startVertex_2P[0];
#else
            startVert = 6;
#endif
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[0], &src, &dst);

#if RETRO_REV02
            startVert = startVertex_2P[1];
#else
            startVert = 12;
#endif
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[1], &src, &dst);
            break;

#if RETRO_REV02
        case 3:
            startVert = startVertex_3P[0];
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[0], &src, &dst);

            startVert = startVertex_3P[1];
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[1], &src, &dst);

            startVert = startVertex_3P[2];
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[2], &src, &dst);

            break;

        case 4:
            startVert = 30;
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[0], &src, &dst);

            startVert = 36;
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[1], &src, &dst);

            startVert = 42;
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[2], &src, &dst);

            startVert = 48;
            _SET_RECTS;

            SDL_RenderTexture(renderer, screenTexture[3], &src, &dst);

            break;
    }
#endif
    if (dimAmount < 1.0f) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF - (dimAmount * 0xFF));
        SDL_RenderFillRect(renderer, NULL);
    }
    if (videoSettings.shaderSupport) {
        SDL_SetGPURenderState(renderer, nullptr);
    }
    SDL_RenderPresent(renderer);
}

void RenderDevice::ReleaseShaderRenderStates() {
    for (int32 i = 0; i < shaderCount; ++i) {
        SDL_DestroyGPURenderState(shaderList[i].state);
    }

    shaderCount = 0;
#if RETRO_USE_MOD_LOADER
    userShaderCount = 0;
#endif
}

void RenderDevice::Release(bool32 isRefresh)
{
    for (int32 s = 0; s < SCREEN_COUNT; ++s) {
        if (screenTexture[s])
            SDL_DestroyTexture(screenTexture[s]);
        screenTexture[s] = NULL;
    }

    if (imageTexture)
        SDL_DestroyTexture(imageTexture);
    imageTexture = NULL;

    ReleaseShaderRenderStates();

    if (!isRefresh) {
        if (displayInfo.displays)
            free(displayInfo.displays);
        displayInfo.displays = NULL;
    }

    if (!isRefresh && device)
        SDL_DestroyGPUDevice(device);

    if (!isRefresh && renderer)
        SDL_DestroyRenderer(renderer);

    if (!isRefresh && window)
        SDL_DestroyWindow(window);

    if (!isRefresh)
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    if (!isRefresh) {
        if (scanlines)
            free(scanlines);
        scanlines = NULL;
    }
}

void RenderDevice::RefreshWindow()
{
    videoSettings.windowState = WINDOWSTATE_UNINITIALIZED;

    Release(true);

    SDL_HideWindow(window);

    if (videoSettings.windowed && videoSettings.bordered)
        SDL_SetWindowBordered(window, true);
    else
        SDL_SetWindowBordered(window, false);

    GetDisplays();

    SDL_Rect winRect;
    winRect.x = SDL_WINDOWPOS_CENTERED;
    winRect.y = SDL_WINDOWPOS_CENTERED;
    if (videoSettings.windowed || !videoSettings.exclusiveFS) {
        SDL_DisplayID currentWindowDisplay = SDL_GetDisplayForWindow(window);
        const SDL_DisplayMode *displayMode = SDL_GetCurrentDisplayMode(currentWindowDisplay);

        if (videoSettings.windowed) {
            if (videoSettings.windowWidth >= displayMode->w || videoSettings.windowHeight >= displayMode->h) {
                videoSettings.windowWidth  = (displayMode->h / 480 * videoSettings.pixWidth);
                videoSettings.windowHeight = displayMode->h / 480 * videoSettings.pixHeight;
            }

            winRect.w = videoSettings.windowWidth;
            winRect.h = videoSettings.windowHeight;
            SDL_SetWindowFullscreen(window, true);
            SDL_HideCursor();
        }
        else {
            winRect.w = displayMode->w;
            winRect.h = displayMode->h;
            SDL_SetWindowFullscreen(window, false);
            SDL_ShowCursor();
        }

        SDL_SetWindowSize(window, winRect.w, winRect.h);
        SDL_SetWindowPosition(window, winRect.x, winRect.y);
    }

    SDL_ShowWindow(window);

    if (!InitGraphicsAPI() || !InitShaders())
        return;

    videoSettings.windowState = WINDOWSTATE_ACTIVE;
}

void RenderDevice::InitFPSCap()
{
    targetFreq = SDL_GetPerformanceFrequency() / videoSettings.refreshRate;
    curTicks   = 0;
    prevTicks  = 0;
}
bool RenderDevice::CheckFPSCap()
{
    curTicks = SDL_GetPerformanceCounter();
    if (curTicks >= prevTicks + targetFreq)
        return true;

    return false;
}
void RenderDevice::UpdateFPSCap() { prevTicks = curTicks; }

void RenderDevice::InitVertexBuffer()
{
    RenderVertex vertBuffer[sizeof(rsdkVertexBuffer) / sizeof(RenderVertex)];
    memcpy(vertBuffer, rsdkVertexBuffer, sizeof(rsdkVertexBuffer));

    // ignore the last 6 verts, they're scaled to the 1024x512 textures already!
    int32 vertCount = (RETRO_REV02 ? 60 : 24) - 6;

    // Regular in-game screen de-normalization stuff
    for (int32 v = 0; v < vertCount; ++v) {
        RenderVertex *vertex = &vertBuffer[v];
        vertex->pos.x        = NORMALIZE(vertex->pos.x, -1.0, 1.0) * videoSettings.pixWidth;
        vertex->pos.y        = (1.0 - NORMALIZE(vertex->pos.y, -1.0, 1.0)) * SCREEN_YSIZE;

        if (vertex->tex.x)
            vertex->tex.x = screens[0].size.x * (1.0 / textureSize.x);

        if (vertex->tex.y)
            vertex->tex.y = screens[0].size.y * (1.0 / textureSize.y);
    }

    // Fullscreen Image/Video de-normalization stuff
    for (int32 v = 0; v < 6; ++v) {
        RenderVertex *vertex = &vertBuffer[vertCount + v];
        vertex->pos.x        = NORMALIZE(vertex->pos.x, -1.0, 1.0) * videoSettings.pixWidth;
        vertex->pos.y        = (1.0 - NORMALIZE(vertex->pos.y, -1.0, 1.0)) * SCREEN_YSIZE;

        // Set the texture to fill the entire screen with all 1024x512 pixels
        if (vertex->tex.x)
            vertex->tex.x = 1.0f;

        if (vertex->tex.y)
            vertex->tex.y = 1.0f;
    }

    memcpy(vertexBuffer, vertBuffer, sizeof(vertBuffer));
}

bool RenderDevice::InitGraphicsAPI()
{
    videoSettings.shaderSupport = true;

    viewSize.x = 0;
    viewSize.y = 0;

    if (videoSettings.windowed || !videoSettings.exclusiveFS) {
        if (videoSettings.windowed) {
            viewSize.x = videoSettings.windowWidth;
            viewSize.y = videoSettings.windowHeight;
        }
        else {
            viewSize.x = displayWidth[displayModeIndex];
            viewSize.y = displayHeight[displayModeIndex];
        }
    }
    else {
        int32 bufferWidth  = videoSettings.fsWidth;
        int32 bufferHeight = videoSettings.fsHeight;
        if (videoSettings.fsWidth <= 0 || videoSettings.fsHeight <= 0) {
            bufferWidth  = displayWidth[displayModeIndex];
            bufferHeight = displayHeight[displayModeIndex];
        }

        viewSize.x = bufferWidth;
        viewSize.y = bufferHeight;
    }

    SDL_SetWindowSize(window, viewSize.x, viewSize.y);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

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

    pixelSize.x = screens[0].size.x;
    pixelSize.y = screens[0].size.y;

    SDL_SetRenderLogicalPresentation(renderer, videoSettings.pixWidth, SCREEN_YSIZE, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

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
    for (int32 s = 0; s < SCREEN_COUNT; ++s) {
        screenTexture[s] = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, textureSize.x, textureSize.y);

        SDL_SetTextureScaleMode(screenTexture[s], SDL_SCALEMODE_NEAREST);

        if (!screenTexture[s]) {
            PrintLog(PRINT_NORMAL, "ERROR: failed to create screen buffer!\nerror msg: %s", SDL_GetError());
            return 0;
        }
    }
    imageTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RETRO_VIDEO_TEXTURE_W, RETRO_VIDEO_TEXTURE_H);
    SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_LINEAR);
    if (!imageTexture)
        return false;
    SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_NEAREST);

    lastShaderID = -1;
    InitVertexBuffer();
    engine.inFocus          = 1;
    videoSettings.viewportX = 0;
    videoSettings.viewportY = 0;
    videoSettings.viewportW = 1.0 / viewSize.x;
    videoSettings.viewportH = 1.0 / viewSize.y;

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

    ShaderEntry *entry = &shaderList[shaderCount];
    sprintf_s(entry->name, sizeof(entry->name), "%s", fileName);

    if (entry->name == std::string("None")) {
        entry->state = nullptr;
        return;
    }

    SDL_GPUShaderFormat format = SDL_GetGPUShaderFormats(device);

    SDL_GPUShader *shader = nullptr;
    SDL_GPUShaderCreateInfo shaderInfo {
        .entrypoint = "main",
        .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = 1,
        .num_uniform_buffers = 1,
    };

    // compiling the shaders during runtime would require the SDL_shadercross library, which I don't feel like going through the effort of including, so that is not supported
    if (format & SDL_GPU_SHADERFORMAT_SPIRV) {
        sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Shaders/CSO-SDL3/SPIRV/%s.frag", fileName);
        InitFileInfo(&info);
        if (LoadFile(&info, fullFilePath, FMODE_RB)) {
            uint8 *fileData = NULL;
            AllocateStorage((void **)&fileData, info.fileSize + 1, DATASET_TMP, false);
            ReadBytes(&info, fileData, info.fileSize);
            fileData[info.fileSize] = 0;
            CloseFile(&info);

            shaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
            shaderInfo.code_size = info.fileSize;
            shaderInfo.code = fileData;
        }
    }
    else if (format & SDL_GPU_SHADERFORMAT_MSL) {
        sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Shaders/CSO-SDL3/MSL/%s.frag", fileName);
        InitFileInfo(&info);
        if (LoadFile(&info, fullFilePath, FMODE_RB)) {
            uint8 *fileData = NULL;
            AllocateStorage((void **)&fileData, info.fileSize + 1, DATASET_TMP, false);
            ReadBytes(&info, fileData, info.fileSize);
            fileData[info.fileSize] = 0;
            CloseFile(&info);

            shaderInfo.format = SDL_GPU_SHADERFORMAT_MSL;
            shaderInfo.code_size = sizeof(fileData);
            shaderInfo.code = fileData;
        }
    }
    else if (format & SDL_GPU_SHADERFORMAT_DXIL) {
        sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Shaders/CSO-SDL3/DXIL/%s.frag", fileName);
        InitFileInfo(&info);
        if (LoadFile(&info, fullFilePath, FMODE_RB)) {
            uint8 *fileData = NULL;
            AllocateStorage((void **)&fileData, info.fileSize + 1, DATASET_TMP, false);
            ReadBytes(&info, fileData, info.fileSize);
            fileData[info.fileSize] = 0;
            CloseFile(&info);

            shaderInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
            shaderInfo.code_size = sizeof(fileData);
            shaderInfo.code = fileData;
        }  
    }

    shader = SDL_CreateGPUShader(device, &shaderInfo);
    if (!shader) {
        PrintLog(PRINT_ERROR, "Failed to create GPU shader for %s: %s", fileName, SDL_GetError());
        return;
    }

    SDL_GPUSamplerCreateInfo samplerInfo {
        .min_filter  = (linear ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST),
        .mag_filter  = (linear ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST),
        .mipmap_mode = (linear ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST),
    };
    SDL_GPUSampler *sampler = SDL_CreateGPUSampler(device, &samplerInfo);

    SDL_PropertiesID textureProps[videoSettings.screenCount];
    SDL_GPUTexture *gpuTexture[videoSettings.screenCount];
    SDL_GPUTextureSamplerBinding sample[videoSettings.screenCount];
    for (int i = 0; i < videoSettings.screenCount; i++) {
        SDL_zero(textureProps[i]);
        gpuTexture[i] = nullptr;

        textureProps[i] = SDL_GetTextureProperties(screenTexture[i]);
        gpuTexture[i] = reinterpret_cast<SDL_GPUTexture *>(SDL_GetPointerProperty(textureProps[i], SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, nullptr));

        sample[i].texture = gpuTexture[i];
        sample[i].sampler = sampler;
    }

    SDL_GPURenderStateCreateInfo stateInfo  {
        .fragment_shader = shader,
        .num_sampler_bindings = videoSettings.screenCount,
        .sampler_bindings = sample
    };

    SDL_BeginGPURenderPass(SDL_GPUCommandBuffer *command_buffer, const SDL_GPUColorTargetInfo *color_target_infos, Uint32 num_color_targets, const SDL_GPUDepthStencilTargetInfo *depth_stencil_target_info)

    SDL_BindGPUFragmentSamplers()

    entry->state = SDL_CreateGPURenderState(renderer, &stateInfo);

    SDL_ReleaseGPUShader(device, shader);
}

bool RenderDevice::InitShaders()
{
    int32 maxShaders = 0;
#if RETRO_USE_MOD_LOADER
    // who knows maybe SDL3 will have shaders
    shaderCount = 0;
#endif

    if (videoSettings.shaderSupport) {
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
    }
    else {
        for (int32 s = 0; s < SHADER_COUNT; ++s) shaderList[s].linear = true;

        shaderList[0].linear = videoSettings.windowed ? false : shaderList[0].linear;
        maxShaders           = 1;
        shaderCount          = 1;
    }

    videoSettings.shaderID = videoSettings.shaderID >= maxShaders ? 0 : videoSettings.shaderID;

    return true;
}

bool RenderDevice::SetupRendering()
{
    renderer = SDL_CreateRenderer(window, SDL_GPU_RENDERER);

    if (!renderer) {
        PrintLog(PRINT_NORMAL, "ERROR: failed to create renderer!");
        return false;
    }

    device = SDL_GetGPURendererDevice(renderer);

    if (!device) {
        PrintLog(PRINT_NORMAL, "ERROR: failed to create GPU device!");
        return false;
    }

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
    SDL_DisplayID currentWindowDisplay = SDL_GetDisplayForWindow(window);

    int32 dispCount;
    SDL_GetDisplays(&dispCount);

    const SDL_DisplayMode *currentDisplay = SDL_GetCurrentDisplayMode(currentWindowDisplay);

    displayModeIndex = 0;
    for (int32 a = 0; a < dispCount; ++a) {
        const SDL_DisplayMode *displayMode = SDL_GetCurrentDisplayMode(currentWindowDisplay);

        displayWidth[a]  = displayMode->w;
        displayHeight[a] = displayMode->h;

        if (memcmp(&currentDisplay, &displayMode, sizeof(displayMode)) == 0) {
            displayModeIndex = a;
        }
    }

    SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(currentWindowDisplay, &displayCount);
    if (displayInfo.displays)
        free(displayInfo.displays);

    displayInfo.displays          = (decltype(displayInfo.displays))malloc(sizeof(SDL_DisplayMode) * displayCount);
    int32 newDisplayCount         = 0;
    bool32 foundFullScreenDisplay = false;

    for (int32 d = displayCount - 1; d >= 0; --d) {
        // SDL_GetDisplayMode(currentWindowDisplay, d, &displayInfo.displays[newDisplayCount].internal);
        displayInfo.displays[newDisplayCount].internal = *modes[d];

        int32 refreshRate = displayInfo.displays[newDisplayCount].refresh_rate;
        if (refreshRate >= 59 && (refreshRate <= 60 || refreshRate >= 120) && displayInfo.displays[newDisplayCount].height >= (SCREEN_YSIZE * 2)) {
            if (newDisplayCount != 0 && refreshRate == 60 && displayInfo.displays[newDisplayCount - 1].refresh_rate == 59) {
                memcpy(&displayInfo.displays[newDisplayCount - 1], &displayInfo.displays[newDisplayCount], sizeof(displayInfo.displays[0]));
                --newDisplayCount;
            }

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

void RenderDevice::GetWindowSize(int32 *width, int32 *height)
{
    if (!videoSettings.windowed) {
        SDL_GetCurrentRenderOutputSize(renderer, width, height);
    }
    else {
        SDL_DisplayID currentWindowDisplay = SDL_GetDisplayForWindow(window);

        const SDL_DisplayMode *display = SDL_GetCurrentDisplayMode(currentWindowDisplay);

        if (width)
            *width = display->w;

        if (height)
            *height = display->h;
    }
}

void RenderDevice::ProcessEvent(SDL_Event event)
{
    switch (event.type) {
        case SDL_ADDEVENT:
            switch (event.window.type) {
                case SDL_EVENT_WINDOW_MAXIMIZED: {
                    SDL_RestoreWindow(window);
                    SDL_SetWindowFullscreen(window, true);
                    SDL_HideCursor();
                    videoSettings.windowed = false;
                    break;
                }

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: isRunning = false; break;

                case SDL_EVENT_WINDOW_FOCUS_GAINED:
#if RETRO_REV02
                    SKU::userCore->focusState = 0;
#endif
                    break;

                case SDL_EVENT_WINDOW_FOCUS_LOST:
#if RETRO_REV02
                    SKU::userCore->focusState = 1;
#endif
                    break;
                default: break;
            }
            break;

        case SDL_EVENT_GAMEPAD_ADDED: {
            SDL_Gamepad *game_controller = SDL_OpenGamepad(event.cdevice.which);

            if (game_controller != NULL) {
                uint32 id;
                char idBuffer[0x20];
                sprintf_s(idBuffer, sizeof(idBuffer), "SDLDevice%d", SDL_GetJoystickID(SDL_GetGamepadJoystick(game_controller)));
                GenerateHashCRC(&id, idBuffer);

                if (SKU::InitSDL3InputDevice(id, game_controller) == NULL)
                    SDL_CloseGamepad(game_controller);
            }

            break;
        }

        case SDL_EVENT_GAMEPAD_REMOVED: {
            uint32 id;
            char idBuffer[0x20];
            sprintf_s(idBuffer, sizeof(idBuffer), "SDLDevice%d", event.cdevice.which);
            GenerateHashCRC(&id, idBuffer);

            RemoveInputDevice(InputDeviceFromID(id));
            break;
        }

        case SDL_EVENT_WILL_ENTER_FOREGROUND:
#if RETRO_REV02
            SKU::userCore->focusState = 0;
#endif
            break;

        case SDL_EVENT_WILL_ENTER_BACKGROUND:
#if RETRO_REV02
            SKU::userCore->focusState = 1;
#endif
            break;

        case SDL_EVENT_TERMINATING: isRunning = false; break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            switch (event.button.button) {
                case SDL_BUTTON_LEFT: touchInfo.down[0] = true; touchInfo.count = 1;
#if !RETRO_REV02
                    RSDK::SKU::buttonDownCount++;
#endif
                    break;

                case SDL_BUTTON_RIGHT:
#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                    RSDK::SKU::specialKeyStates[3] = true;
                    RSDK::SKU::buttonDownCount++;
#endif
                    break;
            }
            break;

        case SDL_EVENT_MOUSE_BUTTON_UP:
            switch (event.button.button) {
                case SDL_BUTTON_LEFT: touchInfo.down[0] = false; touchInfo.count = 0;
#if !RETRO_REV02
                    RSDK::SKU::buttonDownCount--;
#endif
                    break;

                case SDL_BUTTON_RIGHT:
#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                    RSDK::SKU::specialKeyStates[3] = false;
                    RSDK::SKU::buttonDownCount--;
#endif
                    break;
            }
            break;

        case SDL_EVENT_FINGER_MOTION:
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_UP: {
            int32 count;
            SDL_Finger **fingers = SDL_GetTouchFingers(event.tfinger.touchID, &count);
            touchInfo.count = 0;
            for (int32 i = 0; i < count; i++) {
                SDL_Finger *finger = fingers[i];
                if (finger) {
                    touchInfo.down[touchInfo.count] = true;
                    touchInfo.x[touchInfo.count]    = finger->x;
                    touchInfo.y[touchInfo.count]    = finger->y;
                    touchInfo.count++;
                }
            }
            break;
        }

        case SDL_EVENT_KEY_DOWN:
#if !RETRO_REV02
            ++RSDK::SKU::buttonDownCount;
#endif
            switch (event.key.scancode) {
                case SDL_SCANCODE_RETURN:
                    if (event.key.mod & SDL_KMOD_LALT) {
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
                    SKU::UpdateKeyState(event.key.scancode);
#endif
                    break;

                case SDL_SCANCODE_ESCAPE:
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
                        SKU::UpdateKeyState(event.key.scancode);
#endif
                    }

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                    RSDK::SKU::specialKeyStates[0] = true;
#endif
                    break;

#if !RETRO_USE_ORIGINAL_CODE
                case SDL_SCANCODE_F1:
                    if (engine.devMenu) {
                        sceneInfo.listPos--;
                        while (sceneInfo.listPos < sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart
                            || sceneInfo.listPos > sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd
                            || !sceneInfo.listCategory[sceneInfo.activeCategory].sceneCount) {
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

                case SDL_SCANCODE_F2:
                    if (engine.devMenu) {
                        sceneInfo.listPos++;
                        while (sceneInfo.listPos < sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetStart
                            || sceneInfo.listPos > sceneInfo.listCategory[sceneInfo.activeCategory].sceneOffsetEnd
                            || !sceneInfo.listCategory[sceneInfo.activeCategory].sceneCount) {
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

                case SDL_SCANCODE_F3:
                    if (userShaderCount)
                        videoSettings.shaderID = (videoSettings.shaderID + 1) % userShaderCount;
                    break;

#if !RETRO_USE_ORIGINAL_CODE
                case SDL_SCANCODE_F4:
                    if (engine.devMenu)
                        engine.showEntityInfo ^= 1;
                    break;

                case SDL_SCANCODE_F5:
                    if (engine.devMenu) {
                        // Quick-Reload
#if RETRO_USE_MOD_LOADER
                        if (event.key.mod & SDL_KMOD_LCTRL)
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

                case SDL_SCANCODE_F6:
                    if (engine.devMenu && videoSettings.screenCount > 1)
                        videoSettings.screenCount--;
                    break;

                case SDL_SCANCODE_F7:
                    if (engine.devMenu && videoSettings.screenCount < SCREEN_COUNT)
                        videoSettings.screenCount++;
                    break;

                case SDL_SCANCODE_F8:
                    if (engine.devMenu)
                        engine.showUpdateRanges ^= 1;
                    break;

                case SDL_SCANCODE_F9:
                    if (engine.devMenu)
                        showHitboxes ^= 1;
                    break;

                case SDL_SCANCODE_F10:
                    if (engine.devMenu)
                        engine.showPaletteOverlay ^= 1;
                    break;
#endif
                case SDL_SCANCODE_BACKSPACE:
                    if (engine.devMenu)
                        engine.gameSpeed = engine.fastForwardSpeed;
                    break;

                case SDL_SCANCODE_F11:
                case SDL_SCANCODE_INSERT:
                    if (engine.devMenu)
                        engine.frameStep = true;
                    break;

                case SDL_SCANCODE_F12:
                case SDL_SCANCODE_PAUSE:
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

        case SDL_EVENT_KEY_UP:
#if !RETRO_REV02
            --RSDK::SKU::buttonDownCount;
#endif
            switch (event.key.scancode) {
                default:
#if RETRO_INPUTDEVICE_KEYBOARD
                    SKU::ClearKeyState(event.key.scancode);
#endif
                    break;

#if !RETRO_REV02 && RETRO_INPUTDEVICE_KEYBOARD
                case SDL_SCANCODE_ESCAPE:
                    RSDK::SKU::specialKeyStates[0] = false;
                    SKU::ClearKeyState(event.key.keysym.scancode);
                    break;

                case SDL_SCANCODE_RETURN:
                    RSDK::SKU::specialKeyStates[1] = false;
                    SKU::ClearKeyState(event.key.keysym.scancode);
                    break;
#endif
                case SDL_SCANCODE_BACKSPACE: engine.gameSpeed = 1; break;
            }
            break;

        case SDL_EVENT_QUIT: isRunning = false; break;
    }
}

bool RenderDevice::ProcessEvents()
{
    SDL_Event sdlEvent;

    while (SDL_PollEvent(&sdlEvent)) {
        ProcessEvent(sdlEvent);

        if (!isRunning)
            return false;
    }

    return isRunning;
}

void RenderDevice::SetupImageTexture(int32 width, int32 height, uint8 *imagePixels)
{
    if (lastTextureFormat != SHADER_RGB_IMAGE) {
        if (imageTexture)
            SDL_DestroyTexture(imageTexture);
        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_LINEAR);

        imageTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_NEAREST);

        lastTextureFormat = SHADER_RGB_IMAGE;
    }

    int32 texPitch = 0;
    uint32 *pixels = NULL;
    SDL_LockTexture(imageTexture, NULL, (void **)&pixels, &texPitch);

    int32 pitch           = (texPitch >> 2) - width;
    uint32 *imagePixels32 = (uint32 *)imagePixels;
    for (int32 y = 0; y < height; ++y) {
        for (int32 x = 0; x < width; ++x) {
            *pixels++ = *imagePixels32++;
        }

        pixels += pitch;
    }

    SDL_UnlockTexture(imageTexture);
}

void RenderDevice::SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    if (lastTextureFormat != SHADER_YUV_420) {
        if (imageTexture)
            SDL_DestroyTexture(imageTexture);
        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_LINEAR);

        imageTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);

        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_NEAREST);
        lastTextureFormat = SHADER_YUV_420;
    }

    SDL_UpdateYUVTexture(imageTexture, NULL, yPlane, strideY, uPlane, strideU, vPlane, strideV);
}
void RenderDevice::SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    if (lastTextureFormat != SHADER_YUV_422) {
        if (imageTexture)
            SDL_DestroyTexture(imageTexture);
        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_LINEAR);

        imageTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);

        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_NEAREST);
        lastTextureFormat = SHADER_YUV_422;
    }

    SDL_UpdateYUVTexture(imageTexture, NULL, yPlane, strideY, uPlane, strideU, vPlane, strideV);
}
void RenderDevice::SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    if (lastTextureFormat != SHADER_YUV_444) {
        if (imageTexture)
            SDL_DestroyTexture(imageTexture);
        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_LINEAR);

        imageTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);

        SDL_SetTextureScaleMode(imageTexture, SDL_SCALEMODE_LINEAR);
        lastTextureFormat = SHADER_YUV_444;
    }

    SDL_UpdateYUVTexture(imageTexture, NULL, yPlane, strideY, uPlane, strideU, vPlane, strideV);
}
