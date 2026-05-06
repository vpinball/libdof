cmake_minimum_required(VERSION 3.25)

set(PLATFORM "win" CACHE STRING "Platform")
set(ARCH "x64" CACHE STRING "Arch")

option(BUILD_SHARED "Option to build shared library" ON)
option(BUILD_STATIC "Option to build static library" ON)
option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan for Debug builds" OFF)
option(POST_BUILD_COPY_EXT_LIBS "Option to copy external libraries to build directory" ON)

message(STATUS "PLATFORM: ${PLATFORM}")
message(STATUS "ARCH: ${ARCH}")

message(STATUS "BUILD_SHARED: ${BUILD_SHARED}")
message(STATUS "BUILD_STATIC: ${BUILD_STATIC}")
message(STATUS "ENABLE_SANITIZERS: ${ENABLE_SANITIZERS}")
message(STATUS "POST_BUILD_COPY_EXT_LIBS: ${POST_BUILD_COPY_EXT_LIBS}")

if(PLATFORM STREQUAL "ios" OR PLATFORM STREQUAL "ios-simulator")
   set(CMAKE_SYSTEM_NAME iOS)
   if (PLATFORM STREQUAL "ios-simulator")
      set(CMAKE_OSX_SYSROOT iphonesimulator)
   endif()
   set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
elseif(PLATFORM STREQUAL "tvos")
   set(CMAKE_SYSTEM_NAME tvOS)
   set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
elseif(PLATFORM STREQUAL "android")
   set(CMAKE_SYSTEM_NAME Android)
   set(CMAKE_SYSTEM_VERSION 30)
   set(CMAKE_ANDROID_ARCH_ABI arm64-v8a)
endif()

file(READ include/DOF/DOF.h version)
string(REGEX MATCH "LIBDOF_VERSION_MAJOR[ ]+([0-9]+)" _tmp ${version})
set(VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "LIBDOF_VERSION_MINOR[ ]+([0-9]+)" _tmp ${version})
set(VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "LIBDOF_VERSION_PATCH[ ]+([0-9]+)" _tmp ${version})
set(VERSION_PATCH "${CMAKE_MATCH_1}")

project(dof VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
   DESCRIPTION "Cross-platform DOF utilities library")

if(PLATFORM STREQUAL "win")
   if(ARCH STREQUAL "x86")
      add_compile_definitions(WIN32)
   endif()
   if(MSVC)
      add_compile_options(/bigobj)
   endif()
elseif(PLATFORM STREQUAL "win-mingw")
   add_compile_options(-Wa,-mbig-obj)
elseif(PLATFORM STREQUAL "macos")
   if (ARCH STREQUAL "arm64")
      set(CMAKE_OSX_ARCHITECTURES arm64)
   elseif(ARCH STREQUAL "x64")
      set(CMAKE_OSX_ARCHITECTURES x86_64)
   endif()
   set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
   set(CMAKE_INSTALL_RPATH "@executable_path")
elseif(PLATFORM STREQUAL "ios" OR PLATFORM STREQUAL "ios-simulator" STREQUAL "tvos")
   set(CMAKE_OSX_ARCHITECTURES arm64)
elseif(PLATFORM STREQUAL "linux" OR PLATFORM STREQUAL "android")
   set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
   set(CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   if(ENABLE_SANITIZERS AND (PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux"))
      set(SANITIZER_FLAGS -fsanitize=address,undefined)
      add_compile_options(${SANITIZER_FLAGS} -fno-omit-frame-pointer -g)
      add_link_options(${SANITIZER_FLAGS})
   endif()
endif()

set(LIBDOF_SOURCES
   src/Config.cpp
   src/DOF.cpp
   src/DirectOutputHandler.cpp
   src/Log.cpp
   src/Logger.cpp
   src/Pinball.cpp

   src/general/Curve.cpp
   src/general/CurveList.cpp
   src/general/DirectoryInfo.cpp
   src/general/FileInfo.cpp
   src/general/FilePattern.cpp
   src/general/FilePatternList.cpp
   src/general/FileReader.cpp
   src/general/IOConfigurator.cpp
   src/general/MathExtensions.cpp
   src/general/StringExtensions.cpp
   src/general/analog/AnalogAlpha.cpp
   src/general/bitmap/FastBitmap.cpp
   src/general/bitmap/FastImage.cpp
   src/general/bitmap/FastImageList.cpp
   src/general/bitmap/Image.cpp
   src/general/bitmap/PixelData.cpp
   src/general/color/ColorList.cpp
   src/general/color/ColorScale.cpp
   src/general/color/RGBAColor.cpp
   src/general/color/RGBAColorNamed.cpp
   src/general/color/RGBColor.cpp
   src/general/color/RGBColorNamed.cpp
   src/general/generic/NameChangeEventArgs.cpp
   src/general/generic/NamedItemBase.cpp

   src/globalconfiguration/GlobalConfig.cpp

   src/pinballsupport/AlarmHandler.cpp
   src/pinballsupport/InputQueue.cpp

   src/table/Table.cpp
   src/table/TableElement.cpp
   src/table/TableElementData.cpp
   src/table/TableElementList.cpp

   src/fx/AssignedEffect.cpp
   src/fx/AssignedEffectList.cpp
   src/fx/EffectBase.cpp
   src/fx/EffectEffectBase.cpp
   src/fx/EffectEventArgs.cpp
   src/fx/EffectFactory.cpp
   src/fx/EffectList.cpp
   src/fx/EffectRegistrations.cpp
   src/fx/IEffect.cpp
   src/fx/analogtoyfx/AnanlogToyEffectBase.cpp
   src/fx/analogtoyfx/AnalogToyValueEffect.cpp
   src/fx/conditionfx/TableElementConditionEffect.cpp
   src/fx/listfx/ListEffect.cpp
   src/fx/matrixfx/AnalogAlphaMatrixBitmapAnimationEffect.cpp
   src/fx/matrixfx/AnalogAlphaMatrixBitmapEffect.cpp
   src/fx/matrixfx/AnalogAlphaMatrixFlickerEffect.cpp
   src/fx/matrixfx/AnalogAlphaMatrixShiftEffect.cpp
   src/fx/matrixfx/AnalogAlphaMatrixValueEffect.cpp
   src/fx/matrixfx/MatrixBitmapAnimationEffectBase.cpp
   src/fx/matrixfx/MatrixBitmapEffectBase.cpp
   src/fx/matrixfx/MatrixEffectBase.cpp
   src/fx/matrixfx/MatrixFlickerEffectBase.cpp
   src/fx/matrixfx/MatrixPlasmaEffectBase.cpp
   src/fx/matrixfx/MatrixShiftEffectBase.cpp
   src/fx/matrixfx/MatrixValueEffectBase.cpp
   src/fx/matrixfx/RGBAMatrixBitmapAnimationEffect.cpp
   src/fx/matrixfx/RGBAMatrixBitmapEffect.cpp
   src/fx/matrixfx/RGBAMatrixColorEffect.cpp
   src/fx/matrixfx/RGBAMatrixColorScaleBitmapAnimationEffect.cpp
   src/fx/matrixfx/RGBAMatrixColorScaleBitmapEffect.cpp
   src/fx/matrixfx/RGBAMatrixColorScaleShapeEffect.cpp
   src/fx/matrixfx/RGBAMatrixFlickerEffect.cpp
   src/fx/matrixfx/RGBAMatrixPlasmaEffect.cpp
   src/fx/matrixfx/RGBAMatrixShapeEffect.cpp
   src/fx/matrixfx/RGBAMatrixShiftEffect.cpp
   src/fx/matrixfx/bitmapshapes/Shape.cpp
   src/fx/matrixfx/bitmapshapes/ShapeAnimated.cpp
   src/fx/matrixfx/bitmapshapes/ShapeDefinitions.cpp
   src/fx/matrixfx/bitmapshapes/ShapeList.cpp
   src/fx/nullfx/NullEffect.cpp
   src/fx/rgbafx/RGBAColorEffect.cpp
   src/fx/rgbafx/RGBAEffectBase.cpp
   src/fx/timmedfx/BlinkEffect.cpp
   src/fx/timmedfx/DelayEffect.cpp
   src/fx/timmedfx/DurationEffect.cpp
   src/fx/timmedfx/ExtendDurationEffect.cpp
   src/fx/timmedfx/FadeEffect.cpp
   src/fx/timmedfx/MaxDurationEffect.cpp
   src/fx/timmedfx/MinDurationEffect.cpp
   src/fx/valuefx/ValueInvertEffect.cpp
   src/fx/valuefx/ValueMapFullRangeEffect.cpp

   src/cab/Cabinet.cpp
   src/cab/CabinetOutputList.cpp
   src/cab/CabinetOwner.cpp
   src/cab/ICabinetOwner.cpp
   src/cab/out/IAutoConfigOutputController.cpp
   src/cab/out/IOutput.cpp
   src/cab/out/IOutputController.cpp
   src/cab/out/ISupportsSetValues.cpp
   src/cab/out/NullOutputController.cpp
   src/cab/out/Out.cpp
   src/cab/out/Output.cpp
   src/cab/out/OutputControllerBase.cpp
   src/cab/out/OutputControllerCompleteBase.cpp
   src/cab/out/OutputControllerFlexCompleteBase.cpp
   src/cab/out/OutputControllerList.cpp
   src/cab/out/OutputList.cpp
   src/cab/out/dmx/ArtNet.cpp
   src/cab/out/dmx/DMX.cpp
   src/cab/out/dmx/DMXOutput.cpp
   src/cab/out/dmx/artnetengine/ArtnetEngine.cpp
   src/cab/out/dmx/artnetengine/Engine.cpp
   src/cab/overrides/TableOverrideSetting.cpp
   src/cab/overrides/TableOverrideSettingDevice.cpp
   src/cab/overrides/TableOverrideSettings.cpp
   src/cab/schedules/ScheduledSetting.cpp
   src/cab/schedules/ScheduledSettingDevice.cpp
   src/cab/schedules/ScheduledSettings.cpp
   src/cab/sequencer/SequentialOutputSetting.cpp
   src/cab/sequencer/SequentialOutputSettings.cpp
   src/cab/toys/IToy.cpp
   src/cab/toys/IToyUpdatable.cpp
   src/cab/toys/ToyBase.cpp
   src/cab/toys/ToyBaseUpdatable.cpp
   src/cab/toys/ToyEventArgs.cpp
   src/cab/toys/ToyList.cpp
   src/cab/toys/hardware/GearMotor.cpp
   src/cab/toys/hardware/Lamp.cpp
   src/cab/toys/hardware/LedStrip.cpp
   src/cab/toys/hardware/Motor.cpp
   src/cab/toys/hardware/RGBLed.cpp
   src/cab/toys/hardware/Shaker.cpp
   src/cab/toys/layer/AlphaMappingTable.cpp
   src/cab/toys/layer/AnalogAlphaToy.cpp
   src/cab/toys/layer/RGBAToy.cpp
   src/cab/toys/lwequivalent/LedWizEquivalent.cpp
   src/cab/toys/virtual/AnalogAlphaToyGroup.cpp
   src/cab/toys/virtual/RGBAToyGroup.cpp

   src/ledcontrol/loader/ColorConfig.cpp
   src/ledcontrol/loader/ColorConfigList.cpp
   src/ledcontrol/loader/LedControl.cpp
   src/ledcontrol/loader/LedControlConfig.cpp
   src/ledcontrol/loader/LedControlConfigList.cpp
   src/ledcontrol/loader/TableConfig.cpp
   src/ledcontrol/loader/TableConfigColumn.cpp
   src/ledcontrol/loader/TableConfigColumnList.cpp
   src/ledcontrol/loader/TableConfigList.cpp
   src/ledcontrol/loader/TableConfigSetting.cpp
   src/ledcontrol/loader/TableVariablesDictionary.cpp
   src/ledcontrol/loader/VariablesDictionary.cpp
   src/ledcontrol/setup/Configurator.cpp

   third-party/include/tinyxml2/tinyxml2.cpp
)

if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "win-mingw" OR PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
   list(APPEND LIBDOF_SOURCES
      src/cab/out/adressableledstrip/DirectStripController.cpp
      src/cab/out/adressableledstrip/DirectStripControllerApi.cpp
      src/cab/out/adressableledstrip/LedStripOutput.cpp
      src/cab/out/adressableledstrip/TeensyStripController.cpp
      src/cab/out/adressableledstrip/WemosD1StripController.cpp
      src/cab/out/adressableledstrip/WS2811StripController.cpp
      src/cab/out/adressableledstrip/UMXDevice.cpp
      src/cab/out/adressableledstrip/UMXController.cpp
      src/cab/out/adressableledstrip/UMXControllerAutoConfigurator.cpp
      src/cab/out/comport/PinControl.cpp
      src/cab/out/dudescab/DudesCab.cpp
      src/cab/out/dudescab/DudesCabAutoConfigurator.cpp
      src/cab/out/dudescab/UMXDudesCabDevice.cpp
      src/cab/out/ftdichip/FT245RBitbangController.cpp
      src/cab/out/ftdichip/FT245RBitbangControllerAutoConfigurator.cpp
      src/cab/out/ftdichip/FTDI.cpp
      src/cab/out/lw/LedWiz.cpp
      src/cab/out/lw/LedWizOutput.cpp
      src/cab/out/lw/LedWizAutoConfigurator.cpp
      src/cab/out/pac/PacLed64.cpp
      src/cab/out/pac/PacLed64AutoConfigurator.cpp
      src/cab/out/pac/PacDrive.cpp
      src/cab/out/pac/PacDriveAutoConfigurator.cpp
      src/cab/out/pac/PacUIO.cpp
      src/cab/out/pac/PacUIOAutoConfigurator.cpp
      src/cab/out/pac/PacDriveSingleton.cpp
      src/cab/out/pinone/NamedPipeServer.cpp
      src/cab/out/pinone/PinOne.cpp
      src/cab/out/pinone/PinOneAutoConfigurator.cpp
      src/cab/out/pinone/PinOneCommunication.cpp
      src/cab/out/ps/Pinscape.cpp
      src/cab/out/ps/PinscapeAutoConfigurator.cpp
      src/cab/out/pspico/PinscapePico.cpp
      src/cab/out/pspico/PinscapePicoAutoConfigurator.cpp
   )
endif()

set(LIBDOF_INCLUDE_DIRS
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   third-party/include
)

if(BUILD_SHARED)
   add_library(dof_shared SHARED ${LIBDOF_SOURCES})

   target_include_directories(dof_shared PUBLIC ${LIBDOF_INCLUDE_DIRS})

   if(PLATFORM STREQUAL "win")
      target_link_directories(dof_shared PUBLIC
         third-party/build-libs/${PLATFORM}/${ARCH}
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      if(ARCH STREQUAL "x64")
         target_link_libraries(dof_shared PUBLIC libserialport64 hidapi64 libftdi164 libusb64-1.0 ws2_32)
      else()
         target_link_libraries(dof_shared PUBLIC libserialport hidapi libftdi1 libusb-1.0 ws2_32)
      endif()
   elseif(PLATFORM STREQUAL "win-mingw")
      target_link_directories(dof_shared PUBLIC
         third-party/build-libs/${PLATFORM}/${ARCH}
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(dof_shared PUBLIC serialport64 hidapi64 ftdi164 usb64-1.0 ws2_32)
   elseif(PLATFORM STREQUAL "macos")
      target_link_directories(dof_shared PUBLIC
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(dof_shared PUBLIC serialport hidapi ftdi1 usb-1.0)
   elseif(PLATFORM STREQUAL "linux")
      target_link_directories(dof_shared PUBLIC
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(dof_shared PUBLIC serialport hidapi-hidraw ftdi1 usb-1.0)
   elseif(PLATFORM STREQUAL "ios" OR PLATFORM STREQUAL "ios-simulator" OR PLATFORM STREQUAL "tvos")
      target_link_directories(dof_shared PUBLIC
         third-party/build-libs/${PLATFORM}/${ARCH}
      )
   elseif(PLATFORM STREQUAL "android")
      target_link_directories(dof_shared PUBLIC
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
   endif()

   if((PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "win-mingw") AND ARCH STREQUAL "x64")
      set(LIBDOF_OUTPUT_NAME "dof64")
   else()
      set(LIBDOF_OUTPUT_NAME "dof")
   endif()

   if(PLATFORM STREQUAL "win-mingw")
      set_target_properties(dof_shared PROPERTIES
         PREFIX ""
         IMPORT_PREFIX ""
         OUTPUT_NAME ${LIBDOF_OUTPUT_NAME}
         VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
      )
   else()
      set_target_properties(dof_shared PROPERTIES
         OUTPUT_NAME ${LIBDOF_OUTPUT_NAME}
         VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
      )
   endif()

   install(TARGETS dof_shared
      LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
   )
   install(DIRECTORY include/DOF DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

   if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "win-mingw" OR PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
      add_executable(dof_test
         src/tools/dof_test.cpp
      )
      target_link_libraries(dof_test PUBLIC dof_shared)

      if(POST_BUILD_COPY_EXT_LIBS)
         if(PLATFORM STREQUAL "win")
            if(ARCH STREQUAL "x64")
               add_custom_command(TARGET dof_test POST_BUILD
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libusb64-1.0.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libusb64-1.0.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libserialport64.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libserialport64-0.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/hidapi64.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/hidapi64.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libftdi164.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libftdi164.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libgcc_s_seh-1.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libstdc++-6.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libwinpthread-1.dll" "$<TARGET_FILE_DIR:dof_test>"
               )
            else()
               add_custom_command(TARGET dof_test POST_BUILD
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libusb-1.0.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libusb-1.0.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libserialport.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libserialport-0.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/hidapi.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/hidapi.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libftdi1.lib" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libftdi1.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libgcc_s_dw2-1.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libstdc++-6.dll" "$<TARGET_FILE_DIR:dof_test>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libwinpthread-1.dll" "$<TARGET_FILE_DIR:dof_test>"
               )
            endif()
         elseif(PLATFORM STREQUAL "win-mingw")
            add_custom_command(TARGET dof_test POST_BUILD
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libusb64-1.0.dll" "$<TARGET_FILE_DIR:dof_test>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libserialport64-0.dll" "$<TARGET_FILE_DIR:dof_test>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/hidapi64.dll" "$<TARGET_FILE_DIR:dof_test>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libftdi164.dll" "$<TARGET_FILE_DIR:dof_test>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libgcc_s_seh-1.dll" "$<TARGET_FILE_DIR:dof_test>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libstdc++-6.dll" "$<TARGET_FILE_DIR:dof_test>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libwinpthread-1.dll" "$<TARGET_FILE_DIR:dof_test>"
            )
         elseif(PLATFORM STREQUAL "macos")
            add_custom_command(TARGET dof_test POST_BUILD
               COMMAND cp -a "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/*.dylib" "$<TARGET_FILE_DIR:dof_test>"
            )
         elseif(PLATFORM STREQUAL "linux")
            add_custom_command(TARGET dof_test POST_BUILD
               COMMAND cp -a "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/*.so" "$<TARGET_FILE_DIR:dof_test>"
               COMMAND cp -a "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/*.so.*" "$<TARGET_FILE_DIR:dof_test>"
            )
         endif()
      endif()
   endif()

endif()

if(BUILD_STATIC)
   add_library(dof_static STATIC ${LIBDOF_SOURCES})

   target_include_directories(dof_static PUBLIC ${LIBDOF_INCLUDE_DIRS})

   if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "win-mingw")
      set_target_properties(dof_static PROPERTIES
         PREFIX ""
         OUTPUT_NAME "dof_static"
      )
   else()
      set_target_properties(dof_static PROPERTIES
         OUTPUT_NAME "dof"
      )
   endif()

   install(TARGETS dof_static
      LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
   )
   install(DIRECTORY include/DOF DESTINATION ${CMAKE_INSTALL_PREFIX}/include)

   if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "win-mingw" OR PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
      add_executable(dof_test_s
         src/tools/dof_test.cpp
      )

      if(PLATFORM STREQUAL "win")
         target_link_directories(dof_test_s PUBLIC
            third-party/build-libs/${PLATFORM}/${ARCH}
            third-party/runtime-libs/${PLATFORM}/${ARCH}
         )
         if(ARCH STREQUAL "x64")
            target_link_libraries(dof_test_s PUBLIC dof_static libserialport64 hidapi64 libftdi164 libusb64-1.0 ws2_32)
         else()
            target_link_libraries(dof_test_s PUBLIC dof_static libserialport hidapi libftdi1 libusb-1.0 ws2_32)
         endif()
      elseif(PLATFORM STREQUAL "win-mingw")
         target_link_directories(dof_test_s PUBLIC
            third-party/build-libs/${PLATFORM}/${ARCH}
            third-party/runtime-libs/${PLATFORM}/${ARCH}
         )
         target_link_libraries(dof_test_s PUBLIC dof_static serialport64 hidapi64 ftdi164 usb64-1.0 ws2_32)
      elseif(PLATFORM STREQUAL "macos")
         target_link_directories(dof_test_s PUBLIC
            third-party/runtime-libs/${PLATFORM}/${ARCH}
         )
         target_link_libraries(dof_test_s PUBLIC dof_static serialport hidapi ftdi1 usb-1.0)
      elseif(PLATFORM STREQUAL "linux")
         target_link_directories(dof_test_s PUBLIC
            third-party/runtime-libs/${PLATFORM}/${ARCH}
         )
         target_link_libraries(dof_test_s PUBLIC dof_static serialport hidapi-hidraw ftdi1 usb-1.0)
      endif()

      if(POST_BUILD_COPY_EXT_LIBS)
         if(PLATFORM STREQUAL "win")
            if(ARCH STREQUAL "x64")
               add_custom_command(TARGET dof_test_s POST_BUILD
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libusb64-1.0.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libusb64-1.0.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libserialport64.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libserialport64-0.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/hidapi64.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/hidapi64.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libftdi164.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libftdi164.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libgcc_s_seh-1.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libstdc++-6.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libwinpthread-1.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               )
            else()
               add_custom_command(TARGET dof_test_s POST_BUILD
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libusb-1.0.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libusb-1.0.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libserialport.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libserialport-0.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/hidapi.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/hidapi.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/build-libs/${PLATFORM}/${ARCH}/libftdi1.lib" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libftdi1.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libgcc_s_dw2-1.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libstdc++-6.dll" "$<TARGET_FILE_DIR:dof_test_s>"
                  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libwinpthread-1.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               )
            endif()
         elseif(PLATFORM STREQUAL "win-mingw")
            add_custom_command(TARGET dof_test_s POST_BUILD
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libusb64-1.0.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libserialport64-0.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/hidapi64.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libftdi164.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libgcc_s_seh-1.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libstdc++-6.dll" "$<TARGET_FILE_DIR:dof_test_s>"
               COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/libwinpthread-1.dll" "$<TARGET_FILE_DIR:dof_test_s>"
            )
         elseif(PLATFORM STREQUAL "macos")
            add_custom_command(TARGET dof_test_s POST_BUILD
               COMMAND cp -a "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/*.dylib" "$<TARGET_FILE_DIR:dof_test_s>"
            )
         else()
            add_custom_command(TARGET dof_test_s POST_BUILD
               COMMAND cp -a "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/*.so" "$<TARGET_FILE_DIR:dof_test_s>"
               COMMAND cp -a "${CMAKE_SOURCE_DIR}/third-party/runtime-libs/${PLATFORM}/${ARCH}/*.so.*" "$<TARGET_FILE_DIR:dof_test_s>"
            )
         endif()
      endif()
   endif()
endif()

if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "win-mingw" OR PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
   add_executable(ledwiz_test
      src/tools/ledwiz_test.cpp
   )

   target_include_directories(ledwiz_test PRIVATE
      ${CMAKE_SOURCE_DIR}
      ${CMAKE_SOURCE_DIR}/third-party/include
   )

   if(PLATFORM STREQUAL "win")
      target_link_directories(ledwiz_test PUBLIC
         third-party/build-libs/${PLATFORM}/${ARCH}
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      if(ARCH STREQUAL "x64")
         target_link_libraries(ledwiz_test PUBLIC hidapi64)
      else()
         target_link_libraries(ledwiz_test PUBLIC hidapi)
      endif()
   elseif(PLATFORM STREQUAL "win-mingw")
      target_link_directories(ledwiz_test PUBLIC
         third-party/build-libs/${PLATFORM}/${ARCH}
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(ledwiz_test PUBLIC hidapi64)
   elseif(PLATFORM STREQUAL "macos")
      target_link_directories(ledwiz_test PUBLIC
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(ledwiz_test PUBLIC hidapi)
   elseif(PLATFORM STREQUAL "linux")
      target_link_directories(ledwiz_test PUBLIC
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(ledwiz_test PUBLIC hidapi-hidraw)
   endif()

endif()

if(PLATFORM STREQUAL "win" OR PLATFORM STREQUAL "win-mingw" OR PLATFORM STREQUAL "macos" OR PLATFORM STREQUAL "linux")
   add_executable(pacled64_test
      src/tools/pacled64_test.cpp
   )

   target_include_directories(pacled64_test PRIVATE
      ${CMAKE_SOURCE_DIR}
      ${CMAKE_SOURCE_DIR}/third-party/include
   )

   if(PLATFORM STREQUAL "win")
      target_link_directories(pacled64_test PUBLIC
         third-party/build-libs/${PLATFORM}/${ARCH}
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      if(ARCH STREQUAL "x64")
         target_link_libraries(pacled64_test PUBLIC libusb64-1.0)
      else()
         target_link_libraries(pacled64_test PUBLIC libusb-1.0)
      endif()
   elseif(PLATFORM STREQUAL "win-mingw")
      target_link_directories(pacled64_test PUBLIC
         third-party/build-libs/${PLATFORM}/${ARCH}
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(pacled64_test PUBLIC usb64-1.0)
   elseif(PLATFORM STREQUAL "macos")
      target_link_directories(pacled64_test PUBLIC
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(pacled64_test PUBLIC usb-1.0)
   elseif(PLATFORM STREQUAL "linux")
      target_link_directories(pacled64_test PUBLIC
         third-party/runtime-libs/${PLATFORM}/${ARCH}
      )
      target_link_libraries(pacled64_test PUBLIC usb-1.0)
   endif()

endif()
