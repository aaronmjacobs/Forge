#include "ForgeApplication.h"

#include "Core/Assert.h"
#include "Core/Log.h"

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
#if FORGE_DEBUG
      ASSERT(false, "Caught exception: %s", e.what());
#else
      LOG_ERROR_MSG_BOX(e.what());
#endif

      return 1;
   }

   return 0;
}
