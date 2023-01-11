// TODO: VULKAN_USE_GLFW and other stuff
// i'm not gonna worry about those yet

class RenderDevice : public RenderDeviceBase
{

public:
    struct WindowInfo {
        union {
            struct {
                int32 width;
                int32 height;
                int32 _pad[3];
                int32 refresh_rate;
            };
            GLFWvidmode internal;
        } * displays;
    };

    struct SwapChainDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    static WindowInfo displayInfo;

    static bool Init();
    static void CopyFrameBuffer();
    static void FlipScreen();
    static void Release(bool32 isRefresh);

    static void RefreshWindow();
    static void GetWindowSize(int32 *width, int32 *height);

    static void SetupImageTexture(int32 width, int32 height, uint8 *imagePixels);
    static void SetupVideoTexture_YUV420(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);
    static void SetupVideoTexture_YUV422(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);
    static void SetupVideoTexture_YUV444(int32 width, int32 height, uint8 *yPlane, uint8 *uPlane, uint8 *vPlane, int32 strideY, int32 strideU,
                                         int32 strideV);

    static bool ProcessEvents();

    static void InitFPSCap();
    static bool CheckFPSCap();
    static void UpdateFPSCap();

    static bool InitShaders();
    static void LoadShader(const char *fileName, bool32 linear);

    static inline void ShowCursor(bool32 shown) { glfwSetInputMode(window, GLFW_CURSOR, shown ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN); }
    static inline bool GetCursorPos(Vector2 *pos)
    {
        double cursorX, cursorY;
        glfwGetCursorPos(window, &cursorX, &cursorY);
        pos->x = (int32)cursorX;
        pos->y = (int32)cursorY;
        return true;
    };
    inline static void SetWindowTitle() { glfwSetWindowTitle(window, gameVerInfo.gameTitle); };

    static GLFWwindow *window;

private:
    static bool SetupRendering();
    static bool InitVertexBuffer();
    static bool InitGraphicsAPI();

    static void GetDisplays();

    static void ProcessKeyEvent(GLFWwindow *, int32 key, int32 scancode, int32 action, int32 mods);
    static void ProcessFocusEvent(GLFWwindow *, int32 focused);
    static void ProcessMouseEvent(GLFWwindow *, int32 button, int32 action, int32 mods);
    static void ProcessJoystickEvent(int32 ID, int32 event);
    static void ProcessMaximizeEvent(GLFWwindow *, int32 maximized);

    static void SetLinear(bool32 linear);

#if VK_DEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

    static VkDebugUtilsMessengerEXT debugMessenger;
#endif

    static bool CheckExtensionSupport(VkPhysicalDevice device);

    static SwapChainDetails QuerySwapChainDetails(VkPhysicalDevice device);

    static bool IsDeviceSuitable(VkPhysicalDevice device)
    {
        if (!CheckExtensionSupport(device))
            return false;

        QuerySwapChainDetails(device);
        if (currentSwapDetails.formats.empty() || currentSwapDetails.presentModes.empty())
            return false;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        graphicsIndex = 0xFFFFFFFF;
        presentIndex  = 0xFFFFFFFF;

        int i = 0;
        for (const auto &queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                graphicsIndex = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
                presentIndex = i;

            if (graphicsIndex != 0xFFFFFFFF && presentIndex != 0xFFFFFFFF)
                return true;

            ++i;
        }

        return false;
    };

    static bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    static void ReleaseShaderPipelines();

    static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        return (uint32_t)-1;
    };

    static VkInstance instance;
    static VkPhysicalDevice physicalDevice;
    static VkDevice device;
    static VkSurfaceKHR surface;

    static SwapChainDetails currentSwapDetails;
    static VkSwapchainKHR swapChain;
    static VkFormat swapChainImageFormat;
    static VkExtent2D swapChainExtent;

    static std::vector<VkImage> swapChainImages;
    static std::vector<VkImageView> swapChainImageViews;
    static std::vector<VkFramebuffer> swapChainFramebuffers;

    struct ShaderConstants;

    static VkBuffer uniformBuffer;
    static VkDeviceMemory uniformBufferMemory;
    static ShaderConstants *uniformMap;

    static VkDescriptorPool descriptorPool;
    static VkDescriptorSet descriptorSet;

    static VkRenderPass renderPass;
    static VkDescriptorSetLayout setLayout;
    static VkPipelineLayout pipelineLayout;
    static VkGraphicsPipelineCreateInfo basePipeline;

    static VkBuffer vertexBuffer;
    static VkDeviceMemory vertexBufferMemory;

    static VkCommandPool commandPool;
    static VkCommandBuffer commandBuffer;

    static VkQueue graphicsQueue;
    static VkQueue presentQueue;
    static uint32 graphicsIndex;
    static uint32 presentIndex;

    static VkViewport viewport;

    static VkSemaphore imageAvailableSemaphore;
    static VkSemaphore renderFinishedSemaphore;
    static VkFence inFlightFence;

    static int32 monitorIndex;

    struct Texture {
        VkImage image;
        VkDeviceMemory memory;
        VkSubresourceLayout layout;
        VkImageView view;
        void* map;
    };

    static VkSampler samplerPoint;
    static VkSampler samplerLinear;

    static VkDescriptorImageInfo imageInfo;

    static Texture imageTexture;
    static Texture screenTextures[SCREEN_COUNT];

    static double lastFrame;
    static double targetFreq;
};

struct ShaderEntry : public ShaderEntryBase {
    VkPipeline shaderPipeline;
};
