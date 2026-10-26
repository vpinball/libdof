//
// libdof test app
//

#include "DOF/DOF.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>

const int TIMEOUT_OFF = 650;
const int TIMEOUT_ON = 650;
const int TIMEOUT_START_DELAY = 1000;

struct TestRom
{
   std::string name;
   std::string description;
};

std::vector<TestRom> testRoms = { { "ij_l7", "Indiana Jones L7" }, { "tna", "Total Nuclear Annihilation" }, { "gw", "The Getaway High Speed II" }, { "goldcue", "Gold Cue" },
   { "bourne", "Bourne Identity" }, { "twenty4", "24" }, { "afm", "Attack From Mars" }, { "pinupmenu", "PinUP Menu" }, { "bloodmach", "Blood Machines" }, { "genesis", "Genesis" } };

void LIBDOFCALLBACK LogCallback(DOF_LogLevel logLevel, const char* format, va_list args)
{
   const char* levelStr;
   switch (logLevel)
   {
   case DOF_LogLevel_WARN: levelStr = "[WARN] "; break;
   case DOF_LogLevel_ERROR: levelStr = "[ERROR]"; break;
   case DOF_LogLevel_DEBUG: levelStr = "[DEBUG]"; break;
   case DOF_LogLevel_INFO: levelStr = "[INFO] "; break;
   default: break;
   }

   va_list args_copy;
   va_copy(args_copy, args);
   int size = vsnprintf(nullptr, 0, format, args_copy);
   va_end(args_copy);

   if (size > 0)
   {
      char* const buffer = static_cast<char*>(malloc(size + 1));
      vsnprintf(buffer, size + 1, format, args);
      printf("%s %s\n", levelStr, buffer);
      free(buffer);
   }
}

void Log(const char* format, ...)
{
   va_list args;
   va_start(args, format);
   LogCallback(DOF_LogLevel_INFO, format, args);
   va_end(args);
}

void TriggerOutput(DOF::DOF* pDof, char type, int id, int value, int wait_ms = 0)
{
   std::stringstream ss;
   ss << "=== Triggering " << type << id << ": " << (value ? "on" : "off") << " ===";
   Log(ss.str().c_str());
   pDof->DataReceive(type, id, value);
   if (wait_ms > 0)
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
   }
}

void TriggerOutputOnOff(DOF::DOF* pDof, char type, int id, int on_duration_ms = TIMEOUT_ON, int off_duration_ms = TIMEOUT_OFF)
{
   std::stringstream ss;
   ss << "=== Triggering " << type << id << ": on for ";

   if (on_duration_ms > 999)
   {
      ss << std::fixed << std::setprecision(2) << on_duration_ms / 1000.0 << "s";
   }
   else
   {
      ss << on_duration_ms << "ms";
   }

   ss << ", off for ";

   if (off_duration_ms > 999)
   {
      ss << std::fixed << std::setprecision(2) << off_duration_ms / 1000.0 << "s";
   }
   else
   {
      ss << off_duration_ms << "ms";
   }

   ss << " ===";

   Log(ss.str().c_str());

   pDof->DataReceive(type, id, 1);
   std::this_thread::sleep_for(std::chrono::milliseconds(on_duration_ms));
   pDof->DataReceive(type, id, 0);
   std::this_thread::sleep_for(std::chrono::milliseconds(off_duration_ms));
}

void RunIJTests(DOF::DOF* pDof)
{
   Log("========================================");
   Log("Testing ROM: ij_l7");
   Log("========================================");
   pDof->Init("", "ij_l7");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'L', 88, 5000);
   TriggerOutputOnOff(pDof, 'S', 9);
   TriggerOutputOnOff(pDof, 'S', 12);
   TriggerOutputOnOff(pDof, 'S', 51);
   TriggerOutputOnOff(pDof, 'S', 53);
   TriggerOutputOnOff(pDof, 'W', 15);
   TriggerOutputOnOff(pDof, 'W', 16);
   TriggerOutputOnOff(pDof, 'W', 65);
   TriggerOutputOnOff(pDof, 'W', 66);
   TriggerOutputOnOff(pDof, 'W', 67);
   TriggerOutputOnOff(pDof, 'W', 68);
   TriggerOutputOnOff(pDof, 'L', 88);
   TriggerOutputOnOff(pDof, 'S', 10);
   TriggerOutputOnOff(pDof, 'W', 88);

   pDof->Finish();
}

void RunTNATests(DOF::DOF* pDof)
{
   pDof->Init("", "tna");

   Log("========================================");
   Log("Testing ROM: tna");
   Log("========================================");
   ;

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'E', 103);
   TriggerOutputOnOff(pDof, 'E', 108);
   TriggerOutputOnOff(pDof, 'E', 110);
   TriggerOutputOnOff(pDof, 'E', 112);
   TriggerOutputOnOff(pDof, 'E', 116);
   TriggerOutputOnOff(pDof, 'E', 144);
   TriggerOutputOnOff(pDof, 'E', 146);
   TriggerOutputOnOff(pDof, 'E', 147);
   TriggerOutputOnOff(pDof, 'E', 148);
   TriggerOutputOnOff(pDof, 'E', 149);
   TriggerOutputOnOff(pDof, 'E', 150);
   TriggerOutputOnOff(pDof, 'E', 151);
   TriggerOutputOnOff(pDof, 'E', 152);
   TriggerOutputOnOff(pDof, 'E', 153);
   TriggerOutputOnOff(pDof, 'E', 179);

   pDof->Finish();
}

void RunGWTests(DOF::DOF* pDof)
{
   pDof->Init("", "gw");

   Log("========================================");
   Log("Testing ROM: gw");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   Log("=== DOF Protocol Test Scenarios ===");

   // S46 & S48 also require W78 to be on

   TriggerOutputOnOff(pDof, 'L', 52);
   TriggerOutputOnOff(pDof, 'S', 8);
   TriggerOutputOnOff(pDof, 'S', 12);
   TriggerOutputOnOff(pDof, 'S', 16);
   TriggerOutputOnOff(pDof, 'S', 19);
   TriggerOutputOnOff(pDof, 'S', 46);
   TriggerOutputOnOff(pDof, 'S', 48);
   TriggerOutputOnOff(pDof, 'W', 15);
   TriggerOutputOnOff(pDof, 'W', 25);
   TriggerOutputOnOff(pDof, 'W', 26);
   TriggerOutputOnOff(pDof, 'W', 37);
   TriggerOutputOnOff(pDof, 'W', 38);
   TriggerOutputOnOff(pDof, 'W', 42);
   TriggerOutputOnOff(pDof, 'W', 43);
   TriggerOutputOnOff(pDof, 'W', 52);
   TriggerOutputOnOff(pDof, 'W', 53);
   TriggerOutputOnOff(pDof, 'W', 67);
   TriggerOutputOnOff(pDof, 'W', 78);
   TriggerOutputOnOff(pDof, 'W', 81);
   TriggerOutputOnOff(pDof, 'W', 86);
   TriggerOutputOnOff(pDof, 'W', 87);
   TriggerOutputOnOff(pDof, 'W', 88);

   pDof->Finish();
}

void RunGoldcueTests(DOF::DOF* pDof)
{
   pDof->Init("", "goldcue");

   Log("========================================");
   Log("Testing ROM: goldcue");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'L', 73);
   TriggerOutputOnOff(pDof, 'L', 74);
   TriggerOutputOnOff(pDof, 'L', 75);
   TriggerOutputOnOff(pDof, 'L', 76);
   TriggerOutputOnOff(pDof, 'L', 77);
   TriggerOutputOnOff(pDof, 'L', 78);
   TriggerOutputOnOff(pDof, 'S', 3);
   TriggerOutputOnOff(pDof, 'S', 17);

   pDof->Finish();
}

void RunBourneTests(DOF::DOF* pDof)
{
   pDof->Init("", "bourne");

   Log("========================================");
   Log("Testing ROM: bourne");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'E', 142, 1000);
   TriggerOutputOnOff(pDof, 'E', 145, 1000);
   TriggerOutputOnOff(pDof, 'E', 103);
   TriggerOutputOnOff(pDof, 'E', 111, 5000);
   TriggerOutputOnOff(pDof, 'E', 181, 1000);
   TriggerOutputOnOff(pDof, 'E', 199, 1000);

   pDof->Finish();
}

void RunTwenty4Tests(DOF::DOF* pDof)
{
   pDof->Init("", "twenty4");

   Log("========================================");
   Log("Testing ROM: twenty4");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'S', 9);
   TriggerOutputOnOff(pDof, 'S', 19);
   TriggerOutputOnOff(pDof, 'S', 31);
   TriggerOutputOnOff(pDof, 'S', 32);
   TriggerOutputOnOff(pDof, 'W', 3);
   TriggerOutputOnOff(pDof, 'W', 10);
   TriggerOutputOnOff(pDof, 'W', 46);

   TriggerOutputOnOff(pDof, 'S', 17);
   TriggerOutputOnOff(pDof, 'S', 26);

   TriggerOutputOnOff(pDof, 'S', 11);
   TriggerOutputOnOff(pDof, 'W', 43);
   TriggerOutputOnOff(pDof, 'W', 55);

   TriggerOutputOnOff(pDof, 'S', 18);
   TriggerOutputOnOff(pDof, 'S', 26);
   TriggerOutputOnOff(pDof, 'W', 62);
   TriggerOutputOnOff(pDof, 'W', 56);

   TriggerOutputOnOff(pDof, 'S', 10);
   TriggerOutputOnOff(pDof, 'S', 20);
   TriggerOutputOnOff(pDof, 'S', 27);
   TriggerOutputOnOff(pDof, 'S', 31);
   TriggerOutputOnOff(pDof, 'W', 54);
   TriggerOutputOnOff(pDof, 'W', 57);

   TriggerOutputOnOff(pDof, 'S', 8, 1500);
   TriggerOutputOnOff(pDof, 'W', 62, 1500);

   TriggerOutputOnOff(pDof, 'S', 24, 200, 800);

   pDof->Finish();
}

void RunAFMTests(DOF::DOF* pDof)
{
   pDof->Init("", "afm");

   Log("========================================");
   Log("Testing ROM: afm");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'S', 27);
   TriggerOutputOnOff(pDof, 'S', 11);
   TriggerOutputOnOff(pDof, 'S', 28);
   TriggerOutputOnOff(pDof, 'W', 74);
   TriggerOutputOnOff(pDof, 'S', 9);
   TriggerOutputOnOff(pDof, 'S', 25);
   TriggerOutputOnOff(pDof, 'S', 12);
   TriggerOutputOnOff(pDof, 'S', 21);
   TriggerOutputOnOff(pDof, 'S', 23);
   TriggerOutputOnOff(pDof, 'S', 26);
   TriggerOutputOnOff(pDof, 'S', 10);
   TriggerOutputOnOff(pDof, 'S', 17);
   TriggerOutputOnOff(pDof, 'S', 18);
   TriggerOutputOnOff(pDof, 'S', 22);
   TriggerOutputOnOff(pDof, 'W', 38);
   TriggerOutputOnOff(pDof, 'S', 19);
   TriggerOutputOnOff(pDof, 'S', 13);
   TriggerOutputOnOff(pDof, 'S', 20);
   TriggerOutputOnOff(pDof, 'W', 48);
   TriggerOutputOnOff(pDof, 'W', 72);
   TriggerOutputOnOff(pDof, 'S', 39);
   TriggerOutputOnOff(pDof, 'W', 65);

   pDof->Finish();
}

void RunBloodmachTests(DOF::DOF* pDof)
{
   pDof->Init("", "bloodmach");

   Log("========================================");
   Log("Testing ROM: bloodmach");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'E', 135, 6000);

   pDof->Finish();
}

void RunGenesisTests(DOF::DOF* pDof)
{
   pDof->Init("", "genesis");

   Log("========================================");
   Log("Testing ROM: genesis");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'E', 169, 6000);
   TriggerOutputOnOff(pDof, 'W', 43, 6000);
   TriggerOutputOnOff(pDof, 'W', 73, 6000);

   pDof->Finish();
}

void RunPinupMenuTests(DOF::DOF* pDof)
{
   pDof->Init("", "pinupmenu");

   Log("========================================");
   Log("Testing ROM: pinupmenu");
   Log("========================================");

   std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_START_DELAY));

   TriggerOutputOnOff(pDof, 'E', 800, 2000);
   TriggerOutputOnOff(pDof, 'E', 801, 2000);
   TriggerOutputOnOff(pDof, 'E', 805, 2000);
   TriggerOutputOnOff(pDof, 'E', 900, 2000);
   TriggerOutputOnOff(pDof, 'E', 901, 2000);
   TriggerOutputOnOff(pDof, 'E', 902, 2000);
   TriggerOutputOnOff(pDof, 'E', 903, 5000);
   TriggerOutputOnOff(pDof, 'E', 904, 3000);
   TriggerOutputOnOff(pDof, 'E', 905, 5000);
   TriggerOutputOnOff(pDof, 'E', 906, 2000);
   TriggerOutputOnOff(pDof, 'E', 907, 2000);
   TriggerOutputOnOff(pDof, 'E', 908, 2000);
   TriggerOutputOnOff(pDof, 'E', 909, 2000);
   TriggerOutputOnOff(pDof, 'E', 910, 3000);
   TriggerOutputOnOff(pDof, 'E', 911, 2000);
   TriggerOutputOnOff(pDof, 'E', 912, 2000);
   TriggerOutputOnOff(pDof, 'E', 913, 2000);
   TriggerOutputOnOff(pDof, 'E', 914, 2000);
   TriggerOutputOnOff(pDof, 'E', 915, 2000);
   TriggerOutputOnOff(pDof, 'E', 916, 2000);
   TriggerOutputOnOff(pDof, 'E', 917, 2000);
   TriggerOutputOnOff(pDof, 'E', 918, 2000);
   TriggerOutputOnOff(pDof, 'E', 919, 6000);
   TriggerOutputOnOff(pDof, 'E', 920, 2000);
   TriggerOutputOnOff(pDof, 'E', 921, 2000);
   TriggerOutputOnOff(pDof, 'E', 922, 2000);
   TriggerOutputOnOff(pDof, 'E', 923, 2000);
   TriggerOutputOnOff(pDof, 'E', 924, 2000);
   TriggerOutputOnOff(pDof, 'E', 925, 2000);
   TriggerOutputOnOff(pDof, 'E', 926, 2000);
   TriggerOutputOnOff(pDof, 'E', 927, 2000);
   TriggerOutputOnOff(pDof, 'E', 928, 2000);
   TriggerOutputOnOff(pDof, 'E', 929, 2000);
   TriggerOutputOnOff(pDof, 'E', 930, 2000);
   TriggerOutputOnOff(pDof, 'E', 931, 2000);
   TriggerOutputOnOff(pDof, 'E', 932, 2000);
   TriggerOutputOnOff(pDof, 'E', 933, 2000);
   TriggerOutputOnOff(pDof, 'E', 934, 2000);
   TriggerOutputOnOff(pDof, 'E', 935, 2000);
   TriggerOutputOnOff(pDof, 'E', 936, 2000);
   TriggerOutputOnOff(pDof, 'E', 937, 2000);
   TriggerOutputOnOff(pDof, 'E', 938, 2000);
   TriggerOutputOnOff(pDof, 'E', 939, 2000);
   TriggerOutputOnOff(pDof, 'E', 940, 2000);
   TriggerOutputOnOff(pDof, 'E', 941, 2000);
   TriggerOutputOnOff(pDof, 'E', 942, 2000);
   TriggerOutputOnOff(pDof, 'E', 943, 2000);
   TriggerOutputOnOff(pDof, 'E', 944, 2000);
   TriggerOutputOnOff(pDof, 'E', 945, 2000);
   TriggerOutputOnOff(pDof, 'E', 946, 2000);
   TriggerOutputOnOff(pDof, 'E', 947, 2000);
   TriggerOutputOnOff(pDof, 'E', 948, 2000);
   TriggerOutputOnOff(pDof, 'E', 949, 2000);
   TriggerOutputOnOff(pDof, 'E', 950, 2000);
   TriggerOutputOnOff(pDof, 'E', 951, 2000);
   TriggerOutputOnOff(pDof, 'E', 952, 2000);
   TriggerOutputOnOff(pDof, 'E', 953, 2000);
   TriggerOutputOnOff(pDof, 'E', 954, 2000);
   TriggerOutputOnOff(pDof, 'E', 955, 2000);
   TriggerOutputOnOff(pDof, 'E', 956, 2000);
   TriggerOutputOnOff(pDof, 'E', 957, 2000);
   TriggerOutputOnOff(pDof, 'E', 958, 2000);
   TriggerOutputOnOff(pDof, 'E', 959, 2000);
   TriggerOutputOnOff(pDof, 'E', 960, 2000);
   TriggerOutputOnOff(pDof, 'E', 961, 2000);
   TriggerOutputOnOff(pDof, 'E', 962, 2000);
   TriggerOutputOnOff(pDof, 'E', 963, 6000);
   TriggerOutputOnOff(pDof, 'E', 964, 6000);
   TriggerOutputOnOff(pDof, 'E', 965, 6000);
   TriggerOutputOnOff(pDof, 'E', 966, 6000);
   TriggerOutputOnOff(pDof, 'E', 967, 6000);
   TriggerOutputOnOff(pDof, 'E', 968, 6000);
   TriggerOutputOnOff(pDof, 'E', 969, 6000);
   TriggerOutputOnOff(pDof, 'E', 970, 6000);
   TriggerOutputOnOff(pDof, 'E', 971, 6000);
   TriggerOutputOnOff(pDof, 'E', 972, 6000);
   TriggerOutputOnOff(pDof, 'E', 973, 2000);
   TriggerOutputOnOff(pDof, 'E', 974, 2000);
   TriggerOutputOnOff(pDof, 'E', 975, 2000);
   TriggerOutputOnOff(pDof, 'E', 976, 2000);
   TriggerOutputOnOff(pDof, 'E', 977, 2000);
   TriggerOutputOnOff(pDof, 'E', 978, 2000);
   TriggerOutputOnOff(pDof, 'E', 979, 2000);
   TriggerOutputOnOff(pDof, 'E', 980, 2000);
   TriggerOutputOnOff(pDof, 'E', 981, 2000);
   TriggerOutputOnOff(pDof, 'E', 982, 6000);
   TriggerOutputOnOff(pDof, 'E', 983, 2000);
   TriggerOutputOnOff(pDof, 'E', 984, 6000);
   TriggerOutputOnOff(pDof, 'E', 985, 2000);
   TriggerOutputOnOff(pDof, 'E', 986, 2000);
   TriggerOutputOnOff(pDof, 'E', 987, 2000);
   TriggerOutputOnOff(pDof, 'E', 988, 2000);
   TriggerOutputOnOff(pDof, 'E', 989, 2000);
   TriggerOutputOnOff(pDof, 'E', 990, 2000);
   TriggerOutputOnOff(pDof, 'E', 991, 6000);

   pDof->Finish();
}

void PrintUsage(const char* programName)
{
   std::cout << "Usage: " << programName << " --base-path PATH [ROM_NAME]" << std::endl;
   std::cout << std::endl;
   std::cout << "Options:" << std::endl;
   std::cout << "  --base-path  Path to VPinballX data directory (required)" << std::endl;
   std::cout << "  ROM_NAME     Specific ROM to test (optional)" << std::endl;
   std::cout << std::endl;
   std::cout << "Platform-specific base paths:" << std::endl;
   std::cout << "  macOS:   ~/Library/Application Support/VPinballX/10.8" << std::endl;
   std::cout << "  Linux:   ~/.local/share/VPinballX/10.8" << std::endl;
   std::cout << "  Windows: %APPDATA%\\VPinballX\\10.8" << std::endl;
   std::cout << std::endl;
   std::cout << "Available ROMs:" << std::endl;
   for (const auto& testRom : testRoms)
   {
      std::cout << "  " << std::left << std::setw(12) << testRom.name << " " << testRom.description << std::endl;
   }
}

int main(int argc, const char* argv[])
{
   std::string basePath = "";
   std::string romName = "";

   for (int i = 1; i < argc; i++)
   {
      if (strcmp(argv[i], "--base-path") == 0 && i + 1 < argc)
      {
         basePath = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
      {
         PrintUsage(argv[0]);
         return 0;
      }
      else if (argv[i][0] != '-')
      {
         romName = argv[i];
      }
   }

   if (basePath.empty())
   {
      std::cerr << "ERROR: --base-path is required" << std::endl;
      std::cerr << std::endl;
      PrintUsage(argv[0]);
      return 1;
   }

   DOF::Config* pConfig = DOF::Config::GetInstance();
   pConfig->SetLogCallback(LogCallback);
   pConfig->SetLogLevel(DOF_LogLevel_DEBUG);
   pConfig->SetBasePath(basePath.c_str());

   Log("Using base path: %s", basePath.c_str());

   DOF::DOF* pDof = new DOF::DOF();

   if (!romName.empty())
   {
      bool found = false;
      for (const auto& testRom : testRoms)
      {
         if (testRom.name == romName)
         {
            found = true;
            if (testRom.name == "ij_l7")
               RunIJTests(pDof);
            else if (testRom.name == "tna")
               RunTNATests(pDof);
            else if (testRom.name == "gw")
               RunGWTests(pDof);
            else if (testRom.name == "goldcue")
               RunGoldcueTests(pDof);
            else if (testRom.name == "bourne")
               RunBourneTests(pDof);
            else if (testRom.name == "twenty4")
               RunTwenty4Tests(pDof);
            else if (testRom.name == "afm")
               RunAFMTests(pDof);
            else if (testRom.name == "pinupmenu")
               RunPinupMenuTests(pDof);
            else if (testRom.name == "bloodmach")
               RunBloodmachTests(pDof);
            else if (testRom.name == "genesis")
               RunGenesisTests(pDof);
            break;
         }
      }

      if (!found)
      {
         std::cout << "Unknown ROM: " << romName << std::endl;
         std::cout << "Available ROMs: ";
         for (size_t i = 0; i < testRoms.size(); i++)
         {
            std::cout << testRoms[i].name;
            if (i < testRoms.size() - 1)
               std::cout << ", ";
         }
         std::cout << std::endl;
         delete pDof;
         return 1;
      }
   }
   else
   {
      RunIJTests(pDof);
      RunTNATests(pDof);
      RunGWTests(pDof);
      RunGoldcueTests(pDof);
      RunBourneTests(pDof);
      RunTwenty4Tests(pDof);
      RunAFMTests(pDof);
      RunPinupMenuTests(pDof);
   }

   Log("Shutting down...");
   delete pDof;

   Log("All tests completed!");

   return 0;
}
