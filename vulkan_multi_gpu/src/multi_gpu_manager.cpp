#include "multi_gpu_manager.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace vkpt {

MultiGPUManager::MultiGPUManager() 
    : vulkanContext(nullptr), initialized(false) {}

MultiGPUManager::~MultiGPUManager() {
    cleanup();
}

bool MultiGPUManager::initialize(VulkanContext& context, const std::vector<uint32_t>& deviceIndices) {
    vulkanContext = &context;
    
    const auto& availableDevices = context.getDevices();
    
    // Lokale Kopie erstellen für die Geräteauswahl
    std::vector<uint32_t> selectedIndices = deviceIndices;
    
    if (selectedIndices.empty()) {
        std::cout << "Using all available GPUs..." << std::endl;
        for (uint32_t i = 0; i < availableDevices.size(); ++i) {
            selectedIndices.push_back(i);
        }
    }
    
    // Geräte initialisieren
    for (uint32_t index : selectedIndices) {
        if (index >= availableDevices.size()) {
            std::cerr << "Device index " << index << " out of range! Available devices: " 
                      << availableDevices.size() << std::endl;
            continue;
        }
        
        DeviceInfo info = availableDevices[index];
        
        // Device erstellen
        if (!context.createDevice(info)) {
            std::cerr << "Failed to create device for GPU " << index << std::endl;
            continue;
        }
        
        GPURenderResources resources;
        resources.deviceInfo = vulkanContext->getDevices()[index];  // Copy instead of pointer
        
        // Command Pool erstellen
        resources.commandPool = context.createCommandPool(
            resources.deviceInfo.queueFamilyIndex, 
            resources.deviceInfo.device);
        
        // Fence erstellen
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled
        if (vkCreateFence(resources.deviceInfo.device, &fenceInfo, nullptr, &resources.fence) != VK_SUCCESS) {
            std::cerr << "Failed to create fence!" << std::endl;
            continue;
        }
        
        gpuResources.push_back(std::move(resources));
        
        std::cout << "Initialized GPU: " << resources.deviceInfo.name 
                  << " (VRAM: " << (resources.deviceInfo.memorySize / (1024 * 1024)) << " MB)" << std::endl;
    }
    
    if (gpuResources.empty()) {
        std::cerr << "No GPUs could be initialized!" << std::endl;
        return false;
    }
    
    initialized = true;
    return true;
}

void MultiGPUManager::cleanup() {
    for (auto& resources : gpuResources) {
        VkDevice device = resources.deviceInfo.device;
        
        // Resources freigeben
        if (resources.fence != VK_NULL_HANDLE) {
            vkDestroyFence(device, resources.fence, nullptr);
        }
        
        if (resources.computePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, resources.computePipeline, nullptr);
        }
        
        if (resources.pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, resources.pipelineLayout, nullptr);
        }
        
        if (resources.descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, resources.descriptorPool, nullptr);
        }
        
        if (resources.sphereBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, resources.sphereBuffer, nullptr);
        }
        if (resources.sphereBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.sphereBufferMemory, nullptr);
        }
        
        if (resources.materialBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, resources.materialBuffer, nullptr);
        }
        if (resources.materialBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.materialBufferMemory, nullptr);
        }
        
        if (resources.outputBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, resources.outputBuffer, nullptr);
        }
        if (resources.outputBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.outputBufferMemory, nullptr);
        }
        
        if (resources.commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, resources.commandPool, nullptr);
        }
    }
    
    gpuResources.clear();
    vulkanContext = nullptr;
    initialized = false;
}

bool MultiGPUManager::setupScene(const std::vector<Sphere>& spheres, 
                                const std::vector<Material>& materials,
                                bool distributeAcrossGPUs) {
    if (!initialized) {
        std::cerr << "MultiGPUManager not initialized!" << std::endl;
        return false;
    }
    
    if (distributeAcrossGPUs && gpuResources.size() > 1) {
        distributeSceneData(spheres, materials);
    } else {
        // Alle Daten auf jede GPU kopieren (einfacher, aber mehr VRAM)
        for (auto& resources : gpuResources) {
            VkDevice device = resources.deviceInfo.device;
            
            // Sphere Buffer
            VkDeviceSize sphereBufferSize = spheres.size() * sizeof(Sphere);
            if (sphereBufferSize > 0) {
                resources.sphereBuffer = vulkanContext->createBuffer(
                    sphereBufferSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    device);
                
                void* data;
                vkMapMemory(device, resources.sphereBufferMemory, 0, sphereBufferSize, 0, &data);
                memcpy(data, spheres.data(), sphereBufferSize);
                vkUnmapMemory(device, resources.sphereBufferMemory);
            }
            
            // Material Buffer
            VkDeviceSize materialBufferSize = materials.size() * sizeof(Material);
            if (materialBufferSize > 0) {
                resources.materialBuffer = vulkanContext->createBuffer(
                    materialBufferSize,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    device);
                
                void* data;
                vkMapMemory(device, resources.materialBufferMemory, 0, materialBufferSize, 0, &data);
                memcpy(data, materials.data(), materialBufferSize);
                vkUnmapMemory(device, resources.materialBufferMemory);
            }
        }
    }
    
    std::cout << "Scene setup complete on " << gpuResources.size() << " GPU(s)" << std::endl;
    return true;
}

void MultiGPUManager::distributeSceneData(const std::vector<Sphere>& spheres,
                                         const std::vector<Material>& materials) {
    std::cout << "Distributing scene data across " << gpuResources.size() << " GPUs..." << std::endl;
    
    size_t totalSpheres = spheres.size();
    size_t spheresPerGPU = (totalSpheres + gpuResources.size() - 1) / gpuResources.size();
    
    for (size_t gpuIdx = 0; gpuIdx < gpuResources.size(); ++gpuIdx) {
        auto& resources = gpuResources[gpuIdx];
        VkDevice device = resources.deviceInfo.device;
        
        // Berechnen welcher Bereich dieser GPU zugewiesen wird
        size_t startIdx = gpuIdx * spheresPerGPU;
        size_t endIdx = std::min(startIdx + spheresPerGPU, totalSpheres);
        size_t localSphereCount = endIdx - startIdx;
        
        std::cout << "  GPU " << gpuIdx << ": Spheres [" << startIdx << " - " << endIdx << "]" << std::endl;
        
        // Subset der Sphären für diese GPU
        std::vector<Sphere> localSpheres;
        if (localSphereCount > 0 && startIdx < totalSpheres) {
            localSpheres.assign(spheres.begin() + startIdx, spheres.begin() + endIdx);
        }
        
        // Sphere Buffer erstellen
        VkDeviceSize sphereBufferSize = localSpheres.size() * sizeof(Sphere);
        if (sphereBufferSize > 0) {
            resources.sphereBuffer = vulkanContext->createBuffer(
                sphereBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                device);
            
            // Memory allozieren und binden (vereinfacht)
            // In Produktion: findMemoryType mit proper memory type index
            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(device, resources.sphereBuffer, &memReq);
            
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;
            
            // Host-visible memory finden
            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(resources.deviceInfo.physicalDevice, &memProps);
            
            uint32_t memoryTypeIndex = 0;
            for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
                if ((memReq.memoryTypeBits & (1 << i)) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    memoryTypeIndex = i;
                    break;
                }
            }
            allocInfo.memoryTypeIndex = memoryTypeIndex;
            
            if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.sphereBufferMemory) == VK_SUCCESS) {
                vkBindBufferMemory(device, resources.sphereBuffer, resources.sphereBufferMemory, 0);
                
                void* data;
                vkMapMemory(device, resources.sphereBufferMemory, 0, sphereBufferSize, 0, &data);
                memcpy(data, localSpheres.data(), sphereBufferSize);
                vkUnmapMemory(device, resources.sphereBufferMemory);
            }
        }
        
        // Materialien werden auf alle GPUs kopiert (da Referenzen von allen Sphären genutzt werden)
        VkDeviceSize materialBufferSize = materials.size() * sizeof(Material);
        if (materialBufferSize > 0) {
            resources.materialBuffer = vulkanContext->createBuffer(
                materialBufferSize,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                device);
            
            VkMemoryRequirements memReq;
            vkGetBufferMemoryRequirements(device, resources.materialBuffer, &memReq);
            
            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReq.size;
            
            VkPhysicalDeviceMemoryProperties memProps;
            vkGetPhysicalDeviceMemoryProperties(resources.deviceInfo.physicalDevice, &memProps);
            
            uint32_t memoryTypeIndex = 0;
            for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
                if ((memReq.memoryTypeBits & (1 << i)) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
                    (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
                    memoryTypeIndex = i;
                    break;
                }
            }
            allocInfo.memoryTypeIndex = memoryTypeIndex;
            
            if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.materialBufferMemory) == VK_SUCCESS) {
                vkBindBufferMemory(device, resources.materialBuffer, resources.materialBufferMemory, 0);
                
                void* data;
                vkMapMemory(device, resources.materialBufferMemory, 0, materialBufferSize, 0, &data);
                memcpy(data, materials.data(), materialBufferSize);
                vkUnmapMemory(device, resources.materialBufferMemory);
            }
        }
    }
}

void MultiGPUManager::assignTiles(uint32_t imageWidth, uint32_t imageHeight, uint32_t tileSize) {
    if (!initialized || gpuResources.empty()) {
        return;
    }
    
    // Tiles berechnen
    uint32_t tilesX = (imageWidth + tileSize - 1) / tileSize;
    uint32_t tilesY = (imageHeight + tileSize - 1) / tileSize;
    uint32_t totalTiles = tilesX * tilesY;
    
    std::cout << "Assigning " << totalTiles << " tiles (" << tilesX << "x" << tilesY 
              << ") to " << gpuResources.size() << " GPUs..." << std::endl;
    
    // Clear previous assignments
    for (auto& resources : gpuResources) {
        resources.assignedTiles.clear();
    }
    
    // Round-Robin Zuweisung der Tiles an GPUs
    uint32_t tileIndex = 0;
    for (uint32_t y = 0; y < tilesY; ++y) {
        for (uint32_t x = 0; x < tilesX; ++x) {
            uint32_t gpuIdx = tileIndex % gpuResources.size();
            
            RenderTile tile;
            tile.x = x * tileSize;
            tile.y = y * tileSize;
            tile.width = std::min(tileSize, imageWidth - tile.x);
            tile.height = std::min(tileSize, imageHeight - tile.y);
            tile.gpuIndex = gpuIdx;
            tile.completed = false;
            
            gpuResources[gpuIdx].assignedTiles.push_back(tile);
            
            tileIndex++;
        }
    }
    
    // Statistik
    for (size_t i = 0; i < gpuResources.size(); ++i) {
        std::cout << "  GPU " << i << ": " << gpuResources[i].assignedTiles.size() << " tiles" << std::endl;
    }
}

void MultiGPUManager::renderFrame(uint32_t imageWidth, uint32_t imageHeight, int sampleCount) {
    if (!initialized) {
        return;
    }
    
    std::cout << "Rendering frame (" << imageWidth << "x" << imageHeight 
              << ", " << sampleCount << " samples) on " << gpuResources.size() << " GPUs..." << std::endl;
    
    // Für jede GPU den Compute Shader ausführen
    for (auto& resources : gpuResources) {
        if (resources.assignedTiles.empty()) {
            continue;
        }
        
        VkDevice device = resources.deviceInfo.device;
        VkQueue queue = resources.deviceInfo.computeQueue;
        
        // Fence warten (vom vorherigen Frame)
        vkWaitForFences(device, 1, &resources.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &resources.fence);
        
        // Command Buffer aufnehmen und ausführen für jedes Tile
        // (Vereinfacht: hier würde man den Compute Pipeline Dispatch machen)
        
        std::cout << "  GPU " << resources.deviceInfo.deviceId 
                  << " processing " << resources.assignedTiles.size() << " tiles" << std::endl;
        
        // Hier würde der eigentliche Compute Dispatch erfolgen:
        // vkCmdBindPipeline, vkCmdBindDescriptorSets, vkCmdDispatch, etc.
    }
}

void MultiGPUManager::gatherResults(void* outputBuffer, size_t bufferSize) {
    if (!initialized) {
        return;
    }
    
    // Ergebnisse von allen GPUs sammeln
    // Jedes GPU hat seinen eigenen Output-Buffer mit den Tile-Ergebnissen
    // Diese müssen an die richtige Position im Gesamtbuffer kopiert werden
    
    std::cout << "Gathering results from " << gpuResources.size() << " GPUs..." << std::endl;
    
    for (auto& resources : gpuResources) {
        if (resources.mappedOutput && !resources.assignedTiles.empty()) {
            // Daten von dieser GPU in den Hauptbuffer kopieren
            // Position berechnen basierend auf Tile-Offsets
            // (Implementierung abhängig von Buffer-Layout)
        }
    }
}

void MultiGPUManager::waitForAllGPUs() {
    if (!initialized) {
        return;
    }
    
    for (auto& resources : gpuResources) {
        VkDevice device = resources.deviceInfo.device;
        vkWaitForFences(device, 1, &resources.fence, VK_TRUE, UINT64_MAX);
    }
}

} // namespace vkpt
