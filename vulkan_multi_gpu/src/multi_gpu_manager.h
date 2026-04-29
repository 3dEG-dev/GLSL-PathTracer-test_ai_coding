#pragma once

#include "types.h"
#include "vulkan_context.h"
#include <vector>
#include <memory>
#include <functional>
#include <array>

namespace vkpt {

// Tile-Information für Multi-GPU Rendering
struct RenderTile {
    uint32_t x;          // Start X Position
    uint32_t y;          // Start Y Position
    uint32_t width;      // Tile Breite
    uint32_t height;     // Tile Höhe
    uint32_t gpuIndex;   // Zugewiesene GPU
    bool completed;      // Fertigstellungsstatus
};

// Uniform Buffer Struktur für den Compute Shader
struct UniformData {
    uint32_t width;
    uint32_t height;
    uint32_t maxDepth;
    uint32_t seed;
    float cameraOrigin[3];
    float cameraDir[3];
    float cameraUp[3];
    float fov;
};

// Per-GPU Ressourcen
struct GPURenderResources {
    DeviceInfo deviceInfo;  // Copy instead of pointer to avoid const issues
    
    // Uniform Buffer
    VkBuffer uniformBuffer;
    VkDeviceMemory uniformBufferMemory;
    void* mappedUniform;
    
    // Buffers für Scene Data (gestriped)
    VkBuffer sphereBuffer;
    VkDeviceMemory sphereBufferMemory;
    VkBuffer materialBuffer;
    VkDeviceMemory materialBufferMemory;
    
    // Output Buffer (Tile Ergebnis) - als Image für den Shader
    VkImage outputImage;
    VkDeviceMemory outputImageMemory;
    VkImageView outputImageView;
    
    // Compute Pipeline
    VkPipeline computePipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    
    // Command Pool & Buffer
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    
    // Fence für Synchronisation
    VkFence fence;
    
    // Tile Informationen
    std::vector<RenderTile> assignedTiles;
    
    GPURenderResources() 
        : uniformBuffer(VK_NULL_HANDLE), uniformBufferMemory(VK_NULL_HANDLE),
          mappedUniform(nullptr),
          sphereBuffer(VK_NULL_HANDLE), sphereBufferMemory(VK_NULL_HANDLE),
          materialBuffer(VK_NULL_HANDLE), materialBufferMemory(VK_NULL_HANDLE),
          outputImage(VK_NULL_HANDLE), outputImageMemory(VK_NULL_HANDLE),
          outputImageView(VK_NULL_HANDLE),
          computePipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE),
          descriptorSet(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
          descriptorSetLayout(VK_NULL_HANDLE),
          commandPool(VK_NULL_HANDLE), fence(VK_NULL_HANDLE) {}
};

class MultiGPUManager {
public:
    MultiGPUManager();
    ~MultiGPUManager();
    
    // Initialisierung mit Vulkan Context
    bool initialize(VulkanContext& context, const std::vector<uint32_t>& deviceIndices);
    void cleanup();
    
    // Scene Setup mit VRAM-Verteilung
    bool setupScene(const std::vector<Sphere>& spheres, 
                   const std::vector<Material>& materials,
                   bool distributeAcrossGPUs = true);
    
    // Render Tile Zuweisung
    void assignTiles(uint32_t imageWidth, uint32_t imageHeight, uint32_t tileSize = 256);
    
    // Rendering ausführen
    void renderFrame(uint32_t imageWidth, uint32_t imageHeight, int sampleCount);
    
    // Ergebnisse sammeln
    void gatherResults(void* outputBuffer, size_t bufferSize);
    
    // Wait für alle GPUs
    void waitForAllGPUs();
    
    // Getter
    size_t getGPUCount() const { return gpuResources.size(); }
    bool isInitialized() const { return initialized; }
    
private:
    VulkanContext* vulkanContext;
    std::vector<GPURenderResources> gpuResources;
    bool initialized;
    
    // Helper Funktionen
    bool createComputePipeline(GPURenderResources& resources, const std::vector<uint32_t>& shaderCode);
    bool createDescriptorSet(GPURenderResources& resources);
    bool createUniformBuffer(GPURenderResources& resources, uint32_t imageWidth, uint32_t imageHeight);
    bool createOutputImage(GPURenderResources& resources, uint32_t width, uint32_t height);
    void distributeSceneData(const std::vector<Sphere>& spheres,
                            const std::vector<Material>& materials);
    uint32_t findMemoryType(GPURenderResources& resources, uint32_t typeFilter, 
                           VkMemoryPropertyFlags properties);
};

} // namespace vkpt
