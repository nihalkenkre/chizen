@echo off

cl.exe /nologo /MT /Ox /DNDEBUG /DWIN32_LEAN_AND_MEAN launch_blender.c /link kernel32.lib /out:launch_blender.exe

del *.obj