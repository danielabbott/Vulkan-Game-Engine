set -euf -o pipefail
cd ../Pigeon && make -j1
cd ../StandardAssets/Shaders && make -j1
make -j1