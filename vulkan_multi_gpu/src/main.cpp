#include "path_tracer.h"
#include "vulkan_context.h"
#include "multi_gpu_manager.h"
#include "scene.h"
#include <iostream>
#include <fstream>
#include <cstring>

using namespace vkpt;

// Hilfsfunktion: GLSL zu SPIR-V kompilieren (normalerweise mit glslc)
// Hier nur ein Platzhalter - in der Praxis würde man den Shader zur Compile-Zeit kompilieren
std::vector<uint32_t> loadShaderCode(const char* filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return {};
    }
    
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
    file.close();
    
    return buffer;
}

int main(int argc, char** argv) {
    std::cout << "=== Vulkan Multi-GPU Path Tracer ===" << std::endl;
    std::cout << "Unterstützung für:" << std::endl;
    std::cout << "  - Explizite Multi-GPU Steuerung" << std::endl;
    std::cout << "  - VRAM-Verteilung über mehrere GPUs" << std::endl;
    std::cout << "  - Tile-basiertes Rendering" << std::endl;
    std::cout << std::endl;
    
    // Vulkan Context initialisieren
    VulkanContext context;
    
    bool enableValidation = false;
#ifdef NDEBUG
    enableValidation = false;
#else
    enableValidation = true;
#endif
    
    if (!context.initialize(enableValidation)) {
        std::cerr << "Failed to initialize Vulkan!" << std::endl;
        return -1;
    }
    
    const auto& devices = context.getDevices();
    if (devices.empty()) {
        std::cerr << "No Vulkan devices found!" << std::endl;
        return -1;
    }
    
    std::cout << "\nVerfügbare GPUs:" << std::endl;
    for (size_t i = 0; i < devices.size(); ++i) {
        std::cout << "  [" << i << "] " << devices[i].name 
                  << " (VRAM: " << (devices[i].memorySize / (1024 * 1024)) << " MB)" << std::endl;
    }
    std::cout << std::endl;
    
    // Multi-GPU Manager initialisieren (alle verfügbaren GPUs verwenden)
    MultiGPUManager gpuManager;
    
    std::vector<uint32_t> deviceIndices;
    // Alle GPUs verwenden oder spezifische auswählen
    // deviceIndices.push_back(0);  // Nur GPU 0
    // deviceIndices.push_back(1);  // Nur GPU 1
    
    if (!gpuManager.initialize(context, deviceIndices)) {
        std::cerr << "Failed to initialize Multi-GPU Manager!" << std::endl;
        return -1;
    }
    
    std::cout << "\nMulti-GPU Manager initialisiert mit " << gpuManager.getGPUCount() << " GPU(s)" << std::endl;
    
    // Szene erstellen
    Scene scene;
    scene.createDefaultScene();
    
    std::cout << "Szene erstellt: " << scene.getSpheres().size() << " Sphären, " 
              << scene.getMaterials().size() << " Materialien" << std::endl;
    
    // Szene auf GPUs verteilen
    bool distributeData = (gpuManager.getGPUCount() > 1);
    if (!gpuManager.setupScene(scene.getSpheres(), scene.getMaterials(), distributeData)) {
        std::cerr << "Failed to setup scene on GPUs!" << std::endl;
        return -1;
    }
    
    // Render-Parameter
    uint32_t imageWidth = 1920;
    uint32_t imageHeight = 1080;
    uint32_t tileSize = 256;
    int samplesPerPixel = 64;
    
    std::cout << "\nRendering-Konfiguration:" << std::endl;
    std::cout << "  Auflösung: " << imageWidth << "x" << imageHeight << std::endl;
    std::cout << "  Tile-Größe: " << tileSize << "x" << tileSize << std::endl;
    std::cout << "  Samples pro Pixel: " << samplesPerPixel << std::endl;
    std::cout << std::endl;
    
    // Tiles zuweisen
    gpuManager.assignTiles(imageWidth, imageHeight, tileSize);
    
    // Frame rendern
    gpuManager.renderFrame(imageWidth, imageHeight, samplesPerPixel);
    
    // Auf alle GPUs warten
    gpuManager.waitForAllGPUs();
    
    // Ergebnisse sammeln
    std::vector<float> outputBuffer(imageWidth * imageHeight * 4);
    gpuManager.gatherResults(outputBuffer.data(), outputBuffer.size() * sizeof(float));
    
    std::cout << "\nRendering abgeschlossen!" << std::endl;
    std::cout << "Hinweis: Dies ist ein Grundgerüst. Für vollständiges Rendering müssen:" << std::endl;
    std::cout << "  1. Compute Pipeline erstellt werden" << std::endl;
    std::cout << "  2. Descriptor Sets konfiguriert werden" << std::endl;
    std::cout << "  3. Eigentliche Dispatch Calls implementiert werden" << std::endl;
    std::cout << "  4. Shader zu SPIR-V kompiliert werden" << std::endl;
    std::cout << std::endl;
    
    // Cleanup
    gpuManager.cleanup();
    context.cleanup();
    
    std::cout << "Programm erfolgreich beendet." << std::endl;
    
    return 0;
}
