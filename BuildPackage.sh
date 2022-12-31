#!/bin/sh
set -e

#rm -rf build.$(getarch)
#mkdir build.$(getarch)
#cd build.$(getarch)
#meson .. --prefix=/packages/videostreams-0.1-1/.self --includedir=develop/headers

cd build.$(getarch)
rm -rf install

DESTDIR=$PWD/install ninja install
package create -C install/packages/videostreams-0.1-1/.self videostreams-0.1-1-x86_64.hpkg
