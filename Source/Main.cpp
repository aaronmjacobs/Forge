#include "ForgeApplication.h"

#include "Core/Assert.h"

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
      ASSERT(false, "Caught exception: %s", e.what());
      return 1;
   }

   return 0;
}
