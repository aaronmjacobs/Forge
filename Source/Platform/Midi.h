#pragma once

#include <Kontroller/Kontroller.h>

namespace Midi
{
   void initialize();
   void terminate();

   void update();

   const Kontroller::State& getState();
}
