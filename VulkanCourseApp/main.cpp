#define STB_IMAGE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stdexcept>
#include <iostream>

#include "VulkanRenderer.h"

GLFWwindow* window;
VulkanRenderer vulkanRenderer;

void initWindow(std::string wName, const int width, const int height) {
	glfwInit();                                      // 初始化 GLFW    
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);    // 设置 GLFW 不创建 OpenGL 上下文
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
}

int main() 
{
	initWindow("Vulkan Window", 1920, 1080);
	if (vulkanRenderer.init(window) == EXIT_FAILURE) {
		glfwDestroyWindow(window);
		glfwTerminate();
		return EXIT_FAILURE;
	}

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	// 加载模型
	int wuwa_imiss = vulkanRenderer.createMeshModel("Models/wuwa_imiss/wuwa_imiss.pmx");

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float now = static_cast<float>(glfwGetTime());
		deltaTime = now - lastTime;
		lastTime = now;

		angle += deltaTime * 100.0f;
		if (angle > 360.0f) { angle -= 360.0f; }

		// TRS 构造模型矩阵
		glm::mat4 firstModel = glm::mat4(1.0f);
		firstModel = glm::translate(firstModel, glm::vec3(0.0f, -1.0f, -1.0f));
		firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
		firstModel = glm::scale(firstModel, glm::vec3(0.1f));

		vulkanRenderer.updateModel(wuwa_imiss, firstModel);
		// 每轮循环提交一帧绘制任务，并把画好的交换链图像提交给呈现队列。
		vulkanRenderer.draw();
	}

	// 总流程第 4 步：等待 GPU 空闲，并按依赖关系的反方向释放资源。
	vulkanRenderer.cleanup();

	// Vulkan 已不再使用窗口，此时才能销毁窗口并结束 GLFW。
	glfwDestroyWindow(window);
	glfwTerminate();


	return 0;
}
