# Vulkan Multi-GPU Path Tracer

Ein **Vulkan-basiertes Path Tracing Framework** mit expliziter **Multi-GPU-Unterstützung** und **VRAM-Verteilung**.

## Features

### Multi-GPU Unterstützung
- **Explizite GPU-Auswahl**: Wählen Sie spezifische GPUs oder nutzen Sie alle verfügbaren Geräte
- **Tile-basiertes Rendering**: Das Bild wird in Kacheln unterteilt und parallel auf mehreren GPUs berechnet
- **Load Balancing**: Round-Robin-Verteilung der Tiles für optimale Auslastung

### VRAM-Unabhängigkeit
- **Datenverteilung**: Szenendaten (Geometrie, Materialien) können über mehrere GPUs verteilt werden
- **Gestriped Geometry**: Jede GPU hält nur einen Teil der Geometrie im eigenen VRAM
- **Skalierbarkeit**: Größere Szenen durch kombinierten VRAM mehrerer GPUs

### Architektur
- **Compute Shader Path Tracing**: Volle Nutzung der Vulkan Compute Queues
- **Minimaler Overhead**: Kein Graphics-Overhead, reine Compute-Pipeline
- **Erweiterbar**: Einfache Integration von BVH, komplexeren Materialien und Texturen

## Struktur

```
vulkan_multi_gpu/
├── CMakeLists.txt              # Build-Konfiguration
├── src/
│   ├── main.cpp                # Einstiegspunkt & Demo
│   ├── types.h                 # Datenstrukturen (Vec3, Sphere, Material, etc.)
│   ├── vulkan_context.h/.cpp   # Vulkan Initialisierung & Device Management
│   ├── multi_gpu_manager.h/.cpp # Multi-GPU Logik & Tile-Verteilung
│   ├── scene.h/.cpp            # Szenenbeschreibung
│   └── path_tracer.h           # Compute Shader Source
```

## Voraussetzungen

- **Vulkan SDK** (https://vulkan.lunarg.com/)
- **CMake** 3.16+
- **C++17** Compiler (GCC, Clang, MSVC)
- **Mehrere Vulkan-fähige GPUs** (optional, aber empfohlen)

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

```bash
./VulkanMultiGPUPathTracer
```

Das Programm:
1. Enumeriert alle verfügbaren Vulkan-GPUs
2. Initialisiert jede GPU mit eigenem Device und Command Queue
3. Verteilt die Szenendaten auf die GPUs (bei Multi-GPU)
4. Weist Render-Tiles den GPUs zu
5. Führt das Path Tracing parallel aus
6. Sammelt die Ergebnisse ein

## Konzept: VRAM-Verteilung

### Problem
Große Szenen passen nicht in den VRAM einer einzelnen GPU.

### Lösung
Die Szene wird räumlich partitioniert:
- **GPU 0**: Sphären 0-49
- **GPU 1**: Sphären 50-99
- **GPU 2**: Sphären 100-149
- etc.

Jede GPU baut ihre eigene lokale BVH nur für "ihre" Objekte.

### Ray Tracing über Partitionen
Wenn ein Strahl die lokale Partition verlässt:
1. **Bounding Box Check**: Prüfen ob Strahl andere Partitionen schneiden könnte
2. **Handoff**: Strahl wird an zuständige GPU übergeben (über PCIe oder Shared Memory)
3. **Ergebnis-Merge**: Nächster Treffer wird zurückgemeldet

## Erweiterungen

### Nächste Schritte für vollständige Implementierung

1. **SPIR-V Shader Kompilierung**
   ```bash
   glslc shaders/path_tracer.comp -o shaders/path_tracer.spv
   ```

2. **Compute Pipeline Erstellung**
   - Pipeline Layout mit Descriptor Set Bindings
   - Proper Memory Type Selection für jede GPU

3. **Descriptor Sets**
   - Uniform Buffer: Camera, Render Settings
   - Storage Buffer: Spheres (pro GPU subset)
   - Storage Buffer: Materials (auf allen GPUs)
   - Image2D: Output Buffer

4. **Synchronisation**
   - Fence-basierte Synchronisation zwischen Frames
   - Semaphore für inter-GPU Kommunikation

5. **Output**
   - HDR Image Export (EXR Format)
   - Echtzeit-Vorschau mit GLFW + Swapchain

## Performance-Erwartungen

| Konfiguration | Theoretischer Speedup |
|--------------|----------------------|
| 1 GPU        | 1.0x (Baseline)      |
| 2 GPUs       | ~1.8-1.9x            |
| 4 GPUs       | ~3.5-3.8x            |

*Hinweis: Overhead durch PCIe-Transfer und Synchronisation reduziert den idealen Speedup.*

## Lizenz

MIT License
