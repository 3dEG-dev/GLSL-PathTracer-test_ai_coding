#pragma once

#include "types.h"
#include <vector>
#include <random>

namespace vkpt {

class Scene {
public:
    Scene();
    
    // Standard Test-Szene erstellen (Cornell Box-ähnlich)
    void createDefaultScene();
    
    // Benutzerdefinierte Szene
    void addSphere(const Vec3& center, float radius, uint32_t materialIndex);
    void addMaterial(const Material& material);
    
    // Getter
    const std::vector<Sphere>& getSpheres() const { return spheres; }
    const std::vector<Material>& getMaterials() const { return materials; }
    
    // Camera
    void setCamera(const Vec3& origin, const Vec3& target, float fov = 90.0f);
    Vec3 getCameraOrigin() const { return cameraOrigin; }
    Vec3 getCameraDirection() const { return cameraDirection; }
    
private:
    std::vector<Sphere> spheres;
    std::vector<Material> materials;
    
    Vec3 cameraOrigin;
    Vec3 cameraDirection;
    Vec3 cameraUp;
    float fovY;
    
    std::mt19937 rng;
};

} // namespace vkpt
