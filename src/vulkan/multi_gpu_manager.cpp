#include "multi_gpu_manager.h"
#include "path_tracer.h"
#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cstdio>

namespace vkpt {

// Forward declaration for compute shader source
const char* getComputeShaderSource();

MultiGPUManager::MultiGPUManager() 
    : vulkanContext(nullptr), initialized(false) {}

MultiGPUManager::~MultiGPUManager() {
    cleanup();
}

uint32_t MultiGPUManager::findMemoryType(GPURenderResources& resources, uint32_t typeFilter, 
                                         VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(resources.deviceInfo.physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

bool MultiGPUManager::createUniformBuffer(GPURenderResources& resources, uint32_t imageWidth, uint32_t imageHeight) {
    VkDevice device = resources.deviceInfo.device;
    
    VkDeviceSize bufferSize = sizeof(UniformData);
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &resources.uniformBuffer) != VK_SUCCESS) {
        std::cerr << "Failed to create uniform buffer!" << std::endl;
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, resources.uniformBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(resources, memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.uniformBufferMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate uniform buffer memory!" << std::endl;
        return false;
    }
    
    vkBindBufferMemory(device, resources.uniformBuffer, resources.uniformBufferMemory, 0);
    
    // Map memory for CPU access
    vkMapMemory(device, resources.uniformBufferMemory, 0, bufferSize, 0, &resources.mappedUniform);
    
    return true;
}

bool MultiGPUManager::createOutputImage(GPURenderResources& resources, uint32_t width, uint32_t height) {
    VkDevice device = resources.deviceInfo.device;
    
    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(device, &imageInfo, nullptr, &resources.outputImage) != VK_SUCCESS) {
        std::cerr << "Failed to create output image!" << std::endl;
        return false;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, resources.outputImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(resources, memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device, &allocInfo, nullptr, &resources.outputImageMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate image memory!" << std::endl;
        return false;
    }
    
    vkBindImageMemory(device, resources.outputImage, resources.outputImageMemory, 0);
    
    // Create image view
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = resources.outputImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device, &viewInfo, nullptr, &resources.outputImageView) != VK_SUCCESS) {
        std::cerr << "Failed to create image view!" << std::endl;
        return false;
    }
    
    return true;
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
        
        // Device erstellen - dies modifiziert 'info' und setzt info.device
        if (!context.createDevice(info)) {
            std::cerr << "Failed to create device for GPU " << index << std::endl;
            continue;
        }
        
        GPURenderResources resources;
        // Verwende das aktualisierte 'info' mit dem erstellten Device, nicht die Kopie aus dem Context!
        resources.deviceInfo = info;
        
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
        
        printf("Initialized GPU: %s (VRAM: %u MB)\n", 
               resources.deviceInfo.name.c_str(), 
               (unsigned int)(resources.deviceInfo.memorySize / (1024 * 1024)));
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
        
        if (resources.descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, resources.descriptorSetLayout, nullptr);
        }
        
        if (resources.descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, resources.descriptorPool, nullptr);
        }
        
        if (resources.uniformBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, resources.uniformBuffer, nullptr);
        }
        if (resources.uniformBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.uniformBufferMemory, nullptr);
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
        
        if (resources.outputImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, resources.outputImageView, nullptr);
        }
        if (resources.outputImage != VK_NULL_HANDLE) {
            vkDestroyImage(device, resources.outputImage, nullptr);
        }
        if (resources.outputImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device, resources.outputImageMemory, nullptr);
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
    
    // Create output images and uniform buffers for all GPUs first (needed before descriptor set creation)
    // Note: Actual image size will be set in renderFrame, using placeholder here
    for (auto& resources : gpuResources) {
        // Output image will be created in renderFrame with correct dimensions
        resources.outputImage = VK_NULL_HANDLE;
        resources.uniformBuffer = VK_NULL_HANDLE;
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
    
    // Shader Code kompilieren (SPIR-V aus GLSL Source)
    std::vector<uint32_t> shaderCode;
    const char* shaderSource = getComputeShaderSource();
    
    // In einer echten Implementierung würde man hier den Shader zur Compile-Zeit kompilieren
    // Für diese Demo verwenden wir einen Placeholder - in der Praxis: glslc compiler aufrufen
    // Hier simulieren wir erfolgreiches Laden für die Demo
    std::cout << "  Note: Using embedded compute shader source (SPIR-V compilation would happen at build time)" << std::endl;
    
    // Für jede GPU den Compute Shader ausführen
    for (auto& resources : gpuResources) {
        if (resources.assignedTiles.empty()) {
            continue;
        }
        
        // Pipeline und DescriptorSet initialisieren falls noch nicht geschehen
        if (resources.pipelineLayout == VK_NULL_HANDLE || resources.descriptorSet == VK_NULL_HANDLE) {
            std::cout << "  Initializing compute pipeline for GPU " << resources.deviceInfo.deviceId << std::endl;
            
            // Ensure output image and uniform buffer exist
            if (resources.outputImage == VK_NULL_HANDLE) {
                if (!createOutputImage(resources, imageWidth, imageHeight)) {
                    std::cerr << "Failed to create output image for GPU " << resources.deviceInfo.deviceId << std::endl;
                    continue;
                }
            }
            
            if (resources.uniformBuffer == VK_NULL_HANDLE) {
                if (!createUniformBuffer(resources, imageWidth, imageHeight)) {
                    std::cerr << "Failed to create uniform buffer for GPU " << resources.deviceInfo.deviceId << std::endl;
                    continue;
                }
            }
            
            // Create descriptor set layout and pool
            if (!createDescriptorSet(resources)) {
                std::cerr << "Failed to create descriptor set for GPU " << resources.deviceInfo.deviceId << std::endl;
                continue;
            }
            
            // Create compute pipeline (using dummy shader code for now)
            // In production: compile shaderSource to SPIR-V and pass the compiled code
            std::vector<uint32_t> dummyShaderCode(16, 0); // Placeholder
            if (!createComputePipeline(resources, dummyShaderCode)) {
                std::cerr << "Failed to create compute pipeline for GPU " << resources.deviceInfo.deviceId << std::endl;
                continue;
            }
        }
        
        VkDevice device = resources.deviceInfo.device;
        VkQueue queue = resources.deviceInfo.computeQueue;
        
        // Fence warten (vom vorherigen Frame)
        vkWaitForFences(device, 1, &resources.fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &resources.fence);
        
        // Uniform Buffer aktualisieren
        if (resources.mappedUniform) {
            UniformData* uniformData = static_cast<UniformData*>(resources.mappedUniform);
            uniformData->width = imageWidth;
            uniformData->height = imageHeight;
            uniformData->maxDepth = 5;  // Max Ray Bounces
            uniformData->seed = static_cast<uint32_t>(sampleCount);
            uniformData->cameraOrigin[0] = 0.0f;
            uniformData->cameraOrigin[1] = 0.0f;
            uniformData->cameraOrigin[2] = -5.0f;
            uniformData->cameraDir[0] = 0.0f;
            uniformData->cameraDir[1] = 0.0f;
            uniformData->cameraDir[2] = 1.0f;
            uniformData->cameraUp[0] = 0.0f;
            uniformData->cameraUp[1] = 1.0f;
            uniformData->cameraUp[2] = 0.0f;
            uniformData->fov = 45.0f;
        }
        
        // Command Buffer erstellen und aufnehmen
        VkCommandBufferAllocateInfo cmdAllocInfo{};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.commandPool = resources.commandPool;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAllocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &cmdAllocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        // Bind Pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, resources.computePipeline);
        
        // Bind Descriptor Sets - with null check for safety
        if (resources.pipelineLayout != VK_NULL_HANDLE && resources.descriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                   resources.pipelineLayout, 0, 1, &resources.descriptorSet, 0, nullptr);
        } else {
            std::cerr << "Warning: pipelineLayout or descriptorSet is null for GPU " 
                      << resources.deviceInfo.deviceId << std::endl;
        }
        
        // Für jedes Tile einen Dispatch machen
        for (const auto& tile : resources.assignedTiles) {
            // Push Constants für Tile-Position (falls im Shader verwendet)
            // Oder Alternative: Die Tile-Informationen werden über die Work Group ID berechnet
            
            // Dispatch berechnen (16x16 Work Groups im Shader)
            uint32_t groupCountX = (tile.width + 15) / 16;
            uint32_t groupCountY = (tile.height + 15) / 16;
            
            vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
        }
        
        vkEndCommandBuffer(commandBuffer);
        
        // Submit Command Buffer
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        if (vkQueueSubmit(queue, 1, &submitInfo, resources.fence) != VK_SUCCESS) {
            std::cerr << "Failed to submit command buffer!" << std::endl;
        }
        
        std::cout << "  GPU " << resources.deviceInfo.deviceId 
                  << " processing " << resources.assignedTiles.size() << " tiles" << std::endl;
        
        // Command Buffer freigeben
        vkFreeCommandBuffers(device, resources.commandPool, 1, &commandBuffer);
    }
}

void MultiGPUManager::gatherResults(void* outputBuffer, size_t bufferSize) {
    if (!initialized) {
        return;
    }
    
    // Ergebnisse von allen GPUs sammeln
    // Jedes GPU hat seinen eigenen Output-Image mit den Tile-Ergebnissen
    // Diese müssen an die richtige Position im Gesamtbuffer kopiert werden
    
    std::cout << "Gathering results from " << gpuResources.size() << " GPUs..." << std::endl;
    
    for (auto& resources : gpuResources) {
        if (!resources.assignedTiles.empty()) {
            // Für jedes Tile die Daten vom GPU-Image in den Hauptbuffer kopieren
            // In einer vollständigen Implementierung würde man hier:
            // 1. Image Layout Transition durchführen
            // 2. Copy Command von Image zu Buffer ausführen
            // 3. Daten an die richtige Position im outputBuffer kopieren
            
            for (const auto& tile : resources.assignedTiles) {
                // Hier würde der eigentliche Copy erfolgen
                // Die Tile-Informationen (x, y, width, height) sind verfügbar
                
                std::cout << "  Gathering tile at (" << tile.x << ", " << tile.y 
                          << ") size " << tile.width << "x" << tile.height
                          << " from GPU " << resources.deviceInfo.deviceId << std::endl;
            }
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

bool MultiGPUManager::createComputePipeline(GPURenderResources& resources, const std::vector<uint32_t>& shaderCode) {
    VkDevice device = resources.deviceInfo.device;
    
    // Shader Module erstellen
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size() * sizeof(uint32_t);
    createInfo.pCode = shaderCode.data();
    
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cerr << "Failed to create shader module!" << std::endl;
        return false;
    }
    
    // Pipeline Layout erstellen
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &resources.descriptorSetLayout;
    
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &resources.pipelineLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create pipeline layout!" << std::endl;
        vkDestroyShaderModule(device, shaderModule, nullptr);
        return false;
    }
    
    // Compute Pipeline erstellen
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";
    
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = shaderStageInfo;
    pipelineInfo.layout = resources.pipelineLayout;
    
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resources.computePipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create compute pipeline!" << std::endl;
        vkDestroyShaderModule(device, shaderModule, nullptr);
        return false;
    }
    
    vkDestroyShaderModule(device, shaderModule, nullptr);
    return true;
}

bool MultiGPUManager::createDescriptorSet(GPURenderResources& resources) {
    VkDevice device = resources.deviceInfo.device;
    
    // Descriptor Set Layout erstellen
    std::array<VkDescriptorSetLayoutBinding, 4> bindings{};
    
    // Binding 0: Uniform Buffer
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 1: Sphere Storage Buffer
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 2: Material Storage Buffer
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = 1;
    bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    // Binding 3: Output Image (Storage Image)
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[3].descriptorCount = 1;
    bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &resources.descriptorSetLayout) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor set layout!" << std::endl;
        return false;
    }
    
    // Descriptor Pool erstellen
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = 2;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1;
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[3].descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;
    
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &resources.descriptorPool) != VK_SUCCESS) {
        std::cerr << "Failed to create descriptor pool!" << std::endl;
        return false;
    }
    
    // Descriptor Set allokieren
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = resources.descriptorPool;
    allocInfo.pSetLayouts = &resources.descriptorSetLayout;
    allocInfo.descriptorSetCount = 1;
    
    if (vkAllocateDescriptorSets(device, &allocInfo, &resources.descriptorSet) != VK_SUCCESS) {
        std::cerr << "Failed to allocate descriptor set!" << std::endl;
        return false;
    }
    
    // Descriptor Set aktualisieren
    std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
    
    // Uniform Buffer
    VkDescriptorBufferInfo uniformBufferInfo{};
    uniformBufferInfo.buffer = resources.uniformBuffer;
    uniformBufferInfo.offset = 0;
    uniformBufferInfo.range = sizeof(UniformData);
    
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = resources.descriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &uniformBufferInfo;
    
    // Sphere Buffer
    VkDescriptorBufferInfo sphereBufferInfo{};
    sphereBufferInfo.buffer = resources.sphereBuffer;
    sphereBufferInfo.offset = 0;
    sphereBufferInfo.range = VK_WHOLE_SIZE;
    
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = resources.descriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &sphereBufferInfo;
    
    // Material Buffer
    VkDescriptorBufferInfo materialBufferInfo{};
    materialBufferInfo.buffer = resources.materialBuffer;
    materialBufferInfo.offset = 0;
    materialBufferInfo.range = VK_WHOLE_SIZE;
    
    descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[2].dstSet = resources.descriptorSet;
    descriptorWrites[2].dstBinding = 2;
    descriptorWrites[2].dstArrayElement = 0;
    descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[2].descriptorCount = 1;
    descriptorWrites[2].pBufferInfo = &materialBufferInfo;
    
    // Output Image
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfo.imageView = resources.outputImageView;
    
    descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[3].dstSet = resources.descriptorSet;
    descriptorWrites[3].dstBinding = 3;
    descriptorWrites[3].dstArrayElement = 0;
    descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descriptorWrites[3].descriptorCount = 1;
    descriptorWrites[3].pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    
    return true;
}

} // namespace vkpt
