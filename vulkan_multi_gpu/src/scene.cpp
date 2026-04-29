#include "scene.h"
#include <cmath>

namespace vkpt {

Scene::Scene() 
    : cameraOrigin(0, 0, 0), cameraDirection(0, 0, -1), cameraUp(0, 1, 0), fovY(90.0f) {}

void Scene::createDefaultScene() {
    spheres.clear();
    materials.clear();
    
    // Materialien erstellen
    Material emissive;
    emissive.type = MaterialType::Emissive;
    emissive.albedo = Vec3(0, 0, 0);
    emissive.emission = Vec3(20.0f, 20.0f, 20.0f);  // Helles Weiß
    emissive.roughness = 0.0f;
    emissive.ior = 1.0f;
    addMaterial(emissive);
    
    Material redDiffuse;
    redDiffuse.type = MaterialType::Diffuse;
    redDiffuse.albedo = Vec3(0.65f, 0.05f, 0.05f);
    redDiffuse.emission = Vec3(0, 0, 0);
    redDiffuse.roughness = 1.0f;
    redDiffuse.ior = 1.0f;
    addMaterial(redDiffuse);
    
    Material greenDiffuse;
    greenDiffuse.type = MaterialType::Diffuse;
    greenDiffuse.albedo = Vec3(0.05f, 0.65f, 0.05f);
    greenDiffuse.emission = Vec3(0, 0, 0);
    greenDiffuse.roughness = 1.0f;
    greenDiffuse.ior = 1.0f;
    addMaterial(greenDiffuse);
    
    Material whiteDiffuse;
    whiteDiffuse.type = MaterialType::Diffuse;
    whiteDiffuse.albedo = Vec3(0.73f, 0.73f, 0.73f);
    whiteDiffuse.emission = Vec3(0, 0, 0);
    whiteDiffuse.roughness = 1.0f;
    whiteDiffuse.ior = 1.0f;
    addMaterial(whiteDiffuse);
    
    Material metal;
    metal.type = MaterialType::Metal;
    metal.albedo = Vec3(0.8f, 0.8f, 0.8f);
    metal.emission = Vec3(0, 0, 0);
    metal.roughness = 0.0f;
    metal.ior = 1.0f;
    addMaterial(metal);
    
    Material dielectric;
    dielectric.type = MaterialType::Dielectric;
    dielectric.albedo = Vec3(1.0f, 1.0f, 1.0f);
    dielectric.emission = Vec3(0, 0, 0);
    dielectric.roughness = 0.0f;
    dielectric.ior = 1.5f;  // Glas
    addMaterial(dielectric);
    
    // Cornell Box-ähnliche Szene mit Sphären
    
    // Licht (oben)
    addSphere(Vec3(0, 100, 0), 50, 0);  // Emissive
    
    // Rote Wand (links)
    addSphere(Vec3(-50, 0, 0), 45, 1);  // Rot
    
    // Grüne Wand (rechts)
    addSphere(Vec3(50, 0, 0), 45, 2);   // Grün
    
    // Weiße Wände (hinten, oben, unten)
    addSphere(Vec3(0, 0, -100), 95, 3); // Hinten
    addSphere(Vec3(0, 50, 0), 45, 3);   // Oben
    addSphere(Vec3(0, -50, 0), 45, 3);  // Unten
    
    // Zentrale Sphären
    addSphere(Vec3(-20, -35, 20), 15, 4);   // Metall
    addSphere(Vec3(20, -35, 20), 15, 5);    // Glas/Dielektrikum
    
    // Zusätzliche dekorative Sphären
    addSphere(Vec3(0, -35, -20), 10, 3);    // Klein weiß
    
    // Camera Position
    setCamera(Vec3(0, 0, 80), Vec3(0, 0, 0));
}

void Scene::addSphere(const Vec3& center, float radius, uint32_t materialIndex) {
    Sphere sphere;
    sphere.center = center;
    sphere.radius = radius;
    sphere.materialIndex = materialIndex;
    spheres.push_back(sphere);
}

void Scene::addMaterial(const Material& material) {
    materials.push_back(material);
}

void Scene::setCamera(const Vec3& origin, const Vec3& target, float fov) {
    cameraOrigin = origin;
    cameraDirection = Vec3(
        target.x - origin.x,
        target.y - origin.y,
        target.z - origin.z
    );
    
    // Normalisieren
    float len = std::sqrt(cameraDirection.x * cameraDirection.x +
                         cameraDirection.y * cameraDirection.y +
                         cameraDirection.z * cameraDirection.z);
    if (len > 0) {
        cameraDirection.x /= len;
        cameraDirection.y /= len;
        cameraDirection.z /= len;
    }
    
    fovY = fov;
}

} // namespace vkpt
