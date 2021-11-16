REM blender needs to be on the path and the image asset converter needs to have been built in release build

mkdir build\test_assets\textures
mkdir build\test_assets\audio
mkdir build\test_assets\models
mkdir build\standard_assets\models
mkdir build\standard_assets\shaders
mkdir build\debug\standard_assets\shaders
mkdir build\release\standard_assets\shaders

copy standard_assets\ca-certificates.crt build\standard_assets\ca-certificates.crt

copy standard_assets\shaders\* build\standard_assets\shaders\

glslc -O -DNDEBUG standard_assets\shaders\downscale.frag -o build\release\standard_assets\shaders\downscale.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\ssao.frag -o build\release\standard_assets\shaders\ssao.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\downscale_ssao.frag -o build\release\standard_assets\shaders\downscale_ssao.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\kawase_ssao.frag -o build\release\standard_assets\shaders\kawase_ssao.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\kawase_rgb.frag -o build\release\standard_assets\shaders\kawase_rgb.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\kawase_merge.frag -o build\release\standard_assets\shaders\kawase_merge.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\object.frag -o build\release\standard_assets\shaders\object.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\object_depth_alpha.frag -o build\release\standard_assets\shaders\object_depth_alpha.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\post.frag -o build\release\standard_assets\shaders\post.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\skybox.frag -o build\release\standard_assets\shaders\skybox.frag.spv

glslc -O -DNDEBUG standard_assets\shaders\fullscreen.vert -o build\release\standard_assets\shaders\fullscreen.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\post.vert -o build\release\standard_assets\shaders\post.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\skybox.vert -o build\release\standard_assets\shaders\skybox.vert.spv

glslc -O -DNDEBUG standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\object.depth.vert -o build\release\standard_assets\shaders\object.depth.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\object.depth_alpha.vert -o build\release\standard_assets\shaders\object.depth_alpha.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\object.skinned.vert -o build\release\standard_assets\shaders\object.skinned.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\object.skinned.depth.vert -o build\release\standard_assets\shaders\object.skinned.depth.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\object.skinned.depth_alpha.vert -o build\release\standard_assets\shaders\object.skinned.depth_alpha.vert.spv


glslc -O0 -DDEBUG standard_assets\shaders\downscale.frag -o build\debug\standard_assets\shaders\downscale.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\ssao.frag -o build\debug\standard_assets\shaders\ssao.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\downscale_ssao.frag -o build\debug\standard_assets\shaders\downscale_ssao.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\kawase_ssao.frag -o build\debug\standard_assets\shaders\kawase_ssao.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\kawase_rgb.frag -o build\debug\standard_assets\shaders\kawase_rgb.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\kawase_merge.frag -o build\debug\standard_assets\shaders\kawase_merge.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.frag -o build\debug\standard_assets\shaders\object.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\object_depth_alpha.frag -o build\debug\standard_assets\shaders\object_depth_alpha.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\post.frag -o build\debug\standard_assets\shaders\post.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\skybox.frag -o build\debug\standard_assets\shaders\skybox.frag.spv

glslc -O0 -DDEBUG standard_assets\shaders\fullscreen.vert -o build\debug\standard_assets\shaders\fullscreen.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\post.vert -o build\debug\standard_assets\shaders\post.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\skybox.vert -o build\debug\standard_assets\shaders\skybox.vert.spv

glslc -O0 -DDEBUG standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.depth.vert -o build\debug\standard_assets\shaders\object.depth.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.depth_alpha.vert -o build\debug\standard_assets\shaders\object.depth_alpha.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.skinned.vert -o build\debug\standard_assets\shaders\object.skinned.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.skinned.depth.vert -o build\debug\standard_assets\shaders\object.skinned.depth.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.skinned.depth_alpha.vert -o build\debug\standard_assets\shaders\object.skinned.depth_alpha.vert.spv


blender standard_assets\models\cube.blend --background --python-exit-code 1 --python blender_export.py -- build/standard_assets/models/cube.blend.asset
blender standard_assets\models\sphere.blend --background --python-exit-code 1 --python blender_export.py -- build/standard_assets/models/sphere.blend.asset
blender test_assets\models\pete.blend --background --python-exit-code 1 --python blender_export.py -- build/test_assets/models/pete.blend.asset


x64\Release\image_asset_converter.exe test_assets\textures\Ch17_1001_Diffuse.jpg build\test_assets\textures\Ch17_1001_Diffuse.jpg.asset
x64\Release\image_asset_converter.exe test_assets\textures\Ch17_1001_Normal.jpg build\test_assets\textures\Ch17_1001_Normal.jpg.asset
x64\Release\image_asset_converter.exe test_assets\textures\Ch17_1002_Diffuse.png build\test_assets\textures\Ch17_1002_Diffuse.png.asset
x64\Release\image_asset_converter.exe test_assets\textures\Ch17_1002_Normal.jpg build\test_assets\textures\Ch17_1002_Normal.jpg.asset

x64\Release\audio_asset_converter.exe test_assets\audio\pigeon.ogg build\test_assets\audio\pigeon.ogg.asset
copy test_assets\audio\pigeon.ogg build\test_assets\audio\pigeon.ogg.data

pause