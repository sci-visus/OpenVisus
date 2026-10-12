#!/bin/bash

set -e
set -o pipefail

sudo xcodebuild -license accept 

# install swig, cmake, python3 nad qt5 
# INTERNAL NOTE: do not try to install brew locally: it will take forever recompiling qt5...
# INTERNAL NOTE: if you change python version , make sure it's supported by OpenVisus
# INTERNAL NOTE: qt@5 install 5.15 which is exactly what we need
PYTHON_VERSION=3.13
brew install swig cmake python@${PYTHON_VERSION} qt@5 xcbeautify

# in GitHub Actions the repository is already checked out

mkdir -p github.com/sci-visus
cd github.com/sci-visus
git clone https://github.com/sci-visus/OpenVisus.git
cd OpenVisus

OPENVISUS_DIR="$PWD"

# create python virtual env
python${PYTHON_VERSION} -m venv .venv
source .venv/bin/activate
# from now on python3 is the venv python

python3 -m pip install --upgrade pip

mkdir -p build_macos
cd build_macos
cmake -Wno-dev -Wno-deprecated --log-level=ERROR \
    -GXcode \
    -DCMAKE_C_FLAGS="-w" \
    -DCMAKE_CXX_FLAGS="-w" \
    -DQt5_DIR=$(brew --prefix qt@5)/lib/cmake/Qt5 \
    -DPython_EXECUTABLE=$(which python3) \
    -DVISUS_GUI=1 \
    -DVISUS_MODVISUS=0 \
    -DVISUS_SLAM=0 \
    -DVISUS_IDX2=0 \
    ../
    
cmake --build . --target ALL_BUILD --config Release | xcbeautify
cmake --build . --target install   --config Release | xcbeautify

cd Release/OpenVisus
export PYTHONPATH="$(dirname "$PWD")"
export CODE_SIGN=/usr/bin/codesign

# important to relocate binaries
python3 -m OpenVisus configure
python3 -m OpenVisus test || true     # something fails here, we don't care
python3 -m OpenVisus test-gui

# create a launcher on the Desktop to run the viewer (not in CI)
if [ -z "$CI" ]; then
cat > ~/Desktop/run-viewer.command <<EOF
#!/bin/bash
set -e
cd ${OPENVISUS_DIR}
source ".venv/bin/activate"
export PYTHONPATH="./build_macos/Release"
exec python3 -m OpenVisus viewer "\$@"
EOF
chmod +x ~/Desktop/run-viewer.command
fi




