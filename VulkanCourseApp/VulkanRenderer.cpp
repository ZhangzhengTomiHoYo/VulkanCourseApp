#include "VulkanRenderer.h"

VulkanRenderer::VulkanRenderer()
{
}

VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::createInstance()
{
	// 应用程序本身的信息
	// 大部分数据不会影响程序 并且 只是为了开发者方便
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_4;				// Vulkan 版本 

	// Vulkan 实例创建信息结构体
	VkInstanceCreateInfo createInfo{};  // 直接列表初始化（direct-list-initialization）  直接使用空初始化列表初始化对象
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// createInfo.pNext = nullptr;		     // 默认就是 nullptr
	// createInfo.flags = 0;				 // 默认就是 0
	createInfo.pApplicationInfo = &appInfo;  // p开头意味着是一个指针类型

	// 创建向量存储 实例扩展
	std::vector<const char*> instanceExtensions;

	// 设置 实例扩展 将会使用该变量
	uint32_t glfwExtensionCount = 0;  // GLFW 或许需要多个扩展
	const char** glfwExtensions;      // 

	// 获取 GLFW 需要的扩展
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// 将 GLFW 的扩展添加到实例扩展向量中
	for (size_t i = 0; i < glfwExtensionCount; i++) {
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	// 检查Vulkan实例是否支持所需的扩展
	if (!checkInstanceExtensionSupport(&instanceExtensions)) {
		throw std::runtime_error("VkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// TODO: 设置实例将使用的验证层
	// 验证层 比较难 先不做
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	// 创建实例
	// 中间的参数是内存分配器 比较难 先不做
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create Vulkan instance!");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// 获取所选物理设备的队列族索引
	QueueFamilyIndices indices = getQueueFamilies(mainDevices.physicalDevice);

	// 逻辑设备需要创建的队列 即 相关信息
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;		// 指定从哪个队列族创建队列
	queueCreateInfo.queueCount = 1;									// 要创建的队列数量
	float prioity = 1.0f;
	queueCreateInfo.pQueuePriorities = &prioity;					// Vulkan需要知道存在多个队列时如何分配优先级 1.0f是最高优先级 0.0f是最低优先级

	// 创建逻辑设备的信息（有时也称逻辑设备为设备）
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;						// 队列创建信息的数量
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;			// 队列创建信息的指针
	// 注意：之前的扩展是实例扩展 现在的扩展是设备扩展 暂时先不做
	deviceCreateInfo.enabledExtensionCount = 0;
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;
	// 注释：自Vulkan1.1 所有的验证层都由实例处理

	// 逻辑设备  将要使用的 物理设备的特性
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// 为给定的物理设备创建逻辑设备
	VkResult result = vkCreateDevice(mainDevices.physicalDevice, &deviceCreateInfo, nullptr, &mainDevices.logicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	// 队列和设备都已经被创建
	// 所以我们来处理队列
	//
	// 从给定的逻辑设备中，获取指定队列族的队列
	// 队列索引为0， 因为我们只创建了一个队列，并将该队列句柄保存到给定变量中
	vkGetDeviceQueue(mainDevices.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}

void VulkanRenderer::getPhysicalDevice()
{
	// 枚举实例可以访问的物理设备
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// 虽然已经检查过扩展支持 但是还是要检查是否有可用的物理设备
	if (deviceCount == 0) {
		throw std::runtime_error("Failed to find GPUs that support Vulkan Instance!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (checkDeviceSuitable(device)) {
			mainDevices.physicalDevice = device;
			break;
		}
	}
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// 需要得到扩展的数量  去  创建一个正确大小的数组保存扩展名
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// 使用 extensionCount 创建 VkExtensionProperties 数组
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// 检查给定的扩展是否存在
	for (const auto& checkExtension : *checkExtensions) {
		bool hasExtension = false;
		for (const auto& extension : extensions) {
			if (strcmp(checkExtension, extension.extensionName) == 0) {
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension) {
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	// 设备本身的信息 如 ID, name, type, vendor(供应商), etc.
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// 关于设备支持的功能信息 如是否支持几何着色器, 曲面细分着色器, wide lines, 纹理压缩, etc.
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	// 暂时先什么都不做 直接返回true

	// 检查设备是否具有所需的队列族
	QueueFamilyIndices indices = getQueueFamilies(device);

	return indices.isValid();
}

QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// 获取给定设备的队列族信息
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// 遍历队列族 并且 检查它是否具有至少一种所需的队列类型
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		// 首先检查队列族是否包含一个队列
		// 一个队列族可以支持多个操作类型，这些类型通过位标志表示
		// 使用 按位与(&)运算 和 VK_QUEUE_*_BIT 来检查它是否支持所需的操作类型
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		// 检查所需的队列族索引是否已经找到
		if (indices.isValid()) {
			break;
		}

		i++;
	}

	return indices;
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	this->window = newWindow;

	try {
		createInstance();
		getPhysicalDevice();
		createLogicalDevice();
	}
	catch (const std::runtime_error& e) {
		printf("Failed to create Vulkan instance: %s\n", e.what());
		return EXIT_FAILURE;  // #define EXIT_FAILURE 1 所以返回0代表成功，返回1代表失败
	}
	return 0;
}

void VulkanRenderer::cleanup()
{
	vkDestroyDevice(mainDevices.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}
