#!/bin/bash -e


BUILDDIR=build-em


sudo docker run \
    --rm        \
    -ti         \
    -v `pwd`:`pwd` \
    -w `pwd`       \
    --network=host \
    ubuntu:22.04   \
    bash -c "umask 0000                         \
        && apt-get update                       \
        && DEBIAN_FRONTEND=noninteractive       \
            apt-get install -y                  \
            build-essential cmake emscripten    \
        && mkdir -p $BUILDDIR && cd $BUILDDIR   \
        && emcmake cmake                        \
            -DCMAKE_BUILD_TYPE=Release          \
            -DCMAKE_INSTALL_PREFIX=installdir   \
            -DCMAKE_VERBOSE_MAKEFILE=ON         \
            ..                                  \
        && emmake make                          \
        && bash \
        && emcc libexample0.a                   \
            -sMODULARIZE=1                      \
            -sEXPORTED_FUNCTIONS=_example0      \
            -sEXPORTED_RUNTIME_METHODS=cwrap    \
            -o example0.js                      \
        "


# emcmake cmake .....
# emmake make
# emmake make install
# emcc install/lib/libtiff.a -o libtiff.js
# emcc libexample0.a libtiff/libtiff/libtiff.a -sMODULARIZE=1 -sEXPORT_NAME="createModule"  -sEXPORTED_FUNCTIONS=_example0,_write_file,_main,_malloc,_free -s EXPORTED_RUNTIME_METHODS=cwrap -o example0.js


echo done;