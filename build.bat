@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

SET includes=/I%VULKAN_SDK%/Include /Ithird_party/glfw/include /Ithird_party/glm
SET links=/link /LIBPATH:%VULKAN_SDK%/Lib /LIBPATH:third_party/glfw/lib-vc2019 glfw3.lib vulkan-1.lib user32.lib Shell32.lib gdi32.lib
SET defines=/D DEBUG /D WINDOWS_BUILD /D CAKEZGINE

echo "Building main..."

cl /EHsc /MD /nologo /Z7 /Fe"main" %includes% %defines% main.cpp %links%