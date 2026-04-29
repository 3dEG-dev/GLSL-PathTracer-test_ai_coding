#pragma once

#include "types.h"
#include <vector>
#include <cstdint>

namespace vkpt {

// Compute Shader als SPIR-V Bytecode (wird normalerweise aus .spc Datei geladen)
// Dies ist ein minimaler Path Tracing Compute Shader in GLSL, kompiliert zu SPIR-V
// Für das vollständige Projekt würden diese Shader zur Compile-Zeit generiert

extern const char* PATH_TRACING_COMPUTE_SHADER_GLSL;

// Shader Source Code für den Path Tracing Compute Shader
const char* PATH_TRACING_COMPUTE_SHADER_GLSL = R"(
#version 450

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

// Uniforms
layout(set = 0, binding = 0) uniform UniformBuffer {
    uint width;
    uint height;
    uint maxDepth;
    uint seed;
    vec3 cameraOrigin;
    vec3 cameraDir;
    vec3 cameraUp;
    float fov;
} uniforms;

// Scene Buffers
layout(set = 0, binding = 1) buffer SphereBuffer {
    vec4 data[]; // x,y,z,radius für jede Sphäre
} spheres;

layout(set = 0, binding = 2) buffer MaterialBuffer {
    vec4 data[]; // type, albedo, emission, roughness, ior
} materials;

// Output Buffer (RGBA Float)
layout(set = 0, binding = 3, rgba32f) uniform image2D outputImage;

// Zufallszahlengenerator (einfach LCG)
uint randUint() {
    uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * uniforms.width;
    uint state = idx * 7919 + uniforms.seed * 104729;
    state = state * 1664525u + 1013904223u;
    return state;
}

float randFloat() {
    return float(randUint()) / float(0xFFFFFFFFu);
}

vec3 randomInUnitSphere() {
    while (true) {
        vec3 p = vec3(randFloat(), randFloat(), randFloat()) * 2.0 - 1.0;
        if (dot(p, p) < 1.0) return p;
    }
}

// Sphäre Intersection
bool hitSphere(vec3 origin, vec3 direction, float tmin, float tmax, 
               out float t, out vec3 normal, out uint materialIdx) {
    // Durchlaufe alle Sphären (jede GPU hat nur einen Teil!)
    for (uint i = 0; i < spheres.data.length() / 4; i++) {
        vec4 sphere = spheres.data[i];
        vec3 center = sphere.xyz;
        float radius = sphere.w;
        
        vec3 oc = origin - center;
        float b = dot(oc, direction);
        float c = dot(oc, oc) - radius * radius;
        float discriminant = b * b - c;
        
        if (discriminant > 0.0) {
            float temp = (-b - sqrt(discriminant));
            if (temp > tmin && temp < tmax) {
                t = temp;
                normal = normalize(origin + t * direction - center);
                materialIdx = i; // Vereinfacht: Index als Material-Index
                return true;
            }
        }
    }
    return false;
}

// Color berechnen
vec3 getColor(uint materialIdx) {
    if (materialIdx >= materials.data.length() / 4) {
        return vec3(0.0);
    }
    
    // Materialdaten lesen (vereinfacht)
    vec4 matData = materials.data[materialIdx];
    return vec3(matData.x, matData.y, matData.z);
}

void main() {
    uint x = gl_GlobalInvocationID.x;
    uint y = gl_GlobalInvocationID.y;
    
    if (x >= uniforms.width || y >= uniforms.height) return;
    
    // Pixel-Koordinaten zu Normalized Device Coordinates
    float u = (float(x) + 0.5) / float(uniforms.width);
    float v = (float(y) + 0.5) / float(uniforms.height);
    
    // Aspect Ratio
    float aspect = float(uniforms.width) / float(uniforms.height);
    float tanHalfFov = tan(radians(uniforms.fov) * 0.5);
    
    // Ray Direction berechnen
    vec3 horizontal = vec3(tanHalfFov * aspect, 0, 0);
    vec3 vertical = vec3(0, tanHalfFov, 0);
    
    vec3 rayDir = normalize(uniforms.cameraDir + 
                           (u - 0.5) * 2.0 * horizontal +
                           (v - 0.5) * 2.0 * vertical);
    
    // Path Tracing Loop
    vec3 color = vec3(0.0);
    vec3 throughput = vec3(1.0);
    vec3 origin = uniforms.cameraOrigin;
    vec3 direction = rayDir;
    
    for (uint depth = 0; depth < uniforms.maxDepth; depth++) {
        float t;
        vec3 normal;
        uint materialIdx;
        
        if (!hitSphere(origin, direction, 0.001, 10000.0, t, normal, materialIdx)) {
            // Hintergrund (schwarz oder Environment Map)
            color += throughput * vec3(0.0);
            break;
        }
        
        // Hit Point
        vec3 hitPoint = origin + t * direction;
        
        // Material Farbe holen
        vec3 albedo = getColor(materialIdx);
        
        // Einfaches Lambertian Shading
        vec3 scatterDir = normalize(normal + randomInUnitSphere());
        
        // Russian Roulette Termination
        float p = max(scatterDir.r, max(scatterDir.g, scatterDir.b));
        if (depth > 2 && randFloat() > p) {
            break;
        }
        
        throughput *= albedo / p;
        origin = hitPoint;
        direction = scatterDir;
    }
    
    // Ergebnis schreiben
    vec4 finalColor = vec4(color, 1.0);
    imageStore(outputImage, ivec2(x, y), finalColor);
}
)";

} // namespace vkpt
