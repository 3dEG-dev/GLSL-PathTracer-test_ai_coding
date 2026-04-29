#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <cstdint>

namespace vkpt {

// Einfache Vector Strukturen
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct Vec4 {
    float x, y, z, w;
    Vec4() : x(0), y(0), z(0), w(0) {}
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

// Ray Struktur für Path Tracing
struct Ray {
    Vec3 origin;
    Vec3 direction;
    float tmin;
    float tmax;
};

// Hit Record
struct HitRecord {
    float t;
    Vec3 normal;
    uint32_t materialIndex;
    bool hit;
};

// Material Typen
enum class MaterialType {
    Diffuse,
    Metal,
    Dielectric,
    Emissive
};

// Material Struktur
struct Material {
    MaterialType type;
    Vec3 albedo;      // Basisfarbe
    Vec3 emission;    // Selbstleuchtend
    float roughness;  // Rauheit (0 = perfekt reflektierend)
    float ior;        // Brechungsindex für Dielektrika
};

// Geometrie (einfache Sphären als Beispiel)
struct Sphere {
    Vec3 center;
    float radius;
    uint32_t materialIndex;
};

// GPU Device Informationen
struct DeviceInfo {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue computeQueue;
    uint32_t queueFamilyIndex;
    uint32_t deviceId;
    std::string name;
    uint64_t memorySize;  // Geschätzter VRAM
    
    DeviceInfo() : physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE), 
                   computeQueue(VK_NULL_HANDLE), queueFamilyIndex(0), 
                   deviceId(0), memorySize(0) {}
};

} // namespace vkpt
