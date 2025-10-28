#!/bin/bash -e


BUILDDIR=build-wasm


sudo docker run \
    --rm        \
    -ti         \
    -v `pwd`:`pwd` \
    -w `pwd`       \
    --network=host \
    debian:stable-slim   \
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
        && bash"

echo done;