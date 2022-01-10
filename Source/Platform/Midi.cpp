#include "Platform/Midi.h"

#include "Core/Assert.h"

#include <Kontroller/Client.h>

#include <memory>

namespace
{
   std::unique_ptr<Kontroller::Client> kontrollerClient;
   Kontroller::State kontrollerState;
}

namespace Midi
{
   void initialize()
   {
      kontrollerClient = std::make_unique<Kontroller::Client>();
      update();
   }

   void terminate()
   {
      kontrollerClient.reset();
   }

   void update()
   {
      ASSERT(kontrollerClient);
      kontrollerState = kontrollerClient->getState();
   }

   const Kontroller::State& getState()
   {
      return kontrollerState;
   }

   bool isConnected()
   {
      return kontrollerClient && kontrollerClient->isConnected();
   }
}
