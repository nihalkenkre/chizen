@echo off

cl /nologo /MTd /GR- /Od /Zi /DDEBUG /D_HAS_EXCEPTIONS=0 /DWIN32_LEAN_AND_MEAN /std:c++20 /I. /Id3d-helpers/include/ src/*.cpp meshoptimizer/src/*.cpp /link Shlwapi.lib user32.lib ole32.lib dxgi.lib dxguid.lib dxcompiler.lib d3d12.lib /DEBUG:FULL /out:chizen.exe
