set -euf -o pipefail
cd ../Pigeon
make -j8
cd ../StandardAssets/Shaders
make -j8
cd ../../Test1
make -j8