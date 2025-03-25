@echo off

cl /nologo /MT /GR- /DNDEBUG /D_HAS_EXCEPTIONS=0 /DWIN32_LEAN_AND_MEAN /std:c++20 /I. src/world_scene.cpp src/default_scene.cpp src/vulkan_renderer.cpp src/dx12_renderer.cpp src/main.cpp /link user32.lib ole32.lib dxguid.lib dxgi.lib d3dcompiler.lib d3d12.lib /out:chizen.exe

del *.obj *.ilk *.pdb