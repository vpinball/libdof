# libdof

A cross platform library for performing Direct Output Framework tasks.

This library is currently used by [Visual Pinball Standalone](https://github.com/vpinball/vpinball/tree/standalone) to support Direct Output Framework.

## About:

**Direct Output Framework (DOF)** is a system originally developed in C# for controlling feedback devices (like LEDs, solenoids, and motors) on virtual pinball machines. It is widely used in the virtual pinball community, especially in combination with Pinscape controllers and B2S-based backglasses.

**libdof** is a cross-platform C++ library aiming to be a faithful 1:1 port of the original [DirectOutput](https://github.com/mjrgh/DirectOutput) C# codebase maintained by [@mjrgh](https://github.com/mjrgh). This work began on March 23, 2024, and builds on previous porting efforts like B2S and FlexDMD, where accuracy to the original code has always been a priority.

> [!WARNING]
> Given the scope of Direct Output Framework - over 250 C# files - this C++ port leveraged a large amount of AI assistance (ChatGPT and Claude). I know for sure it is **NOT** fully accurate and **DOES** contain bugs. Now that most of the files are in place, I will continue refining it until we achieve a true faithful 1:1 port.

## Supported Output Controllers

`libdof` supports a wide range of output controllers for driving LEDs, solenoids, and other feedback devices:

### **Tested & Working**
- **[Pinscape](http://mjrnet.org/pinscape)** - @mjrgh's popular pinball controller with 32+ outputs
- **[Pinscape Pico](https://github.com/mjrgh/PinscapePico)** - RP2040-based version with enhanced features  
- **[TeensyStripController](https://github.com/DirectOutput/TeensyStripController)** - Teensy based WS2812 LED strip controller
- **[WemosD1MPStripController](https://github.com/aetios50/PincabLedStrip)** - Wemos D1 Mini Pro based WS2812 LED strip controller
- **[PacLED64](https://www.ultimarc.com/output/led-and-output-controllers/pacled64/)** - Ultimarc's 64-output LED controller with PWM support
- **[LedWiz](https://groovygamegear.com/webstore/index.php?main_page=product_info&products_id=239)** - LED-Wiz's 32-port USB compatible lighting and output controller
- **[PinOne](https://www.clevelandsoftwaredesign.com/pinball-parts/pinone)** - Cleveland Software Design controller with 63 outputs
- **[FTDI Controllers](https://directoutput.github.io/DirectOutput/hardware.html#hardware_FT245bitbang)** - FT245R bitbang controllers  

### **Implemented & Ready To Test**
- **DudesCab** - RP2040-based controller with 128 PWM outputs
- **UMXController** - Unified MX protocol for addressable LED strips (via DudesCab MX)
- **ArtNet/DMX** - Professional lighting control via Ethernet (all platforms)
- **PinControl** - Arduino-based controller with 10 outputs
- **WS2811/WS2812 LED Strips** - Addressable LED strip support
- **PAC Controllers** (PacDrive, PacUIO) - Cross-platform libusb implementation

### **Not Implemented**
- **SSF Controllers** - *Audio-based feedback systems*  
- **Philips Hue** - *Smart lighting integration*

`libdof` is actively used by [Visual Pinball Standalone](https://github.com/vpinball/vpinball/tree/standalone) and continues expanding controller support. 

> [!NOTE]
> On macOS, accessing HID devices with **usage page 0x06** and **usage 0x00** (like Pinscape Pico) requires elevated permissions. You must run as sudo to communicate with the device. See [here](https://gist.github.com/jsm174/d8711b7ea4af4501090f96f1db808202) for more details.

## Usage:

```cpp
#include "DOF/DOF.h"
.
.

void LIBDOFCALLBACK LogCallback(DOF_LogLevel logLevel, const char* format, va_list args)
{
   va_list args_copy;
   va_copy(args_copy, args);
   int size = vsnprintf(nullptr, 0, format, args_copy);
   va_end(args_copy);

   if (size > 0)
   {
      char* const buffer = static_cast<char*>(malloc(size + 1));
      vsnprintf(buffer, size + 1, format, args);
      printf("%s\n", buffer);
      free(buffer);
   }
}

void test()
{
   DOF::Config* pConfig = DOF::Config::GetInstance();
   pConfig->SetLogCallback(LogCallback);
   pConfig->SetLogLevel(DOF_LogLevel_DEBUG);
   pConfig->SetBasePath("/Users/jmillard/Library/Application Support/VPinballX/10.8/");

   DOF::DOF* pDof = new DOF::DOF();
   pDof->Init("", "ij_l7");
   .
   .
   pDof->DataReceive('L', 88, 1);
   .
   .
   pDof->DataReceive('L', 88, 0);
   .
   .
   pDof->Finish();
   delete pDof;
}
```

## Building:

#### Windows (x64)

Requires MSYS2 for building external dependencies:

```shell
pacman -S --noconfirm make diffutils \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-libwinpthread \
  mingw-w64-ucrt-x86_64-cmake
```

```shell
platforms/win/x64/external.sh
cmake -G "Visual Studio 18 2026" -DPLATFORM=win -DARCH=x64 -B build
cmake --build build --config Release
```

#### Windows (x86)

Requires MSYS2 for building external dependencies:

```shell
pacman -S --noconfirm make diffutils \
  mingw-w64-i686-gcc mingw-w64-i686-libwinpthread \
  mingw-w64-i686-cmake
```

```shell
platforms/win/x86/external.sh
cmake -G "Visual Studio 18 2026" -A Win32 -DPLATFORM=win -DARCH=x86 -B build
cmake --build build --config Release
```

#### Windows MinGW / MSYS2 UCRT64 (x64)

Requires MSYS2 with UCRT64 environment. Install dependencies:

```shell
pacman -S --noconfirm make diffutils autoconf automake libtool \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-libwinpthread \
  mingw-w64-ucrt-x86_64-cmake
```

Build (entire build runs inside the MSYS2 UCRT64 shell):

```shell
MSYSTEM=UCRT64 /c/msys64/usr/bin/bash.exe -l -c "
  cd \"$(pwd)\" &&
  ./platforms/win-mingw/x64/external.sh &&
  cmake -DCMAKE_BUILD_TYPE=Release -DPLATFORM=win-mingw -DARCH=x64 -B build &&
  cmake --build build -- -j\$(nproc)
"
```

#### Linux (x64)
```shell
platforms/linux/x64/external.sh
cmake -DPLATFORM=linux -DARCH=x64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### Linux (aarch64)
```shell
platforms/linux/aarch64/external.sh
cmake -DPLATFORM=linux -DARCH=aarch64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### MacOS (arm64)
```shell
platforms/macos/arm64/external.sh
cmake -DPLATFORM=macos -DARCH=arm64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### MacOS (x64)
```shell
platforms/macos/x64/external.sh
cmake -DPLATFORM=macos -DARCH=x64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### iOS (arm64)
```shell
platforms/ios/arm64/external.sh
cmake -DPLATFORM=ios -DARCH=arm64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### tvOS (arm64)
```shell
platforms/tvos/arm64/external.sh
cmake -DPLATFORM=tvos -DARCH=arm64 -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

#### Android (arm64-v8a)
```shell
platforms/android/arm64-v8a/external.sh
cmake -DPLATFORM=android -DARCH=arm64-v8a -DCMAKE_BUILD_TYPE=Release -B build
cmake --build build
```

## Testing:

The `dof_test` tool requires `--base-path` to specify the VPinballX data directory.

Available ROMs: `ij_l7`, `tna`, `gw`, `goldcue`, `bourne`, `twenty4`, `afm`, `pinupmenu`

#### macOS
```shell
./build/dof_test --base-path "$HOME/Library/Application Support/VPinballX/10.8" gw
```

#### Linux
```shell
./build/dof_test --base-path ~/.local/share/VPinballX/10.8 gw
```

#### Windows
```shell
build\Release\dof_test.exe --base-path %APPDATA%\VPinballX\10.8 gw
```

## Acknowledgments:

PAC controller USB communication protocols were figured out with significant help from the [Ultimarc-linux](https://github.com/katie-snow/Ultimarc-linux) repository by @katie-snow. This project provided essential insights into the libusb control transfer implementation for PacLed64, PacDrive, and PacUIO devices.

## Configuration:

For custom setups, create a `CabinetConfig.xml` file in your DOF config directory:

```xml
<?xml version="1.0" encoding="utf-8"?>
<Cabinet xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" 
         xmlns:xsd="http://www.w3.org/2001/XMLSchema">
  <Name>Test Cabinet with Multiple Controllers</Name>
  <OutputControllers>
    <!-- Teensy LED Strip Controller -->
    <TeensyStripController>
      <Name>Cabinet Teensy</Name>
      <ComPortName>/dev/cu.usbmodem104409301</ComPortName>
      <ComPortBaudRate>9600</ComPortBaudRate>
      <ComPortParity>None</ComPortParity>
      <ComPortDataBits>8</ComPortDataBits>
      <ComPortStopBits>One</ComPortStopBits>
      <ComPortTimeOutMs>200</ComPortTimeOutMs>
      <ComPortOpenWaitMs>50</ComPortOpenWaitMs>
      <ComPortHandshakeStartWaitMs>20</ComPortHandshakeStartWaitMs>
      <ComPortHandshakeEndWaitMs>50</ComPortHandshakeEndWaitMs>
      <ComPortDtrEnable>false</ComPortDtrEnable>
      <NumberOfLedsStrip1>36</NumberOfLedsStrip1>
      <NumberOfLedsStrip2>36</NumberOfLedsStrip2>
      <NumberOfLedsStrip3>0</NumberOfLedsStrip3>
      <NumberOfLedsStrip4>0</NumberOfLedsStrip4>
      <NumberOfLedsStrip5>0</NumberOfLedsStrip5>
      <NumberOfLedsStrip6>0</NumberOfLedsStrip6>
      <NumberOfLedsStrip7>0</NumberOfLedsStrip7>
      <NumberOfLedsStrip8>0</NumberOfLedsStrip8>
      <NumberOfLedsStrip9>0</NumberOfLedsStrip9>
      <NumberOfLedsStrip10>0</NumberOfLedsStrip10>
    </TeensyStripController>

    <!-- Wemos LED Strip Controller -->
    <WemosD1MPStripController>
      <Name>Main Wemos</Name>
      <ComPortName>/dev/cu.usbserial-02G5PBVF</ComPortName>
      <ComPortBaudRate>2000000</ComPortBaudRate>
      <ComPortParity>None</ComPortParity>
      <ComPortDataBits>8</ComPortDataBits>
      <ComPortStopBits>One</ComPortStopBits>
      <ComPortTimeOutMs>500</ComPortTimeOutMs>
      <ComPortOpenWaitMs>50</ComPortOpenWaitMs>
      <ComPortHandshakeStartWaitMs>20</ComPortHandshakeStartWaitMs>
      <ComPortHandshakeEndWaitMs>50</ComPortHandshakeEndWaitMs>
      <ComPortDtrEnable>true</ComPortDtrEnable>
      <SendPerLedstripLength>true</SendPerLedstripLength>
      <UseCompression>false</UseCompression>
      <TestOnConnect>false</TestOnConnect>
      <NumberOfLedsStrip1>20</NumberOfLedsStrip1>
      <NumberOfLedsStrip2>0</NumberOfLedsStrip2>
      <NumberOfLedsStrip3>0</NumberOfLedsStrip3>
      <NumberOfLedsStrip4>0</NumberOfLedsStrip4>
      <NumberOfLedsStrip5>0</NumberOfLedsStrip5>
      <NumberOfLedsStrip6>0</NumberOfLedsStrip6>
      <NumberOfLedsStrip7>0</NumberOfLedsStrip7>
      <NumberOfLedsStrip8>0</NumberOfLedsStrip8>
      <NumberOfLedsStrip9>0</NumberOfLedsStrip9>
      <NumberOfLedsStrip10>0</NumberOfLedsStrip10>
    </WemosD1MPStripController>
    
    <!-- Pinscape Controller -->
    <Pinscape>
      <Name>Main Pinscape</Name>
      <Number>1</Number>
    </Pinscape>
    
    <!-- Pinscape Pico Controller -->
    <PinscapePico>
      <Name>Backbox Pico</Name>
      <Number>1</Number>
    </PinscapePico>
  </OutputControllers>
  
  <Toys>    
    <!-- LED Strip Toys for Teensy Controller -->
    <LedStrip>
      <Name>PF Left</Name>
      <Width>36</Width>
      <Height>1</Height>
      <Brightness>100</Brightness>
      <LedStripArrangement>TopDownLeftRight</LedStripArrangement>
      <ColorOrder>GRB</ColorOrder>
      <FirstLedNumber>36</FirstLedNumber> 
      <FadingCurveName>SwissLizardsLedCurve</FadingCurveName>
      <OutputControllerName>Cabinet Teensy</OutputControllerName>
    </LedStrip>

    <!-- LED Strip Toys for Wemos Controller -->
    <LedStrip>
      <Name>PF Right</Name> 
      <Width>20</Width> 
      <Brightness>100</Brightness>
      <Height>1</Height>
      <LedStripArrangement>TopDownLeftRight</LedStripArrangement>
      <ColorOrder>RGB</ColorOrder> 
      <FirstLedNumber>0</FirstLedNumber> 
      <FadingCurveName>SwissLizardsLedCurve</FadingCurveName>
      <OutputControllerName>Main Wemos</OutputControllerName>
    </LedStrip>
    
    <!-- LedWizEquivalent for LED Strips (LedWiz #30) -->
    <LedWizEquivalent>
      <Name>LED Strip Equivalent</Name> 
      <LedWizNumber>30</LedWizNumber>
      <Outputs>
        <LedWizEquivalentOutput>
          <OutputName>PF Left</OutputName>
          <LedWizEquivalentOutputNumber>4</LedWizEquivalentOutputNumber> 
        </LedWizEquivalentOutput>
        <LedWizEquivalentOutput>
          <OutputName>PF Right</OutputName> 
          <LedWizEquivalentOutputNumber>1</LedWizEquivalentOutputNumber>
        </LedWizEquivalentOutput> 
      </Outputs>
    </LedWizEquivalent> 
    
    <!-- LedWizEquivalent for Pinscape (Unit 1 = LedWiz #51) -->
    <LedWizEquivalent>
      <Name>Main Pinscape Equivalent</Name>
      <LedWizNumber>51</LedWizNumber>
      <Outputs>
        <LedWizEquivalentOutput>
          <OutputName>Main Pinscape.08</OutputName>
          <LedWizEquivalentOutputNumber>8</LedWizEquivalentOutputNumber>
        </LedWizEquivalentOutput>
      </Outputs>
    </LedWizEquivalent>
    
    <!-- LedWizEquivalent for PinscapePico (Unit 1 = LedWiz #120) -->
    <LedWizEquivalent>
      <Name>Backbox Pico Equivalent</Name>
      <LedWizNumber>120</LedWizNumber>
      <Outputs>
        <LedWizEquivalentOutput>
          <OutputName>Backbox Pico.01</OutputName>
          <LedWizEquivalentOutputNumber>1</LedWizEquivalentOutputNumber>
        </LedWizEquivalentOutput>
      </Outputs>
    </LedWizEquivalent>
  </Toys>
  
  <!-- Disable auto-configuration to use manual config -->
  <AutoConfigEnabled>false</AutoConfigEnabled> 
</Cabinet>
```
