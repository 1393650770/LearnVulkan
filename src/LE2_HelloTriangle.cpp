#include "LE2_HelloTriangle.h"
#include"vulkan/vulkan.h"
#include<stdexcept>
#include <map>
#include <set>
#include<algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <set>

//���ݴ������ķ�������������Ϣ��Ϊʵ������debugMessenger
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

//�ص�����������debugMessenger
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	//�������ָ������Ϣ��������
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	//���������ʾ��Ϣ����
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	//���������ʾ������Ϣ������ϸ��Ϣ�Ľṹ��
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	//pUserData��������һ���ڻص������ڼ�ָ����ָ�룬�������Լ������ݴ��ݸ�����
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void LE2_HelloTriangle::initWindow()
{
	//��ʼ��GLFW��
	glfwInit();
	//����GLFW�ⲻҪ����Opengl������
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//���ô��ڿɵ�����С
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	//����ʵ�ʵĴ���
	window = glfwCreateWindow(WIDTH, HEIGHT, "LearnVulkan", nullptr, nullptr);
	//���ô��ڵ��û�ָ�롣
	glfwSetWindowUserPointer(window, this);
	//����ָ�����ڵ�framebuffer������С�ص�������
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void LE2_HelloTriangle::initVulkan() {
	createInstance();
	setupDebugMessenger();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSyncObjects();
}

void LE2_HelloTriangle::mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		//�¼�ѭ��
		glfwPollEvents();

		//����һ֡ͼ��
		drawFrame();
	}
	vkDeviceWaitIdle(device);
}

void LE2_HelloTriangle::cleanup()
{
	cleanupSwapChain();
	//�����ź�����Fence
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	//���������
	vkDestroyCommandPool(device, commandPool, nullptr);

	//�����߼��豸
	vkDestroyDevice(device, nullptr);
	//��������˵��Ծ�����debugMessenger
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	//���ٴ���Surface
	vkDestroySurfaceKHR(instance, surface, nullptr);
	//����Vulkan��Դ
	vkDestroyInstance(instance, nullptr);
	//���ٴ�����Դ
	glfwDestroyWindow(window);
	//��ֹGLFW��
	glfwTerminate();
}

void LE2_HelloTriangle::cleanupSwapChain()
{
	//����֡������
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	//�������
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	//������߲���
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	//�����Ⱦpass
	vkDestroyRenderPass(device, renderPass, nullptr);

	//����ͼ����ͼ
	for (auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}
	//���ٽ�����
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void LE2_HelloTriangle::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	//�������Ϊ0���᲻��ѭ����ֱ������
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
}

void LE2_HelloTriangle::drawFrame()
{
	//�ȴ���һ֡����
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	
	//�ӽ������л�ȡͼ��
	uint32_t imageIndex;
	//ǰ��������������ϣ�����л�ȡͼ����߼��豸�ͽ�����
	//����������ָ��ͼ����õĳ�ʱʱ�䣨������Ϊ��λ��
	//ʹ�� 64 λ�޷������������ֵ��ζ��������Ч�ؽ����˳�ʱ��
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//VK_ERROR_OUT_OF_DATE_KHR������������治���ݣ�������������Ⱦ��ͨ�������ڴ��ڵ�����С֮��
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		recreateSwapChain();
		return;
	}
	//VK_SUBOPTIMAL_KHR����������Ȼ���������ɹ����ֵ����棬���������Բ�����ȫƥ�䡣
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	//��դ������Ϊ���ź�״̬vkResetFences
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	//���������������֤�ܹ���¼�������
	vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
	
	//��¼������Ҫ������
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//���õȴ����ź���
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	//���õȴ��Ľ׶�
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	//���õ�info��
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	//�����ύ�����������
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	//��������������ִ�к󷢳��źŵ��ź���
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// ����������ύ��ͼ�ζ���
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	//��ʾ��Ϣ
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	//��ʾ��Ϣ�ȴ����ź���
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	//presentInfo.pResults = nullptr; // Optional
	
	//����ʾ��Ϣ�ύ����ʾ����
	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}


	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void LE2_HelloTriangle::createInstance()
{
	//���������֤���������֤�����������򷵻�
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	//����Application��Ϣ
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "LE2_Hello_Triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "NO Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	//Application��Ϣ���õ�VkInstance��
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &app_info;


	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	//����ʵ��
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}

}

std::vector<const char*> LE2_HelloTriangle::getRequiredExtensions()
{
	//�����������֤��չ��
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);


	//��֤��Ҫ����չ�Ƿ�֧��
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		if (!checkExtension(glfwExtensions[i]))
			throw std::runtime_error("failed to create instance : Extensions dont support !");
	}

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}


	return extensions;
}

bool LE2_HelloTriangle::checkExtension(const char* extension_name)
{
	//���ؿ����õ���չ����
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	//����vector�������ȡ����vector��Ϣ
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());


	for (const auto& extension : extensions) {
		if (strcmp(extension_name, extension.extensionName) == 0)
		{
			return true;
		}
	}
	std::cout << "unavailable extensions: "<< extension_name<<"\n";
	return false;
}

bool LE2_HelloTriangle::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	//����SDK�еĲ�����ж�
	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		//�ж����еĿ�����չ�Ƿ�֧��
		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}
	return true;
}

void LE2_HelloTriangle::setupDebugMessenger()
{
	//��������õ�����ֱ�ӷ���
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	//����debugMessenger
	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void LE2_HelloTriangle::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	//createInfo.pUserData = nullptr;
}

void LE2_HelloTriangle::pickPhysicalDevice()
{

	//��ȡ֧�ֵ������豸����
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	}
	//�����GPU�����ȡ�豸��Ϣ
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	//��������GPU���Ƿ�֧��VK
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}
	//�����ϣ������������
	std::multimap<int, VkPhysicalDevice> candidates;

	//����ÿ���豸
	for (const auto& device : devices) {
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	//�����������һ���豸�������
	if (candidates.rbegin()->first > 0) {
		physicalDevice = candidates.rbegin()->second;
		//���û���ҵ����ʵĶ���
		if (!isDeviceSuitable(physicalDevice))
		{
			throw std::runtime_error("failed to find a suitable QueueFamily!");
		}
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
	
}

bool LE2_HelloTriangle::isDeviceSuitable(VkPhysicalDevice device)
{
	//��������
	QueueFamilyIndices indices = findQueueFamilies(device);

	//�����չ֧��
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	//��齻����֧��
	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}


	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

int LE2_HelloTriangle::rateDeviceSuitability(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	//��ȡ�����豸����Ϣ���������ơ����ͺ�֧�ֵ� Vulkan �汾�Ȼ����豸����
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	//��ȡ�����豸��֧�ֹ�����Ϣ����������ѹ����64 λ�������Ͷ��ӿ���Ⱦ���� VR ���ã��ȿ�ѡ���ܵ�֧��
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	//��ɢͼ�δ����������������������ƣ���ȡ�ķ������
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// ���õ������������
	score += deviceProperties.limits.maxImageDimension2D;

	// ����֧�ּ�����ɫ��
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}

QueueFamilyIndices LE2_HelloTriangle::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	//��ȡ�����������
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	
	//���û�п��õĶ������׳��쳣
	if (queueFamilyCount == 0)
	{
		throw std::runtime_error("failed to find any QueueFamily!");
	}

	//��ȡ������Ϣ
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		//��������ܹ��ύͼ���������д洢����ֵ
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		//�������֧�ֻ�ͼ�������д洢����ֵ
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		//����ҵ�һ������ǰ�˳�
		if (indices.isComplete()) {
			break;
		}

		i++;
	}
	//���û���ҵ����ʵĶ���
	if (!indices.isComplete())
	{
		throw std::runtime_error("failed to find a suitable QueueFamily!");
	}
	return indices;
}

void LE2_HelloTriangle::createLogicalDevice()
{
	//�ҵ�֧��ͼ��/��ʾ����Ķ���
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	//����к��ʵ��򴴽�������Ϣ
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };


	//ָ���������ȼ�/����/������
	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	
	VkPhysicalDeviceFeatures deviceFeatures{};

	//�߼��豸������Ϣ
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//��������Ϣ�������豸��Ϣ
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();

	createInfo.pEnabledFeatures = &deviceFeatures;

	//������չ
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	//���������֤��
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//�����߼��豸
	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	//��ȡ�󶨵Ķ��о��
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void LE2_HelloTriangle::createSurface()
{
	//Ϊ��ǰwindow����һ��vk surface
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void LE2_HelloTriangle::createSwapChain()
{
	//������������
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//�趨�������������ͼ������
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 2;
	//���ƽ�������֧�ֵ��������
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	//������������Ϣ��������
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//Ȼ���ȡ����Ӧ�Ķ���
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//������в���ͬ�򣬸���VK_SHARING_MODE_CONCURRENT:ͼ������ڶ����������ʹ�ã�������ʽ����Ȩת�ơ�
	if (indices.graphicsFamily != indices.presentFamily) 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else 
	{
		//������VK_SHARING_MODE_EXCLUSIVE��ͼ��һ����һ��������ӵ�У������ڽ���������һ��������֮ǰ��������ȷת������Ȩ����ѡ���ṩ������ܡ�
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	//�趨Ĭ��ͼ��ת��
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	//ָ����ǰ�����ܹ������������ں�
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	//ָ����ʾģʽ�������������ڵ��޳�
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	//��һ��������
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	//����ո��趨�����ã�����������
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	//��ȡ�������������Ϣ�ľ��
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

}

bool LE2_HelloTriangle::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	//��ȡ�豸��չ��
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	if (extensionCount == 0)
	{
		throw std::runtime_error("failed to find any DeviceExtension!");
	}

	//��ȡ���������չ��Ϣ
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	
	//���֧�ֵ���չ
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails LE2_HelloTriangle::querySwapChainSupport(VkPhysicalDevice device)
{

	SwapChainSupportDetails details;
	//��ѯ�����豸��Ϣ�Ļ���������Ϣ
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	//��ȡ�����ʽ����
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	//��������ʽ���ڣ����Ƶ����ǵĸ�ʽ��
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	//��ȡ��ʾģʽ����
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	//�����ʾģʽ���ڣ����Ƶ����ǵĸ�ʽ��
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR LE2_HelloTriangle::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	//����ѡ��Ӳ���Ѻõ�srgb
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR LE2_HelloTriangle::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	//ģʽ�У���������ģʽ��VK_PRESENT_MODE_IMMEDIATE_KHR
	//����ģʽ��VK_PRESENT_MODE_FIFO_KHR
	//VK_PRESENT_MODE_FIFO_RELAXED_KHR �����ģʽ��ͬ�������Ϊ�ջ���������
	//VK_PRESENT_MODE_MAILBOX_KHR �����ģʽ��ͬ����������ʱ����ʵ��滻�����е�Ԫ��
	return  VkPresentModeKHR::VK_PRESENT_MODE_MAILBOX_KHR;
}

VkExtent2D LE2_HelloTriangle::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilities.currentExtent;
	}
	else 
	{
		int width, height;
		//����windows�ĳ���
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		//Ȼ��ǯ�Ƶ�֧�ֵ������С��Χ
		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void LE2_HelloTriangle::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) 
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		//�趨ͼ�������Լ���ʽ
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		//����ͼ����ɫͨ��
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		//����ͼ����;�Լ�mip����
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		//����ͼ��
		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void LE2_HelloTriangle::createGraphicsPipeline()
{
	//��ȡshader spir-v
	auto vertShaderCode = readFile("D:/Vulkan/Project/LearnVulkan/shader/vert.spv");
	auto fragShaderCode = readFile("D:/Vulkan/Project/LearnVulkan/shader/frag.spv");

	//������ʱshaderģ��
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	//������Ӧ��shader�Ĵ�����Ϣ
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
	
	//�������������Ϣ
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	//�������������Ϣ
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	
	//����ͼԪ���ԣ�����㣬�ߣ���
	//VK_PRIMITIVE_TOPOLOGY_POINT_LIST: ���Զ���ĵ�
	//VK_PRIMITIVE_TOPOLOGY_LINE_LIST��ÿ2��������ߣ����ظ�ʹ��
	//VK_PRIMITIVE_TOPOLOGY_LINE_STRIP��ÿ�еĽ�������������һ�еĿ�ʼ����
	//VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : ����ÿ 3 ������������Σ����ظ�ʹ��
	//VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP��ÿ�������εĵڶ����͵���������������һ�������ε�ǰ��������
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//�����ӿ�����
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//���òü�����
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	//�ӿڴ�����Ϣ����������ӿ����ԺͲü�����
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//��դ������������
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	//���ǯ�ƣ�����ѽ�������ƽ���Զƽ���Ƭ�μ���������
	rasterizer.depthClampEnable = VK_FALSE;
	//��ȥ����ƬԪ����
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	//���ü���������״̬��
	//VK_POLYGON_MODE_FILL����Ƭ�������������
	//VK_POLYGON_MODE_LINE: ����α߱�������
	//VK_POLYGON_MODE_POINT������ζ��㱻����Ϊ��
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//����������ϸ
	rasterizer.lineWidth = 1.0f;
	//���޳����ͣ������޳������߱����޳�
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//��������ĵ������˳��˳ʱ�������ʱ��
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	//���ֵ��������
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//���ز������ã��������û�����Ӻ���Ӧ�Ĺ�ʽ
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//֡��������ɫ�������
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	//ȫ����ɫ�������
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	//�Ƿ�ʹ�ð�λ���	
	colorBlending.logicOpEnable = VK_FALSE;
	//���ð�λ��ϲ���
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//���ùܵ����֣�������������uniform
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	//�������߲���
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	//���ù�����Ϣ
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	//���ù�����ɫ��״̬
	pipelineInfo.pStages = shaderStages;
	//���ù��߶�������
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	//���ù����������
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	//���ù����ӿ�״̬
	pipelineInfo.pViewportState = &viewportState;
	//���ù��߹�դ����
	pipelineInfo.pRasterizationState = &rasterizer;
	//���ù��߶��ز���״̬
	pipelineInfo.pMultisampleState = &multisampling;
	//���ù�����ɫ���״̬
	pipelineInfo.pColorBlendState = &colorBlending;
	//���ù��ߵĲ���
	pipelineInfo.layout = pipelineLayout;
	//���ù��ߵ���Ⱦpass
	pipelineInfo.renderPass = renderPass;
	//���ù��ߵ���pass
	pipelineInfo.subpass = 0;
	//��������������Ҫ�Ǵ����й����������µĹ���
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1; // Optional

	//��������
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}


	//�����ʱshaderģ��
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule LE2_HelloTriangle::createShaderModule(const std::vector<char>& code)
{
	//������ɫ��ģ����Ϣ
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//�������ƴ��븳ֵ��ȥ
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	//����shaderModule
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void LE2_HelloTriangle::createRenderPass()
{
	//������ɫcolorAttachment�������Ϣ
	VkAttachmentDescription colorAttachment{};
	//formatӦ���뽻����ͼ��ĸ�ʽƥ��
	colorAttachment.format = swapChainImageFormat;
	//����������
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	//��������
	//VK_ATTACHMENT_LOAD_OP_LOAD��������������������
	//VK_ATTACHMENT_LOAD_OP_CLEAR���ڿ�ʼʱ��ֵ���Ϊ����
	//VK_ATTACHMENT_LOAD_OP_DONT_CARE����������δ���壻���ǲ��ں�����
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	//���ô洢��ʽ
	//VK_ATTACHMENT_STORE_OP_STORE: ��Ⱦ�����ݽ��洢���ڴ��У��Ժ���Զ�ȡ
	//VK_ATTACHMENT_STORE_OP_DONT_CARE: ��Ⱦ������֡�����������ݽ���ȷ��
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//ģ�建����������
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//��������
	//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL��������ɫ������ͼ��
	//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR��Ҫ�ڽ������г��ֵ�ͼ��
	//VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL�������ڴ渴�Ʋ���Ŀ���ͼ��
	//initialLayoutָ������Ⱦ���̿�ʼ֮ǰͼ�񽫾��еĲ���
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//finalLayoutָ��Ҫ����Ⱦ�������ʱ�Զ�ת�����Ĳ���
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//colorAttachment����
	VkAttachmentReference colorAttachmentRef{};
	//attachmentͨ���������������е�����ָ��Ҫ���õĸ���,������ֱ��ʹ��layout(location = 0) 
	colorAttachmentRef.attachment = 0;

	//layoutָ������ϣ��������ʹ�ô����õ���ͨ���ڼ���еĲ��֡�
	//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL����˼�壬���Ǵ��㽫����������ɫ�����������ֽ�Ϊ�����ṩ������ܡ�
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//��pass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	//renderpass��Ϣ����
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	//�Ѹոյ�colorAttachment����pass��Ϣ������
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;


	VkSubpassDependency dependency{};
	//ǰ�����ֶ�ָ��������������ͨ��������������ֵVK_SUBPASS_EXTERNAL��ָ��Ⱦͨ��֮ǰ��֮�����ʽ��ͨ��
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	//�������ֶ�ָ��Ҫ�ȴ��Ĳ����Լ���Щ���������Ľ׶Ρ�������Ҫ�ȴ���������ɶ�ͼ��Ķ�ȡ��Ȼ����ܷ�������
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	//Ӧ�õȴ��Ĳ���������ɫ�����׶Σ��漰����ɫ������д�롣��Щ���ý���ֹ���ɷ�����ֱ����������Ҫ��������������������Ҫ��ʼ����д����ɫʱ��
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//ָ��һ���������顣
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	//����pass
	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create render pass!");
	}
}

void LE2_HelloTriangle::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) 
	{
		//�ѽ������е�ͼ��ֵ��֡������
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};
		//֡������info
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		//���ð�attachment
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		//���ý�����ͼ�����
		framebufferInfo.layers = 1;
		//����֡������
		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void LE2_HelloTriangle::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

	//��������ر�־
	//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT����ʾ����������������������¼�¼�����ܻ�ı��ڴ������Ϊ��
	//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT�����������¼�¼������������û�д˱�־�����Ƕ�����һ������
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	//���������
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void LE2_HelloTriangle::createCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	//�����������info
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;

	//��������ļ�����������������Ǹ������������
	//VK_COMMAND_BUFFER_LEVEL_PRIMARY�������ύ������ִ�У������ܴ���������������á�
	//VK_COMMAND_BUFFER_LEVEL_SECONDARY������ֱ���ύ�������Դ�������������á�
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	//�����������
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	//�����������
	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void LE2_HelloTriangle::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	//VkCommandBufferBeginInfo��ʼ��¼����������ýṹָ���йش��ض��������ʹ�õ�һЩϸ�ڡ�
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	//flags����ָ�����ǽ����ʹ�����������
	//VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT�������������ִ��һ�κ��������¼�¼��
	//VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT������һ�������������������ȫ�ڵ�����Ⱦ�����С�
	//VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT�����������������Ҳ�Ѿ��ȴ�ִ��ʱ�����ύ��
	beginInfo.flags = 0; // Optional

	//pInheritanceInfo�������븨�����������ء���ָ���ӵ�������������̳е�״̬��
	beginInfo.pInheritanceInfo = nullptr; // Optional

	//����һ��beginInfo
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	//��ȾPassBeginInfo����
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	//������Ⱦpass
	renderPassInfo.renderPass = renderPass;
	//����ʹ�õ�֡��������ΪҪ���ƵĽ�����ͼ���֡������
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	//��Ⱦ����ƫ�����С
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;

	//������ɫ���ʱ���ֵ
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	//���һ��������ֵ�������ʹ����Ⱦ�����еĻ�ͼ����
	//VK_SUBPASS_CONTENTS_INLINE����Ⱦͨ�����Ƕ�뵽��������������У����Ҳ���ִ�и������������
	//VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS����Ⱦͨ������Ӹ����������ִ�С�
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//Ϊ�����������Ⱦ���ߣ��ڶ�������ָ���ܵ�������ͼ�λ��Ǽ���ܵ���
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	//������Ⱦ������λ���������,�����ĸ���������
	//vertexCount����ʹ����û�ж��㻺���������Ӽ����Ͻ���������Ȼ��Ҫ���� 3 �����㡣
	//instanceCount������ʵ����Ⱦ��1����㲻����������ʹ�á�
	//firstVertex : �������㻺������ƫ���������� ����Сֵgl_VertexIndex��
	//firstInstance������ʵ����Ⱦ��ƫ���������� ����Сֵgl_InstanceIndex��
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	//������Ⱦ���̡���Ϊ�Ѿ��������������ļ�¼
	vkCmdEndRenderPass(commandBuffer);
	//������¼�����������
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void LE2_HelloTriangle::createSyncObjects()
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//�ź�״̬�´���դ�����Ա��һ�ε��� vkWaitForFences()�������أ���Ϊդ���Ѿ������źš�
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	//�����ź�����Fence
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) 
	{
		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

}

void LE2_HelloTriangle::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}
