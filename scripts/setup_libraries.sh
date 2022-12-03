#!/bin/bash

# Prepare git submodules.
git submodule | while read -r line ; do
    submodule="$(echo "$line" | cut -d' ' -f 2)"
    status="$(git submodule status "$submodule")"
    if echo "$status" | grep -q '^-' ; then
        echo "Initializing submodule $submodule..."
        git submodule update --init --recursive "$submodule"
    fi
done

LIB_DIR="$(realpath libraries/lib)"
INCLUDE_DIR="$(realpath libraries/include)"

# Install submodule libraries and includes in-tree.
(
    echo "Installing glfw in-tree..."
    cd submodules/glfw

    if [ ! -d build ] ; then
        mkdir build
    fi
    cmake -S . -B build -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Debug
    cd build
    make
    cd ..
    rm -rf $INCLUDE_DIR/GLFW
    cp -r "$(realpath include/GLFW)" "$INCLUDE_DIR/GLFW"
    rm -f $LIB_DIR/libglfw.so*
    cp build/src/libglfw.so* $LIB_DIR
)
(
    echo "Installing glm in-tree..."
    cd submodules/glm

    rm -rf $INCLUDE_DIR/glm
    cp -r glm $INCLUDE_DIR/glm
)
(
    echo "Installing jsoncpp in-tree..."
    cd submodules/jsoncpp

    if [ ! -d build ] ; then
        mkdir build
    fi
    cd build
    cmake -S .. -B . -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Debug
    make
    cd ..
    rm -rf $INCLUDE_DIR/json
    cp -r include/json $INCLUDE_DIR/json
    rm -f $LIB_DIR/libjsoncpp.so*
    cp build/lib/libjsoncpp.so* $LIB_DIR
)
