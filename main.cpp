#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <iostream>
#include<stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance , const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator,VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance , VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyUtilsMessengerEXT");

	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

struct QueueFamilyIndices
{
	std::optional<unsigned int> graphicsFamily;

	bool IsCompleted()
	{
		return graphicsFamily.has_value();
	}
};

class HelloTriangleApplication
{
public:
	void Run()
	{
		InitWindow();
		InitVulkan();
		MainLoop();
		Cleanup();
	}
private:
	GLFWwindow* _window;
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;

	VkDevice _device;
	VkQueue _graphicsQueue;

	void InitWindow()
	{
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void InitVulkan() 
	{
		CreateInstance();
		SetupDebugMessenger();
		PickPhysicalDevice();
		CreateLogicalDevice();
	}

	void MainLoop()
	{
		while (!glfwWindowShouldClose(_window))
		{
			glfwPollEvents();
		}

	}

	void Cleanup()
	{
		vkDestroyDevice(_device, nullptr);

		if (enableValidationLayers)
		{
			DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
		}
		vkDestroyInstance(_instance, nullptr);
		glfwDestroyWindow(_window);
		glfwTerminate();
	}

	void CreateInstance()
	{
		if (enableValidationLayers && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("Validation layers requested, but not available");
		}

		VkApplicationInfo appinfo{};
		appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appinfo.pApplicationName = "Hello Triangle";
		appinfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appinfo.pEngineName = "SoSy Game Engine";
		appinfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appinfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appinfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<unsigned int>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo { };

		if (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<unsigned int>( validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			PopulateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}


		if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan Instance!");
		}
/*
std::vector<VkExtensionProperties> extensions(glfwExtensionCount);
vkEnumerateInstanceExtensionProperties(nullptr, &glfwExtensionCount, extensions.data());

std::cout << "Available Extentions: " << std::endl;

for (const auto& extension : extensions)
{
	std::cout << "\t" << extension.extensionName << std::endl;

}
*/
	}


	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = { };
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	void SetupDebugMessenger()
	{
		if (!enableValidationLayers)
		{
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("Fialed to setup debug messenger");
		}
	}

	void PickPhysicalDevice()
	{
		unsigned int deviceCount = 0;
		vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("Failed to find any GPUs with Vulkan");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());
		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				_physicalDevice = device;
				break;
			}
		}
		if (_physicalDevice == VK_NULL_HANDLE)
		{
			throw std::runtime_error("Failed to find a suitable GPU");
		}
	}

	void CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
		queueCreateInfo.queueCount = 1;

		float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = 0;

		if (enableValidationLayers)
		{
			createInfo.enabledExtensionCount = static_cast<unsigned int>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
		
		if (vkCreateDevice(_physicalDevice, &createInfo, NULL, &_device) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create logical deivice");
		}

		vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);

	}


	bool IsDeviceSuitable(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices = FindQueueFamilies(device);

		return indices.IsCompleted();

	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		unsigned int queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;

		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			if (indices.IsCompleted())
			{
				break;
			}
		}
		return indices;
	}

	std::vector<const char*>GetRequiredExtensions()
	{
		unsigned int glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*>extensions(glfwExtensions, glfwExtensionCount + glfwExtensions);


		if (enableValidationLayers)
		{
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool CheckValidationLayerSupport()
	{
		unsigned int layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				return false;
			}

		}

		return true;
	}
	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		std::cerr << " Validation Layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}


};


int main()
{
	HelloTriangleApplication app;

	try
	{
		app.Run();


	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;

	}

	return EXIT_SUCCESS;
}






















