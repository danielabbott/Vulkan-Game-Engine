REM blender needs to be on the path and the image asset converter needs to have been built in release build

mkdir build\test_assets\textures
mkdir build\test_assets\audio
mkdir build\test_assets\models
mkdir build\standard_assets\models
mkdir build\standard_assets\shaders
mkdir build\debug\standard_assets\shaders
mkdir build\release\standard_assets\shaders


copy standard_assets\shaders\downscale.frag build\standard_assets\shaders\downscale.frag
copy standard_assets\shaders\ssao.frag build\standard_assets\shaders\ssao.frag
copy standard_assets\shaders\downscale_ssao.frag build\standard_assets\shaders\downscale_ssao.frag
copy standard_assets\shaders\kawase_ssao.frag build\standard_assets\shaders\kawase_ssao.frag
copy standard_assets\shaders\kawase_rgb.frag build\standard_assets\shaders\kawase_rgb.frag
copy standard_assets\shaders\kawase_merge.frag build\standard_assets\shaders\kawase_merge.frag
copy standard_assets\shaders\object.frag build\standard_assets\shaders\object.frag
copy standard_assets\shaders\object_depth_alpha.frag build\standard_assets\shaders\object_depth_alpha.frag
copy standard_assets\shaders\post.frag build\standard_assets\shaders\post.frag
copy standard_assets\shaders\skybox.frag build\standard_assets\shaders\skybox.frag
copy standard_assets\shaders\fullscreen.vert build\standard_assets\shaders\fullscreen.vert
copy standard_assets\shaders\post.vert build\standard_assets\shaders\post.vert
copy standard_assets\shaders\skybox.vert build\standard_assets\shaders\skybox.vert
copy standard_assets\shaders\object.vert build\standard_assets\shaders\object.vert
copy standard_assets\shaders\object.vert build\standard_assets\shaders\object.vert.depth
copy standard_assets\shaders\object.vert build\standard_assets\shaders\object.vert.depth_alpha
copy standard_assets\shaders\object.vert build\standard_assets\shaders\object.vert.skinned
copy standard_assets\shaders\object.vert build\standard_assets\shaders\object.vert.skinned.depth
copy standard_assets\shaders\object.vert build\standard_assets\shaders\object.vert.skinned.depth_alpha
copy standard_assets\shaders\common.glsl build\standard_assets\shaders\common.glsl
copy standard_assets\shaders\random.glsl build\standard_assets\shaders\random.glsl
copy standard_assets\shaders\ubo.glsl build\standard_assets\shaders\ubo.glsl

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
glslc -O -DNDEBUG -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.depth.spv
glslc -O -DNDEBUG -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.depth_alpha.spv
glslc -O -DNDEBUG -DSKINNED standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.skinned.spv
glslc -O -DNDEBUG -DSKINNED -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.skinned.depth.spv
glslc -O -DNDEBUG -DSKINNED -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.skinned.depth_alpha.spv


glslc -O -DDEBUG standard_assets\shaders\downscale.frag -o build\debug\standard_assets\shaders\downscale.frag.spv
glslc -O -DDEBUG standard_assets\shaders\ssao.frag -o build\debug\standard_assets\shaders\ssao.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\downscale_ssao.frag -o build\debug\standard_assets\shaders\downscale_ssao.frag.spv
glslc -O -DDEBUG standard_assets\shaders\kawase_ssao.frag -o build\debug\standard_assets\shaders\kawase_ssao.frag.spv
glslc -O -DDEBUG standard_assets\shaders\kawase_rgb.frag -o build\debug\standard_assets\shaders\kawase_rgb.frag.spv
glslc -O -DDEBUG standard_assets\shaders\kawase_merge.frag -o build\debug\standard_assets\shaders\kawase_merge.frag.spv
glslc -O -DDEBUG standard_assets\shaders\object.frag -o build\debug\standard_assets\shaders\object.frag.spv
glslc -O -DDEBUG standard_assets\shaders\object_depth_alpha.frag -o build\debug\standard_assets\shaders\object_depth_alpha.frag.spv
glslc -O -DDEBUG standard_assets\shaders\post.frag -o build\debug\standard_assets\shaders\post.frag.spv
glslc -O -DDEBUG standard_assets\shaders\skybox.frag -o build\debug\standard_assets\shaders\skybox.frag.spv

glslc -O -DDEBUG standard_assets\shaders\fullscreen.vert -o build\debug\standard_assets\shaders\fullscreen.vert.spv
glslc -O -DDEBUG standard_assets\shaders\post.vert -o build\debug\standard_assets\shaders\post.vert.spv
glslc -O -DDEBUG standard_assets\shaders\skybox.vert -o build\debug\standard_assets\shaders\skybox.vert.spv

glslc -O -DDEBUG standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.spv
glslc -O -DDEBUG -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.depth.spv
glslc -O -DDEBUG -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.depth_alpha.spv
glslc -O -DDEBUG -DSKINNED standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.skinned.spv
glslc -O -DDEBUG -DSKINNED -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.skinned.depth.spv
glslc -O -DDEBUG -DSKINNED -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.skinned.depth_alpha.spv



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