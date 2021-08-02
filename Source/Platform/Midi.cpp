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
