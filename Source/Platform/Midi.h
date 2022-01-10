#pragma once

#include <Kontroller/State.h>

namespace Midi
{
   void initialize();
   void terminate();

   void update();

   const Kontroller::State& getState();
   bool isConnected();
}
