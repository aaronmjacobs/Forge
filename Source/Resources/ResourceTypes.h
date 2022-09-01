#pragma once

#include "Core/Containers/GenerationalArrayHandle.h"

#include <memory>

template<typename T>
using ResourceHandle = GenerationalArrayHandle<std::unique_ptr<T>>;

using MaterialHandle = ResourceHandle<class Material>;
using MeshHandle = ResourceHandle<class Mesh>;
using ShaderModuleHandle = ResourceHandle<class ShaderModule>;
using TextureHandle = ResourceHandle<class Texture>;
