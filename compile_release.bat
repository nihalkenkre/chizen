@echo off

cl /nologo /MT /GR- /DNDEBUG /D_HAS_EXCEPTIONS=0 /DWIN32_LEAN_AND_MEAN /DVK_USE_PLATFORM_WIN32_KHR /std:c++20 /I. /I%VULKAN_SDK%/Include /Id3d-helpers/include/ src/*.cpp meshoptimizer/src/*.cpp %VULKAN_SDK%/Include/Volk/volk.c /link comctl32.lib Shlwapi.lib user32.lib ole32.lib dxguid.lib dxgi.lib dxcompiler.lib d3d12.lib /out:chizen.exe

del *.obj *.ilk *.pdb *.lib *.exp