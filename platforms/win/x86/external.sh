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
CURRENT_DIR="$(pwd)"
MSYSTEM=MINGW32 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   ./autogen.sh &&
   ./configure --enable-shared &&
   make -j\$(nproc)
"
mkdir -p ../../third-party/include/libusb-1.0
cp libusb/libusb.h ../../third-party/include/libusb-1.0
cp libusb/.libs/libusb-1.0.dll.a ../../third-party/build-libs/win/x86/libusb-1.0.lib
cp libusb/.libs/libusb-1.0.dll ../../third-party/runtime-libs/win/x86/
cd ..

#
# build libserialport and copy to third-party
#

curl -sL https://github.com/sigrokproject/libserialport/archive/${LIBSERIALPORT_SHA}.tar.gz -o libserialport-${LIBSERIALPORT_SHA}.tar.gz
tar xzf libserialport-${LIBSERIALPORT_SHA}.tar.gz
mv libserialport-${LIBSERIALPORT_SHA} libserialport
cd libserialport
cp libserialport.h ../../third-party/include
CURRENT_DIR="$(pwd)"
MSYSTEM=MINGW32 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   ./autogen.sh &&
   ./configure &&
   make -j\$(nproc)
"
cp .libs/libserialport.dll.a ../../third-party/build-libs/win/x86/libserialport.lib
cp .libs/libserialport-0.dll ../../third-party/runtime-libs/win/x86/
cd ..

#
# build hidapi and copy to third-party
#

curl -sL https://github.com/libusb/hidapi/archive/${HIDAPI_SHA}.tar.gz -o hidapi-${HIDAPI_SHA}.tar.gz
tar xzf hidapi-${HIDAPI_SHA}.tar.gz
mv hidapi-${HIDAPI_SHA} hidapi
cd hidapi
sed -i.bak 's/OUTPUT_NAME "hidapi"/OUTPUT_NAME "hidapi"\n  PREFIX ""\n  IMPORT_PREFIX ""/' windows/CMakeLists.txt
CURRENT_DIR="$(pwd)"
MSYSTEM=MINGW32 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   cmake \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -B build &&
   cmake --build build -- -j\$(nproc)
"
cp -r hidapi ../../third-party/include/
cp build/src/windows/hidapi.dll.a ../../third-party/build-libs/win/x86/hidapi.lib
cp build/src/windows/hidapi.dll ../../third-party/runtime-libs/win/x86/
cd ..

#
# build libftdi and copy to third-party
#

curl -sL https://github.com/jsm174/libftdi/archive/${LIBFTDI_SHA}.tar.gz -o libftdi-${LIBFTDI_SHA}.tar.gz
tar xzf libftdi-${LIBFTDI_SHA}.tar.gz
mv libftdi-${LIBFTDI_SHA} libftdi
cd libftdi
sed -i.bak 's/cmake_minimum_required([^)]*)/cmake_minimum_required(VERSION 3.10)/' CMakeLists.txt
CURRENT_DIR="$(pwd)"
MSYSTEM=MINGW32 "${MSYS2_PATH}/usr/bin/bash.exe" -l -c "
   cd \"${CURRENT_DIR}\" &&
   cmake \
      -DFTDI_EEPROM=OFF \
      -DEXAMPLES=OFF \
      -DSTATICLIBS=OFF \
      -DLIBUSB_INCLUDE_DIR=../libusb/libusb \
      -DLIBUSB_LIBRARIES=$(pwd)/../../third-party/runtime-libs/win/x86/libusb-1.0.dll \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -B build &&
   cmake --build build -- -j\$(nproc)
"
mkdir -p ../../third-party/include/libftdi1
cp src/ftdi.h ../../third-party/include/libftdi1
cp build/src/libftdi1.dll.a ../../third-party/build-libs/win/x86/libftdi1.lib
cp build/src/libftdi1.dll ../../third-party/runtime-libs/win/x86/
cd ..

#
# copy MINGW32 runtime DLLs (needed by MinGW-built DLLs)
#

MINGW32_BIN="${MSYS2_PATH}/mingw32/bin"

cp "${MINGW32_BIN}/libgcc_s_dw2-1.dll" ../third-party/runtime-libs/win/x86/
cp "${MINGW32_BIN}/libstdc++-6.dll" ../third-party/runtime-libs/win/x86/
cp "${MINGW32_BIN}/libwinpthread-1.dll" ../third-party/runtime-libs/win/x86/
