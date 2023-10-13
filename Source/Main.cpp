#if FORGE_PLATFORM_WINDOWS && !FORGE_WITH_DEBUG_UTILS
#  pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif // FORGE_PLATFORM_WINDOWS && !FORGE_WITH_DEBUG_UTILS

#include "ForgeApplication.h"

#include "Core/Assert.h"

#include <boxer/boxer.h>

#include <exception>

int main(int argc, char* argv[])
{
   try
   {
      ForgeApplication application;
      application.run();
   }
   catch (const std::exception& e)
   {
#if FORGE_WITH_DEBUG_UTILS
      ASSERT(false, "Caught exception: %s", e.what());
#else
      boxer::show((std::string("Caught exception: ") + e.what()).c_str(), "Error");
#endif

      return 1;
   }

   return 0;
}
