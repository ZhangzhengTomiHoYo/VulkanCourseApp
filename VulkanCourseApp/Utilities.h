#pragma once

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// 队列族 的 索引(位置)
struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentationFamily = -1;

	// 检查队列族是否有效
	bool isValid() {
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities;        // 表面属性，例如图像尺寸
	std::vector<VkSurfaceFormatKHR> formats;             // 表面图像格式，例如 RGBA 和每个颜色分量的位数
	std::vector<VkPresentModeKHR> presentationModes;     // 图像呈现到屏幕的方式
};

struct SwapchainImage {
	VkImage image;         // 交换链提供的图像句柄，随交换链销毁，不由我们单独释放。
	VkImageView imageView; // 我们创建的访问视图，必须在销毁交换链之前释放。
};
