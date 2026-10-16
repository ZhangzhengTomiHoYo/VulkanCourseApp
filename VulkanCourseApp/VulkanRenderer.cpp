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

	// depthClamp 是物理设备的可选功能：只有创建设备时启用后，管线才能使用深度钳制。
	// 注意：当前管线的 depthClampEnable 仍为 VK_FALSE，所以目前其实用不到该功能；
	// 如果以后设为 VK_TRUE，还应先用 vkGetPhysicalDeviceFeatures() 检查 GPU 是否支持。
	deviceFeatures.depthClamp = VK_TRUE;

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
	// presentationQueue 必须从支持当前 Surface 呈现的队列族中取得。
	// 前面在 VkDeviceQueueCreateInfo 中申请队列，并不会自动给这个成员变量赋值。
	vkGetDeviceQueue(mainDevices.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
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

void VulkanRenderer::createRenderPass()
{
	// Attachment（附件）可以理解为“一次渲染要读写的图像槽位”。
	// 这里暂时只有一个颜色附件，真正放进该槽位的会是某张交换链图像的 Image View。
	VkAttachmentDescription colorAttachment{};
	// 附件格式必须与以后创建 Framebuffer 时绑定的交换链图像格式一致。
	colorAttachment.format = swapChainImageFormat;
	// 每个像素只采样一次，说明目前未启用 MSAA。
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// 开始 Render Pass 时不保留旧颜色，而是用 vkCmdBeginRenderPass 提供的 clearColor 清屏。
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// 结束后保留渲染结果，因为交换链还要把这张图像呈现到屏幕。
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	// 当前没有模板附件，因此模板数据在开始和结束时都不需要处理。
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// UNDEFINED 表示不关心图像进入 Render Pass 前的内容和布局；这与上面的 CLEAR 配合使用。
	// Vulkan 会在 Render Pass 内部把图像转换到子通道需要的颜色附件布局。
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// Render Pass 结束时再转换为呈现布局，让 vkQueuePresentKHR 可以读取它。
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// AttachmentDescription 说明“槽位里的图像怎样使用”；AttachmentReference 则说明
	// “当前子通道要使用第几个槽位，以及使用时需要什么布局”。
	VkAttachmentReference colorAttachmentRef{};
	// attachment = 0 对应下面 pAttachments 数组中的第 0 个，也就是 colorAttachment。
	colorAttachmentRef.attachment = 0;
	// 子通道写颜色时，图像应处于最适合颜色附件读写的布局。
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Subpass（子通道）描述 Render Pass 内的一段实际渲染工作。
	// 当前只有一个子通道：使用图形管线，并把结果写入上面的颜色附件。
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	// 这里保存的是引用数组的地址；只有一个颜色附件，所以直接传单个变量的地址。
	subpass.pColorAttachments = &colorAttachmentRef;

	// Dependency（依赖）规定子通道与外部操作之间的执行、内存先后关系。
	// 阅读一条依赖时，可以把 src 理解为“前一项工作”，把 dst 理解为“后一项工作”：
	// 先确定 srcSubpass 和 dstSubpass 分别是谁，再用 Stage 指定要同步到管线的哪个阶段，
	// 最后用 Access 指定该阶段的哪种内存读写必须完成或可以开始。
	// [0] 控制进入子通道前的等待，[1] 控制离开子通道后的可见性。
	std::array<VkSubpassDependency, 2> subpassDependencies {};

	// 第一条依赖的来源：VK_SUBPASS_EXTERNAL 表示当前 Render Pass 外部、发生在它之前的操作。
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	// 来源阶段：等外部操作运行到管线底部，表示它已经走到执行流程的末端。
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	// 来源访问：外部操作在进入这个同步点之前进行的内存读取必须已经完成。
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

	// 第一条依赖的目标：Render Pass 中编号为 0 的子通道，也就是上面唯一的 subpass。
	subpassDependencies[0].dstSubpass = 0;
	// 目标阶段：第 0 个子通道执行到“颜色附件输出”阶段时，需要受到这条依赖约束。
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// 目标访问：依赖满足后，该阶段才能读取或写入颜色附件。
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	// 依赖选项：0 表示不启用 BY_REGION 等额外行为，按普通的完整依赖处理。
	subpassDependencies[0].dependencyFlags = 0;

	// 第二条依赖的来源：编号为 0 的子通道；方向与第一条依赖相反。
	subpassDependencies[1].srcSubpass = 0;
	// 来源阶段：关注第 0 个子通道的“颜色附件输出”阶段。
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// 来源访问：该阶段对颜色附件的读取和写入必须完成后，外部操作才能继续。
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// 第二条依赖的目标：VK_SUBPASS_EXTERNAL 表示 Render Pass 结束后的外部操作。
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	// 目标阶段：外部操作在管线底部等待，直到上面的颜色附件工作完成。
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	// 目标访问：依赖满足后，外部操作才能读取这张已经完成渲染的图像。
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// 依赖选项：同样不启用任何额外依赖标志。
	subpassDependencies[1].dependencyFlags = 0;

	// 最后把“有哪些附件、有哪些子通道、它们如何同步”汇总成 Render Pass 创建信息。
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	// attachmentCount 与 pAttachments 必须配套：这里传入一份颜色附件描述。
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	// 当前 Render Pass 只有一个子通道。
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	// std::array::size() 给出依赖数量，data() 给出第一个依赖的地址。
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	// Render Pass 隶属于逻辑设备；创建成功后，句柄被保存到成员 renderPass。
	VkResult result = vkCreateRenderPass(mainDevices.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}
}

void VulkanRenderer::createGraphicsPipeline()
{
	// 这里读取的不是 .vert/.frag GLSL 源码，而是 glslangValidator 编译出的 SPIR-V 二进制。
	// Vulkan 驱动接收 SPIR-V，这样不同显卡驱动不必各自解析一套高级着色器语言。
	auto vertexShaderCode = readFile("Shaders/shader.vert.spv");
	auto fragmentShaderCode = readFile("Shaders/shader.frag.spv");

	// Shader Module 是逻辑设备对一份 SPIR-V 代码的临时包装。
	// 创建 Module 不等于执行着色器；真正绘制时使用的是后面创建的图形管线。
	VkShaderModule verteShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

	// 描述“哪一个 Shader Module 用作顶点着色器，以及从哪个入口函数开始执行”。
	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	// 同一份创建信息结构可描述不同阶段；这里明确指定为顶点阶段。
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	// 绑定上面由 shader.vert.spv 创建的 Shader Module。
	vertexShaderStageInfo.module = verteShaderModule;
	// GLSL 中的入口函数是 void main()，因此入口名称写作 "main"。
	vertexShaderStageInfo.pName = "main";

	// 片段阶段的填写方式相同，只是阶段类型和 Shader Module 不同。
	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";

	// 图形管线创建信息最终需要一个“着色器阶段数组”。
	// 目前只有顶点和片段两个可编程阶段；后面调用 vkCreateGraphicsPipelines() 时会用到它。
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderStageInfo, fragmentShaderStageInfo };

	// -- Vertex Input（顶点输入）--
	// 这个阶段描述“如何从顶点缓冲区读取每个顶点的数据”。
	// 当前顶点位置和颜色直接写在 shader.vert 数组中，并通过 gl_VertexIndex 读取，
	// 所以暂时没有顶点缓冲区，也就不需要绑定描述和属性描述。
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	// binding 描述一整条顶点数据流，例如每个顶点占多少字节、按顶点还是按实例前进。
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
	// attribute 描述数据流中各属性的位置和格式，例如 position 是 vec3、color 是 vec3。
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

	// -- Input Assembly（输入装配）--
	// 输入装配决定怎样把顶点组合成“图元”。图元可以是点、线或三角形。
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	// TRIANGLE_LIST 每三个顶点组成一个互不相连的三角形：0/1/2 是第一个，3/4/5 是第二个。
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	// 图元重启主要用于带索引的 STRIP：遇到特殊索引时结束当前条带并重新开始。
	// 当前使用 TRIANGLE_LIST，而且还没有索引缓冲区，因此关闭它。
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	// -- Viewport & Scissor（视口与裁剪矩形）--
	// 顶点着色器输出的是标准化坐标，viewport 负责把它映射到交换链图像的像素区域。
	VkViewport viewport{};
	// 左上角从交换链图像的 (0, 0) 开始，宽高覆盖整张交换链图像。
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	// Vulkan 的视口深度范围是 0.0 到 1.0，之后会用于深度测试。
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// scissor 是硬裁剪区域：落在矩形外面的片段不会写入颜色附件。
	// 这里同样覆盖整张交换链图像，因此暂时不会额外裁掉画面。
	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapChainExtent;

	// 管线通过这个汇总结构取得视口和裁剪矩形。
	// 这里保存的是指针，所以 viewport 和 scissor 必须活到创建图形管线的调用结束。
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	// -- Dynamic States（动态状态）--
	// 这里暂不启用动态状态，因此 viewport、scissor 等值会固定在图形管线中。
	// 将来若把它们设为动态状态，就能在录制命令时修改，而不必重新创建整条管线。

	// -- Rasterizer（光栅化）--
	// 光栅化器把输入装配产生的三角形转换成许多片段，交给片段着色器处理。
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	// FALSE：超出近、远深度范围的部分会被裁掉；TRUE 才会把深度钳制到范围内。
	// 开启 TRUE 需要在逻辑设备中启用 deviceFeatures.depthClamp。
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	// TRUE 会在光栅化前直接丢弃所有图元，不产生任何片段；正常绘制应保持 FALSE。
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	// FILL 填满三角形内部；LINE/WIREFRAME 等非填充模式需要额外设备功能支持。
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	// 只有绘制线图元或线框时线宽才明显；大于 1.0 通常需要 wideLines 功能。
	rasterizerCreateInfo.lineWidth = 1.0f;
	// 背面剔除：只光栅化朝向摄像机的正面，减少不可见片段的工作量。
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	// 顺时针排列的顶点被认定为正面。该判断还会受到 viewport 的 Y 方向影响。
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	// 深度偏移常用于阴影贴图等场景，稍微移动深度以减轻 Z-fighting；当前不需要。
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

	// -- Multisampling（多重采样抗锯齿）--
	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	// sampleShadingEnable 控制是否让片段着色器按采样点运行，它本身不是 MSAA 总开关。
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	// 每像素只有一个采样点，等于当前未启用 MSAA；以后可改为 2/4/8 个采样点。
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// -- Color Blending（颜色混合）--
	// 颜色混合决定“片段着色器的新颜色”怎样与“附件里已有的颜色”合并。
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	// 允许写入颜色附件的 R、G、B、A 四个通道；缺少某一位就不会更新对应通道。
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// 开启后使用下面的因子和操作计算输出；关闭时通常直接覆盖原颜色。
	colorBlendAttachment.blendEnable = VK_TRUE;

	// RGB 公式：最终颜色 = 新颜色 × 新 Alpha + 旧颜色 × (1 - 新 Alpha)。
	// 这是最常见的普通透明混合。例如新 Alpha 为 0.25，新颜色占 25%，旧颜色占 75%。
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

	// Alpha 公式：最终 Alpha = 1 × 新 Alpha + 0 × 旧 Alpha = 新 Alpha。
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	// 逻辑操作是按位 AND/OR/XOR 等另一套合并方式；这里使用上面的数值混合，因此关闭。
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	// 当前只有一个颜色附件，因此只需要一份 attachment 混合配置。
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;

	// Pipeline Layout 是着色器访问外部资源时的“接口布局”。
	// 当前着色器只使用自身写死的顶点数组，没有描述符集合和 Push Constant，因此两项数量都为 0。
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0;
	pipelineLayoutCreateInfo.pSetLayouts = nullptr;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// 即使布局内容为空，也必须创建一个 VkPipelineLayout，图形管线不能省略它。
	VkResult result = vkCreatePipelineLayout(mainDevices.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	// -- Depth Stencil Test（深度与模板测试）--
	// 当前 Render Pass 没有深度/模板附件，所以不提供该阶段的创建信息。

	// 前面每个 CreateInfo 只描述管线的一部分；这里把它们的地址汇总起来，创建完整图形管线。
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	// 两个可编程阶段：顶点着色器和片段着色器。
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages;
	// 以下指针分别连接顶点输入、图元装配、视口、光栅化、多重采样和颜色混合状态。
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	// 没有启用动态状态，因此创建后不能在命令中临时修改这些固定配置。
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	// 没有深度/模板附件，因此这里可以是 nullptr。
	pipelineCreateInfo.pDepthStencilState = nullptr;
	// 管线布局规定着色器资源接口；Render Pass 与 subpass 规定这条管线的输出目标必须兼容什么结构。
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;

	// 当前没有从另一条管线派生创建，所以基管线句柄为空，索引为 -1。
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = -1;

	// 第二个参数本可传 Pipeline Cache 来复用编译结果；当前传空表示不使用缓存。
	// 成功后，Vulkan 把新建的 VkPipeline 句柄写入 graphicsPipeline。
	result = vkCreateGraphicsPipelines(mainDevices.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}

	// 管线会保存创建后所需的着色器状态，因此 Module 不必一直保留到程序结束。
	vkDestroyShaderModule(mainDevices.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(mainDevices.logicalDevice, verteShaderModule, nullptr);
}

void VulkanRenderer::createFramebuffers()
{
	// 交换链有几张图像，就创建几个 Framebuffer。
	// resize() 不仅修改数量，也提前创建好对应数量的空 VkFramebuffer 句柄槽位。
	swapchainFramebuffers.resize(SwapChainImages.size());

	// i 同时表示交换链图像编号和 Framebuffer 编号，二者保持一一对应。
	for (size_t i = 0; i < swapchainFramebuffers.size(); ++i) {
		// attachments 是“这个 Framebuffer 实际绑定了哪些 ImageView”的数组。
		// 当前 RenderPass 只有一个颜色附件，所以数组中也只有一个 ImageView。
		// 第 i 个 Framebuffer 绑定第 i 张交换链图像的视图。
		std::array<VkImageView, 1> attachments = {
			SwapChainImages[i].imageView
		};

		// Framebuffer 不保存渲染规则，它只负责把具体图像与 RenderPass 声明的附件槽位对应起来。
		VkFramebufferCreateInfo framebufferCreateInfo{};
		// sType 告诉 Vulkan 当前结构体是 Framebuffer 的创建信息。
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		// 指定这个 Framebuffer 要与哪个 RenderPass 配合。
		// 因此 attachments 的数量、格式和用途必须与该 RenderPass 的附件描述兼容。
		framebufferCreateInfo.renderPass = renderPass;
		// 告诉 Vulkan attachments 数组里共有几个 ImageView；当前只有一个颜色附件。
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		// data() 返回数组第一个元素的地址，Vulkan 从这里读取全部 ImageView 句柄。
		framebufferCreateInfo.pAttachments = attachments.data();
		// Framebuffer 的宽度和高度必须覆盖这批交换链图像的实际尺寸。
		framebufferCreateInfo.width = swapChainExtent.width;
		framebufferCreateInfo.height = swapChainExtent.height;
		// layers 表示 Framebuffer 包含多少个图像数组层；普通二维窗口图像只有一层。
		framebufferCreateInfo.layers = 1;

		// 创建出的句柄直接写入 vector 的第 i 个位置，保持与交换链图像下标一致。
		VkResult result = vkCreateFramebuffer(mainDevices.logicalDevice, &framebufferCreateInfo, nullptr, &swapchainFramebuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create a Framebuffer!");
		}
	}
}

void VulkanRenderer::createCommandPool()
{
	// Command Buffer 最终要提交给 graphicsQueue，而 Command Pool 必须属于同一个队列族。
	// 因此先重新取得图形队列族编号。
	QueueFamilyIndices queueFamilyIndices = getQueueFamilies(mainDevices.physicalDevice);

	// Command Pool 是 Command Buffer 的内存管理者；它本身不保存绘图内容。
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	// 指明当前结构体是 Command Pool 创建信息。
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	// 从这个 Pool 分配的 Command Buffer 只能提交给属于该队列族的队列。
	commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	// flags 保持默认值 0：当前不要求单独重置某一个 Command Buffer，也不提供短生命周期提示。

	// 创建成功后，把 Command Pool 句柄保存到 graphicsCommandPool。
	VkResult result = vkCreateCommandPool(mainDevices.logicalDevice, &commandPoolCreateInfo, nullptr, &graphicsCommandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool!");
	}
}

void VulkanRenderer::createCommandBuffers()
{
	// 为每个 Framebuffer 准备一份 Command Buffer。
	// 下标继续保持对应：commandBuffers[i] 会向 swapchainFramebuffers[i] 录制绘图命令。
	commandBuffers.resize(swapchainFramebuffers.size());

	// 这里是“批量分配命令缓冲区”的申请信息。
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	// 指明当前结构体是 Command Buffer 分配信息。
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	// 指定由哪个 Pool 提供这些 Command Buffer 的内存。
	commandBufferAllocateInfo.commandPool = graphicsCommandPool;
	// PRIMARY 可以直接提交给 Queue，也可以直接包含 RenderPass 和 Draw 等命令。
	// SECONDARY 通常由 PRIMARY 调用，不能像这里一样单独完成整套提交。
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	// 分配数量与 vector 大小一致，也就是每个 Framebuffer 一份。
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	// data() 提供 vector 首地址，Vulkan 会依次把创建出的句柄写入其中。
	VkResult result = vkAllocateCommandBuffers(mainDevices.logicalDevice, &commandBufferAllocateInfo, commandBuffers.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}
}

void VulkanRenderer::recordCommands()
{
	// Command Buffer 可以理解为交给 GPU 的“操作清单”。
	// 这里做的是录制，不会立即执行；以后把它提交给 graphicsQueue 才真正运行。
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	// 指明当前结构体用于开始录制 Command Buffer。
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;


	// 下面的信息规定录制 vkCmdBeginRenderPass 时要使用哪个 RenderPass、哪块区域以及如何清屏。
	VkRenderPassBeginInfo renderPassBeginInfo{};
	// 指明当前结构体用于开始 RenderPass。
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	// 使用之前创建的 RenderPass 规则；具体 Framebuffer 会在循环里按下标填写。
	renderPassBeginInfo.renderPass = renderPass;
	// 渲染区域从 Framebuffer 左上角 (0, 0) 开始。
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	// 渲染区域覆盖整张交换链图像。
	renderPassBeginInfo.renderArea.extent = swapChainExtent;

	// RenderPass 的颜色附件使用 LOAD_OP_CLEAR，因此开始时需要提供清除颜色。
	// 四个分量依次是 R、G、B、A，取值范围通常为 0.0 到 1.0。
	VkClearValue clearValues[] = {
		{0.6f, 0.65f, 0.4f, 1.0f}
	};
	// pClearValues 指向清除值数组；数组下标与 RenderPass 中附件的下标对应。
	renderPassBeginInfo.pClearValues = clearValues;
	// 当前只有一个颜色附件，所以只提供一个清除值。
	renderPassBeginInfo.clearValueCount = 1;

	// 每个 Command Buffer 都绑定同下标的 Framebuffer；绘制规则相同，最终写入的交换链图像不同。
	for (size_t i = 0; i < commandBuffers.size(); ++i) {
		// 第 i 份命令将 RenderPass 的附件槽位绑定到第 i 个 Framebuffer 中的 ImageView。
		renderPassBeginInfo.framebuffer = swapchainFramebuffers[i];

		// 进入录制状态；从这里开始的 vkCmd* 调用会依次写进 commandBuffers[i]。
		VkResult result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to begin command buffer!");
		}

		// 开始一次 RenderPass。INLINE 表示 Subpass 的命令直接写在当前 PRIMARY Command Buffer 中。
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// 指定后续 Draw 使用哪条图形管线；Bind Point 必须与 Subpass 的 GRAPHICS 类型一致。
		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		// 录制一次非索引绘制：
		// 3 = 每个实例绘制 3 个顶点；1 = 只绘制 1 个实例；两个 0 分别表示从顶点 0、实例 0 开始。
		// 顶点着色器会收到 gl_VertexIndex = 0、1、2，从写死的数组中取出三角形顶点。
		vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

		// 结束 RenderPass；Vulkan 会按 RenderPass 描述保存颜色并把图像转为最终布局。
		vkCmdEndRenderPass(commandBuffers[i]);
		// 结束整份 Command Buffer 的录制，使它可以在之后提交给 Queue。
		result = vkEndCommandBuffer(commandBuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to end command buffer!");
		}
	}
}

void VulkanRenderer::createSynchronization()
{
	// 程序用 currentFrame 在多组同步对象之间循环，避免上一帧尚未执行完成就复用同一组对象。
	// 因此下面两类对象的数量都等于 MAX_FRAME_DRAWS。
	// imageAvailable：通知图形队列“本次取得的交换链图像已经可以写入”。
	imageAvailableSemaphores.resize(MAX_FRAME_DRAWS);
	// drawFence：通知 CPU“currentFrame 所指的这组对象上一次对应的绘制提交已经完成”。
	drawFences.resize(MAX_FRAME_DRAWS);

	// renderFinished 会交给呈现操作等待，所以它按交换链图像编号选择，而不按 currentFrame 选择。
	// 再次取得同一张交换链图像时，才能确定上一次呈现已经用完该图像对应的信号量。
	renderFinishedSemaphores.resize(SwapChainImages.size());

	// Semaphore 不需要额外参数；这里只需声明结构体的实际类型。
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence 用于让 CPU 查询或等待图形队列中的一帧绘制任务是否完成。
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	// Fence 初始为“已触发”，使第一帧调用 vkWaitForFences() 时可以立即通过。
	// 之后每次使用前会重置，图形队列执行完对应提交后再把它设为“已触发”。
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	// 为 currentFrame 可以指向的每个编号，分别创建“图像可用”信号量和绘制完成 Fence。
	for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i) {
		if (vkCreateSemaphore(mainDevices.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image available semaphore!");
		}
		if (vkCreateFence(mainDevices.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create draw fence!");
		}
	}

	// 为每张交换链图像创建自己的“绘制完成”信号量。
	// 该数组使用 vkAcquireNextImageKHR() 返回的 imageIndex 访问。
	for (size_t i = 0; i < renderFinishedSemaphores.size(); ++i) {
		if (vkCreateSemaphore(mainDevices.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render finished semaphore!");
		}
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

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
	// 这个结构体告诉 Vulkan：SPIR-V 在内存的什么位置，以及共有多少字节。
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	// codeSize 的单位是字节，因此直接使用 vector<char>::size()。
	createInfo.codeSize = code.size();
	// readFile() 得到的是 char*，而 Vulkan 按 32 位字读取 SPIR-V，需要把指针解释成 uint32_t*。
	// reinterpret_cast 只改变“如何看待这段内存”，不会转换或复制其中的数据。
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	// Vulkan 成功后会把新建的 Shader Module 句柄写入这个变量。
	VkShaderModule shaderModule;
	if (vkCreateShaderModule(mainDevices.logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	// 返回的是 Vulkan 对象句柄，不是 SPIR-V 数据的副本。
	return shaderModule;
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
		createRenderPass();
		// 图形管线依赖逻辑设备和交换链格式；当前阶段先读取着色器并配置两个着色器阶段。
		createGraphicsPipeline();
		createFramebuffers();
		// Command Pool 提供内存，Command Buffer 从中分配，最后再把实际绘图步骤录进去。
		createCommandPool();
		createCommandBuffers();
		recordCommands();
		createSynchronization();
	}
	catch (const std::runtime_error& e) {
		printf("Failed to initialize Vulkan renderer: %s\n", e.what());
		// 中途失败时，也释放之前已经成功创建的对象。
		cleanup();
		return EXIT_FAILURE;  // #define EXIT_FAILURE 1 所以返回0代表成功，返回1代表失败
	}
	return 0;
}

void VulkanRenderer::draw()
{
	// currentFrame 表示程序当前使用第几组 imageAvailableSemaphore 和 drawFence。
	// CPU 在复用这组对象前，先等待它上一次对应的绘制提交执行完成。
	// VK_TRUE 表示等待传入的全部 Fence；这里只有一个。
	// UINT64_MAX 表示一直等待，不设置超时时间。
	vkWaitForFences(mainDevices.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	// 等待成功后，将 Fence 恢复为“未触发”，供本次 vkQueueSubmit() 记录完成状态。
	vkResetFences(mainDevices.logicalDevice, 1, &drawFences[currentFrame]);

	// imageIndex 与 currentFrame 含义不同：它是交换链这次提供的图像编号。
	uint32_t imageIndex;
	// 请求取得一张可以写入的交换链图像：
	// - UINT64_MAX：一直等待，直到有图像可用；
	// - imageAvailableSemaphores[currentFrame]：取得成功后由 Vulkan 触发；
	// - VK_NULL_HANDLE：这里用信号量接收通知，不另外使用 Fence；
	// - imageIndex：接收实际取得的交换链图像编号。
	vkAcquireNextImageKHR(mainDevices.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	// 描述准备提交给 graphicsQueue 的一帧绘制任务。
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	// 本次提交需要等待一个信号量。
	submitInfo.waitSemaphoreCount = 1;
	// 等待 currentFrame 所指的“图像可用”信号量，避免交换链图像尚未取得就开始写入。
	submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];
	// 只让“颜色附件输出”阶段等待交换链图像；更早且不访问该图像的阶段无需被阻塞。
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;
	// 本次只提交一份 Command Buffer。
	submitInfo.commandBufferCount = 1;
	// 选择与本次交换链图像对应的命令；它会把绘制结果写入同下标的 Framebuffer。
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
	// 这帧绘制任务执行完成后触发一个信号量。
	submitInfo.signalSemaphoreCount = 1;
	// 这个信号量随后交给 vkQueuePresentKHR 等待，所以它跟随交换链图像。
	// 若按 currentFrame 复用，呈现操作尚未用完时，下一帧可能再次触发同一个信号量。
	submitInfo.pSignalSemaphores = &renderFinishedSemaphores[imageIndex];

	// 把这一帧绘制任务放入图形队列。
	// drawFences[currentFrame] 会在该提交执行完成后变成“已触发”，供 CPU 下次复用这组对象时等待。
	VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	// 描述准备提交给 presentationQueue 的呈现操作。
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	// 呈现前需要等待一个“绘制完成”信号量，避免显示尚未画完的图像。
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[imageIndex];
	// 本次只向一个交换链提交呈现请求。
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	// 指定要呈现刚才取得并完成绘制的那张交换链图像。
	presentInfo.pImageIndices = &imageIndex;

	// 把呈现请求放入呈现队列；函数返回不等于窗口系统已经显示完这张图像。
	result = vkQueuePresentKHR(presentationQueue, &presentInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	// 切换到下一组帧同步对象。取模使编号在 0 和 MAX_FRAME_DRAWS - 1 之间循环。
	// currentFrame 的变化不代表下一次会取得哪张交换链图像；图像编号仍由 Vulkan 返回。
	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::cleanup()
{
	vkDeviceWaitIdle(mainDevices.logicalDevice);

	// renderFinished 的数量等于交换链图像数，要单独全部销毁。
	for (VkSemaphore semaphore : renderFinishedSemaphores) {
		vkDestroySemaphore(mainDevices.logicalDevice, semaphore, nullptr);
	}

	// imageAvailable 和 Fence 的数量等于同时处理的帧数。
	for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i) {
		vkDestroySemaphore(mainDevices.logicalDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(mainDevices.logicalDevice, drawFences[i], nullptr);
	}

	// 销毁 Command Pool 时，由它分配的全部 Command Buffer 会一起被隐式释放，不需要逐个销毁。
	vkDestroyCommandPool(mainDevices.logicalDevice, graphicsCommandPool, nullptr);
	// Framebuffer 引用了交换链 ImageView 和 RenderPass，所以必须先销毁 Framebuffer，
	// 然后才能销毁 RenderPass、ImageView 和交换链。
	for (auto framebuffer : swapchainFramebuffers) {
		vkDestroyFramebuffer(mainDevices.logicalDevice, framebuffer, nullptr);
	}
	vkDestroyPipeline(mainDevices.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mainDevices.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(mainDevices.logicalDevice, renderPass, nullptr);
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
