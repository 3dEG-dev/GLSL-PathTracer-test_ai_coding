#include "vulkan_context.h"
#include <iostream>
#include <cstring>
#include <set>

namespace vkpt {

// Validation Layers (für Debugging)
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

// Device Extensions (für Multi-GPU und Compute)
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VulkanContext::VulkanContext() 
    : instance(VK_NULL_HANDLE), debugMessenger(VK_NULL_HANDLE), validationEnabled(false) {}

VulkanContext::~VulkanContext() {
    cleanup();
}

bool VulkanContext::initialize(bool enableValidation) {
    validationEnabled = enableValidation;
    
    if (!createInstance()) {
        std::cerr << "Failed to create Vulkan instance!" << std::endl;
        return false;
    }
    
    if (enableValidation && !setupDebugMessenger()) {
        std::cerr << "Failed to setup debug messenger!" << std::endl;
        // Continue without debug messenger
    }
    
    auto physicalDevices = enumerateDevices();
    if (physicalDevices.empty()) {
        std::cerr << "No Vulkan devices found!" << std::endl;
        return false;
    }
    
    std::cout << "Found " << physicalDevices.size() << " Vulkan device(s):" << std::endl;
    for (const auto& dev : physicalDevices) {
        std::cout << "  - " << dev.name << " (ID: " << dev.deviceId << ")" << std::endl;
    }
    
    devices = std::move(physicalDevices);
    return true;
}

void VulkanContext::cleanup() {
    for (auto& dev : devices) {
        if (dev.device != VK_NULL_HANDLE) {
            vkDestroyDevice(dev.device, nullptr);
        }
    }
    devices.clear();
    
    if (enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, nullptr);
        }
    }
    
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
}

std::vector<DeviceInfo> VulkanContext::enumerateDevices() {
    std::vector<DeviceInfo> result;
    
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    
    if (deviceCount == 0) {
        return result;
    }
    
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());
    
    for (uint32_t i = 0; i < deviceCount; ++i) {
        VkPhysicalDevice physicalDevice = physicalDevices[i];
        
        // Device Properties
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        
        // Queue Family finden (Compute Queue)
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
        
        int32_t computeQueueFamilyIndex = -1;
        for (uint32_t j = 0; j < queueFamilyCount; ++j) {
            if (queueFamilies[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                computeQueueFamilyIndex = static_cast<int32_t>(j);
                break;
            }
        }
        
        if (computeQueueFamilyIndex == -1) {
            std::cerr << "Device " << properties.deviceName << " has no compute queue!" << std::endl;
            continue;
        }
        
        DeviceInfo info;
        info.physicalDevice = physicalDevice;
        info.device = VK_NULL_HANDLE;  // Wird später erstellt
        info.queueFamilyIndex = static_cast<uint32_t>(computeQueueFamilyIndex);
        info.deviceId = properties.deviceID;
        info.name = properties.deviceName;
        info.memorySize = 0;  // Wird später abgefragt
        
        // Memory Informationen
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t m = 0; m < memProperties.memoryHeapCount; ++m) {
            if (memProperties.memoryHeaps[m].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                info.memorySize += memProperties.memoryHeaps[m].size;
            }
        }
        
        result.push_back(info);
    }
    
    return result;
}

bool VulkanContext::createDevice(const DeviceInfo& info) {
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = info.queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateDevice(info.physicalDevice, &createInfo, nullptr, &info.device) != VK_SUCCESS) {
        return false;
    }
    
    vkGetDeviceQueue(info.device, info.queueFamilyIndex, 0, &info.computeQueue);
    return true;
}

VkBuffer VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                    VkMemoryPropertyFlags properties, VkDevice device) {
    VkBuffer buffer;
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }
    
    return buffer;
}

VkDeviceMemory VulkanContext::allocateMemory(VkBuffer buffer, VkMemoryPropertyFlags properties,
                                            VkDevice device) {
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    
    // Memory Type finden
    VkMemoryPropertyFlags requiredProps = properties;
    VkPhysicalDeviceMemoryProperties memProperties;
    // Hinweis: Hier müsste das physische Device übergeben werden für korrekte Zuordnung
    // Vereinfachte Version für Demo
    
    VkDeviceMemory memory;
    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }
    
    return memory;
}

void VulkanContext::bindBuffer(VkBuffer buffer, VkDeviceMemory memory, VkDevice device) {
    vkBindBufferMemory(device, buffer, memory, 0);
}

VkCommandPool VulkanContext::createCommandPool(uint32_t queueFamilyIndex, VkDevice device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    
    VkCommandPool commandPool;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
    
    return commandPool;
}

VkCommandBuffer VulkanContext::beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void VulkanContext::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkDevice device) {
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    
    vkFreeCommandBuffers(device, VK_NULL_HANDLE, 1, &commandBuffer);
}

bool VulkanContext::createInstance() {
    if (enableValidationLayers) {
        // Validation Layers prüfen
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        
        bool allSupported = true;
        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                std::cerr << "Validation layer requested but not found: " << layerName << std::endl;
                allSupported = false;
            }
        }
        if (!allSupported) {
            std::cerr << "Not all validation layers are available!" << std::endl;
        }
    }
    
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Multi-GPU Path Tracer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "VKPT Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    
    // Extensions
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;
    
    #ifdef USE_GLFW
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    #endif
    
    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    
    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }
    
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        return false;
    }
    
    return true;
}

bool VulkanContext::setupDebugMessenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        if (func(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            return false;
        }
    }
    
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Vulkan Validation: " << pCallbackData->pMessage << std::endl;
    }
    
    return VK_FALSE;
}

} // namespace vkpt
