#pragma once

#include <fstream>

// 程序准备两组可循环使用的帧同步对象，限制最多有两帧绘制任务处于处理中。
// 它不是交换链图像数量；交换链图像数量由 Vulkan 和窗口系统共同决定。
const int MAX_FRAME_DRAWS = 2;

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

// 把整个二进制文件读进连续内存，主要用于读取编译后的 SPIR-V 着色器。
// static 在这里表示“当前编译单元私有”。由于本函数定义在头文件中，
// 每个包含 Utilities.h 的 .cpp 都会得到自己的一份 readFile()。
// filename 使用 const 引用传入：不复制字符串，也不允许函数修改文件名。
static std::vector<char> readFile(const std::string& filename) {
	// binary：按原始字节读取，避免文本模式转换内容。
	// ate：打开后先把读取位置放到文件末尾，方便用 tellg() 得到文件大小。
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	// 文件不存在、相对路径错误或没有读取权限时，打开会失败。
	if (!file.is_open()) {
		throw std::runtime_error("Failed to open file: " + filename);
	}

	// 因为使用了 ate，当前读取位置就在文件末尾，所以该位置也就是文件字节数。
	// tellg() 返回流的位置类型；转换成 size_t 后可用于创建 vector。
	size_t fileSize = static_cast<size_t>(file.tellg());

	// 一次准备与文件等大的连续内存。char 占一个字节，适合原样保存二进制数据。
	std::vector<char> buffer(fileSize);

	// 刚才只是到末尾测量大小；正式读取前把读取位置移回文件开头。
	file.seekg(0);

	// data() 返回 vector 第一个字节的地址，read() 从这里开始写入 fileSize 个字节。
	file.read(buffer.data(), fileSize);

	// close() 在这里写出来便于学习；即使省略，ifstream 析构时也会自动关闭文件。
	file.close();
	// 返回装有全部 SPIR-V 字节的 vector。现代 C++ 通常会省略不必要的复制。
	return buffer;
}
