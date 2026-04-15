# Chizen

This is a test project to tryuout real time rasterization, offline rasterization, progressive path tracing with Embree, Vulkan.

## Components
### GUI
Lets the user select the file to display, controls for sample count, max bounces, and output image size.

### Viewport
Displays the current scene through the indirect draw commands.

### Raytracer
Progressively raytraces the scenes using the raytracing API, and Slang shaders, writing the output of each sample to an image. Currently just writes the normals to the output buffer

### SW Rasterizer
A very feeble attempt at a sw rasterizer. Using oneTBB to distribute the "buckets" to different CPU cores available.

### Display
Contains a quad to which the render target of the raytracer is mapped to. It can be zoomed into and panned around using the mouse.

### Vulkan Interface
Collection of the most commonly used vulkan objects e.g. device, memory allocator, transfer batches.

### Vulkan Objects
Interfaces around the raw vulkan handles, and a few high level operation oriented objects. They follow the RAII pattern.

`FrameObjects` deal with handling semaphores, frame in flight, for a typical vulkan drawing session.

`ComputeHelper` helps to batch transfer operations and submit them together on the compute queue. It optionally waits on other operations through incoming semaphores.

## Depends on
- cgltf
- glm
- imgui
- imguifiledialog
- SDL
- stb