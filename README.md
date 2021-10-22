![Sponza Screenshot](https://media.githubusercontent.com/media/danielabbott/Pigeon-Vulkan-Game-Engine/dev/screenshots/sponza.jpg)

Dependencies

The versions listed are the versions that have been tested on Solus Linux.
Different versions will probably work fine.

clang 12.0.1 or gcc 11.2.0
Make 4.3
GLFW 3.3.4
Vulkan 1.0 or OpenGL 3.3 with GL_ARB_clip_control
CGLM 0.8.4 (Not available on vcpkg- copy the include/cglm folder to the Deps folder)
glslc shaderc v2021.0 - spirv-tools v2021.2 - glslang v11.5.0
zstd 1.5.0
Blender 2.92.0
OpenAL 1.1
stb_image.h (included)
stb_dxt.h (included)
stb_vorbis.c (included)
Glad (included)



To compile and run (Linux):

make CC=clang DEBUG=1 -j`nproc` && build/debug/test1

This compiles source files, converts assets, and builds build/release/test1
