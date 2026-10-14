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

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	// 绘图和呈现可能属于同一个队列族。set 去重，避免为同一队列族重复填写申请。
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// 创建信息只保存 priority 的地址，不会复制它指向的数值。
	// 所以变量必须活到下面 vkCreateDevice() 调用结束，不能定义在循环体内。
	float priority = 1.0f;
	for (int queueFamileIndex : queueFamilyIndices)
	{
		// 逻辑设备需要创建的队列 即 相关信息
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamileIndex;		// 指定从哪个队列族创建队列
		queueCreateInfo.queueCount = 1;									// 要创建的队列数量
		queueCreateInfo.pQueuePriorities = &priority; // 每个申请的队列对应一个优先级数值，范围为 0 到 1。

		// 复制这份申请信息到数组；其中的指针仍然指向上面的 priority。
		queueCreateInfos.push_back(queueCreateInfo);
	}



	// 创建逻辑设备的信息（有时也称逻辑设备为设备）
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	// size() 告诉 Vulkan 有几份申请；static_cast 把数量转成接口要求的 uint32_t。
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	// data() 给出第一份申请的地址，Vulkan 据此读取整个数组，不会在这里创建队列。
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	// 选 GPU 时检查“是否支持这些设备扩展”；这里才指定“创建逻辑设备时启用它们”。
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
	// 注释：自Vulkan1.1 所有的验证层都由实例处理

	// 逻辑设备  将要使用的 物理设备的特性
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// 根据选中的物理设备创建逻辑设备，同时创建上面申请的队列。
	// vkCreateDevice 的输入是 physicalDevice 和创建信息，不需要再传 instance。
	// physicalDevice 已经来自实例的枚举结果；这不表示逻辑设备可以脱离实例独立存在。
	VkResult result = vkCreateDevice(mainDevices.physicalDevice, &deviceCreateInfo, nullptr, &mainDevices.logicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device!");
	}

	// 队列已经创建好了：下面只是取出句柄，保存到成员变量，方便以后使用。
	// 每个队列族只申请了一条队列，所以族内的队列索引是 0。
	// 如果绘图族和呈现族编号相同，这两个成员保存的就是同一个队列句柄。
	vkGetDeviceQueue(mainDevices.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
}

void VulkanRenderer::createSurface()
{
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window surface!");
	}

}

void VulkanRenderer::createSwapChain()
{
	SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevices.physicalDevice);
	// 查询的是“这块 GPU + 这个窗口表面”支持的配置，接下来从中选择交换链参数。

	// 1. 选择最佳的表面格式
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);

	// 2. 选择最佳的呈现模式
	VkPresentModeKHR presentationMode = chooseBestPresentationMode(swapChainDetails.presentationModes);

	// 3. 选择交换链图像分辨率
	VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && imageCount > swapChainDetails.surfaceCapabilities.maxImageCount) {
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}


	//	交换链的创建信息
	VkSwapchainCreateInfoKHR swapChainCreateInfo{};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;	// 交换链将关联的表面
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// 选出呈现模式还不够，必须填进创建信息，vkCreateSwapchainKHR 才会使用它。
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.clipped = VK_TRUE;


	QueueFamilyIndices indices = getQueueFamilies(mainDevices.physicalDevice);
	// pQueueFamilyIndices 也只保存地址；数组必须活到创建交换链的调用结束。
	uint32_t queueFamilyIndices[] = {
		static_cast<uint32_t>(indices.graphicsFamily),
		static_cast<uint32_t>(indices.presentationFamily)
	};
	if (indices.graphicsFamily != indices.presentationFamily) {
		// 不同队列族都要访问交换链图像，使用并发共享，避免手动转移队列族所有权。
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		// 同一个队列族即可完成绘图与呈现，无需列出多个共享队列族。
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;			// Optional
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;	// Optional
	}

	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;


	// 创建交换链
	VkResult result = vkCreateSwapchainKHR(mainDevices.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	// 图像由交换链提供：先问数量，再准备数组取回句柄，无需自己调用 vkCreateImage。
	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(mainDevices.logicalDevice, swapchain, &swapChainImageCount, nullptr);
	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(mainDevices.logicalDevice, swapchain, &swapChainImageCount, images.data());

	for (VkImage image : images)
	{
		SwapchainImage swapchainImage{};
		swapchainImage.image = image;

		// 图像是实际的资源；图像视图描述“怎样访问这张图”，不会复制像素。
		// 交换链图像用来显示颜色，所以视图选择 COLOR 部分。
		swapchainImage.imageView = createImageView(image, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);

		SwapChainImages.push_back(swapchainImage);

	}
}

void VulkanRenderer::getPhysicalDevice()
{
	mainDevices.physicalDevice = VK_NULL_HANDLE;
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
	// “有 GPU”不等于“有满足要求的 GPU”；没选中时不能继续创建逻辑设备。
	if (mainDevices.physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find a suitable GPU!");
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

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	if (extensionCount == 0) {
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// 检查给定的扩展是否存在
	for (const auto& checkExtension : deviceExtensions) {
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

	// 上面的属性和特性暂不参与筛选；下面检查队列族、设备扩展和交换链支持。

	// 检查设备是否具有所需的队列族
	QueueFamilyIndices indices = getQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainValid = false;
	if (extensionsSupported) {
		SwapChainDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
	}

	return indices.isValid() && extensionsSupported && swapChainValid;
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

		// 检查队列族是否支持呈现到给定的表面
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
		// 如果队列族支持呈现到给定的表面，则将其索引存储在 presentationFamily 中
		if (presentationSupport) {
			indices.presentationFamily = i;
		}

		// 检查所需的队列族索引是否已经找到
		if (indices.isValid()) {
			break;
		}

		i++;
	}

	return indices;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails;

	// -- 表面能力 --
	// 获取指定物理设备对指定表面支持的能力
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

	// -- 表面格式 --
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	// 如果存在可用格式，则获取格式列表
	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}

	// -- 呈现模式 --
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	// 如果存在可用呈现模式，则获取呈现模式列表
	if (presentationCount != 0)
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}

VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
		// 如果表面格式列表中只有一个元素，并且其格式为 VK_FORMAT_UNDEFINED，则表示表面没有首选格式
		// 在这种情况下，我们可以选择任何格式
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& format : formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	//
	for (const auto& mode : presentationModes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return surfaceCapabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D newExtent = {		};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));
		return newExtent;
	}
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	// 指定访问图像的哪一部分：这里传入 COLOR，深度图则会使用 DEPTH。
	// 参数传进来后还要写入结构体，不能让 aspectMask 保持默认的 0。
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	// 当前交换链图像只有一个 mip 层级和一个数组层，从第 0 层开始访问。
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VkResult result = vkCreateImageView(mainDevices.logicalDevice, &viewCreateInfo, nullptr, &imageView);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image view!");
	}

	return imageView;
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	this->window = newWindow;

	try {
		createInstance();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		// 逻辑设备就绪后才能创建交换链；函数内部还会获取图像并逐张创建图像视图。
		createSwapChain();

	}
	catch (const std::runtime_error& e) {
		printf("Failed to initialize Vulkan renderer: %s\n", e.what());
		// 中途失败时，也释放之前已经成功创建的对象。
		cleanup();
		return EXIT_FAILURE;  // #define EXIT_FAILURE 1 所以返回0代表成功，返回1代表失败
	}
	return 0;
}

void VulkanRenderer::cleanup()
{
	if (mainDevices.logicalDevice != VK_NULL_HANDLE) {
		// 先销毁引用图像的视图，再销毁提供这些图像的交换链。
		for (const auto& image : SwapChainImages) {
			vkDestroyImageView(mainDevices.logicalDevice, image.imageView, nullptr);
		}
		SwapChainImages.clear();
		vkDestroySwapchainKHR(mainDevices.logicalDevice, swapchain, nullptr);
		swapchain = VK_NULL_HANDLE;
		// 交换链图像不单独调用 vkDestroyImage；队列也不单独销毁。
		vkDestroyDevice(mainDevices.logicalDevice, nullptr);
		mainDevices.logicalDevice = VK_NULL_HANDLE;
		graphicsQueue = VK_NULL_HANDLE;
		presentationQueue = VK_NULL_HANDLE;
	}
	if (instance != VK_NULL_HANDLE) {
		vkDestroySurfaceKHR(instance, surface, nullptr);
		surface = VK_NULL_HANDLE;
		vkDestroyInstance(instance, nullptr);
		instance = VK_NULL_HANDLE;
	}
	mainDevices.physicalDevice = VK_NULL_HANDLE;
}
