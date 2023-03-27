#pragma once

class Scene;

class System
{
public:
   System(Scene& owningScene)
      : scene(owningScene)
   {
   }

   virtual ~System() = default;

   virtual int getPriority() const
   {
      return 0;
   }

   virtual void tick(float dt)
   {
   }

protected:
   Scene& scene;
};
