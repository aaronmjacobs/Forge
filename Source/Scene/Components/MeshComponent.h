#pragma once

#include "Resources/ResourceTypes.h"

struct MeshComponent
{
   StrongMeshHandle meshHandle;
   bool castsShadows = true;
};
