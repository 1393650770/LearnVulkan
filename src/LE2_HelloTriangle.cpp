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

//根据传进来的分配器、创建信息等为实例创建debugMessenger
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

//回调函数，销毁debugMessenger
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	//这个参数指定了信息的严重性
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	//这个参数表示消息类型
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	//这个参数表示包含消息本身详细信息的结构体
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	//pUserData参数包含一个在回调设置期间指定的指针，并允许自己的数据传递给它。
	void* pUserData) {

	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void LE2_HelloTriangle::initWindow()
{
	//初始化GLFW库
	glfwInit();
	//告诉GLFW库不要创建Opengl上下文
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//禁用窗口可调整大小
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	//创建实际的窗口
	window = glfwCreateWindow(WIDTH, HEIGHT, "LearnVulkan", nullptr, nullptr);
	//设置窗口的用户指针。
	glfwSetWindowUserPointer(window, this);
	//设置指定窗口的framebuffer调整大小回调函数。
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
		//事件循环
		glfwPollEvents();

		//绘制一帧图像
		drawFrame();
	}
	vkDeviceWaitIdle(device);
}

void LE2_HelloTriangle::cleanup()
{
	cleanupSwapChain();
	//销毁信号量和Fence
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(device, inFlightFences[i], nullptr);
	}
	//销毁命令池
	vkDestroyCommandPool(device, commandPool, nullptr);

	//销毁逻辑设备
	vkDestroyDevice(device, nullptr);
	//如果开启了调试就销毁debugMessenger
	if (enableValidationLayers) {
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	}
	//销毁窗口Surface
	vkDestroySurfaceKHR(instance, surface, nullptr);
	//销毁Vulkan资源
	vkDestroyInstance(instance, nullptr);
	//销毁窗口资源
	glfwDestroyWindow(window);
	//终止GLFW库
	glfwTerminate();
}

void LE2_HelloTriangle::cleanupSwapChain()
{
	//销毁帧缓冲区
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
	//清除管线
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	//清除管线布局
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	//清楚渲染pass
	vkDestroyRenderPass(device, renderPass, nullptr);

	//销毁图像视图
	for (auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}
	//销毁交换链
	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void LE2_HelloTriangle::recreateSwapChain()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	//如果长宽为0，会不断循环，直到拉大
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
	//等待上一帧结束
	vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
	
	//从交换链中获取图像
	uint32_t imageIndex;
	//前两个参数是我们希望从中获取图像的逻辑设备和交换链
	//第三个参数指定图像可用的超时时间（以纳秒为单位）
	//使用 64 位无符号整数的最大值意味着我们有效地禁用了超时。
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	//VK_ERROR_OUT_OF_DATE_KHR：交换链与表面不兼容，不能再用于渲染。通常发生在窗口调整大小之后。
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		recreateSwapChain();
		return;
	}
	//VK_SUBOPTIMAL_KHR：交换链仍然可以用来成功呈现到表面，但表面属性不再完全匹配。
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	//将栅栏重置为无信号状态vkResetFences
	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	//重置命令缓冲区，保证能够记录命令缓冲区
	vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
	
	//记录我们想要的命令
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//设置等待的信号量
	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	//设置等待的阶段
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	//设置到info中
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	//设置提交到的命令缓冲区
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

	//设置命令缓冲区完成执行后发出信号的信号量
	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	// 将命令缓冲区提交到图形队列
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	//显示信息
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	//显示信息等待的信号量
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	//presentInfo.pResults = nullptr; // Optional
	
	//将显示信息提交到显示队列
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
	//如果启用验证，则进行验证，发生错误则返回
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	//设置Application信息
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "LE2_Hello_Triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "NO Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	//Application信息设置到VkInstance上
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

	//创建实例
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}

}

std::vector<const char*> LE2_HelloTriangle::getRequiredExtensions()
{
	//返回所需的验证拓展数
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);


	//验证需要的拓展是否支持
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
	//返回可以用的拓展综述
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	//创建vector来保存获取到的vector信息
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
	//根据SDK中的层进行判断
	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		//判断所有的可用扩展是否支持
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
	//如果不启用调试则直接返回
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	//创建debugMessenger
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

	//获取支持的物理设备数量
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	}
	//如果有GPU，则获取设备信息
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	//遍历所有GPU看是否支持VK
	for (const auto& device : devices) {
		if (isDeviceSuitable(device)) {
			physicalDevice = device;
			break;
		}
	}
	//排序哈希表，递增的序列
	std::multimap<int, VkPhysicalDevice> candidates;

	//评估每个设备
	for (const auto& device : devices) {
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	//递增序列最后一个设备分数最高
	if (candidates.rbegin()->first > 0) {
		physicalDevice = candidates.rbegin()->second;
		//如果没有找到合适的队列
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
	//检查队列族
	QueueFamilyIndices indices = findQueueFamilies(device);

	//检查拓展支持
	bool extensionsSupported = checkDeviceExtensionSupport(device);

	//检查交换链支持
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
	//获取物理设备的信息，比如名称、类型和支持的 Vulkan 版本等基本设备属性
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	//获取物理设备的支持功能信息，比如纹理压缩、64 位浮点数和多视口渲染（对 VR 有用）等可选功能的支持
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	//离散图形处理器具有显著的性能优势，获取的分数最高
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// 可用的最大纹理数量
	score += deviceProperties.limits.maxImageDimension2D;

	// 必须支持几何着色器
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}

QueueFamilyIndices LE2_HelloTriangle::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	//获取队列族的数量
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	
	//如果没有可用的队列则抛出异常
	if (queueFamilyCount == 0)
	{
		throw std::runtime_error("failed to find any QueueFamily!");
	}

	//获取队列信息
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		//如果队列能够提交图形命令，则进行存储返回值
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}
		//如果队列支持绘图命令，则进行存储返回值
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		//如果找到一个则提前退出
		if (indices.isComplete()) {
			break;
		}

		i++;
	}
	//如果没有找到合适的队列
	if (!indices.isComplete())
	{
		throw std::runtime_error("failed to find a suitable QueueFamily!");
	}
	return indices;
}

void LE2_HelloTriangle::createLogicalDevice()
{
	//找到支持图形/演示命令的队列
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

	//如果有合适的则创建队列信息
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };


	//指定队列优先级/类型/队列族
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

	//逻辑设备创建信息
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	//填充队列信息和物理设备信息
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = queueCreateInfos.size();

	createInfo.pEnabledFeatures = &deviceFeatures;

	//启用拓展
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	//如果启用验证层
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	//创建逻辑设备
	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}
	//获取绑定的队列句柄
	vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void LE2_HelloTriangle::createSurface()
{
	//为当前window创建一个vk surface
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void LE2_HelloTriangle::createSwapChain()
{
	//创建交换链接
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	//设定交换链中允许的图像数量
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 2;
	//控制交换链中支持的最大数量
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	//创建交换链信息，并复制
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	//然后获取到对应的队列
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//如果队列不相同则，赋予VK_SHARING_MODE_CONCURRENT:图像可以在多个队列族中使用，无需显式所有权转移。
	if (indices.graphicsFamily != indices.presentFamily) 
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else 
	{
		//否则赋予VK_SHARING_MODE_EXCLUSIVE：图像一次由一个队列族拥有，并且在将其用于另一个队列族之前，必须明确转移所有权。此选项提供最佳性能。
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	//设定默认图像转换
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	//指定当前窗口能够与其他窗口融合
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	//指定演示模式，并开启像素遮挡剔除
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	//上一个交换链
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	//传入刚刚设定的设置，创建交换链
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	//获取交换链中相关信息的句柄
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

}

bool LE2_HelloTriangle::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	//获取设备拓展数
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	if (extensionCount == 0)
	{
		throw std::runtime_error("failed to find any DeviceExtension!");
	}

	//获取到具体的拓展信息
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	
	//检查支持的拓展
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails LE2_HelloTriangle::querySwapChainSupport(VkPhysicalDevice device)
{

	SwapChainSupportDetails details;
	//查询物理设备信息的基本表面信息
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	//获取表面格式数量
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	//如果表面格式存在，则复制到我们的格式中
	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	//获取演示模式数量
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	//如果演示模式存在，则复制到我们的格式中
	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR LE2_HelloTriangle::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	//我们选择硬件友好的srgb
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}
	return availableFormats[0];
}

VkPresentModeKHR LE2_HelloTriangle::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	//模式有：立即交换模式：VK_PRESENT_MODE_IMMEDIATE_KHR
	//队列模式：VK_PRESENT_MODE_FIFO_KHR
	//VK_PRESENT_MODE_FIFO_RELAXED_KHR 与队列模式不同如果队列为空会立即传输
	//VK_PRESENT_MODE_MAILBOX_KHR 与队列模式不同，队列满的时候会适当替换队列中的元素
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
		//返回windows的长宽
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		//然后钳制到支持的最大最小范围
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
		//设定图像类型以及格式
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		//调整图像颜色通道
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		//设置图像用途以及mip属性
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		//创建图像
		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void LE2_HelloTriangle::createGraphicsPipeline()
{
	//读取shader spir-v
	auto vertShaderCode = readFile("D:/Vulkan/Project/LearnVulkan/shader/vert.spv");
	auto fragShaderCode = readFile("D:/Vulkan/Project/LearnVulkan/shader/frag.spv");

	//创建临时shader模块
	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	//创建对应的shader的创建信息
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
	
	//顶点输出创建信息
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	//输入组件创建信息
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	
	//描述图元属性，比如点，线，面
	//VK_PRIMITIVE_TOPOLOGY_POINT_LIST: 来自顶点的点
	//VK_PRIMITIVE_TOPOLOGY_LINE_LIST：每2个顶点的线，不重复使用
	//VK_PRIMITIVE_TOPOLOGY_LINE_STRIP：每行的结束顶点用作下一行的开始顶点
	//VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : 来自每 3 个顶点的三角形，不重复使用
	//VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP：每个三角形的第二个和第三个顶点用作下一个三角形的前两个顶点
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	//设置视口属性
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	//设置裁剪属性
	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	//视口创建信息，负责组合视口属性和裁剪属性
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	//光栅化器创建属性
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	//深度钳制，负责把将超出近平面和远平面的片段夹在它们上
	rasterizer.depthClampEnable = VK_FALSE;
	//丢去所有片元属性
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	//设置几何体生成状态。
	//VK_POLYGON_MODE_FILL：用片段填充多边形区域；
	//VK_POLYGON_MODE_LINE: 多边形边被画成线
	//VK_POLYGON_MODE_POINT：多边形顶点被绘制为点
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	//设置线条粗细
	rasterizer.lineWidth = 1.0f;
	//面剔除类型，正面剔除，或者背面剔除
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//设置正面的点的排列顺序，顺时针或者逆时针
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	//深度值便宜设置
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	//多重采样设置，可以设置混合因子和相应的公式
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//帧缓冲区颜色混合设置
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	//全局颜色混合设置
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	//是否使用按位混合	
	colorBlending.logicOpEnable = VK_FALSE;
	//设置按位混合操作
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	//设置管道布局，可以用来设置uniform
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	//创建管线布局
	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	//设置管线信息
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	//设置管线着色器状态
	pipelineInfo.pStages = shaderStages;
	//设置管线顶点输入
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	//设置管线输入组件
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	//设置管线视口状态
	pipelineInfo.pViewportState = &viewportState;
	//设置管线光栅化器
	pipelineInfo.pRasterizationState = &rasterizer;
	//设置管线多重采样状态
	pipelineInfo.pMultisampleState = &multisampling;
	//设置管线颜色混合状态
	pipelineInfo.pColorBlendState = &colorBlending;
	//设置管线的布局
	pipelineInfo.layout = pipelineLayout;
	//设置管线的渲染pass
	pipelineInfo.renderPass = renderPass;
	//设置管线的子pass
	pipelineInfo.subpass = 0;
	//以下两个参数主要是从现有管线来派生新的管线
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1; // Optional

	//创建管线
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}


	//清除临时shader模块
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

VkShaderModule LE2_HelloTriangle::createShaderModule(const std::vector<char>& code)
{
	//创建着色器模块信息
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	//将二进制代码赋值上去
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	//创建shaderModule
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void LE2_HelloTriangle::createRenderPass()
{
	//设置颜色colorAttachment的相关信息
	VkAttachmentDescription colorAttachment{};
	//format应该与交换链图像的格式匹配
	colorAttachment.format = swapChainImageFormat;
	//采样样本数
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	//加载设置
	//VK_ATTACHMENT_LOAD_OP_LOAD：保留附件的现有内容
	//VK_ATTACHMENT_LOAD_OP_CLEAR：在开始时将值清除为常量
	//VK_ATTACHMENT_LOAD_OP_DONT_CARE：现有内容未定义；我们不在乎他们
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

	//设置存储格式
	//VK_ATTACHMENT_STORE_OP_STORE: 渲染的内容将存储在内存中，以后可以读取
	//VK_ATTACHMENT_STORE_OP_DONT_CARE: 渲染操作后帧缓冲区的内容将不确定
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//模板缓冲区的设置
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//布局设置
	//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL：用作颜色附件的图像
	//VK_IMAGE_LAYOUT_PRESENT_SRC_KHR：要在交换链中呈现的图像
	//VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL：用作内存复制操作目标的图像
	//initialLayout指定在渲染过程开始之前图像将具有的布局
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//finalLayout指定要在渲染过程完成时自动转换到的布局
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//colorAttachment引用
	VkAttachmentReference colorAttachmentRef{};
	//attachment通过附件描述数组中的索引指定要引用的附件,该索引直接使用layout(location = 0) 
	colorAttachmentRef.attachment = 0;

	//layout指定我们希望附件在使用此引用的子通道期间具有的布局。
	//VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL顾名思义，我们打算将附件用作颜色缓冲区，布局将为我们提供最佳性能。
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//子pass
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	//renderpass信息设置
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	//把刚刚的colorAttachment和子pass信息附加上
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;


	VkSubpassDependency dependency{};
	//前两个字段指定依赖和依赖子通道的索引。特殊值VK_SUBPASS_EXTERNAL是指渲染通道之前或之后的隐式子通道
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	//这两个字段指定要等待的操作以及这些操作发生的阶段。我们需要等待交换链完成对图像的读取，然后才能访问它。
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	//应该等待的操作是在颜色附件阶段，涉及到颜色附件的写入。这些设置将阻止过渡发生，直到它真正需要（并且允许）：当我们想要开始向它写入颜色时。
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	//指定一个依赖数组。
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	//创建pass
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
		//把交换链中的图像赋值给帧缓冲区
		VkImageView attachments[] = {
			swapChainImageViews[i]
		};
		//帧缓冲区info
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		//设置绑定attachment
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		//设置交换区图像层数
		framebufferInfo.layers = 1;
		//创建帧缓冲区
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

	//设置命令池标志
	//VK_COMMAND_POOL_CREATE_TRANSIENT_BIT：提示命令缓冲区经常用新命令重新记录（可能会改变内存分配行为）
	//VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT：允许单独重新记录命令缓冲区，如果没有此标志，它们都必须一起重置
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	//创建命令池
	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void LE2_HelloTriangle::createCommandBuffers()
{
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	//命令缓冲区分配info
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;

	//命令缓冲区的级别，是主命令缓冲区还是辅助命令缓冲区。
	//VK_COMMAND_BUFFER_LEVEL_PRIMARY：可以提交到队列执行，但不能从其他命令缓冲区调用。
	//VK_COMMAND_BUFFER_LEVEL_SECONDARY：不能直接提交，但可以从主命令缓冲区调用。
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	//命令缓冲区数量
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	//创建命令缓冲区
	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void LE2_HelloTriangle::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	//VkCommandBufferBeginInfo开始记录命令缓冲区，该结构指定有关此特定命令缓冲区使用的一些细节。
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	//flags参数指定我们将如何使用命令缓冲区。
	//VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT：命令缓冲区将在执行一次后立即重新记录。
	//VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT：这是一个辅助命令缓冲区，将完全在单个渲染过程中。
	//VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT：命令缓冲区可以在它也已经等待执行时重新提交。
	beginInfo.flags = 0; // Optional

	//pInheritanceInfo参数仅与辅助命令缓冲区相关。它指定从调用主命令缓冲区继承的状态。
	beginInfo.pInheritanceInfo = nullptr; // Optional

	//创建一个beginInfo
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	//渲染PassBeginInfo设置
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	//设置渲染pass
	renderPassInfo.renderPass = renderPass;
	//设置使用的帧缓冲区，为要绘制的交换链图像绑定帧缓冲区
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	//渲染区域偏移与大小
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;

	//设置颜色清除时候的值
	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	//最后一个参数是值我们如何使用渲染过程中的绘图命令
	//VK_SUBPASS_CONTENTS_INLINE：渲染通道命令将嵌入到主命令缓冲区本身中，并且不会执行辅助命令缓冲区。
	//VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS：渲染通道命令将从辅助命令缓冲区执行。
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	//为命令缓冲区绑定渲染管线，第二个参数指定管道对象是图形还是计算管道。
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	//设置渲染命令如何绘制三角形,后面四个参数如下
	//vertexCount：即使我们没有顶点缓冲区，但从技术上讲，我们仍然需要绘制 3 个顶点。
	//instanceCount：用于实例渲染，1如果你不这样做，请使用。
	//firstVertex : 用作顶点缓冲区的偏移量，定义 的最小值gl_VertexIndex。
	//firstInstance：用作实例渲染的偏移量，定义 的最小值gl_InstanceIndex。
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	//结束渲染过程。因为已经完成了命令缓冲区的记录
	vkCmdEndRenderPass(commandBuffer);
	//结束记录命令缓冲区过程
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
	//信号状态下创建栅栏，以便第一次调用 vkWaitForFences()立即返回，因为栅栏已经发出信号。
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	//创建信号量和Fence
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
