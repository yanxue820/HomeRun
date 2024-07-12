#!/bin/bash

wget https://boostorg.jfrog.io/artifactory/main/release/1.77.0/source/boost_1_77_0.tar.gz
tar -xzf boost_1_77_0.tar.gz
cd boost_1_77_0
./bootstrap.sh
./b2 stage --toolset=gcc --with=all
./b2 install --toolset=gcc --with=all
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

cd ..

git clone https://github.com/Visa-Research/volepsi.git
cd volepsi
python3 build.py -DVOLE_PSI_ENABLE_BOOST=ON
python3 build.py --install

cd ..

git clone https://github.com/yanxue820/HomeRun.git
cd HomeRun
mkdir build && cd build
cmake ..
make
