REM blender needs to be on the path and the image asset converter needs to have been built in release build

mkdir build\test_assets\textures
mkdir build\test_assets\audio
mkdir build\test_assets\models
mkdir build\standard_assets\models
mkdir build\debug\standard_assets\shaders
mkdir build\release\standard_assets\shaders


glslc -O -DNDEBUG standard_assets\shaders\downsample.frag -o build\release\standard_assets\shaders\downsample.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\gaussian_light.frag -o build\release\standard_assets\shaders\gaussian_light.frag.spv

glslc -O -DNDEBUG -DCOLOUR_TYPE_R standard_assets\shaders\gaussian_light.frag -o build\release\standard_assets\shaders\gaussian_light.1.frag.spv
glslc -O -DNDEBUG -DCOLOUR_TYPE_RGB standard_assets\shaders\gaussian_light.frag -o build\release\standard_assets\shaders\gaussian_light.3.frag.spv
glslc -O -DNDEBUG -DCOLOUR_TYPE_RGBA standard_assets\shaders\gaussian_light.frag -o build\release\standard_assets\shaders\gaussian_light.4.frag.spv

glslc -O -DNDEBUG standard_assets\shaders\gaussian_rgb.frag -o build\release\standard_assets\shaders\gaussian_rgb.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\object.frag -o build\release\standard_assets\shaders\object.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\object_depth_alpha.frag -o build\release\standard_assets\shaders\object_depth_alpha.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\object_light.frag -o build\release\standard_assets\shaders\object_light.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\post.frag -o build\release\standard_assets\shaders\post.frag.spv
glslc -O -DNDEBUG standard_assets\shaders\skybox.frag -o build\release\standard_assets\shaders\skybox.frag.spv

glslc -O -DNDEBUG standard_assets\shaders\fullscreen.vert -o build\release\standard_assets\shaders\fullscreen.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\post.vert -o build\release\standard_assets\shaders\post.vert.spv
glslc -O -DNDEBUG standard_assets\shaders\skybox.vert -o build\release\standard_assets\shaders\skybox.vert.spv

glslc -O -DNDEBUG standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.spv
glslc -O -DNDEBUG -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.depth.spv
glslc -O -DNDEBUG -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.depth_alpha.spv
glslc -O -DNDEBUG -DOBJECT_LIGHT standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.light.spv
glslc -O -DNDEBUG -DSKINNED standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.skinned.spv
glslc -O -DNDEBUG -DSKINNED -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.skinned.depth.spv
glslc -O -DNDEBUG -DSKINNED -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.skinned.depth_alpha.spv
glslc -O -DNDEBUG -DSKINNED -DOBJECT_LIGHT standard_assets\shaders\object.vert -o build\release\standard_assets\shaders\object.vert.skinned.light.spv




glslc -O0 -DDEBUG standard_assets\shaders\downsample.frag -o build\debug\standard_assets\shaders\downsample.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\gaussian_light.frag -o build\debug\standard_assets\shaders\gaussian_light.frag.spv

glslc -O0 -DDEBUG -DCOLOUR_TYPE_R standard_assets\shaders\gaussian_light.frag -o build\debug\standard_assets\shaders\gaussian_light.1.frag.spv
glslc -O0 -DDEBUG -DCOLOUR_TYPE_RGB standard_assets\shaders\gaussian_light.frag -o build\debug\standard_assets\shaders\gaussian_light.3.frag.spv
glslc -O0 -DDEBUG -DCOLOUR_TYPE_RGBA standard_assets\shaders\gaussian_light.frag -o build\debug\standard_assets\shaders\gaussian_light.4.frag.spv

glslc -O0 -DDEBUG standard_assets\shaders\gaussian_rgb.frag -o build\debug\standard_assets\shaders\gaussian_rgb.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\object.frag -o build\debug\standard_assets\shaders\object.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\object_depth_alpha.frag -o build\debug\standard_assets\shaders\object_depth_alpha.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\object_light.frag -o build\debug\standard_assets\shaders\object_light.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\post.frag -o build\debug\standard_assets\shaders\post.frag.spv
glslc -O0 -DDEBUG standard_assets\shaders\skybox.frag -o build\debug\standard_assets\shaders\skybox.frag.spv

glslc -O0 -DDEBUG standard_assets\shaders\fullscreen.vert -o build\debug\standard_assets\shaders\fullscreen.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\post.vert -o build\debug\standard_assets\shaders\post.vert.spv
glslc -O0 -DDEBUG standard_assets\shaders\skybox.vert -o build\debug\standard_assets\shaders\skybox.vert.spv

glslc -O0 -DDEBUG standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.spv
glslc -O0 -DDEBUG -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.depth.spv
glslc -O0 -DDEBUG -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.depth_alpha.spv
glslc -O0 -DDEBUG -DOBJECT_LIGHT standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.light.spv
glslc -O0 -DDEBUG -DSKINNED standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.skinned.spv
glslc -O0 -DDEBUG -DSKINNED -DOBJECT_DEPTH standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.skinned.depth.spv
glslc -O0 -DDEBUG -DSKINNED -DOBJECT_DEPTH_ALPHA standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.skinned.depth_alpha.spv
glslc -O0 -DDEBUG -DSKINNED -DOBJECT_LIGHT standard_assets\shaders\object.vert -o build\debug\standard_assets\shaders\object.vert.skinned.light.spv



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