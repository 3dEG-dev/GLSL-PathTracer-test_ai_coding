#pragma once

#include "types.h"
#include "vulkan_context.h"
#include <vector>
#include <memory>
#include <functional>

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

// Per-GPU Ressourcen
struct GPURenderResources {
    DeviceInfo* deviceInfo;
    
    // Buffers für Scene Data (gestriped)
    VkBuffer sphereBuffer;
    VkDeviceMemory sphereBufferMemory;
    VkBuffer materialBuffer;
    VkDeviceMemory materialBufferMemory;
    
    // Output Buffer (Tile Ergebnis)
    VkBuffer outputBuffer;
    VkDeviceMemory outputBufferMemory;
    void* mappedOutput;  // CPU-zugänglicher Pointer
    
    // Compute Pipeline
    VkPipeline computePipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSet descriptorSet;
    VkDescriptorPool descriptorPool;
    
    // Command Pool & Buffer
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    
    // Fence für Synchronisation
    VkFence fence;
    
    // Tile Informationen
    std::vector<RenderTile> assignedTiles;
    
    GPURenderResources() 
        : deviceInfo(nullptr), 
          sphereBuffer(VK_NULL_HANDLE), sphereBufferMemory(VK_NULL_HANDLE),
          materialBuffer(VK_NULL_HANDLE), materialBufferMemory(VK_NULL_HANDLE),
          outputBuffer(VK_NULL_HANDLE), outputBufferMemory(VK_NULL_HANDLE),
          mappedOutput(nullptr),
          computePipeline(VK_NULL_HANDLE), pipelineLayout(VK_NULL_HANDLE),
          descriptorSet(VK_NULL_HANDLE), descriptorPool(VK_NULL_HANDLE),
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
    void distributeSceneData(const std::vector<Sphere>& spheres,
                            const std::vector<Material>& materials);
};

} // namespace vkpt
