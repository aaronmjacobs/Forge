#include "Platform/Midi.h"

#include "Core/Assert.h"

#include <KontrollerSock/Client.h>

#include <thread>

namespace
{
   KontrollerSock::Client kontrollerClient;
   Kontroller::State kontrollerState{};

   std::thread clientThread;
}

namespace Midi
{
   void initialize()
   {
      kontrollerState = {};
      for (Kontroller::Group& group : kontrollerState.groups)
      {
         // Assume sliders are in the "up" position by default
         group.slider = 1.0f;
      }

      clientThread = std::thread([]()
      {
         kontrollerClient.run("127.0.0.1");
      });
   }

   void terminate()
   {
      ASSERT(clientThread.joinable());

      kontrollerClient.shutDown();
      clientThread.join();
   }

   void update()
   {
      kontrollerState = kontrollerClient.getState();
   }

   const Kontroller::State& getState()
   {
      return kontrollerState;
   }
}
