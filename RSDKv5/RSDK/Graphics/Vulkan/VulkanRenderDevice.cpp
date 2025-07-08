#ifdef VK_INTELLISENSE
#include "RetroEngine.hpp"
#include "VulkanRenderDevice.hpp"
#endif

#include <set>
#include <cfloat> // FLT_MAX

// :sob: :sob: :sob:
#include "BackupShaders.hpp"

#ifdef VK_DEBUG
#define VK_DEBUG_THRESHOLD VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT

const char *validationLayers[] = { "VK_LAYER_KHRONOS_validation" };

VKAPI_ATTR VkBool32 VKAPI_CALL RenderDevice::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_THRESHOLD) {
        PrintLog(PRINT_NORMAL, "[VK DEBUG] %s", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

VkDebugUtilsMessengerEXT RenderDevice::debugMessenger;
#endif

const char *requiredExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

bool RenderDevice::CheckExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> extensionList(std::begin(requiredExtensions), std::end(requiredExtensions));

    for (const auto &extension : availableExtensions) {
        extensionList.erase(extension.extensionName);
    }

    return extensionList.empty();
}

RenderDevice::SwapChainDetails RenderDevice::QuerySwapChainDetails(VkPhysicalDevice device)
{
    SwapChainDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    currentSwapDetails = details;
    return details;
}

bool RenderDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer,
                                VkDeviceMemory &bufferMemory)
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;

    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        return false;
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
    return true;
}

struct RenderDevice::ShaderConstants {
    float2 pixelSize;
    float2 textureSize;
    float2 viewSize;
#if RETRO_REV02
    float screenDim;
#endif
};

GLFWwindow *RenderDevice::window;

VkInstance RenderDevice::instance;
VkPhysicalDevice RenderDevice::physicalDevice;
VkDevice RenderDevice::device;
VkSurfaceKHR RenderDevice::surface;

RenderDevice::SwapChainDetails RenderDevice::currentSwapDetails;
VkSwapchainKHR RenderDevice::swapChain;
VkFormat RenderDevice::swapChainImageFormat;
VkExtent2D RenderDevice::swapChainExtent;

std::vector<VkImage> RenderDevice::swapChainImages;
std::vector<VkImageView> RenderDevice::swapChainImageViews;
std::vector<VkFramebuffer> RenderDevice::swapChainFramebuffers;

VkBuffer RenderDevice::uniformBuffer;
VkDeviceMemory RenderDevice::uniformBufferMemory;
RenderDevice::ShaderConstants *RenderDevice::uniformMap;

VkDescriptorPool RenderDevice::descriptorPool;
VkDescriptorSet RenderDevice::descriptorSet;

VkRenderPass RenderDevice::renderPass;
VkDescriptorSetLayout RenderDevice::setLayout;
VkPipelineLayout RenderDevice::pipelineLayout;
VkGraphicsPipelineCreateInfo RenderDevice::basePipeline;

VkBuffer RenderDevice::vertexBuffer;
VkDeviceMemory RenderDevice::vertexBufferMemory;

VkCommandPool RenderDevice::commandPool;
VkCommandBuffer RenderDevice::commandBuffer;

VkQueue RenderDevice::graphicsQueue;
VkQueue RenderDevice::presentQueue;
uint32 RenderDevice::graphicsIndex;
uint32 RenderDevice::presentIndex;

VkViewport RenderDevice::viewport;

VkSemaphore RenderDevice::imageAvailableSemaphore;
VkSemaphore RenderDevice::renderFinishedSemaphore;
VkFence RenderDevice::inFlightFence;

int32 RenderDevice::monitorIndex;

VkSampler RenderDevice::samplerPoint;
VkSampler RenderDevice::samplerLinear;

VkDescriptorImageInfo RenderDevice::imageInfo;

RenderDevice::Texture RenderDevice::imageTexture;
RenderDevice::Texture RenderDevice::screenTextures[SCREEN_COUNT];

double RenderDevice::lastFrame;
double RenderDevice::targetFreq;

//! PIPELINE INFOS
VkPipelineVertexInputStateCreateInfo vertexInputInfo;
VkPipelineInputAssemblyStateCreateInfo inputAssembly;
VkPipelineViewportStateCreateInfo viewportState;
VkPipelineRasterizationStateCreateInfo rasterizer;
VkPipelineDynamicStateCreateInfo dynamicState;
VkDynamicState dynamicStates[1];
VkPipelineColorBlendAttachmentState colorBlendAttachment;
VkPipelineColorBlendStateCreateInfo colorBlending;
VkPipelineMultisampleStateCreateInfo multisampleInfo;

bool RenderDevice::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (!videoSettings.bordered)
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWmonitor *monitor = NULL;
    int32 w, h;

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
        return false;
    }
    PrintLog(PRINT_NORMAL, "w: %d h: %d windowed: %d", w, h, videoSettings.windowed);

    glfwSetKeyCallback(window, ProcessKeyEvent);
    glfwSetJoystickCallback(ProcessJoystickEvent);
    glfwSetMouseButtonCallback(window, ProcessMouseEvent);
    glfwSetWindowFocusCallback(window, ProcessFocusEvent);
    glfwSetWindowMaximizeCallback(window, ProcessMaximizeEvent);

    if (!SetupRendering() || !AudioDevice::Init())
        return false;

    InitInputDevices();
    return true;
}

bool RenderDevice::SetupRendering()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    PrintLog(PRINT_NORMAL, "[VK] %d extension supported", extensionCount);

    //! DEFINE APP
    VkApplicationInfo appInfo{};
    appInfo.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "RSDK" ENGINE_V_NAME;
    appInfo.pEngineName      = "RSDK" ENGINE_V_NAME;
    // i aint trying to populate these vers
    appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.engineVersion      = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

#if VULKAN_USE_GLFW
    //! GET REQUIRED GLFW EXTENSIONS
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#endif

    // some macOS code here might be needed:
    // https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Instance#page_Encountered-VK_ERROR_INCOMPATIBLE_DRIVER

#ifndef VK_DEBUG
    instanceInfo.enabledLayerCount = 0;
#else
    //! VALIDATION LAYERS
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                break;
            }
        }

        PrintLog(PRINT_NORMAL, "[VK] Requested validation layer %s not found, we won't have it!!!!", layerName);
    }

    instanceInfo.enabledLayerCount   = sizeof(validationLayers) / sizeof(const char *);
    instanceInfo.ppEnabledLayerNames = validationLayers;

    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    //! CHECK FOR OTHER EXTENSIONS (none required probably)

    instanceInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Could not create instance");
        return false;
    }

#if VK_DEBUG
    //! SETUP CALLBACK

    VkDebugUtilsMessengerCreateInfoEXT callbackInfo{};
    callbackInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    callbackInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    callbackInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                               | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    callbackInfo.pfnUserCallback = DebugCallback;
    callbackInfo.pUserData       = nullptr;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        if (func(instance, &callbackInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create debug callback messenger");
            return false;
        }
    }
    else {
        PrintLog(PRINT_NORMAL, "[VK] Requested debug callback extension not found");
        return false;
    }
#endif

    //! SURFACE SETUP
#ifdef VULKAN_USE_GLFW
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Vulkan surface could not be created");
        return false;
    }
#endif

    //! PICK PHYSICAL DEVICE (we'll settle for any with VK_QUEUE_GRAPHICS_BIT)
    physicalDevice       = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (!deviceCount) {
        PrintLog(PRINT_NORMAL, "[VK] No GPU with Vulkan support found");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families#page_Base-device-suitability-checks

    for (const auto &device : devices) {
        if (IsDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        PrintLog(PRINT_NORMAL, "[VK] No suitable GPU found (but %d GPUs support Vulkan)", deviceCount);
        return false;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32> uniqueQueueFamilies = { graphicsIndex, presentIndex };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos    = queueCreateInfos.data();
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    deviceInfo.pEnabledFeatures = &deviceFeatures;

#ifndef VK_DEBUG
    deviceInfo.enabledLayerCount = 0;
#else
    deviceInfo.enabledLayerCount   = sizeof(validationLayers) / sizeof(const char *);
    deviceInfo.ppEnabledLayerNames = validationLayers;
#endif

    deviceInfo.enabledExtensionCount   = sizeof(requiredExtensions) / sizeof(const char *);
    deviceInfo.ppEnabledExtensionNames = requiredExtensions;

    if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create device");
        return false;
    }

    vkGetDeviceQueue(device, graphicsIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentIndex, 0, &presentQueue);

    swapChain = VK_NULL_HANDLE;

    GetDisplays();

    descriptorSet = VK_NULL_HANDLE;

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

    //! PICK SWAPCHAIN DETAILS
    VkSurfaceFormatKHR pickedFormat = currentSwapDetails.formats[0];
    VkPresentModeKHR pickedMode     = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D pickedExtent         = currentSwapDetails.capabilities.currentExtent;

    for (const auto &availableFormat : currentSwapDetails.formats) {
        if ((availableFormat.format == VK_FORMAT_R8G8B8_UNORM || availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            pickedFormat = availableFormat;
            break;
        }
    }

    if (!videoSettings.vsync) {
        for (const auto &availablePresentMode : currentSwapDetails.presentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                pickedMode = availablePresentMode;
            }
        }
    }

    pickedExtent = { static_cast<uint32_t>(viewSize.x), static_cast<uint32_t>(viewSize.y) };

    pickedExtent.width =
        CLAMP(pickedExtent.width, currentSwapDetails.capabilities.minImageExtent.width, currentSwapDetails.capabilities.maxImageExtent.width);
    pickedExtent.height =
        CLAMP(pickedExtent.height, currentSwapDetails.capabilities.minImageExtent.height, currentSwapDetails.capabilities.maxImageExtent.height);

    //! CREATE SWAPCHAIN
    uint32_t imageCount = currentSwapDetails.capabilities.minImageCount + 1;
    if (currentSwapDetails.capabilities.maxImageCount > 0 && imageCount > currentSwapDetails.capabilities.maxImageCount) {
        imageCount = currentSwapDetails.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapCreateInfo{};
    swapCreateInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapCreateInfo.surface          = surface;
    swapCreateInfo.minImageCount    = imageCount;
    swapCreateInfo.imageFormat      = pickedFormat.format;
    swapCreateInfo.imageColorSpace  = pickedFormat.colorSpace;
    swapCreateInfo.imageExtent      = pickedExtent;
    swapCreateInfo.imageArrayLayers = 1;
    swapCreateInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (graphicsIndex != presentIndex) {
        swapCreateInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        uint32_t queueFamilyIndices[]        = { graphicsIndex, presentIndex };
        swapCreateInfo.queueFamilyIndexCount = 2;
        swapCreateInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else {
        swapCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapCreateInfo.preTransform   = currentSwapDetails.capabilities.currentTransform;
    swapCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapCreateInfo.presentMode    = pickedMode;
    swapCreateInfo.clipped        = VK_TRUE;

    // swapCreateInfo.oldSwapchain = swapChain;

    if (vkCreateSwapchainKHR(device, &swapCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create swapchain");
        return false;
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = pickedFormat.format;
    swapChainExtent      = pickedExtent;

    //! CREATE IMAGE VIEWS
    swapChainImageViews.resize(swapChainImages.size());
    for (int i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image        = swapChainImages[i];
        createInfo.viewType     = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format       = swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create image chain #%d", i);
            return false;
        }
    }

    //! CREATE RENDERPASS
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format  = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

    colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create render pass");
    }

    //! SET DYNAMIC STATE
    dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;

    dynamicState                   = {};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState);
    dynamicState.pDynamicStates    = dynamicStates;

    //! SET FIXED FUNCTION CONTROLLERS
    inputAssembly                        = {};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    viewport.x        = (float)viewportPos.x;
    viewport.y        = (float)viewportPos.y;
    viewport.width    = (float)viewportSize.x;
    viewport.height   = (float)viewportSize.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    viewportState               = {};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = (VkRect2D *)&viewport;

    rasterizer                         = {};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    colorBlendAttachment                = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    colorBlending                 = {};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    multisampleInfo                      = {};
    multisampleInfo.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    //! CREATE UNIFORM AND SAMPLER LAYOUT
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding            = 0;
    samplerLayoutBinding.descriptorCount    = 1;
    samplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = 1;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = { samplerLayoutBinding, uboLayoutBinding };

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings    = bindings;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &setLayout) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create descriptor set layout");
        return false;
    }

    //! CREATE PIPELINE LAYOUT
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &setLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges    = nullptr;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create pipeline layout");
        return false;
    }

    //! CREATE FRAMEBUFFERS
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = { swapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = swapChainExtent.width;
        framebufferInfo.height          = swapChainExtent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create framebuffer %d", i);
            return false;
        }
    }

    //! CREATE COMMAND POOL
    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = graphicsIndex;

    if (vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create command pool");
        return false;
    }

    //! CREATE COMMAND BUFFER
    VkCommandBufferAllocateInfo cmdAllocInfo{};
    cmdAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAllocInfo.commandPool        = commandPool;
    cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create command buffer");
        return false;
    }

    //! CREATE SYNCHRONIZATION OBJECTS
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS
        || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS
        || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Unable to create sephamores");
    }

    //! TEXTURE CREATION
    for (int32 i = 0; i < SCREEN_COUNT; ++i) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = static_cast<uint32_t>(textureSize.x);
        imageInfo.extent.height = static_cast<uint32_t>(textureSize.y);
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = VK_FORMAT_R5G6B5_UNORM_PACK16;
        imageInfo.tiling        = VK_IMAGE_TILING_LINEAR;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &screenTextures[i].image) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create image for screen texture %d", i);
            return false;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, screenTextures[i].image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &screenTextures[i].memory) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to allocate memory for screen texture %d", i);
            return false;
        }

        vkBindImageMemory(device, screenTextures[i].image, screenTextures[i].memory, 0);
        vkMapMemory(device, screenTextures[i].memory, 0, memRequirements.size, 0, &screenTextures[i].map);

        VkImageSubresource subresource{};
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkGetImageSubresourceLayout(device, screenTextures[i].image, &subresource, &screenTextures[i].layout);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = screenTextures[i].image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = VK_FORMAT_R5G6B5_UNORM_PACK16;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &screenTextures[i].view) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create image view for screen texture %d", i);
            return false;
        }

        //! TRANSITION TO GENERAL
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandPool        = commandPool;
        cmdAllocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = screenTextures[i].image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

        //! WE DON'T MAKE THE SAMPLER YET
    }

    {
        //! IMAGE TEXTURE
        VkImageCreateInfo imageInfo{};
        imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = RETRO_VIDEO_TEXTURE_W;
        imageInfo.extent.height = RETRO_VIDEO_TEXTURE_H;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = VK_FORMAT_B8G8R8A8_UNORM;
        imageInfo.tiling        = VK_IMAGE_TILING_LINEAR;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        imageInfo.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(device, &imageInfo, nullptr, &imageTexture.image) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create image for image texture");
            return false;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, imageTexture.image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageTexture.memory) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to allocate memory for image texturre");
            return false;
        }

        vkBindImageMemory(device, imageTexture.image, imageTexture.memory, 0);
        vkMapMemory(device, imageTexture.memory, 0, memRequirements.size, 0, &imageTexture.map);

        VkImageSubresource subresource{};
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vkGetImageSubresourceLayout(device, imageTexture.image, &subresource, &imageTexture.layout);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image                           = imageTexture.image;
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = VK_FORMAT_B8G8R8A8_UNORM;
        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &imageTexture.view) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create image view for image texture");
            return false;
        }

        //! TRANSITION TO GENERAL
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandPool        = commandPool;
        cmdAllocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_PREINITIALIZED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = imageTexture.image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &barrier);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    lastShaderID = -1;

    InitVertexBuffer();

    //! CREATE UNIFORM BUFFER
    if (!CreateBuffer(sizeof(ShaderConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory)) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create uniform buffer");
        return false;
    }

    vkMapMemory(device, uniformBufferMemory, 0, sizeof(ShaderConstants), 0, (void **)&uniformMap);

    //! CREATE FULL BASE PIPELINE
    basePipeline            = {};
    basePipeline.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    basePipeline.stageCount = 0;
    basePipeline.pStages    = nullptr; //! WE FILL THESE LATER!!

    basePipeline.pVertexInputState   = &vertexInputInfo;
    basePipeline.pInputAssemblyState = &inputAssembly;
    basePipeline.pViewportState      = &viewportState;
    basePipeline.pRasterizationState = &rasterizer;
    basePipeline.pDynamicState       = &dynamicState;
    basePipeline.pColorBlendState    = &colorBlending;
    basePipeline.pMultisampleState   = &multisampleInfo;
    basePipeline.layout              = pipelineLayout;
    basePipeline.renderPass          = renderPass;
    basePipeline.subpass             = 0;

    //! CREATE DESCRIPTOR POOL
    VkDescriptorPoolSize poolSizes[] = { {}, {} };
    poolSizes[0].type                = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount     = 1;
    poolSizes[1].type                = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount     = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes    = poolSizes;
    poolInfo.maxSets       = 1;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create descriptor pool");
    }

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
const RenderVertex rsdkVKVertexBuffer[60] = {
    // 1 Screen (0)
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 2 Screens - Bordered (Top Screen) (6)
    { { +0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +0.5, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -0.5, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -0.5, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 2 Screens - Bordered (Bottom Screen) (12)
    { { +0.5,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +0.5,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -0.5,  1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -0.5,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 2 Screens - Stretched (Top Screen) (18)
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
  
    // 2 Screens - Stretched (Bottom Screen) (24)
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 4 Screens (Top-Left) (30)
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },

    // 4 Screens (Top-Right) (36)
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { {  0.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { {  0.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 4 Screens (Bottom-Right) (42)
    { {  0.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { {  0.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // 4 Screens (Bottom-Left) (48)
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0,  0.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { {  0.0,  1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { {  0.0,  0.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    
    // Image/Video (54)
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { +1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  0.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } },
    { { +1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  1.0,  1.0 } },
    { { -1.0,  1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  1.0 } },
    { { -1.0, -1.0,  1.0 }, 0xFFFFFFFF, {  0.0,  0.0 } }
};
#else
const RenderVertex rsdkVKVertexBuffer[24] =
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
// clang-format on

VkVertexInputBindingDescription bindingDescription;
VkVertexInputAttributeDescription attributeDescriptions[3];

bool RenderDevice::InitVertexBuffer()
{
    RenderVertex vertBuffer[sizeof(rsdkVKVertexBuffer) / sizeof(RenderVertex)];
    memcpy(vertBuffer, rsdkVKVertexBuffer, sizeof(rsdkVKVertexBuffer));

    float x = 0.5 / (float)viewSize.x;
    float y = 0.5 / (float)viewSize.y;

    // ignore the last 6 verts, they're scaled to the 1024x512 textures already!
    int32 vertCount = (RETRO_REV02 ? 60 : 24) - 6;
    for (int32 v = 0; v < vertCount; ++v) {
        RenderVertex *vertex = &vertBuffer[v];
        vertex->pos.x        = vertex->pos.x + x;
        vertex->pos.y        = vertex->pos.y + y;

        if (vertex->tex.x)
            vertex->tex.x = screens[0].size.x * (1.0 / textureSize.x);

        if (vertex->tex.y)
            vertex->tex.y = screens[0].size.y * (1.0 / textureSize.y);
    }

    //! CONFIGURE MEMORY
    if (!CreateBuffer(sizeof(vertBuffer), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory)) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create vertex buffer");
        return false;
    }

    void *data;
    vkMapMemory(device, vertexBufferMemory, 0, sizeof(vertBuffer), 0, &data);
    memcpy(data, vertBuffer, sizeof(vertBuffer));
    vkUnmapMemory(device, vertexBufferMemory);

    // remember to bind it later!

    //! SET VERTEX BINDING DESCRIPTION
    bindingDescription           = {};
    bindingDescription.binding   = 0;
    bindingDescription.stride    = sizeof(RenderVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    //! SET ATTRIBUTE DESCRIPTIONS
    // in_pos
    attributeDescriptions[0]          = {};
    attributeDescriptions[0].binding  = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset   = offsetof(RenderVertex, pos);
    // in_color
    attributeDescriptions[1]          = {};
    attributeDescriptions[1].binding  = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format   = VK_FORMAT_A8B8G8R8_UNORM_PACK32;
    attributeDescriptions[1].offset   = offsetof(RenderVertex, color);
    // in_UV
    attributeDescriptions[2]          = {};
    attributeDescriptions[2].binding  = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset   = offsetof(RenderVertex, tex);

    vertexInputInfo                                 = {};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions;

    return true;
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
        uint16 *pixels      = (uint16 *)screenTextures[s].map;
        uint16 *frameBuffer = screens[s].frameBuffer;

        int32 screenPitch = screens[s].pitch;
        int32 pitch       = (screenTextures[s].layout.rowPitch >> 1) - screenPitch;

        for (int32 y = 0; y < SCREEN_YSIZE; ++y) {
            int32 pixelCount = screenPitch >> 4;
            for (int32 x = 0; x < pixelCount; ++x) {
                pixels[0]  = frameBuffer[0];
                pixels[1]  = frameBuffer[1];
                pixels[2]  = frameBuffer[2];
                pixels[3]  = frameBuffer[3];
                pixels[4]  = frameBuffer[4];
                pixels[5]  = frameBuffer[5];
                pixels[6]  = frameBuffer[6];
                pixels[7]  = frameBuffer[7];
                pixels[8]  = frameBuffer[8];
                pixels[9]  = frameBuffer[9];
                pixels[10] = frameBuffer[10];
                pixels[11] = frameBuffer[11];
                pixels[12] = frameBuffer[12];
                pixels[13] = frameBuffer[13];
                pixels[14] = frameBuffer[14];
                pixels[15] = frameBuffer[15];

                frameBuffer += 16;
                pixels += 16;
            }

            pixels += pitch;
        }
    }
}

bool RenderDevice::ProcessEvents()
{
    glfwPollEvents();
    if (glfwWindowShouldClose(window))
        isRunning = false;
    return false;
}

VkWriteDescriptorSet descriptorWrites[2];

void RenderDevice::FlipScreen()
{
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // RefreshWindow();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to acquire swapchain image");
        return;
    }

    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to begin recording command buffer");
        return;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor        = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (lastShaderID != videoSettings.shaderID) {
        lastShaderID = videoSettings.shaderID;

        imageInfo.sampler = shaderList[videoSettings.shaderID].linear ? samplerLinear : samplerPoint;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shaderList[videoSettings.shaderSupport ? videoSettings.shaderID : 0].shaderPipeline);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[]   = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    if (windowRefreshDelay > 0) {
        windowRefreshDelay--;
        if (!windowRefreshDelay)
            UpdateGameWindow();
        return;
    }

    if (videoSettings.shaderSupport) {
        uniformMap->textureSize = textureSize;
        uniformMap->pixelSize   = pixelSize;
        uniformMap->viewSize    = viewSize;
#if RETRO_REV02
        uniformMap->screenDim = videoSettings.dimMax * videoSettings.dimPercent;
#endif
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
            imageInfo.imageView = imageTexture.view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, startVert, 0);

            break;

        case 1:
            imageInfo.imageView = screenTextures[0].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, 0, 0);
            break;

        case 2:
#if RETRO_REV02
            startVert = startVertex_2P[0];
#else
            startVert = 6;
#endif
            imageInfo.imageView = screenTextures[0].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, startVert, 0);

#if RETRO_REV02
            startVert = startVertex_2P[1];
#else
            startVert = 12;
#endif
            imageInfo.imageView = screenTextures[1].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, startVert, 0);
            break;

#if RETRO_REV02
        case 3:
            imageInfo.imageView = screenTextures[0].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, startVertex_3P[0], 0);

            imageInfo.imageView = screenTextures[1].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, startVertex_3P[1], 0);

            imageInfo.imageView = screenTextures[2].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, startVertex_3P[2], 0);
            break;

        case 4:
            imageInfo.imageView = screenTextures[0].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, 30, 0);

            imageInfo.imageView = screenTextures[1].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, 36, 0);

            imageInfo.imageView = screenTextures[2].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, 42, 0);

            imageInfo.imageView = screenTextures[3].view;
            vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
            vkCmdDraw(commandBuffer, 6, 1, 48, 0);
            break;
#endif
    }

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to record command buffer");
        return;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]      = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphores;
    submitInfo.pWaitDstStageMask      = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    VkSemaphore signalSemaphores[]  = { renderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to submit to graphics queue");
        return;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(presentQueue, &presentInfo);
}

void RenderDevice::ReleaseShaderPipelines()
{
    vkDeviceWaitIdle(device);

    for (int32 i = 0; i < shaderCount; ++i) {
        vkDestroyPipeline(device, shaderList[i].shaderPipeline, nullptr);
    }

    shaderCount = 0;
#if RETRO_USE_MOD_LOADER
    userShaderCount = 0;
#endif
}

void RenderDevice::Release(bool32 isRefresh)
{
    vkDeviceWaitIdle(device);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    ReleaseShaderPipelines();

    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    
    vkDestroyBuffer(device, uniformBuffer, nullptr);
    vkFreeMemory(device, uniformBufferMemory, nullptr);

    vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, setLayout, nullptr);

    vkDestroySampler(device, samplerPoint, nullptr);
    vkDestroySampler(device, samplerLinear, nullptr);

    for (int32 i = 0; i < SCREEN_COUNT; ++i) {
        vkDestroyImageView(device, screenTextures[i].view, nullptr);
        vkDestroyImage(device, screenTextures[i].image, nullptr);
        vkUnmapMemory(device, screenTextures[i].memory);
        vkFreeMemory(device, screenTextures[i].memory, nullptr);
    }
    vkDestroyImageView(device, imageTexture.view, nullptr);
    vkDestroyImage(device, imageTexture.image, nullptr);
    vkUnmapMemory(device, imageTexture.memory);
    vkFreeMemory(device, imageTexture.memory, nullptr);

    vkDestroyBuffer(device, vertexBuffer, nullptr);
    vkFreeMemory(device, vertexBufferMemory, nullptr);

    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroyFence(device, inFlightFence, nullptr);

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroySurfaceKHR(instance, surface, nullptr);
    glfwDestroyWindow(window);

    if (!isRefresh) {
        if (displayInfo.displays)
            free(displayInfo.displays);
        displayInfo.displays = NULL;

        if (scanlines)
            free(scanlines);
        scanlines = NULL;

        vkDestroyDevice(device, nullptr);

#ifdef VK_DEBUG
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, nullptr);
        }
#endif

        vkDestroyInstance(instance, nullptr);

        glfwTerminate();
    }
}

VkDescriptorBufferInfo bufferInfo{};

bool RenderDevice::InitShaders()
{
    if (descriptorSet == VK_NULL_HANDLE) {
        VkSamplerCreateInfo samplerPointInfo{};
        samplerPointInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerPointInfo.magFilter               = VK_FILTER_NEAREST;
        samplerPointInfo.minFilter               = VK_FILTER_NEAREST;
        samplerPointInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerPointInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerPointInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerPointInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerPointInfo.unnormalizedCoordinates = VK_FALSE;
        samplerPointInfo.anisotropyEnable        = VK_FALSE;
        samplerPointInfo.maxAnisotropy           = 1.0f;
        samplerPointInfo.compareEnable           = VK_FALSE;
        samplerPointInfo.compareOp               = VK_COMPARE_OP_NEVER;
        samplerPointInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerPointInfo.mipLodBias              = 0.0f;
        samplerPointInfo.minLod                  = -FLT_MAX;
        samplerPointInfo.maxLod                  = FLT_MAX;

        VkSamplerCreateInfo samplerLinearInfo = samplerPointInfo;
        samplerLinearInfo.magFilter           = VK_FILTER_LINEAR;
        samplerLinearInfo.minFilter           = VK_FILTER_LINEAR;

        if (vkCreateSampler(device, &samplerPointInfo, nullptr, &samplerPoint) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create point sampler");
            return false;
        }
        if (vkCreateSampler(device, &samplerLinearInfo, nullptr, &samplerLinear) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create linear sampler");
            return false;
        }

        //! CREATE DESCRIPTOR SET
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &setLayout;

        if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to allocate descriptor set");
            return false;
        }

        //! CREATE DESCRIPTOR SET HERE BC I HAVE VULKAN
        bufferInfo        = {};
        bufferInfo.buffer = uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(ShaderConstants);

        // THIS WILL BE CHANGED REGULARLY
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView   = imageTexture.view;
        imageInfo.sampler     = samplerLinear;

        descriptorWrites[0]                 = {};
        descriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet          = descriptorSet;
        descriptorWrites[0].dstBinding      = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo      = &imageInfo;

        descriptorWrites[1]                 = {};
        descriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet          = descriptorSet;
        descriptorWrites[1].dstBinding      = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo     = &bufferInfo;

        vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
    }

    // Release old shader pipelines
    if (shaderCount) {
        ReleaseShaderPipelines();
    }

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
        PrintLog(PRINT_NORMAL, "[VK] No shaders loaded correctly, attempting backup");

        ShaderEntry *shader = &shaderList[0];
        sprintf_s(shader->name, sizeof(shader->name), "%s", "BACKUP");
        videoSettings.shaderSupport = false;

        // let's load
        maxShaders  = 1;
        shaderCount = 1;

        VkShaderModule vertModule, fragModule;
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = sizeof(backupVert);
        createInfo.pCode    = reinterpret_cast<const uint32_t *>(backupVert);

        if (vkCreateShaderModule(device, &createInfo, nullptr, &vertModule) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create vertex module for backup shader");
            return false;
        }

        createInfo.codeSize = sizeof(backupFrag);
        createInfo.pCode    = reinterpret_cast<const uint32_t *>(backupFrag);

        if (vkCreateShaderModule(device, &createInfo, nullptr, &fragModule) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create fragment module for backup shader");
            return false;
        }

        VkPipelineShaderStageCreateInfo shaderStages[] = { {}, {} };

        shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertModule;
        shaderStages[0].pName  = "main";

        shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragModule;
        shaderStages[1].pName  = "main";

        VkGraphicsPipelineCreateInfo pipelineInfo = basePipeline;
        pipelineInfo.stageCount                   = 2;
        pipelineInfo.pStages                      = shaderStages;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader->shaderPipeline) != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create pipeline for backup shader");
            return false;
        }

        vkDestroyShaderModule(device, fragModule, nullptr);
        vkDestroyShaderModule(device, vertModule, nullptr);

        shader->linear = videoSettings.windowed ? false : shader->linear;
    }
    videoSettings.shaderID = MAX(videoSettings.shaderID >= maxShaders ? 0 : videoSettings.shaderID, 0);

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

    VkShaderModule vertModule, fragModule;

    sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Shaders/CSO-Vulkan/None.vert");
    InitFileInfo(&info);
    if (LoadFile(&info, fullFilePath, FMODE_RB)) {
        uint8 *fileData = NULL;
        AllocateStorage((void **)&fileData, info.fileSize + 1, DATASET_TMP, false);
        ReadBytes(&info, fileData, info.fileSize);
        fileData[info.fileSize] = 0;
        CloseFile(&info);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = info.fileSize;
        createInfo.pCode    = reinterpret_cast<const uint32_t *>(fileData);

        VkResult res = vkCreateShaderModule(device, &createInfo, nullptr, &vertModule);
        RemoveStorageEntry((void **)&fileData);

        if (res != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create vertex module for %s", shader->name);
            return;
        }
    }
    else
        return;

    sprintf_s(fullFilePath, sizeof(fullFilePath), "Data/Shaders/CSO-Vulkan/%s.frag", fileName);
    InitFileInfo(&info);
    if (LoadFile(&info, fullFilePath, FMODE_RB)) {
        uint8 *fileData = NULL;
        AllocateStorage((void **)&fileData, info.fileSize + 1, DATASET_TMP, false);
        ReadBytes(&info, fileData, info.fileSize);
        fileData[info.fileSize] = 0;
        CloseFile(&info);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = info.fileSize;
        createInfo.pCode    = reinterpret_cast<const uint32_t *>(fileData);

        VkResult res = vkCreateShaderModule(device, &createInfo, nullptr, &fragModule);
        RemoveStorageEntry((void **)&fileData);

        if (res != VK_SUCCESS) {
            PrintLog(PRINT_NORMAL, "[VK] Failed to create fragment module for %s", shader->name);
            return;
        }
    }
    else
        return;

    VkPipelineShaderStageCreateInfo shaderStages[] = { {}, {} };

    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName  = "main";

    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName  = "main";

    VkGraphicsPipelineCreateInfo pipelineInfo = basePipeline;
    pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount                   = 2;
    pipelineInfo.pStages                      = shaderStages;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader->shaderPipeline) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Failed to create pipeline for shader %s", shader->name);
    }
    else
        shaderCount++;

    vkDestroyShaderModule(device, fragModule, nullptr);
    vkDestroyShaderModule(device, vertModule, nullptr);
};

void RenderDevice::RefreshWindow()
{
    videoSettings.windowState = WINDOWSTATE_UNINITIALIZED;

    Release(true);
    if (!videoSettings.bordered)
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWmonitor *monitor = NULL;
    int32 w, h;

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
        return;
    }
    PrintLog(PRINT_NORMAL, "w: %d h: %d windowed: %d", w, h, videoSettings.windowed);

    glfwSetKeyCallback(window, ProcessKeyEvent);
    glfwSetMouseButtonCallback(window, ProcessMouseEvent);
    glfwSetWindowFocusCallback(window, ProcessFocusEvent);
    glfwSetWindowMaximizeCallback(window, ProcessMaximizeEvent);

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        PrintLog(PRINT_NORMAL, "[VK] Vulkan surface could not be created");
        return;
    }

    descriptorSet = VK_NULL_HANDLE;

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
    uint32 *pixels = (uint32 *)imageTexture.map;
    int32 pitch    = (imageTexture.layout.rowPitch >> 2) - width;

    uint32 *imagePixels32 = (uint32 *)imagePixels;
    for (int32 y = 0; y < height; ++y) {
        for (int32 x = 0; x < width; ++x) {
            *pixels++ = *imagePixels32++;
        }

        pixels += pitch;
    }
}

void RenderDevice::SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    uint32 *pixels = (uint32 *)imageTexture.map;
    int32 pitch    = (imageTexture.layout.rowPitch >> 2) - width;

    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = (yPlane[x] << 16) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }

        pixels = (uint32 *)imageTexture.map;
        pitch  = (imageTexture.layout.rowPitch >> 2) - (width >> 1);
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
}

void RenderDevice::SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    uint32 *pixels = (uint32 *)imageTexture.map;
    int32 pitch    = (imageTexture.layout.rowPitch >> 2) - width;

    if (videoSettings.shaderSupport) {
        for (int32 y = 0; y < height; ++y) {
            for (int32 x = 0; x < width; ++x) {
                *pixels++ = (yPlane[x] << 16) | 0xFF000000;
            }

            pixels += pitch;
            yPlane += strideY;
        }

        pixels = (uint32 *)imageTexture.map;
        pitch  = (imageTexture.layout.rowPitch >> 2) - (width >> 1);
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
}
void RenderDevice::SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                            int32 strideV)
{
    uint32 *pixels = (uint32 *)imageTexture.map;
    int32 pitch    = (imageTexture.layout.rowPitch >> 2) - width;

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

                case GLFW_KEY_F2:
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
