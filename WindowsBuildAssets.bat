REM blender needs to be on the path and the image asset converter needs to have been built in release build
REM TODO: Use nmake makefile or custom asset converter program or something

mkdir build\test_assets\textures
mkdir build\standard_assets\textures
mkdir build\test_assets\models
mkdir build\standard_assets\models
mkdir build\debug\standard_assets\shaders
mkdir build\release\standard_assets\shaders

glslc -O -DNDEBUG standard_assets\shaders\bloom_gaussian.frag -o build\release\standard_assets\shaders\bloom_gaussian.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\downsample.frag -o build\release\standard_assets\shaders\downsample.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\object.frag -o build\release\standard_assets\shaders\object.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\post.frag -o build\release\standard_assets\shaders\post.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\skybox.frag -o build\release\standard_assets\shaders\skybox.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\ssao.frag -o build\release\standard_assets\shaders\ssao.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\ssao_blur.frag -o build\release\standard_assets\shaders\ssao_blur.frag.spv

glslc -O -DNDEBUG standard_assets\shaders\downsample.vert -o build\release\standard_assets\shaders\downsample.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\gaussian.vert -o build\release\standard_assets\shaders\gaussian.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\post.vert -o build\release\standard_assets\shaders\post.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\skybox.vert -o build\release\standard_assets\shaders\skybox.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\ssao.vert -o build\release\standard_assets\shaders\ssao.vert.spv

glslc -O -DNDEBUG -DDEPTH_ONLY standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.depth.spv

REM ---

glslc -O0 -DDEBUG standard_assets\shaders\bloom_gaussian.frag -o build\debug\standard_assets\shaders\bloom_gaussian.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\downsample.frag -o build\debug\standard_assets\shaders\downsample.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.frag -o build\debug\standard_assets\shaders\object.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\post.frag -o build\debug\standard_assets\shaders\post.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\skybox.frag -o build\debug\standard_assets\shaders\skybox.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\ssao.frag -o build\debug\standard_assets\shaders\ssao.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\ssao_blur.frag -o build\debug\standard_assets\shaders\ssao_blur.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\ssao_blur2.frag -o build\debug\standard_assets\shaders\ssao_blur2.frag.spv

glslc -O0 -DDEBUG standard_assets\shaders\downsample.vert -o build\debug\standard_assets\shaders\downsample.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\gaussian.vert -o build\debug\standard_assets\shaders\gaussian.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\post.vert -o build\debug\standard_assets\shaders\post.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\skybox.vert -o build\debug\standard_assets\shaders\skybox.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\ssao.vert -o build\debug\standard_assets\shaders\ssao.vert.spv

glslc -O0 -DDEBUG -DDEPTH_ONLY standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.depth.spv


blender standard_assets\models\cube.blend --background --python-exit-code 1 --python blender_export.py -- build/standard_assets/models/cube.blend.asset
blender standard_assets\models\sphere.blend --background --python-exit-code 1 --python blender_export.py -- build/standard_assets/models/sphere.blend.asset
blender test_assets\models\alien.blend --background --python-exit-code 1 --python blender_export.py -- build/test_assets/models/alien.blend.asset

x64\Release\ImageAssetConverter.exe test_assets\textures\t_biomech_mutant_skin_1_bottom_a.jpg build\test_assets\textures\t_biomech_mutant_skin_1_bottom_a.jpg.asset
x64\Release\ImageAssetConverter.exe test_assets\textures\t_biomech_mutant_skin_1_bottom_n.jpg build\test_assets\textures\t_biomech_mutant_skin_1_bottom_n.jpg.asset
x64\Release\ImageAssetConverter.exe test_assets\textures\t_biomech_mutant_skin_1_top_a.jpg build\test_assets\textures\t_biomech_mutant_skin_1_top_a.jpg.asset
x64\Release\ImageAssetConverter.exe test_assets\textures\t_biomech_mutant_skin_1_top_n.jpg build\test_assets\textures\t_biomech_mutant_skin_1_top_n.jpg.asset

x64\Release\AudioAssetConverter.exe test_assets\audio\pigeon.ogg build\test_assets\audio\pigeon.ogg.asset
