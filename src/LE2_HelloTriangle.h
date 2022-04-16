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

//�Զ�����ķ�װ
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

//�Խ�������ϸ�ڷ�װ
//��������
//�������湦�ܣ���������ͼ�����С / ���������ͼ�����С / ����Ⱥ͸߶ȣ�
//�����ʽ�����ظ�ʽ��ɫ�ʿռ䣩
//���õ���ʾģʽ
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

    //VK�߼��豸���
    VkDevice device;

    //ͼ�ζ��У�������л�ͬ�豸һ�����٣�����Ҫ�ֶ�����
    VkQueue graphicsQueue;
    //��ʾ����
    VkQueue presentQueue;
    //VK�����豸���
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    //�������Լ������������Ϣ
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    //��������ͼ����ͼ
    std::vector<VkImageView> swapChainImageViews;

    //��Ⱦpass
    VkRenderPass renderPass;
    //�ܵ�����
    VkPipelineLayout pipelineLayout;
    //ͼ�ι���
    VkPipeline graphicsPipeline;
    //ͼ�νӿ�
    VkSurfaceKHR surface;
    //֡������
    std::vector<VkFramebuffer> swapChainFramebuffers;

    //����أ��������ڴ洢���������ڴ棬�����з����������
    VkCommandPool commandPool;
    //�������
    std::vector<VkCommandBuffer> commandBuffers;

    //�ź�������
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    //Fence����
    std::vector<VkFence> inFlightFences;

    //��ǰ��֡��������ʹ�����ػ���
    uint32_t currentFrame = 0;
    //�ؽ�framebuffer
    bool framebufferResized = false;
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    void drawFrame();

    //����Vulkanʵ��
    void createInstance();
    //��ȡ�������չ
    std::vector<const char*> getRequiredExtensions();
    //�����չ�Ƿ�֧��
    bool checkExtension(const char* extension_name);
    //�����֤��֧��
    bool checkValidationLayerSupport();
    //����Debug�ַ���
    void setupDebugMessenger();
    //�趨DebugMessenger�������������Ϣ
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    //ѡȡ�����豸
    void pickPhysicalDevice();
    //�ж������豸�Ƿ��ʺ�
    bool isDeviceSuitable(VkPhysicalDevice device);
    //��ÿ���豸������������
    int rateDeviceSuitability(VkPhysicalDevice device);
    //Ѱ��֧��ͼ������Ķ���
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    //�����߼��豸������
    void createLogicalDevice();
    //��������
    void createSurface();

    //����������
    void createSwapChain();

    //���Vk�豸��չ֧��
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    //��ѯ������֧������
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    //ѡ�񽻻����ı����ʽ
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    //ѡ����ʾģʽ
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    //ѡ�񽻻���Χ
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    //����ͼ����ͼ
    void createImageViews();
    //������Ⱦ����
    void createGraphicsPipeline();
    //������ɫ��ģ��
    VkShaderModule createShaderModule(const std::vector<char>& code);
    //������Ⱦpass
    void createRenderPass();
    //����֡������
    void createFramebuffers();

    //���������
    void createCommandPool();
    //�����������
    void createCommandBuffers();
    //��¼���д���������
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    //����ͬ������
    void createSyncObjects();

    //���������
    void cleanupSwapChain();
    //���´���������
    void recreateSwapChain();

    //�ؽ�framebufferʱ����õĻص�
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


