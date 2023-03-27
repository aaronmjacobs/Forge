#pragma once

#include "Scene/System.h"

class OscillatingMovementSystem : public System
{
public:
   OscillatingMovementSystem(Scene& owningScene);

   void tick(float dt) override;

private:
   float lastTime = 0.0f;
};
