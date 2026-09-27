#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stdexcept>
#include <iostream>

#include "Utilities.h"

class VulkanRenderer
{
private:
	GLFWwindow* window;

	VkInstance instance;

	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevices;
	VkQueue graphicsQueue;

	// Vulkan 函数
	// - Create
	void createInstance();
	void createLogicalDevice();
	// - Get
	void getPhysicalDevice();
	// - Support
	// -- Checker
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceSuitable(VkPhysicalDevice device);
	// -- Getter
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device); 

public:
	VulkanRenderer();
	~VulkanRenderer();
	
	int init(GLFWwindow* newWindow);
	void cleanup();

	 
};

