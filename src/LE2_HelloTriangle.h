#pragma once
#ifndef _LE2_HelloTriangle_
#define _LE2_HelloTriangle_
#include<iostream>
#include<vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <optional>
#include <fstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 3;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

//对队列族的封装
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

//对交换链的细节封装
//三种属性
//基本表面功能（交换链中图像的最小 / 最大数量，图像的最小 / 最大宽度和高度）
//表面格式（像素格式、色彩空间）
//可用的演示模式
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


class LE2_HelloTriangle
{
private:
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    //VK逻辑设备句柄
    VkDevice device;

    //图形队列，这个队列会同设备一起被销毁，不需要手动销毁
    VkQueue graphicsQueue;
    //演示队列
    VkQueue presentQueue;
    //VK物理设备句柄
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    //交换链以及交换链相关信息
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    //交换链的图像视图
    std::vector<VkImageView> swapChainImageViews;

    //渲染pass
    VkRenderPass renderPass;
    //管道布局
    VkPipelineLayout pipelineLayout;
    //图形管线
    VkPipeline graphicsPipeline;
    //图形接口
    VkSurfaceKHR surface;
    //帧缓冲区
    std::vector<VkFramebuffer> swapChainFramebuffers;

    //命令池，管理用于存储缓冲区的内存，并从中分配命令缓冲区
    VkCommandPool commandPool;
    //命令缓冲区
    std::vector<VkCommandBuffer> commandBuffers;

    //信号量对象
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    //Fence对象
    std::vector<VkFence> inFlightFences;

    //当前的帧数。我们使用三重缓冲
    uint32_t currentFrame = 0;
    //重建framebuffer
    bool framebufferResized = false;
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    void drawFrame();

    //创建Vulkan实例
    void createInstance();
    //获取所需的拓展
    std::vector<const char*> getRequiredExtensions();
    //检查拓展是否支持
    bool checkExtension(const char* extension_name);
    //检查验证层支持
    bool checkValidationLayerSupport();
    //创建Debug分发器
    void setupDebugMessenger();
    //设定DebugMessenger创建器的相关信息
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    //选取物理设备
    void pickPhysicalDevice();
    //判断物理设备是否适合
    bool isDeviceSuitable(VkPhysicalDevice device);
    //对每个设备进行评估分数
    int rateDeviceSuitability(VkPhysicalDevice device);
    //寻找支持图形命令的队列
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    //创建逻辑设备来交互
    void createLogicalDevice();
    //创建窗口
    void createSurface();

    //创建交换链
    void createSwapChain();

    //检查Vk设备拓展支持
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    //查询交换链支持详情
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    //选择交换链的表面格式
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    //选择演示模式
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    //选择交换范围
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    //创建图像视图
    void createImageViews();
    //创建渲染管线
    void createGraphicsPipeline();
    //创建着色器模块
    VkShaderModule createShaderModule(const std::vector<char>& code);
    //创建渲染pass
    void createRenderPass();
    //创建帧缓冲区
    void createFramebuffers();

    //创建命令池
    void createCommandPool();
    //创建命令缓冲区
    void createCommandBuffers();
    //记录命令，写入命令缓冲区
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    //创建同步对象
    void createSyncObjects();

    //清除交换链
    void cleanupSwapChain();
    //重新创建交换链
    void recreateSwapChain();

    //重建framebuffer时候调用的回调
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<LE2_HelloTriangle*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

public:
    void run();
};

#endif //_LE2_HelloTriangle_

static std::vector<char> readFile(const std::string& filename) 
{

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) 
    {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}


