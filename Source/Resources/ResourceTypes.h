#include "Core/Containers/GenerationalArray.h"

#include <memory>

template<typename T>
using ResourceHandle = typename GenerationalArray<std::unique_ptr<T>>::Handle;

using MaterialHandle = ResourceHandle<class Material>;
using MeshHandle = ResourceHandle<class Mesh>;
using ShaderModuleHandle = ResourceHandle<class ShaderModule>;
using TextureHandle = ResourceHandle<class Texture>;
