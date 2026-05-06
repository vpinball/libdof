#!/bin/bash

set -e

if [ -z "${MSYS2_PATH}" ]; then
   MSYS2_PATH="/c/msys64"
fi

echo "MSYS2_PATH: ${MSYS2_PATH}"
echo ""

source ./platforms/config.sh

echo "Building libraries..."
echo "  LIBUSB_SHA: ${LIBUSB_SHA}"
echo "  LIBSERIALPORT_SHA: ${LIBSERIALPORT_SHA}"
echo "  HIDAPI_SHA: ${HIDAPI_SHA}"
echo "  LIBFTDI_SHA: ${LIBFTDI_SHA}"
echo ""

rm -rf external
mkdir external
cd external

#
# build libusb and copy to third-party
#

curl -sL https://github.com/libusb/libusb/archive/${LIBUSB_SHA}.tar.gz -o libusb-${LIBUSB_SHA}.tar.gz
tar xzf libusb-${LIBUSB_SHA}.tar.gz
mv libusb-${LIBUSB_SHA} libusb
cd libusb
sed -i.bak 's/libusb-1\.0/libusb64-1.0/g' libusb/Makefile.am
sed -i.bak 's/libusb_1_0/libusb64_1_0/g' libusb/Makefile.am
mv libusb/libusb-1.0.def libusb/libusb64-1.0.def
mv libusb/libusb-1.0.rc libusb/libusb64-1.0.rc
sed -i.bak 's/libusb-1\.0/libusb64-1.0/g' libusb/libusb64-1.0.def
sed -i.bak 's/libusb-1\.0/libusb64-1.0/g' libusb/libusb64-1.0.rc
CURRENT_DIR="$(pwd)"
MSYSTEM=UCRT64 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   ./autogen.sh &&
   ./configure --enable-shared &&
   make -j\$(nproc)
"
mkdir -p ../../third-party/include/libusb-1.0
cp libusb/libusb.h ../../third-party/include/libusb-1.0
cp libusb/.libs/libusb64-1.0.dll.a ../../third-party/build-libs/win/x64/libusb64-1.0.lib
cp libusb/.libs/libusb64-1.0.dll ../../third-party/runtime-libs/win/x64/
cd ..

#
# build libserialport and copy to third-party
#

curl -sL https://github.com/sigrokproject/libserialport/archive/${LIBSERIALPORT_SHA}.tar.gz -o libserialport-${LIBSERIALPORT_SHA}.tar.gz
tar xzf libserialport-${LIBSERIALPORT_SHA}.tar.gz
mv libserialport-${LIBSERIALPORT_SHA} libserialport
cd libserialport
cp libserialport.h ../../third-party/include
sed -i.bak 's/libserialport\.la/libserialport64.la/g; s/libserialport_la/libserialport64_la/g' Makefile.am
CURRENT_DIR="$(pwd)"
MSYSTEM=UCRT64 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   ./autogen.sh &&
   ./configure &&
   make -j\$(nproc)
"
cp .libs/libserialport64.dll.a ../../third-party/build-libs/win/x64/libserialport64.lib
cp .libs/libserialport64-0.dll ../../third-party/runtime-libs/win/x64/
cd ..

#
# build hidapi and copy to third-party
#

curl -sL https://github.com/libusb/hidapi/archive/${HIDAPI_SHA}.tar.gz -o hidapi-${HIDAPI_SHA}.tar.gz
tar xzf hidapi-${HIDAPI_SHA}.tar.gz
mv hidapi-${HIDAPI_SHA} hidapi
cd hidapi
sed -i.bak 's/OUTPUT_NAME "hidapi"/OUTPUT_NAME "hidapi64"\n  PREFIX ""\n  IMPORT_PREFIX ""/' windows/CMakeLists.txt
CURRENT_DIR="$(pwd)"
MSYSTEM=UCRT64 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   cmake \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -B build &&
   cmake --build build -- -j\$(nproc)
"
cp -r hidapi ../../third-party/include/
cp build/src/windows/hidapi64.dll.a ../../third-party/build-libs/win/x64/hidapi64.lib
cp build/src/windows/hidapi64.dll ../../third-party/runtime-libs/win/x64/
cd ..

#
# build libftdi and copy to third-party
#

curl -sL https://github.com/jsm174/libftdi/archive/${LIBFTDI_SHA}.tar.gz -o libftdi-${LIBFTDI_SHA}.tar.gz
tar xzf libftdi-${LIBFTDI_SHA}.tar.gz
mv libftdi-${LIBFTDI_SHA} libftdi
cd libftdi
sed -i.bak 's/cmake_minimum_required([^)]*)/cmake_minimum_required(VERSION 3.10)/' CMakeLists.txt
sed -i.bak 's/set_target_properties(ftdi1 PROPERTIES VERSION ${VERSION_FIXUP}\.${MINOR_VERSION}\.0 SOVERSION 2)/set_target_properties(ftdi1 PROPERTIES OUTPUT_NAME "ftdi164" VERSION ${VERSION_FIXUP}.${MINOR_VERSION}.0 SOVERSION 2)/' src/CMakeLists.txt
CURRENT_DIR="$(pwd)"
MSYSTEM=UCRT64 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   cmake \
      -DFTDI_EEPROM=OFF \
      -DEXAMPLES=OFF \
      -DSTATICLIBS=OFF \
      -DLIBUSB_INCLUDE_DIR=../libusb/libusb \
      -DLIBUSB_LIBRARIES=$(pwd)/../../third-party/runtime-libs/win/x64/libusb64-1.0.dll \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -B build &&
   cmake --build build -- -j\$(nproc)
"

mkdir -p ../../third-party/include/libftdi1
cp src/ftdi.h ../../third-party/include/libftdi1
cp build/src/libftdi164.dll.a ../../third-party/build-libs/win/x64/libftdi164.lib
cp build/src/libftdi164.dll ../../third-party/runtime-libs/win/x64/
cd ..

#
# copy UCRT64 runtime DLLs (needed by MinGW-built DLLs)
#

UCRT64_BIN="${MSYS2_PATH}/ucrt64/bin"

cp "${UCRT64_BIN}/libgcc_s_seh-1.dll" ../third-party/runtime-libs/win/x64/
cp "${UCRT64_BIN}/libstdc++-6.dll" ../third-party/runtime-libs/win/x64/
cp "${UCRT64_BIN}/libwinpthread-1.dll" ../third-party/runtime-libs/win/x64/
