#!/bin/bash
cd build && rm -rf * 
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j8
cd ..
# 获取系统架构
ARCH=$(uname -m)
if [[ "$ARCH" == "x86_64" ]]; then
    export LD_LIBRARY_PATH=$(pwd)/lib/x86:$LD_LIBRARY_PATH
elif [[ "$ARCH" == "aarch64"* ]]; then
    export LD_LIBRARY_PATH=$(pwd)/lib/arm:$LD_LIBRARY_PATH
else
    echo "Unsupported architecture: $ARCH"
    exit 1
fi
cd build