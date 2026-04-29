#pragma once

#include "types.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>

namespace vkpt {

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    // Initialisierung
    bool initialize(bool enableValidation = false);
    void cleanup();

    // Multi-GPU Unterstützung
    std::vector<DeviceInfo> enumerateDevices();
    bool createDevice(DeviceInfo& info);
    
    // Compute Pipeline
    bool createComputePipeline(const std::vector<uint32_t>& shaderCode);
    
    // Buffer Erstellung
    VkBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, 
                         VkMemoryPropertyFlags properties, VkDevice device);
    VkDeviceMemory allocateMemory(VkBuffer buffer, VkMemoryPropertyFlags properties,
                                 VkDevice device);
    void bindBuffer(VkBuffer buffer, VkDeviceMemory memory, VkDevice device);
    
    // Command Buffer
    VkCommandPool createCommandPool(uint32_t queueFamilyIndex, VkDevice device);
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device);
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkDevice device);
    
    // Getter
    VkInstance getInstance() const { return instance; }
    const std::vector<DeviceInfo>& getDevices() const { return devices; }
    
private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    std::vector<DeviceInfo> devices;
    bool validationEnabled;
    
    // Helper Funktionen
    bool createInstance();
    bool setupDebugMessenger();
    bool selectPhysicalDevices();
    
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

} // namespace vkpt
